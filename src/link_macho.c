#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
// SPDX-License-Identifier: LGPL-2.1-or-later
// Native Mach-O 64-bit linker for rcc (macOS).
#include "link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

// ---------------------------------------------------------------------------
// Mach-O constants
// ---------------------------------------------------------------------------

#define MH_MAGIC_64   0xFEEDFACFU
#define MH_OBJECT     1
#define MH_EXECUTE    2
#define MH_DYLIB       6
#define CPU_TYPE_ARM64    0x0100000C
#define CPU_TYPE_X86_64   0x01000007
#define CPU_SUBTYPE_ALL   0x00000003
#define CPU_SUBTYPE_ARM64_ALL 0x00000000

// Load commands
#define LC_SEGMENT_64     0x19
#define LC_SYMTAB         0x02
#define LC_DYSYMTAB       0x0B
#define LC_LOAD_DYLIB     0x0C
#define LC_ID_DYLIB       0x0D
#define LC_LOAD_DYLINKER   0x0E
#define LC_UUID            0x1B
#define LC_DYLD_EXPORTS_TRIE 0x80000033
#define LC_MAIN           0x80000028
#define LC_DYLD_CHAINED_FIXUPS 0x80000034
#define LC_BUILD_VERSION  0x32
#define LC_DYLD_INFO_ONLY 0x80000022
#define LC_CODE_SIGNATURE 0x1d

// Section flags
#define S_REGULAR                   0x0
#define S_ZEROFILL                  0x1
#define S_ATTR_PURE_INSTRUCTIONS    0x80000000
#define S_ATTR_SOME_INSTRUCTIONS    0x400

// ARM64 relocation types
#define ARM64_RELOC_UNSIGNED         0
#define ARM64_RELOC_SUBTRACTOR       1
#define ARM64_RELOC_BRANCH26         2
#define ARM64_RELOC_PAGE21           3
#define ARM64_RELOC_PAGEOFF12        4
#define ARM64_RELOC_GOT_LOAD_PAGE21  5
#define ARM64_RELOC_GOT_LOAD_PAGEOFF12 6

// x86-64 relocation types
#define X86_64_RELOC_UNSIGNED  0
#define X86_64_RELOC_SIGNED    1
#define X86_64_RELOC_BRANCH    2
#define X86_64_RELOC_GOT_LOAD  3
#define X86_64_RELOC_GOT       4
#define X86_64_RELOC_SIGNED_1  6
#define X86_64_RELOC_SIGNED_2  7
#define X86_64_RELOC_SIGNED_4  8

// nlist_64 type bits
#define N_STAB  0xE0
#define N_TYPE  0x0E
#define N_UNDF  0x00
#define N_ABS   0x02
#define N_SECT  0x0E
#define N_EXT   0x01
#define N_PEXT  0x10

// PLATFORM_MACOS = 1
#define PLATFORM_MACOS 1

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static uint32_t mo_r32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t mo_r64(const uint8_t *p) {
    return (uint64_t)mo_r32(p) | ((uint64_t)mo_r32(p + 4) << 32);
}
static void mo_w32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static void mo_w32(FILE *f, uint32_t v) {
    fputc(v & 0xFF, f);
    fputc((v >> 8) & 0xFF, f);
    fputc((v >> 16) & 0xFF, f);
    fputc((v >> 24) & 0xFF, f);
}
static void mo_w64(FILE *f, uint64_t v) {
    mo_w32(f, (uint32_t)v);
    mo_w32(f, (uint32_t)(v >> 32));
}
static void mo_wbuf(FILE *f, const void *b, size_t n) { fwrite(b, 1, n, f); }
static void mo_wzeros(FILE *f, size_t n) {
    static const uint8_t z[512] = {0};
    while (n > 512) {
        mo_wbuf(f, z, 512);
        n -= 512;
    }
    mo_wbuf(f, z, n);
}
static uint64_t mo_align(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }

// ---------------------------------------------------------------------------
// Mach-O export trie builder (used for dylib -shared output).  A proper
// prefix (radix) trie: symbols sharing a prefix share a path, and each node
// branches on the next distinguishing character, so no node exceeds the
// one-byte childCount limit even for thousands of exports.  Node offsets are
// ULEB128 from the trie start; their lengths depend on the offsets, so the
// layout is fixed-pointed before serialization.
typedef struct MoTrieNode {
    int terminal; // this node is an exact exported symbol
    uint64_t addr; // image-relative export address (terminal only)
    struct MoTrieEdge *edges;
    int n_edges;
    size_t offset; // assigned trie-start offset
} MoTrieNode;
typedef struct MoTrieEdge {
    char *label;
    struct MoTrieNode *child;
} MoTrieEdge;
typedef struct {
    uint64_t addr;
    const char *name;
    int len;
} MoTrieSym;

static int mo_uleb_len(uint64_t v) {
    int n = 1;
    while (v >= 128) {
        v >>= 7;
        n++;
    }
    return n;
}

// Build a trie node covering sorted syms[lo,hi), all sharing the first `pfx`
// characters.  Recurses, grouping the remainder by next character.
static MoTrieNode *mo_trie_build(MoTrieSym *syms, int lo, int hi, int pfx) {
    MoTrieNode *node = calloc(1, sizeof(MoTrieNode));
    int i = lo;
    if (syms[lo].len == pfx) { // exact match consumes the whole name here
        node->terminal = 1;
        node->addr = syms[lo].addr;
        i = lo + 1;
    }
    int cap = 0;
    while (i < hi) {
        char c = syms[i].name[pfx];
        int j = i;
        while (j < hi && syms[j].name[pfx] == c) j++;
        // longest common prefix of [i,j) beyond pfx (sorted -> compare ends)
        int lcp = 0;
        const char *a = syms[i].name, *b = syms[j - 1].name;
        int amax = syms[i].len, bmax = syms[j - 1].len;
        while (pfx + lcp < amax && pfx + lcp < bmax &&
               a[pfx + lcp] == b[pfx + lcp])
            lcp++;
        if (lcp == 0) lcp = 1; // grouped by shared char -> at least 1
        MoTrieEdge e;
        e.label = malloc((size_t)lcp + 1);
        memcpy(e.label, syms[i].name + pfx, (size_t)lcp);
        e.label[lcp] = '\0';
        e.child = mo_trie_build(syms, i, j, pfx + lcp);
        if (node->n_edges >= cap) {
            cap = cap ? cap * 2 : 4;
            node->edges = realloc(node->edges, (size_t)cap * sizeof(MoTrieEdge));
        }
        node->edges[node->n_edges++] = e;
        i = j;
    }
    return node;
}

// Collect nodes into `list` in BFS order (root first).
static void mo_trie_collect(MoTrieNode *root, MoTrieNode ***list, int *n, int *cap) {
    int head = *n;
    if (*n >= *cap) {
        *cap = *cap ? *cap * 2 : 16;
        *list = realloc(*list, (size_t)*cap * sizeof(MoTrieNode *));
    }
    (*list)[(*n)++] = root;
    for (int r = head; r < *n; r++) {
        MoTrieNode *nd = (*list)[r];
        for (int e = 0; e < nd->n_edges; e++) {
            if (*n >= *cap) {
                *cap *= 2;
                *list = realloc(*list, (size_t)*cap * sizeof(MoTrieNode *));
            }
            (*list)[(*n)++] = nd->edges[e].child;
        }
    }
}

static size_t mo_trie_node_size(MoTrieNode *nd) {
    size_t sz;
    if (nd->terminal) {
        int tinfo = mo_uleb_len(0) + mo_uleb_len(nd->addr); // flags + addr
        sz = (size_t)mo_uleb_len((uint64_t)tinfo) + (size_t)tinfo;
    } else {
        sz = 1; // terminalSize = 0
    }
    sz += 1; // childCount byte
    for (int e = 0; e < nd->n_edges; e++)
        sz += strlen(nd->edges[e].label) + 1 + (size_t)mo_uleb_len(nd->edges[e].child->offset);
    return sz;
}

static void mo_trie_free(MoTrieNode *nd) {
    for (int e = 0; e < nd->n_edges; e++) {
        free(nd->edges[e].label);
        mo_trie_free(nd->edges[e].child);
    }
    free(nd->edges);
    free(nd);
}

// ---------------------------------------------------------------------------
// SHA-256 (single-shot/streaming, from FIPS 180-4) -- the only consumer is
// the ad-hoc code-signature builder below; this is not a general crypto API.
// Verified against the NIST test vectors (empty string, "abc", and the
// classic one-million-'a' multi-block vector) before integration.
// ---------------------------------------------------------------------------
typedef struct {
    uint32_t h[8];
    uint8_t buf[64];
    size_t buflen;
    uint64_t total;
} MoSha256;

static const uint32_t mo_sha256_k[64] = {
    0x428a2f98,
    0x71374491,
    0xb5c0fbcf,
    0xe9b5dba5,
    0x3956c25b,
    0x59f111f1,
    0x923f82a4,
    0xab1c5ed5,
    0xd807aa98,
    0x12835b01,
    0x243185be,
    0x550c7dc3,
    0x72be5d74,
    0x80deb1fe,
    0x9bdc06a7,
    0xc19bf174,
    0xe49b69c1,
    0xefbe4786,
    0x0fc19dc6,
    0x240ca1cc,
    0x2de92c6f,
    0x4a7484aa,
    0x5cb0a9dc,
    0x76f988da,
    0x983e5152,
    0xa831c66d,
    0xb00327c8,
    0xbf597fc7,
    0xc6e00bf3,
    0xd5a79147,
    0x06ca6351,
    0x14292967,
    0x27b70a85,
    0x2e1b2138,
    0x4d2c6dfc,
    0x53380d13,
    0x650a7354,
    0x766a0abb,
    0x81c2c92e,
    0x92722c85,
    0xa2bfe8a1,
    0xa81a664b,
    0xc24b8b70,
    0xc76c51a3,
    0xd192e819,
    0xd6990624,
    0xf40e3585,
    0x106aa070,
    0x19a4c116,
    0x1e376c08,
    0x2748774c,
    0x34b0bcb5,
    0x391c0cb3,
    0x4ed8aa4a,
    0x5b9cca4f,
    0x682e6ff3,
    0x748f82ee,
    0x78a5636f,
    0x84c87814,
    0x8cc70208,
    0x90befffa,
    0xa4506ceb,
    0xbef9a3f7,
    0xc67178f2,
};

#define MO_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void mo_sha256_block(MoSha256 *c, const uint8_t *p) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) |
            ((uint32_t)p[i * 4 + 2] << 8) | (uint32_t)p[i * 4 + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = MO_ROTR(w[i - 15], 7) ^ MO_ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = MO_ROTR(w[i - 2], 17) ^ MO_ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = c->h[0], b = c->h[1], cc = c->h[2], d = c->h[3];
    uint32_t e = c->h[4], f = c->h[5], g = c->h[6], h = c->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = MO_ROTR(e, 6) ^ MO_ROTR(e, 11) ^ MO_ROTR(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + mo_sha256_k[i] + w[i];
        uint32_t S0 = MO_ROTR(a, 2) ^ MO_ROTR(a, 13) ^ MO_ROTR(a, 22);
        uint32_t maj = (a & b) ^ (a & cc) ^ (b & cc);
        uint32_t t2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = cc;
        cc = b;
        b = a;
        a = t1 + t2;
    }
    c->h[0] += a;
    c->h[1] += b;
    c->h[2] += cc;
    c->h[3] += d;
    c->h[4] += e;
    c->h[5] += f;
    c->h[6] += g;
    c->h[7] += h;
}

static void mo_sha256_init(MoSha256 *c) {
    static const uint32_t iv[8] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19,
    };
    memcpy(c->h, iv, sizeof iv);
    c->buflen = 0;
    c->total = 0;
}

static void mo_sha256_update(MoSha256 *c, const uint8_t *data, size_t len) {
    c->total += len;
    if (c->buflen) {
        size_t take = 64 - c->buflen;
        if (take > len) take = len;
        memcpy(c->buf + c->buflen, data, take);
        c->buflen += take;
        data += take;
        len -= take;
        if (c->buflen == 64) {
            mo_sha256_block(c, c->buf);
            c->buflen = 0;
        }
    }
    while (len >= 64) {
        mo_sha256_block(c, data);
        data += 64;
        len -= 64;
    }
    if (len) {
        memcpy(c->buf, data, len);
        c->buflen = len;
    }
}

static void mo_sha256_final(MoSha256 *c, uint8_t out[32]) {
    uint64_t bitlen = c->total * 8;
    size_t i = c->buflen;
    c->buf[i++] = 0x80;
    if (i > 56) {
        while (i < 64) c->buf[i++] = 0;
        mo_sha256_block(c, c->buf);
        i = 0;
    }
    while (i < 56) c->buf[i++] = 0;
    for (int j = 7; j >= 0; j--) c->buf[i++] = (uint8_t)(bitlen >> (j * 8));
    mo_sha256_block(c, c->buf);
    for (int j = 0; j < 8; j++) {
        out[j * 4] = (uint8_t)(c->h[j] >> 24);
        out[j * 4 + 1] = (uint8_t)(c->h[j] >> 16);
        out[j * 4 + 2] = (uint8_t)(c->h[j] >> 8);
        out[j * 4 + 3] = (uint8_t)(c->h[j]);
    }
}

// ---------------------------------------------------------------------------
// Ad-hoc Mach-O code signature (CSMAGIC_EMBEDDED_SIGNATURE SuperBlob wrapping
// a single CodeDirectory blob, no cryptographic identity). Apple Silicon's
// kernel (AMFI) refuses to map the executable pages of -- and instantly
// SIGKILLs -- any Mach-O binary lacking a valid signature, even a purely
// local one with no trust chain behind it; unlike ELF/PE this check happens
// before user code ever runs. Layout/field values follow the format Apple's
// own linker/codesign_allocate produce for ad-hoc signing, cross-checked
// against the Go toolchain's from-scratch implementation
// (cmd/internal/codesign in the Go source tree), which is proven correct
// against the real kernel loader. All multi-byte fields in the signature
// blob are big-endian, unlike the little-endian Mach-O header/load commands
// above.
// ---------------------------------------------------------------------------
#define MO_CS_PAGE_SIZE 4096
#define MO_CS_PAGE_BITS 12
#define MO_CS_HASH_SIZE 32 // SHA-256
#define MO_CS_SUPERBLOB_SIZE 12 // magic(4) + length(4) + count(4)
#define MO_CS_BLOB_SIZE 8 // type(4) + offset(4)
#define MO_CS_CODEDIR_SIZE 88 // fixed CodeDirectory header, see mo_codesign_sign

#define MO_CSMAGIC_CODEDIRECTORY 0xfade0c02U
#define MO_CSMAGIC_EMBEDDED_SIGNATURE 0xfade0cc0U
#define MO_CSSLOT_CODEDIRECTORY 0U
#define MO_CS_HASHTYPE_SHA256 2U
#define MO_CS_ADHOC 0x00000002U
#define MO_CS_LINKER_SIGNED 0x00020000U
#define MO_CS_EXECSEG_MAIN_BINARY 0x1ULL

static void mo_put32be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}
static void mo_put64be(uint8_t *p, uint64_t v) {
    mo_put32be(p, (uint32_t)(v >> 32));
    mo_put32be(p + 4, (uint32_t)v);
}

// Total byte size of the ad-hoc signature blob for a file whose signed
// content (everything before the signature itself) is `code_size` bytes.
static uint64_t mo_codesign_size(uint64_t code_size, const char *id) {
    uint64_t nhashes = (code_size + MO_CS_PAGE_SIZE - 1) / MO_CS_PAGE_SIZE;
    uint64_t id_off = MO_CS_CODEDIR_SIZE;
    uint64_t hash_off = id_off + strlen(id) + 1;
    uint64_t cdir_sz = hash_off + nhashes * MO_CS_HASH_SIZE;
    return MO_CS_SUPERBLOB_SIZE + MO_CS_BLOB_SIZE + cdir_sz;
}

// Builds the ad-hoc signature blob into `out` (must be
// mo_codesign_size(code_size, id) bytes). Reads the first `code_size` bytes
// already written to `f` (opened "wb+") to compute the per-4KB-page
// SHA-256 hashes the CodeDirectory embeds -- this must run after every
// other byte of the signed range has been written, since it hashes exactly
// what is already on disk. `text_off`/`text_size` are the __TEXT segment's
// file offset/size (the CodeDirectory's execSegBase/execSegLimit, used by
// the kernel to bound the executable range it enforces the signature over).
static void mo_codesign_sign(FILE *f, uint8_t *out, uint64_t code_size, const char *id,
                             uint64_t text_off, uint64_t text_size, bool is_main) {
    uint64_t nhashes = (code_size + MO_CS_PAGE_SIZE - 1) / MO_CS_PAGE_SIZE;
    size_t id_len = strlen(id);
    uint64_t id_off = MO_CS_CODEDIR_SIZE;
    uint64_t hash_off = id_off + id_len + 1;
    uint64_t cdir_sz = hash_off + nhashes * MO_CS_HASH_SIZE;
    uint64_t total = MO_CS_SUPERBLOB_SIZE + MO_CS_BLOB_SIZE + cdir_sz;

    // SuperBlob header
    uint8_t *p = out;
    mo_put32be(p, MO_CSMAGIC_EMBEDDED_SIGNATURE);
    p += 4;
    mo_put32be(p, (uint32_t)total);
    p += 4;
    mo_put32be(p, 1);
    p += 4; // count = 1 (single CodeDirectory blob)

    // BlobIndex[0]: CodeDirectory
    uint32_t cdir_blob_off = MO_CS_SUPERBLOB_SIZE + MO_CS_BLOB_SIZE;
    mo_put32be(p, MO_CSSLOT_CODEDIRECTORY);
    p += 4;
    mo_put32be(p, cdir_blob_off);
    p += 4;

    // CodeDirectory header (88 bytes)
    uint8_t *cd = out + cdir_blob_off;
    mo_put32be(cd + 0, MO_CSMAGIC_CODEDIRECTORY);
    mo_put32be(cd + 4, (uint32_t)cdir_sz); // length of this blob
    mo_put32be(cd + 8, 0x00020400); // version
    mo_put32be(cd + 12, MO_CS_ADHOC | MO_CS_LINKER_SIGNED); // flags
    mo_put32be(cd + 16, (uint32_t)hash_off); // hashOffset
    mo_put32be(cd + 20, (uint32_t)id_off); // identOffset
    mo_put32be(cd + 24, 0); // nSpecialSlots
    mo_put32be(cd + 28, (uint32_t)nhashes); // nCodeSlots
    mo_put32be(cd + 32, (uint32_t)code_size); // codeLimit
    cd[36] = MO_CS_HASH_SIZE; // hashSize
    cd[37] = MO_CS_HASHTYPE_SHA256; // hashType
    cd[38] = 0; // _pad1
    cd[39] = MO_CS_PAGE_BITS; // pageSize = log2(4096)
    mo_put32be(cd + 40, 0); // _pad2
    mo_put32be(cd + 44, 0); // scatterOffset
    mo_put32be(cd + 48, 0); // teamOffset
    mo_put32be(cd + 52, 0); // _pad3
    mo_put64be(cd + 56, code_size); // codeLimit64 (unused when codeLimit fits in 32 bits, but set for consistency)
    mo_put64be(cd + 64, text_off); // execSegBase
    mo_put64be(cd + 72, text_size); // execSegLimit
    mo_put64be(cd + 80, is_main ? MO_CS_EXECSEG_MAIN_BINARY : 0); // execSegFlags

    // Identifier string
    memcpy(cd + id_off, id, id_len + 1);

    // Per-page SHA-256 hashes over the signed range already on disk
    if (fseek(f, 0, SEEK_SET) != 0) return;
    uint8_t page[MO_CS_PAGE_SIZE];
    uint64_t remaining = code_size;
    uint8_t *hp = cd + hash_off;
    while (remaining > 0) {
        size_t chunk = remaining < MO_CS_PAGE_SIZE ? (size_t)remaining : MO_CS_PAGE_SIZE;
        size_t got = fread(page, 1, chunk, f);
        if (got != chunk) break; // short read: leave remaining hashes zeroed (should not happen)
        MoSha256 h;
        mo_sha256_init(&h);
        mo_sha256_update(&h, page, chunk);
        mo_sha256_final(&h, hp);
        hp += MO_CS_HASH_SIZE;
        remaining -= chunk;
    }
}

// ---------------------------------------------------------------------------
// Map Mach-O relocation type to internal RL_ type
// ---------------------------------------------------------------------------
static int mo_map_reloc_arm64(uint32_t r_type) {
    switch (r_type) {
    case ARM64_RELOC_UNSIGNED: return RL_ABS64;
    case ARM64_RELOC_BRANCH26: return RL_ARM64_B26;
    case ARM64_RELOC_PAGE21: return RL_ARM64_ADR_PG;
    case ARM64_RELOC_PAGEOFF12: return RL_ARM64_ADD_LO;
    case ARM64_RELOC_GOT_LOAD_PAGE21: return RL_ARM64_GOT_PG;
    case ARM64_RELOC_GOT_LOAD_PAGEOFF12: return RL_ARM64_GOT_LO;
    default: return -1;
    }
}
static int mo_map_reloc_x86_64(uint32_t r_type) {
    switch (r_type) {
    case X86_64_RELOC_UNSIGNED: return RL_ABS64;
    case X86_64_RELOC_SIGNED: return RL_PC32;
    case X86_64_RELOC_BRANCH: return RL_PC32;
    case X86_64_RELOC_GOT_LOAD: return RL_GOTPCREL;
    case X86_64_RELOC_GOT: return RL_GOTPCREL;
    case X86_64_RELOC_SIGNED_1: return RL_PC32;
    case X86_64_RELOC_SIGNED_2: return RL_PC32;
    case X86_64_RELOC_SIGNED_4: return RL_PC32;
    default: return -1;
    }
}

// ---------------------------------------------------------------------------
// Mach-O object file loader
// ---------------------------------------------------------------------------

int link_load_object(LinkState *s, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "rcc: link: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }
    uint8_t *image = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (image == MAP_FAILED) return -1;
    if (st.st_size < 32) {
        munmap(image, (size_t)st.st_size);
        return -1;
    }

    uint32_t magic = mo_r32(image);
    uint32_t cpu_type = mo_r32(image + 4);
    uint32_t filetype = mo_r32(image + 12);
    uint32_t ncmds = mo_r32(image + 16);
    uint32_t sizeofcmds = mo_r32(image + 20);
    (void)sizeofcmds;

    if (magic != MH_MAGIC_64 || filetype != MH_OBJECT) {
        fprintf(stderr, "rcc: link: %s: not a Mach-O 64 relocatable object\n", path);
        munmap(image, (size_t)st.st_size);
        return -1;
    }
    if (cpu_type != CPU_TYPE_ARM64 && cpu_type != CPU_TYPE_X86_64) {
        fprintf(stderr, "rcc: link: %s: unsupported cpu type 0x%x\n", path, cpu_type);
        munmap(image, (size_t)st.st_size);
        return -1;
    }

    // Track object path for diagnostics only -- image stays local to this
    // function (unlike ELF's LinkObj tracking, which keeps its own copy).
    // mmap()'d memory must be released with munmap(), never free(); handing
    // a live mmap pointer to link_state_free()'s free(s->objs[i].image)
    // corrupts the heap allocator (observed as an immediate SIGABRT/SIGSEGV
    // in free() on the next allocation, since every byte of this object's
    // data has already been copied out via link_sec_append()/link_add_sym()
    // by the time this function returns -- the mapping is not needed after).
    LinkObj obj = {.path = strdup(path), .image = NULL, .image_size = 0};
    if (s->n_objs == s->cap_objs) {
        s->cap_objs = s->cap_objs ? s->cap_objs * 2 : 4;
        s->objs = realloc(s->objs, (size_t)s->cap_objs * sizeof(LinkObj));
    }
    s->objs[s->n_objs++] = obj;

    // Parse load commands
    const uint8_t *lc = image + 32;
    int *sec_map = NULL; // section index → output section index
    int n_sections = 0;
    int *sym_map = NULL; // symbol index → LinkSym index
    int n_syms = 0;
    // Byte offset within the (possibly already non-empty, shared-by-name)
    // output section where each of THIS object's own Mach-O sections
    // begins -- see the identical `out_sec_off` comment in link_pe.c's
    // link_load_object(): a later-loaded object's bytes are appended
    // AFTER an earlier object's, so a symbol's raw nlist value (an
    // offset within its OWN object's un-merged section) must be rebased
    // by this amount to become a correct offset within the merged
    // output section. Grown alongside `sec_map` below, same indexing.
    uint64_t *sec_base_off = NULL;
    // Snapshot every already-loaded output section's reloc count before
    // this object contributes any of its own, so the remap pass below
    // can scope itself to only the relocations THIS object just added --
    // `s->secs[]` is a single array shared (by section name) across
    // every loaded object, so remapping the whole thing on every load
    // would reinterpret an earlier object's already-resolved global
    // symbol indices as raw indices into this object's unrelated
    // symbol table.
    int old_n_secs = s->n_secs;
    int *reloc_base = calloc((size_t)old_n_secs, sizeof(int));
    for (int i = 0; i < old_n_secs; i++) reloc_base[i] = s->secs[i].n_relocs;

    for (uint32_t i = 0; i < ncmds; i++) {
        uint32_t cmd = mo_r32(lc);
        uint32_t cmdsize = mo_r32(lc + 4);
        if (cmdsize < 8) break;

        if (cmd == LC_SEGMENT_64) {
            char segname[17] = {0};
            memcpy(segname, lc + 8, 16);
            uint32_t nsects = mo_r32(lc + 64);
            // section_64 entries start at lc+72, each 80 bytes
            const uint8_t *sc = lc + 72;

            for (uint32_t j = 0; j < nsects; j++) {
                char sectname[17] = {0}, segname_s[17] = {0};
                memcpy(sectname, sc, 16);
                memcpy(segname_s, sc + 16, 16);
                // section_64 layout: sectname[16]+segname[16] then addr(8)
                // at +32, size(8) at +40, offset(4) at +48, align(4) at +52,
                // reloff(4) at +56, nreloc(4) at +60, flags(4) at +64. This
                // previously read size/offset/align/reloff/nreloc/flags all
                // 8 bytes too early (starting from the always-zero `addr`
                // field instead of skipping past it), making every
                // section's observed size 0 -- the writer's section data
                // was silently dropped from every linked executable.
                uint64_t size = mo_r64(sc + 40);
                uint32_t offset = mo_r32(sc + 48);
                uint32_t align_p2 = mo_r32(sc + 52);
                uint32_t reloff = mo_r32(sc + 56);
                uint32_t nreloc = mo_r32(sc + 60);
                uint32_t flags = mo_r32(sc + 64);
                (void)align_p2;

                bool is_text = (strcmp(segname_s, "__TEXT") == 0 &&
                                strcmp(sectname, "__text") == 0);
                bool exec = is_text; // only __text is executable
                bool write = (strcmp(segname_s, "__DATA") == 0);
                bool is_bss = (flags & S_ZEROFILL) != 0;

                // Map section name to output section. A section whose
                // *sectname* half doesn't start with "__" is never one of
                // Apple's own reserved system sections (__cstring,
                // __common, __la_symbol_ptr, ...  -- all "__"-prefixed by
                // convention) -- it can only be a user's own
                // __attribute__((section("SEG,SECT"))) global (e.g.
                // "__DATA,const_heap"). Keep that distinct instead of
                // folding it into the generic .data/.rdata bucket below,
                // so section$start$SEG$SECT/section$end$SEG$SECT
                // (resolved in link_macho()) bracket only its own
                // contribution -- not every other unrelated .data global.
                bool is_custom_sect = !is_text &&
                    (sectname[0] != '_' || sectname[1] != '_');
                char out_name[34];
                if (is_text) snprintf(out_name, sizeof(out_name), ".text");
                else if (is_custom_sect)
                    snprintf(out_name, sizeof(out_name), "%s,%s", segname_s, sectname);
                else if (is_bss)
                    snprintf(out_name, sizeof(out_name), ".bss");
                else if (strcmp(sectname, "__const") == 0)
                    snprintf(out_name, sizeof(out_name), ".rodata");
                else if (strcmp(sectname, "__mod_init_func") == 0)
                    snprintf(out_name, sizeof(out_name), ".init_array");
                else if (write)
                    snprintf(out_name, sizeof(out_name), ".data");
                else
                    snprintf(out_name, sizeof(out_name), ".rdata");

                size_t sec_align = 1u << (align_p2 > 0 ? align_p2 : 4);
                int out_idx = link_find_or_create_sec(s, out_name, true, write, exec,
                                                      is_bss, false, sec_align);
                sec_map = realloc(sec_map, (size_t)(n_sections + 1) * sizeof(int));
                sec_map[n_sections] = out_idx;
                sec_base_off = realloc(sec_base_off, (size_t)(n_sections + 1) * sizeof(uint64_t));

                if (!is_bss && size > 0 && offset > 0) {
                    uint64_t base_off = link_sec_append(s, out_idx,
                                                        image + offset, (size_t)size, 16);
                    sec_base_off[n_sections] = base_off;
                    // Parse relocations
                    for (uint32_t k = 0; k < nreloc; k++) {
                        // Mach-O relocation entry: 8 bytes (int32 addr, uint32 sym+type)
                        const uint8_t *rp = image + reloff + k * 8;
                        int32_t r_addr = (int32_t)mo_r32(rp);
                        uint32_t r_symtype = mo_r32(rp + 4);
                        int r_sym = (int)(r_symtype & 0xFFFFFF);
                        uint32_t r_type = (r_symtype >> 28) & 0xF; // 4-bit type in bits 28-31
                        int rl = -1;
                        if (cpu_type == CPU_TYPE_ARM64)
                            rl = mo_map_reloc_arm64(r_type);
                        else
                            rl = mo_map_reloc_x86_64(r_type);
                        if (rl < 0) continue;
                        link_add_reloc(s, out_idx, base_off + (uint64_t)r_addr,
                                       (uint32_t)rl, r_sym, 0);
                    }
                } else if (is_bss && size > 0) {
                    sec_base_off[n_sections] = s->secs[out_idx].len;
                    s->secs[out_idx].len += (size_t)size;
                } else {
                    sec_base_off[n_sections] = s->secs[out_idx].len;
                }
                n_sections++;
                sc += 80;
            }
        } else if (cmd == LC_SYMTAB) {
            uint32_t symoff = mo_r32(lc + 8);
            uint32_t nsyms = mo_r32(lc + 12);
            uint32_t stroff = mo_r32(lc + 16);

            sym_map = calloc((size_t)nsyms, sizeof(int));
            n_syms = (int)nsyms;
            const uint8_t *symbase = image + symoff;
            const char *strbase = (const char *)(image + stroff);

            for (uint32_t j = 0; j < nsyms; j++) {
                const uint8_t *nl = symbase + j * 16; // nlist_64 = 16 bytes
                uint32_t n_strx = mo_r32(nl);
                uint8_t n_type = nl[4];
                uint8_t n_sect = nl[5];
                int16_t n_desc = (int16_t)((nl[6] << 8) | nl[7]);
                uint64_t n_value = mo_r64(nl + 8);
                (void)n_desc;

                const char *name = strbase + n_strx;
                if (!name[0]) {
                    sym_map[j] = -1;
                    continue;
                }

                int bind, type, out_sec;
                uint8_t nt = n_type & N_TYPE;
                if (nt == N_UNDF) {
                    bind = (n_type & N_EXT) ? 1 : 0;
                    type = 0;
                    out_sec = -1;
                } else if (nt == N_SECT && n_sect > 0 && n_sect <= n_sections) {
                    bind = (n_type & N_EXT) ? 1 : 0;
                    type = 0;
                    out_sec = sec_map[n_sect - 1];
                } else if (nt == N_ABS) {
                    bind = (n_type & N_EXT) ? 1 : 0;
                    type = 0;
                    out_sec = -1;
                    n_value = 0;
                } else {
                    sym_map[j] = -1;
                    continue;
                }

                uint64_t sym_value = (out_sec >= 0) ? n_value + sec_base_off[n_sect - 1] : 0;
                int sym_idx = link_add_sym(s, name, out_sec, sym_value,
                                           0, bind, type, -1);
                sym_map[j] = sym_idx;
            }
        }
        lc += cmdsize;
    }

    // Re-map relocations from object symbol index to LinkSym index. Scoped
    // to just the relocations THIS object added -- see reloc_base's
    // comment above for why remapping the whole shared sec->relocs[]
    // array on every object load would corrupt an earlier object's
    // already-resolved cross-object relocations.
    if (sym_map) {
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            int start = (i < old_n_secs) ? reloc_base[i] : 0;
            for (int j = start; j < sec->n_relocs; j++) {
                int ms = sec->relocs[j].sym;
                if (ms >= 0 && ms < n_syms)
                    sec->relocs[j].sym = sym_map[ms];
            }
        }
        free(sym_map);
    }
    free(reloc_base);
    free(sec_base_off);
    free(sec_map);
    munmap(image, (size_t)st.st_size);
    return 0;
}

// ---------------------------------------------------------------------------
// Helper: symbol address for relocation resolution
// ---------------------------------------------------------------------------
static uint64_t mo_symbol_address(LinkState *s, int idx) {
    LinkSym *sym = &s->syms[idx];
    if (sym->sec >= 0) return s->secs[sym->sec].addr + sym->value;
    return sym->value;
}

// ---------------------------------------------------------------------------
// Mach-O executable writer
// ---------------------------------------------------------------------------

int link_macho(LinkState *s) {
    // Static linking is not implemented — fall back to the external linker.
    if (s->opt_static) return -1;
    bool is_dylib = s->opt_shared;

    // Synthesize section$start$SEG$SECT/section$end$SEG$SECT boundary
    // symbols -- ld64/clang's own convention for `extern char x[]
    // __asm("section$start$__DATA$const_heap")`-style boundary markers
    // (see test/test_attribute_section.c's Apple branch), the Mach-O
    // equivalent of ELF's __start_/__stop_ synthesis in link_elf.c.
    // Must run before the external-symbol-strategy scan below: an
    // unresolved section$start$/section$end$ reference is loaded via a
    // GOT-relative instruction (extern char[] has no known size), which
    // would otherwise look like a genuinely external symbol and bounce
    // this whole link to the system linker.
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec >= 0 || !sym->name) continue;
        bool is_start = !strncmp(sym->name, "section$start$", 14);
        bool is_stop = !is_start && !strncmp(sym->name, "section$end$", 12);
        if (!is_start && !is_stop) continue;
        const char *rest = sym->name + (is_start ? 14 : 12);
        const char *dollar = strchr(rest, '$');
        if (!dollar || !dollar[1]) continue;
        size_t seglen = (size_t)(dollar - rest);
        size_t sectlen = strlen(dollar + 1);
        if (seglen > 16 || sectlen > 16) continue;
        char segsect[34];
        memcpy(segsect, rest, seglen);
        segsect[seglen] = ',';
        memcpy(segsect + seglen + 1, dollar + 1, sectlen);
        segsect[seglen + 1 + sectlen] = '\0';
        int target = -1;
        for (int j = 0; j < s->n_secs; j++) {
            if (!strcmp(s->secs[j].name, segsect)) {
                target = j;
                break;
            }
        }
        if (target < 0) continue;
        sym->sec = target;
        sym->value = is_start ? 0 : s->secs[target].len;
        sym->resolved = true;
    }
    // External-symbol strategy:
    //   * dylibs (-shared) bind natively via the __got + dyld bind opcodes
    //     built below -- this is the path the -shared benchmark (sqlite) and
    //     dylib exports need, and it works because such libraries import only
    //     libSystem (ordinal 1, what the bind opcodes assume).
    //   * executables still defer to the external linker whenever a genuinely
    //     external symbol is referenced: binding every undefined symbol to
    //     libSystem ordinal 1 is only correct when the symbol truly lives
    //     there, which cannot be verified here (e.g. on_exit, or a symbol
    //     that another linked dylib provides).  The external linker resolves
    //     those robustly and reports real errors.
    if (!is_dylib) {
        for (int si = 0; si < s->n_secs; si++) {
            LinkSec *sec = &s->secs[si];
            for (int rj = 0; rj < sec->n_relocs; rj++) {
                LinkReloc *r = &sec->relocs[rj];
                if (r->sym < 0 || s->syms[r->sym].sec >= 0) continue;
                if (r->type == RL_ARM64_B26 || r->type == RL_PC32_PLT ||
                    r->type == RL_ARM64_GOT_PG || r->type == RL_ARM64_GOT_LO ||
                    r->type == RL_GOTPCREL)
                    return -1;
            }
        }
    }
    // Build GOT and PLT for external symbols.
    int got_sec = link_find_or_create_sec(s, ".got", true, true, false, false, false, 8);
    int stubs_sec = link_find_or_create_sec(s, ".stubs", true, false, true, false, false, 16);
    // got_off[sym_idx] = offset in .got, or -1. stub_off[sym_idx] = offset in .stubs.
    int *got_off = calloc((size_t)s->n_syms, sizeof(int));
    int *stub_off = calloc((size_t)s->n_syms, sizeof(int));
    for (int i = 0; i < s->n_syms; i++) got_off[i] = -1;
    for (int i = 0; i < s->n_syms; i++) stub_off[i] = -1;
    int n_got = 0, n_stubs = 0;
    for (int si = 0; si < s->n_secs; si++) {
        LinkSec *sec = &s->secs[si];
        for (int rj = 0; rj < sec->n_relocs; rj++) {
            LinkReloc *r = &sec->relocs[rj];
            if (r->sym < 0) continue;
            // GOT entry needed for ANY symbol referenced via GOT_PG/GOT_LO.
            if ((r->type == RL_ARM64_GOT_PG || r->type == RL_ARM64_GOT_LO ||
                 r->type == RL_GOTPCREL) &&
                got_off[r->sym] < 0) {
                got_off[r->sym] = n_got * 8;
                uint8_t z[8] = {0};
                link_sec_append(s, got_sec, z, 8, 8);
                n_got++;
            }
            // PLT stub for external BRANCH26 only.
            LinkSym *sym = &s->syms[r->sym];
            if (sym->sec >= 0) continue;
            if ((r->type == RL_ARM64_B26 || r->type == RL_PC32_PLT) &&
                stub_off[r->sym] < 0) {
                // PLT stub needs a GOT entry for the lazy bind pointer.
                if (got_off[r->sym] < 0) {
                    got_off[r->sym] = n_got * 8;
                    uint8_t z[8] = {0};
                    link_sec_append(s, got_sec, z, 8, 8);
                    n_got++;
                }
                stub_off[r->sym] = n_stubs * 12;
                uint8_t z[16] = {0};
                link_sec_append(s, stubs_sec, z, 12, 16);
                n_stubs++;
            }
        }
    }
    // Create standard sections
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 8);
    link_find_or_create_sec(s, ".rodata", true, false, false, false, false, 8);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 8);
    link_find_or_create_sec(s, ".init_array", true, true, false, false, false, 8);

    uint64_t base = is_dylib ? 0 : 0x100000000ULL;

    // Section listing and address assignment. This -- not the shared,
    // ELF/PE-oriented link_layout() (which spreads text/rodata/data across
    // separate page-aligned regions) -- is Mach-O's real segment model:
    // __TEXT houses every executable and read-only section packed
    // 16-byte-aligned, __DATA houses everything else the same way.
    // Addresses must be assigned here, before relocations and the entry
    // point are resolved: link_apply_relocs()/mo_symbol_address() read
    // sec->addr, and this file's own writer below places every section
    // exactly where this loop says it goes (identical arithmetic, so the
    // two can never disagree). The previous code called the shared
    // link_layout() for this instead, then only computed these real
    // addresses afterward purely for the writer -- relocations and the
    // entry point silently used link_layout()'s incompatible addresses,
    // corrupting every reference into .text/.rodata/.data (a jump to
    // __PAGEZERO for the entry point in the simplest case).
    typedef struct {
        LinkSec *sec;
        const char *segname;
        const char *sectname;
    } MOSec;
    MOSec *mo_secs = NULL;
    int n_mo = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;
        mo_secs = realloc(mo_secs, (size_t)(n_mo + 1) * sizeof(MOSec));
        if (strcmp(sec->name, ".got") == 0) {
            mo_secs[n_mo].segname = "__DATA";
            mo_secs[n_mo].sectname = "__got";
        } else if (strcmp(sec->name, ".stubs") == 0) {
            mo_secs[n_mo].segname = "__TEXT";
            mo_secs[n_mo].sectname = "__stubs";
        } else if (sec->exec) {
            mo_secs[n_mo].segname = "__TEXT";
            mo_secs[n_mo].sectname = "__text";
        } else if (strcmp(sec->name, ".rodata") == 0 ||
                   strcmp(sec->name, ".rdata") == 0) {
            mo_secs[n_mo].segname = "__TEXT";
            mo_secs[n_mo].sectname = "__const";
        } else if (strcmp(sec->name, ".init_array") == 0) {
            mo_secs[n_mo].segname = "__DATA";
            mo_secs[n_mo].sectname = "__mod_init_func";
        } else if (sec->is_bss) {
            mo_secs[n_mo].segname = "__DATA";
            mo_secs[n_mo].sectname = "__bss";
        } else if (strchr(sec->name, ',')) {
            // A user's own __attribute__((section("SEG,SECT"))) global,
            // preserved verbatim from link_load_object()'s is_custom_sect
            // branch above (macho_write.c emits the identical "SEG,SECT"
            // internal section name for the same source construct) --
            // keep it distinct instead of collapsing into __DATA,__data
            // so section$start$SEG$SECT/section$end$SEG$SECT (resolved
            // above) bracket only this section's own contribution.
            const char *comma = strchr(sec->name, ',');
            size_t seglen = (size_t)(comma - sec->name);
            char *sg = calloc(1, 17);
            memcpy(sg, sec->name, seglen > 16 ? 16 : seglen);
            char *sn = calloc(1, 17);
            size_t sectlen = strlen(comma + 1);
            memcpy(sn, comma + 1, sectlen > 16 ? 16 : sectlen);
            mo_secs[n_mo].segname = sg;
            mo_secs[n_mo].sectname = sn;
        } else {
            mo_secs[n_mo].segname = "__DATA";
            mo_secs[n_mo].sectname = "__data";
        }
        mo_secs[n_mo].sec = sec;
        n_mo++;
    }
    // Order mo_secs so the write/layout passes see every __TEXT section
    // first, then __DATA non-bss, then __DATA bss.  Two independent reasons:
    //  - The section-data write pass walks mo_secs linearly and switches from
    //    __TEXT to __DATA exactly once; a __TEXT section (e.g. __stubs, which
    //    is created after the __DATA __got) appearing after a __DATA section
    //    would be written into the __DATA file region, mismatching the
    //    segment/section headers (which group by segment) -> the stub bytes
    //    land at __got's file offset and the real __stubs stays zero (SIGILL).
    //  - dyld maps a segment's file range contiguously onto its VM range, so
    //    a zerofill __bss must sit at the tail of __DATA (segment vmsize >
    //    filesize); a __bss before a real __DATA section (e.g. __got) desyncs
    //    following file offsets from vmaddrs.
    if (n_mo > 1) {
        MOSec *ord = malloc((size_t)n_mo * sizeof(MOSec));
        int w = 0;
        for (int i = 0; i < n_mo; i++)
            if (strcmp(mo_secs[i].segname, "__TEXT") == 0) ord[w++] = mo_secs[i];
        for (int i = 0; i < n_mo; i++)
            if (strcmp(mo_secs[i].segname, "__TEXT") != 0 && !mo_secs[i].sec->is_bss)
                ord[w++] = mo_secs[i];
        for (int i = 0; i < n_mo; i++)
            if (strcmp(mo_secs[i].segname, "__TEXT") != 0 && mo_secs[i].sec->is_bss)
                ord[w++] = mo_secs[i];
        free(mo_secs);
        mo_secs = ord;
    }


    // Identify undefined symbols for dynamic linking
    // Identify undefined symbols (for dynamic linking) and defined globals (for dylib export).
    int n_undef = 0, n_defsym = 0;
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (!sym->name || !sym->name[0]) continue;
        if (sym->sec < 0) n_undef++;
        else if (sym->bind == 1)
            n_defsym++;
    }

    // Header size computation
    uint32_t nsects_text = 0, nsects_data = 0;
    for (int i = 0; i < n_mo; i++) {
        if (strcmp(mo_secs[i].segname, "__TEXT") == 0) nsects_text++;
        else
            nsects_data++;
    }
    uint32_t text_lc_size = 72 + nsects_text * 80;
    uint32_t data_lc_size = 72 + nsects_data * 80;
    uint32_t lc_build_version = 24;
    uint32_t lc_main = 24;
    // Must match the padded size link_macho() actually writes below for
    // LC_LOAD_DYLIB, byte for byte -- this feeds cmds_end/text_fileoff,
    // which every later file offset (section data, symtab, strtab) is
    // computed from. A stale hardcoded constant here desyncs those offsets
    // from what the LC_SEGMENT_64 section headers claim, corrupting the
    // whole file layout (ld64/dyld reject it outright; observed as
    // "malformed object" / an immediate crash on load).
    uint32_t lc_symtab = 24;
    uint32_t lc_dysymtab = 80;
    // LC_LOAD_DYLINKER: dylinker_command (cmd/cmdsize/name.offset) = 12 bytes
    // plus the padded string "/usr/lib/dyld".
    uint32_t lc_dylinker = (uint32_t)mo_align(12 + strlen("/usr/lib/dyld") + 1, 8);
    uint32_t lc_dylib = (uint32_t)mo_align(24 + strlen("/usr/lib/libSystem.B.dylib") + 1, 8);
    uint32_t lc_dyld_info = 48; // dyld_info_command
    // LC_DYLD_EXPORTS_TRIE: a tiny export trie with a single terminal node
    // (two zero bytes: \0 terminal-info-size \0).  dyld expects this.
    uint32_t lc_export_trie = 16; // linkedit_data_command
    uint32_t lc_uuid = 24; // uuid_command: cmd+cmdsize+16-byte uuid
    uint32_t lc_id_dylib = is_dylib ? (uint32_t)mo_align(24 + strlen(s->out_path) + 1, 8) : 0;
    bool has_data = nsects_data > 0;
    uint32_t ncmds = is_dylib ? 2 : 3; // TEXT, LINKEDIT (+PAGEZERO for exec)
    if (has_data) ncmds += 1; // DATA
    ncmds += 1; // LC_SYMTAB
    ncmds += 1; // LC_DYSYMTAB
    ncmds += 1; // LC_LOAD_DYLIB
    if (!is_dylib) ncmds += 1; // LC_LOAD_DYLINKER
    if (is_dylib) ncmds += 1; // LC_ID_DYLIB
    ncmds += 1; // LC_UUID
    ncmds += 1; // LC_BUILD_VERSION
    if (!is_dylib) ncmds += 1; // LC_MAIN
    ncmds += 1; // LC_DYLD_EXPORTS_TRIE
    ncmds += 1; // LC_DYLD_INFO_ONLY
    ncmds += 1; // LC_CODE_SIGNATURE
    uint32_t lc_pagezero = is_dylib ? 0 : 72;
    uint32_t lc_linkedit = 72;
    uint32_t lc_codesig_cmd = 16;

    // Bind opcodes are built into a buffer after the layout assigns GOT
    // addresses (see below); bind_size is the exact byte length then.
    uint8_t *bind_data = NULL;
    uint32_t bind_size = 0;

    uint32_t header_size = 32;
    uint32_t total_lc = lc_pagezero + lc_linkedit + text_lc_size;
    if (has_data) total_lc += data_lc_size;
    total_lc += lc_build_version + (is_dylib ? 0 : lc_main) + lc_dylib;
    total_lc += lc_symtab + lc_dysymtab + lc_codesig_cmd;
    total_lc += (is_dylib ? 0 : lc_dylinker) + lc_export_trie + lc_uuid + lc_id_dylib;
    total_lc += lc_dyld_info;

    // --- Layout: assign vm addresses ---
    uint64_t text_vmaddr = base; // encompass headers in __TEXT
    uint64_t text_vmsize = 0, data_vmsize = 0;
    {
        // Start section data after the padded Mach-O header + load commands,
        // same offset in file and vmaddr (both relative to __TEXT start).
        uint64_t hdr_pad = mo_align(header_size + total_lc, 16);
        uint64_t cur_vm = text_vmaddr + hdr_pad;
        for (int i = 0; i < n_mo; i++) {
            if (strcmp(mo_secs[i].segname, "__TEXT") != 0) continue;
            mo_secs[i].sec->addr = cur_vm;
            uint64_t sz = mo_align(mo_secs[i].sec->len, 16);
            cur_vm += sz;
            text_vmsize += sz;
        }
        text_vmsize += hdr_pad; // __TEXT vmsize covers headers + section data
    }
    uint64_t data_vmaddr = mo_align(text_vmaddr + text_vmsize, 0x4000);
    {
        uint64_t cur_vm = data_vmaddr;
        for (int i = 0; i < n_mo; i++) {
            if (strcmp(mo_secs[i].segname, "__TEXT") == 0) continue;
            mo_secs[i].sec->addr = cur_vm;
            uint64_t sz = mo_align(mo_secs[i].sec->len, 16);
            cur_vm += sz;
            data_vmsize += sz;
        }
    }

    // Build rebase opcodes for absolute in-image pointers stored in __DATA
    // (file-scope initializers like `T *p = &g;`, encoded as RL_ABS64 relocs
    // against a defined symbol).  A PIE executable / dylib is ASLR-slid at
    // load, so dyld must add the slide to every such stored pointer; without
    // a rebase entry the pointer keeps its unslid link-time value and
    // dereferencing it faults.  (External data pointers, sym->sec < 0, would
    // need a bind instead and are not emitted here.)
    uint8_t *rebase_data = NULL;
    uint32_t rebase_size = 0;
    if (has_data) {
        int n_reb = 0;
        for (int si = 0; si < s->n_secs; si++) {
            LinkSec *sec = &s->secs[si];
            for (int rj = 0; rj < sec->n_relocs; rj++) {
                LinkReloc *r = &sec->relocs[rj];
                if (r->type != RL_ABS64 || r->sym < 0) continue;
                if (s->syms[r->sym].sec < 0) continue; // external -> bind
                if (sec->addr + r->offset < data_vmaddr) continue; // not __DATA
                n_reb++;
            }
        }
        if (n_reb > 0) {
            uint64_t *offs = malloc((size_t)n_reb * sizeof(uint64_t));
            int k = 0;
            for (int si = 0; si < s->n_secs; si++) {
                LinkSec *sec = &s->secs[si];
                for (int rj = 0; rj < sec->n_relocs; rj++) {
                    LinkReloc *r = &sec->relocs[rj];
                    if (r->type != RL_ABS64 || r->sym < 0) continue;
                    if (s->syms[r->sym].sec < 0) continue;
                    uint64_t a = sec->addr + r->offset;
                    if (a < data_vmaddr) continue;
                    offs[k++] = a - data_vmaddr; // offset within __DATA segment
                }
            }
            // sort ascending (simple insertion sort; counts are small)
            for (int i = 1; i < n_reb; i++) {
                uint64_t v = offs[i];
                int j = i - 1;
                while (j >= 0 && offs[j] > v) {
                    offs[j + 1] = offs[j];
                    j--;
                }
                offs[j + 1] = v;
            }
            // Worst case: 1 type + n*(1 seg op + 5 ULEB + 1 do) + 1 done.
            rebase_data = malloc(2 + (size_t)n_reb * 7);
            uint8_t *w = rebase_data;
            *w++ = 0x11; // REBASE_OPCODE_SET_TYPE_IMM | REBASE_TYPE_POINTER
            uint8_t seg = (uint8_t)(is_dylib ? 1 : 2); // TEXT(,PAGEZERO),DATA
            for (int i = 0; i < n_reb; i++) {
                *w++ = (uint8_t)(0x20 | seg); // SET_SEGMENT_AND_OFFSET_ULEB
                uint64_t o = offs[i];
                do {
                    uint8_t b = (uint8_t)(o & 0x7f);
                    o >>= 7;
                    if (o) b |= 0x80;
                    *w++ = b;
                } while (o);
                *w++ = 0x51; // REBASE_OPCODE_DO_REBASE_IMM_TIMES | 1
            }
            *w++ = 0x00; // REBASE_OPCODE_DONE
            rebase_size = (uint32_t)(w - rebase_data);
            free(offs);
        }
    }

    // Build bind opcodes: tell dyld to fill each external GOT slot with the
    // imported symbol's address at load time.  Correct BIND_OPCODE values
    // (the earlier constants were wrong, producing a garbage stream dyld
    // asserted on):
    //   0x10|ord  SET_DYLIB_ORDINAL_IMM (ord 1 = libSystem)
    //   0x40|flg  SET_SYMBOL_TRAILING_FLAGS_IMM, then name\0
    //   0x50|typ  SET_TYPE_IMM (typ 1 = BIND_TYPE_POINTER)
    //   0x70|seg  SET_SEGMENT_AND_OFFSET_ULEB, then ULEB(offset in segment)
    //   0x90      DO_BIND
    //   0x00      BIND_OPCODE_DONE
    {
        int n_bind = 0;
        for (int i = 0; i < s->n_syms; i++) {
            if (got_off[i] < 0 || s->syms[i].sec >= 0) continue;
            n_bind++;
        }
        if (n_bind > 0) {
            uint8_t seg = (uint8_t)(is_dylib ? 1 : 2); // __DATA segment index
            size_t cap = 1;
            for (int i = 0; i < s->n_syms; i++) {
                if (got_off[i] < 0 || s->syms[i].sec >= 0) continue;
                cap += 6 + strlen(s->syms[i].name) + 1 + 5; // opcodes + name + ULEB
            }
            bind_data = malloc(cap);
            uint8_t *w = bind_data;
            for (int i = 0; i < s->n_syms; i++) {
                if (got_off[i] < 0 || s->syms[i].sec >= 0) continue;
                LinkSym *sym = &s->syms[i];
                *w++ = 0x11; // SET_DYLIB_ORDINAL_IMM | 1
                *w++ = 0x40; // SET_SYMBOL_TRAILING_FLAGS_IMM | flags(0)
                size_t nl = strlen(sym->name) + 1;
                memcpy(w, sym->name, nl);
                w += nl;
                *w++ = 0x51; // SET_TYPE_IMM | BIND_TYPE_POINTER
                *w++ = (uint8_t)(0x70 | seg); // SET_SEGMENT_AND_OFFSET_ULEB
                uint64_t off = (s->secs[got_sec].addr + (uint64_t)got_off[i]) - data_vmaddr;
                do {
                    uint8_t b = (uint8_t)(off & 0x7f);
                    off >>= 7;
                    if (off) b |= 0x80;
                    *w++ = b;
                } while (off);
                *w++ = 0x90; // DO_BIND
            }
            *w++ = 0x00; // BIND_OPCODE_DONE
            bind_size = (uint32_t)(w - bind_data);
        }
    }

    // Resolve GOT/PLT relocations to GOT entries and PLT stubs.
    {
        uint64_t got_addr = s->secs[got_sec].addr;
        uint64_t stubs_addr = s->secs[stubs_sec].addr;
        for (int si = 0; si < s->n_secs; si++) {
            LinkSec *sec = &s->secs[si];
            for (int rj = 0; rj < sec->n_relocs; rj++) {
                LinkReloc *r = &sec->relocs[rj];
                if (r->sym < 0) continue;
                uint8_t *p = sec->data + r->offset;
                uint64_t pc = sec->addr + r->offset;
                if (r->type == RL_ARM64_GOT_PG || r->type == RL_ARM64_GOT_LO) {
                    LinkSym *sym = &s->syms[r->sym];
                    if (sym->sec >= 0 && got_off[r->sym] >= 0) {
                        // Local symbol: bypass GOT, resolve to direct address.
                        uint64_t tgt = s->secs[sym->sec].addr + sym->value;
                        if (r->type == RL_ARM64_GOT_PG) {
                            uint32_t ins = mo_r32(p);
                            int64_t d = (int64_t)(tgt - (pc & ~(uint64_t)0xfff));
                            int64_t imm = d >> 12;
                            ins = (ins & 0x9f00001f) |
                                ((uint32_t)(imm & 3) << 29) |
                                ((uint32_t)((imm >> 2) & 0x7ffff) << 5);
                            mo_w32le(p, ins);
                        } else {
                            // Convert LDR to ADD xN, xN, #offset
                            uint32_t ins = mo_r32(p);
                            uint32_t rd = ins & 0x1F;
                            uint32_t rn = (ins >> 5) & 0x1F;
                            ins = 0x91000000 | ((uint32_t)(tgt & 0xfff) << 10) | (rn << 5) | rd;
                            mo_w32le(p, ins);
                        }
                        r->type = 0;
                    } else if (got_off[r->sym] >= 0) {
                        // External symbol: point to GOT entry.
                        uint64_t tgt = got_addr + (uint64_t)got_off[r->sym];
                        if (r->type == RL_ARM64_GOT_PG) {
                            uint32_t ins = mo_r32(p);
                            int64_t d = (int64_t)(tgt - (pc & ~(uint64_t)0xfff));
                            int64_t imm = d >> 12;
                            ins = (ins & 0x9f00001f) |
                                ((uint32_t)(imm & 3) << 29) |
                                ((uint32_t)((imm >> 2) & 0x7ffff) << 5);
                            mo_w32le(p, ins);
                        } else {
                            uint32_t ins = mo_r32(p);
                            ins = (ins & 0xffc003ff) |
                                ((uint32_t)((tgt & 0xfff) >> 3) << 10);
                            mo_w32le(p, ins);
                        }
                        r->type = 0;
                    }
                }
                if (stub_off[r->sym] >= 0 && r->type == RL_ARM64_B26) {
                    uint64_t tgt = stubs_addr + (uint64_t)stub_off[r->sym];
                    uint32_t ins = mo_r32(p);
                    int64_t d = (int64_t)(tgt - pc);
                    d >>= 2;
                    ins = (ins & ~0x03ffffffu) | ((uint32_t)(d & 0x03ffffffu));
                    mo_w32le(p, ins);
                    r->type = 0;
                }
            }
        }
    }

    link_apply_relocs(s, 0);

    // Write PLT stub bodies now that GOT entry addresses are known.
    // Each stub: adrp x16, GOT_page; ldr x16, [x16, #off]; br x16
    {
        uint64_t got_addr = s->secs[got_sec].addr;
        uint64_t stubs_addr = s->secs[stubs_sec].addr;
        uint8_t *sd = s->secs[stubs_sec].data;
        for (int i = 0; i < s->n_syms; i++) {
            if (stub_off[i] < 0 || got_off[i] < 0) continue;
            uint64_t tgt = got_addr + (uint64_t)got_off[i];
            uint8_t *sp = sd + stub_off[i];
            // adrp x16, page_of(tgt) — PC is at this stub instruction.
            uint64_t stub_pc = stubs_addr + (uint64_t)stub_off[i];
            int64_t delta = (int64_t)(tgt - (stub_pc & ~(uint64_t)0xfff));
            int64_t imm = delta >> 12;
            uint32_t adrp = 0x90000010u |
                ((uint32_t)(imm & 3) << 29) |
                ((uint32_t)((imm >> 2) & 0x7ffff) << 5);
            mo_w32le(sp, adrp);
            // ldr x16, [x16, #(tgt & 0xfff)]
            uint32_t ldr = 0xF9400210u |
                ((uint32_t)((tgt & 0xfff) >> 3) << 10);
            mo_w32le(sp + 4, ldr);
            // br x16
            mo_w32le(sp + 8, 0xD61F0200u);
        }
    }

    // Fill local GOT entries with symbol addresses (dyld handles external ones).
    {
        uint8_t *gd = s->secs[got_sec].data;
        for (int i = 0; i < s->n_syms; i++) {
            if (got_off[i] < 0) continue;
            LinkSym *sym = &s->syms[i];
            if (sym->sec < 0) continue; // external, dyld handles
            uint64_t addr = s->secs[sym->sec].addr + sym->value;
            uint8_t *p = gd + got_off[i];
            p[0] = (uint8_t)(addr);
            p[1] = (uint8_t)(addr >> 8);
            p[2] = (uint8_t)(addr >> 16);
            p[3] = (uint8_t)(addr >> 24);
            p[4] = (uint8_t)(addr >> 32);
            p[5] = (uint8_t)(addr >> 40);
            p[6] = (uint8_t)(addr >> 48);
            p[7] = (uint8_t)(addr >> 56);
        }
    }

    // Entry point
    int entry_sym = link_find_sym(s, "_main");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "main");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "start");
    uint64_t entry_addr = 0;
    if (entry_sym >= 0) entry_addr = mo_symbol_address(s, entry_sym);

    // Compute file offsets.  __TEXT starts at file offset 0 to encompass
    // the Mach-O header + load commands; section data follows at the first
    // 16-byte-aligned boundary after the load commands (same offset as the
    // old text_fileoff was, now kept solely as the section-data start).
    uint64_t text_fileoff = 0;
    uint64_t linkedit_fileoff;
    if (has_data) {
        uint64_t data_fileoff = mo_align(text_fileoff + text_vmsize, 0x4000);
        linkedit_fileoff = data_fileoff + mo_align(data_vmsize, 0x4000);
    } else {
        linkedit_fileoff = mo_align(text_fileoff + text_vmsize, 0x4000);
    }

    // Rebase opcodes come first in __LINKEDIT's dyld-info region, then bind.
    uint64_t rebase_off = linkedit_fileoff;
    // Bind opcodes: tell dyld to fill GOT entries with imported symbol
    // addresses.  Each __LINKEDIT sub-blob must be 8-byte aligned (ld/dyld
    // reject "mis-aligned LINKEDIT content" otherwise).
    uint64_t bind_off = mo_align(rebase_off + rebase_size, 8);

    // Symbol table: 1 null + undef externals (+ defined globals for dylibs).
    uint32_t nsyms = 1 + (uint32_t)n_undef;
    if (is_dylib) nsyms += (uint32_t)n_defsym;
    uint64_t symtab_off = mo_align(bind_off + bind_size, 8); // 8-aligned nlist_64
    uint64_t strtab_off = symtab_off + nsyms * 16;
    // String table.
    size_t strtab_size = 1;
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (!sym->name || !sym->name[0]) continue;
        if (sym->sec < 0) strtab_size += strlen(sym->name) + 1;
        else if (is_dylib && sym->bind == 1)
            strtab_size += strlen(sym->name) + 1;
    }
    const char *dylib_path = "/usr/lib/libSystem.B.dylib";
    strtab_size += strlen(dylib_path) + 1;
    strtab_size = mo_align(strtab_size, 8);
    uint64_t linkedit_end = strtab_off + strtab_size;

    // Export trie: compute size (minimal for exe, proper for dylib).
    uint64_t export_off = linkedit_end;
    uint32_t export_size;
    uint8_t *export_data = NULL;
    if (is_dylib && n_defsym > 0) {
        // Build a proper prefix (radix) export trie so libraries with many
        // exports (e.g. sqlite, hundreds of symbols) don't overflow a node's
        // one-byte childCount.  Symbols use their full mangled name (incl the
        // leading '_'); addresses are image-relative (dylib base is 0).
        MoTrieSym *syms = malloc((size_t)n_defsym * sizeof(MoTrieSym));
        int nx = 0;
        for (int i = 0; i < s->n_syms; i++) {
            LinkSym *sym = &s->syms[i];
            if (sym->sec < 0 || sym->bind != 1 || !sym->name || !sym->name[0])
                continue;
            syms[nx].addr = s->secs[sym->sec].addr + sym->value;
            syms[nx].name = sym->name;
            syms[nx].len = (int)strlen(sym->name);
            nx++;
        }
        // Sort by name (the trie builder requires sorted input).
        for (int a = 1; a < nx; a++) {
            MoTrieSym key = syms[a];
            int b = a - 1;
            while (b >= 0 && strcmp(syms[b].name, key.name) > 0) {
                syms[b + 1] = syms[b];
                b--;
            }
            syms[b + 1] = key;
        }
        MoTrieNode *root = mo_trie_build(syms, 0, nx, 0);
        // Flatten into BFS order and fixed-point the offsets.
        MoTrieNode **list = NULL;
        int nn = 0, lcap = 0;
        mo_trie_collect(root, &list, &nn, &lcap);
        for (int iter = 0; iter < 16; iter++) {
            size_t pos = 0;
            int changed = 0;
            for (int k = 0; k < nn; k++) {
                if (list[k]->offset != pos) {
                    list[k]->offset = pos;
                    changed = 1;
                }
                pos += mo_trie_node_size(list[k]);
            }
            if (!changed) break;
        }
        size_t total = 0;
        for (int k = 0; k < nn; k++) total += mo_trie_node_size(list[k]);
        export_data = malloc(total ? total : 1);
        size_t len = 0;
#define TW(b) do { export_data[len++] = (uint8_t)(b); } while (0)
#define TWU(v) do { uint64_t _v = (v); do { uint8_t _b = (uint8_t)(_v & 0x7f); _v >>= 7; if (_v) _b |= 0x80; export_data[len++] = _b; } while (_v); } while (0)
        for (int k = 0; k < nn; k++) {
            MoTrieNode *nd = list[k];
            if (nd->terminal) {
                int tinfo = mo_uleb_len(0) + mo_uleb_len(nd->addr);
                TWU((uint64_t)tinfo); // terminalSize
                TWU(0); // flags = 0
                TWU(nd->addr); // image-relative address
            } else {
                TW(0); // terminalSize = 0
            }
            TW((uint8_t)nd->n_edges); // childCount (<=255 by construction)
            for (int e = 0; e < nd->n_edges; e++) {
                const char *lab = nd->edges[e].label;
                size_t ll = strlen(lab) + 1;
                memcpy(export_data + len, lab, ll);
                len += ll;
                TWU(nd->edges[e].child->offset);
            }
        }
#undef TW
#undef TWU
        export_size = (uint32_t)len; // == total
        mo_trie_free(root);
        free(list);
        free(syms);
    } else {
        export_size = 2;
    }
    linkedit_end = export_off + export_size;

    // Ad-hoc code signature: must start 16-byte aligned (matches what
    // Apple's codesign_allocate expects and avoids needing it to move the
    // load command itself) and covers every byte written before it --
    // codesig_off IS the "codeSize" the CodeDirectory hashes.
    uint64_t codesig_off = mo_align(linkedit_end, 16);
    uint64_t codesig_size = mo_codesign_size(codesig_off, "a.out");
    uint64_t linkedit_total_end = codesig_off + codesig_size;

    // Open output file. "wb+" (not "wb") so the code-signing pass below can
    // seek back and re-read the bytes already written to hash them --
    // write-only would make that impossible.
    FILE *f = fopen(s->out_path, "wb+");
    if (!f) {
        fprintf(stderr, "rcc: link: cannot create %s: %s\n", s->out_path, strerror(errno));
        free(mo_secs);
        return -1;
    }

    // --- Mach-O header ---
    uint32_t cpu_type = CPU_TYPE_ARM64;
    uint32_t cpu_subtype = CPU_SUBTYPE_ARM64_ALL;
    if (s->arch == ARCH_X86_64) {
        cpu_type = CPU_TYPE_X86_64;
        cpu_subtype = CPU_SUBTYPE_ALL;
    }

    mo_w32(f, MH_MAGIC_64);
    mo_w32(f, cpu_type);
    mo_w32(f, cpu_subtype);
    mo_w32(f, is_dylib ? MH_DYLIB : MH_EXECUTE);
    mo_w32(f, ncmds);
    mo_w32(f, total_lc);
    // MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE
    mo_w32(f, 0x00200085); // flags
    mo_w32(f, 0); // reserved

    // --- LC_SEGMENT_64: __PAGEZERO (executables only) ---
    if (!is_dylib) {
        mo_w32(f, LC_SEGMENT_64);
        mo_w32(f, 72);
        mo_wbuf(f, "__PAGEZERO\0\0\0\0\0\0", 16);
        mo_w64(f, 0);
        mo_w64(f, base); // vmaddr=0, vmsize=base (4GB guard)
        mo_w64(f, 0);
        mo_w64(f, 0); // fileoff=0, filesize=0
        mo_w32(f, 0);
        mo_w32(f, 0); // maxprot=0, initprot=0
        mo_w32(f, 0);
        mo_w32(f, 0); // nsects=0, flags=0
    }
    mo_w32(f, LC_SEGMENT_64);
    mo_w32(f, text_lc_size);
    mo_wbuf(f, "__TEXT\0\0\0\0\0\0\0\0\0\0", 16);
    // vmsize/filesize page-aligned: __TEXT is the first mapped segment and
    // dyld derives the whole image's contiguous VM layout from it; an
    // unaligned vmsize left a sub-page dangling region before __DATA that
    // made dyld apply a nonzero ASLR slide the unrebased absolute data
    // pointers don't reflect.  The file already contains a full aligned page
    // here (section-data write pads to the page-aligned DATA/LINKEDIT
    // fileoff), so claiming the aligned size never runs past EOF.
    uint64_t text_seg_size = mo_align(text_vmsize, 0x4000);
    mo_w64(f, text_vmaddr);
    mo_w64(f, text_seg_size); // vmsize (covers headers + section data)
    mo_w64(f, text_fileoff); // fileoff = 0
    mo_w64(f, text_seg_size); // filesize
    mo_w32(f, 5); // maxprot=r-x
    mo_w32(f, 5); // initprot=r-x
    mo_w32(f, nsects_text);
    mo_w32(f, 0); // flags

    // Section data starts after the aligned load commands, both in file and vm.
    uint64_t hdr_pad = mo_align(header_size + total_lc, 16);
    uint64_t cur_vm = text_vmaddr + hdr_pad;
    uint64_t cur_fo = text_fileoff + hdr_pad;
    for (int i = 0; i < n_mo; i++) {
        if (strcmp(mo_secs[i].segname, "__TEXT") != 0) continue;
        LinkSec *sec = mo_secs[i].sec;
        char sn[17] = {0}, sg[17] = {0};
        strncpy(sn, mo_secs[i].sectname, 16);
        strncpy(sg, "__TEXT", 16);
        mo_wbuf(f, sn, 16);
        mo_wbuf(f, sg, 16);
        mo_w64(f, cur_vm);
        mo_w64(f, sec->len);
        mo_w32(f, (uint32_t)cur_fo);
        mo_w32(f, 4); // offset, align=16
        mo_w32(f, 0);
        mo_w32(f, 0); // reloff=0, nreloc=0
        uint32_t sflags = S_REGULAR;
        if (sec->exec) sflags |= S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS;
        mo_w32(f, sflags);
        mo_w32(f, 0);
        mo_w32(f, 0);
        mo_w32(f, 0);
        cur_vm += mo_align(sec->len, 16);
        cur_fo += mo_align(sec->len, 16);
    }

    // --- LC_SEGMENT_64: __DATA ---
    if (has_data) {
        uint64_t data_fileoff = mo_align(text_fileoff + text_vmsize, 0x4000);
        cur_vm = data_vmaddr;
        cur_fo = data_fileoff;
        mo_w32(f, LC_SEGMENT_64);
        mo_w32(f, data_lc_size);
        mo_wbuf(f, "__DATA\0\0\0\0\0\0\0\0\0\0", 16);
        uint64_t data_total_vmsize = 0;
        for (int i = 0; i < n_mo; i++)
            if (strcmp(mo_secs[i].segname, "__TEXT") != 0)
                data_total_vmsize += mo_align(mo_secs[i].sec->len, 16);
        // vmsize/filesize must be page-aligned: dyld maps/mprotects segments
        // at page granularity, and linkedit_fileoff (below) already reserves
        // a full aligned page of file space for this segment's content
        // (mo_align(data_vmsize, 0x4000)) -- an unaligned, undersized vmsize
        // here left dangling VM space between __DATA and __LINKEDIT that
        // caused dyld to apply a nonzero ASLR slide the (unrebased) absolute
        // data pointers written into __data don't reflect, corrupting any
        // `T *p = &global;`-style initializer at runtime.
        uint64_t data_seg_size = mo_align(data_total_vmsize, 0x4000);
        mo_w64(f, data_vmaddr);
        mo_w64(f, data_seg_size);
        mo_w64(f, data_fileoff);
        mo_w64(f, data_seg_size);
        mo_w32(f, 7);
        mo_w32(f, 3); // maxprot=rwx, initprot=rw-
        mo_w32(f, nsects_data);
        mo_w32(f, 0);
        for (int i = 0; i < n_mo; i++) {
            if (strcmp(mo_secs[i].segname, "__TEXT") == 0) continue;
            LinkSec *sec = mo_secs[i].sec;
            char sn[17] = {0}, sg[17] = {0};
            strncpy(sn, mo_secs[i].sectname, 16);
            strncpy(sg, "__DATA", 16);
            mo_wbuf(f, sn, 16);
            mo_wbuf(f, sg, 16);
            mo_w64(f, cur_vm);
            mo_w64(f, sec->len);
            mo_w32(f, (uint32_t)(sec->is_bss ? 0 : cur_fo));
            mo_w32(f, 3); // offset, align=8
            mo_w32(f, 0);
            mo_w32(f, 0);
            mo_w32(f, sec->is_bss ? S_ZEROFILL : S_REGULAR);
            mo_w32(f, 0);
            mo_w32(f, 0);
            mo_w32(f, 0);
            cur_vm += mo_align(sec->len, 16);
            if (!sec->is_bss) cur_fo += mo_align(sec->len, 16);
        }
    }

    // --- LC_SEGMENT_64: __LINKEDIT ---
    // linkedit must start at a page boundary.
    if (!has_data) cur_vm = data_vmaddr;
    else
        cur_vm = mo_align(cur_vm, 0x4000);
    mo_w32(f, LC_SEGMENT_64);
    mo_w32(f, 72);
    mo_wbuf(f, "__LINKEDIT\0\0\0\0\0", 16);
    mo_w64(f, cur_vm);
    mo_w64(f, mo_align(linkedit_total_end - linkedit_fileoff, 0x4000));
    mo_w64(f, linkedit_fileoff);
    // filesize must equal exactly what gets written below (LINKEDIT's
    // content, including the trailing ad-hoc code-signature blob, ends at
    // linkedit_total_end with no further rounding); rounding up here
    // claims more bytes than are ever written, which dyld rejects as
    // "fileoff+filesize extends past the end of the file".
    mo_w64(f, linkedit_total_end - linkedit_fileoff);
    mo_w32(f, 1); // maxprot=r--
    mo_w32(f, 1); // initprot=r--
    mo_w32(f, 0); // nsects
    mo_w32(f, 0); // flags

    // --- LC_DYLD_INFO_ONLY (bind opcodes for GOT entries) ---
    mo_w32(f, LC_DYLD_INFO_ONLY);
    mo_w32(f, lc_dyld_info);
    // rebase_off, rebase_size
    mo_w32(f, rebase_size ? (uint32_t)rebase_off : 0);
    mo_w32(f, rebase_size);
    // bind_off, bind_size
    mo_w32(f, (uint32_t)bind_off);
    mo_w32(f, bind_size);
    // lazy_bind_off=0, lazy_bind_size=0
    mo_w32(f, 0);
    mo_w32(f, 0);
    // weak_bind_off=0, weak_bind_size=0
    mo_w32(f, 0);
    mo_w32(f, 0);
    // export_off=0, export_size=0
    mo_w32(f, 0);
    mo_w32(f, 0);

    // --- LC_SYMTAB ---
    mo_w32(f, LC_SYMTAB);
    mo_w32(f, 24);
    mo_w32(f, (uint32_t)symtab_off);
    mo_w32(f, nsyms);
    mo_w32(f, (uint32_t)strtab_off);
    mo_w32(f, (uint32_t)strtab_size);

    // --- LC_DYSYMTAB ---
    mo_w32(f, LC_DYSYMTAB);
    mo_w32(f, 80);
    // The symbol table is [null, (dylib: defined globals,) undefined externs].
    // dyld requires the dysymtab to partition it in order: locals, then
    // external-defined, then undefined, with no overlap.  Treat the index-0
    // null entry as the sole local symbol.
    if (!is_dylib) {
        mo_w32(f, 0); // ilocalsym
        mo_w32(f, 1); // nlocalsym = null entry
        mo_w32(f, 1); // iextdefsym
        mo_w32(f, 0); // nextdefsym
        mo_w32(f, 1); // iundefsym
        mo_w32(f, (uint32_t)n_undef); // nundefsym
    } else {
        mo_w32(f, 0); // ilocalsym
        mo_w32(f, 1); // nlocalsym = null entry
        mo_w32(f, 1); // iextdefsym
        mo_w32(f, (uint32_t)n_defsym); // nextdefsym
        mo_w32(f, 1 + (uint32_t)n_defsym); // iundefsym
        mo_w32(f, (uint32_t)n_undef); // nundefsym
    }
    mo_w32(f, 0); // tocoff
    mo_w32(f, 0); // ntoc
    mo_w32(f, 0); // modtaboff
    mo_w32(f, 0); // nmodtab
    mo_w32(f, 0); // extrefsymoff
    mo_w32(f, 0); // nextrefsyms
    mo_w32(f, 0); // indirectsymoff
    mo_w32(f, 0); // nindirectsyms
    mo_w32(f, 0); // extreloff
    mo_w32(f, 0); // nextrel
    mo_w32(f, 0); // locreloff
    mo_w32(f, 0); // nlocrel

    // --- LC_LOAD_DYLIB ---
    uint32_t dylib_padded = lc_dylib;
    mo_w32(f, LC_LOAD_DYLIB);
    mo_w32(f, dylib_padded);
    mo_w32(f, 24); // offset to string
    mo_w32(f, 0);
    mo_w32(f, 0);
    mo_w32(f, 0); // timestamp, version, compat version
    mo_wbuf(f, dylib_path, strlen(dylib_path) + 1);
    for (uint32_t p = (uint32_t)strlen(dylib_path) + 1; p < dylib_padded - 24; p++)
        fputc(0, f);

    // --- LC_LOAD_DYLINKER (executables only) ---
    if (!is_dylib) {
        mo_w32(f, LC_LOAD_DYLINKER);
        mo_w32(f, lc_dylinker);
        mo_w32(f, 12); // offset to string within this command
        mo_wbuf(f, "/usr/lib/dyld", strlen("/usr/lib/dyld") + 1);
        for (uint32_t p = (uint32_t)(12 + strlen("/usr/lib/dyld") + 1); p < lc_dylinker; p++)
            fputc(0, f);
    }

    // --- LC_ID_DYLIB (dylibs only) ---
    if (is_dylib) {
        uint32_t id_padded = lc_id_dylib;
        mo_w32(f, LC_ID_DYLIB);
        mo_w32(f, id_padded);
        mo_w32(f, 24); // offset to install name string
        mo_w32(f, 1); // timestamp
        mo_w32(f, 0); // current_version
        mo_w32(f, 0); // compatibility_version
        mo_wbuf(f, s->out_path, strlen(s->out_path) + 1);
        for (uint32_t p = (uint32_t)(24 + strlen(s->out_path) + 1); p < id_padded; p++)
            fputc(0, f);
    }

    // --- LC_UUID ---
    mo_w32(f, LC_UUID);
    mo_w32(f, lc_uuid);
    for (int i = 0; i < 16; i++) fputc(0, f);

    // --- LC_BUILD_VERSION ---
    mo_w32(f, LC_BUILD_VERSION);
    mo_w32(f, 24);
    mo_w32(f, PLATFORM_MACOS);
    mo_w32(f, 0x000E0000); // minos 14.0
    mo_w32(f, 0x000E0000); // sdk 14.0
    mo_w32(f, 0); // ntools = 0

    // --- LC_MAIN (executables only) ---
    if (!is_dylib) {
        mo_w32(f, LC_MAIN);
        mo_w32(f, 24);
        mo_w64(f, entry_addr - base); // entry offset
        mo_w64(f, 0); // stack size (default)
    }

    // --- LC_DYLD_EXPORTS_TRIE ---
    mo_w32(f, LC_DYLD_EXPORTS_TRIE);
    mo_w32(f, lc_export_trie);
    mo_w32(f, (uint32_t)export_off);
    mo_w32(f, export_size);

    // --- LC_CODE_SIGNATURE ---
    // codesig_off/codesig_size were fixed above, before file offsets were
    // finalized, so the load command's claimed range matches exactly what
    // gets hashed and written after every other byte below.
    mo_w32(f, LC_CODE_SIGNATURE);
    mo_w32(f, lc_codesig_cmd);
    mo_w32(f, (uint32_t)codesig_off);
    mo_w32(f, (uint32_t)codesig_size);

    uint64_t cur;

    // Pad to section data start (hdr_pad = first 16-byte boundary after
    // the load commands, same as the old text_fileoff when it was != 0).
    {
        uint64_t hdr_pad = mo_align(header_size + total_lc, 16);
        cur = ftell(f);
        if (hdr_pad > cur) mo_wzeros(f, (size_t)(hdr_pad - cur));
    }

    // --- Write section data ---
    // Sections are written in MO order (TEXT first, then DATA), with a
    // page-aligned gap between segments matching the segment file offsets.
    {
        uint64_t hdr_pad = mo_align(header_size + total_lc, 16);
        cur_fo = text_fileoff + hdr_pad;
    }
    const char *cur_seg = "__TEXT";
    for (int i = 0; i < n_mo; i++) {
        LinkSec *sec = mo_secs[i].sec;
        if (sec->is_bss) continue;
        // When switching from TEXT to DATA, pad to the DATA segment file offset.
        if (strcmp(mo_secs[i].segname, "__TEXT") != 0 && cur_seg &&
            strcmp(cur_seg, "__TEXT") == 0) {
            cur_seg = "__DATA";
            uint64_t data_fileoff = mo_align(text_fileoff + text_vmsize, 0x4000);
            cur = ftell(f);
            if (data_fileoff > (uint64_t)cur)
                mo_wzeros(f, (size_t)(data_fileoff - (uint64_t)cur));
            cur_fo = data_fileoff;
        }
        mo_wbuf(f, sec->data, sec->len);
        uint64_t padded = mo_align(sec->len, 16);
        if (padded > sec->len) mo_wzeros(f, (size_t)(padded - sec->len));
        cur_fo += padded;
    }
    // --- Write rebase opcodes ---
    cur = ftell(f);
    if (rebase_off > (uint64_t)cur)
        mo_wzeros(f, (size_t)(rebase_off - (uint64_t)cur));
    if (rebase_size > 0) mo_wbuf(f, rebase_data, rebase_size);
    free(rebase_data);
    // --- Write bind opcodes (built during layout) ---
    cur = ftell(f);
    if (bind_off > (uint64_t)cur)
        mo_wzeros(f, (size_t)(bind_off - (uint64_t)cur));
    if (bind_size > 0) {
        mo_wbuf(f, bind_data, bind_size);
        free(bind_data);
    }
    // Pad to symbol table (bind opcodes are the last __LINKEDIT bind data;
    // no chained fixups are emitted -- LC_DYLD_INFO_ONLY's bind opcodes are
    // sufficient and coexisting with a chained-fixups load command, even an
    // empty/no-op one, made dyld mishandle rebasing of plain data-pointer
    // initializers such as `int *p = &x;`).
    cur = ftell(f);
    if (symtab_off > (uint64_t)cur)
        mo_wzeros(f, (size_t)(symtab_off - (uint64_t)cur));

    cur = ftell(f);

    // --- Write symbol table ---
    // Null entry (nlist_64 = 16 bytes)
    mo_w32(f, 0);
    fputc(0, f);
    fputc(0, f);
    fputc(0 & 0xFF, f);
    fputc((0 >> 8) & 0xFF, f);
    // Undefined external symbols
    uint32_t str_off = 1; // skip leading \0
    // For dylibs: write defined globals first (indices 1..n_defsym),
    // then undefined symbols (indices 1+n_defsym..).
    if (is_dylib) {
        for (int i = 0; i < s->n_syms; i++) {
            LinkSym *sym = &s->syms[i];
            if (sym->sec < 0 || sym->bind != 1 || !sym->name || !sym->name[0])
                continue;
            uint8_t n_sect = 0;
            for (int m = 0; m < n_mo; m++) {
                if (mo_secs[m].sec == &s->secs[sym->sec]) {
                    n_sect = (uint8_t)(m + 1);
                    break;
                }
            }
            mo_w32(f, str_off);
            fputc(N_SECT | N_EXT, f);
            fputc(n_sect, f);
            fputc(0, f);
            fputc(0, f);
            mo_w64(f, s->secs[sym->sec].addr + sym->value);
            str_off += (uint32_t)strlen(sym->name) + 1;
        }
    }
    // Undefined external symbols
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec >= 0 || !sym->name || !sym->name[0]) continue;
        mo_w32(f, str_off);
        fputc(N_UNDF | N_EXT, f);
        fputc(0, f);
        fputc(0 & 0xFF, f);
        fputc((0 >> 8) & 0xFF, f);
        mo_w64(f, 0);
        str_off += (uint32_t)strlen(sym->name) + 1;
    }

    // --- Write string table ---
    fputc(0, f); // leading \0
    if (is_dylib) {
        for (int i = 0; i < s->n_syms; i++) {
            LinkSym *sym = &s->syms[i];
            if (sym->sec < 0 || sym->bind != 1 || !sym->name || !sym->name[0])
                continue;
            mo_wbuf(f, sym->name, strlen(sym->name) + 1);
        }
    }
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec >= 0 || !sym->name || !sym->name[0]) continue;
        mo_wbuf(f, sym->name, strlen(sym->name) + 1);
    }
    mo_wbuf(f, dylib_path, strlen(dylib_path) + 1);
    // Pad to next 8-byte boundary as computed above.
    cur = ftell(f);
    if (mo_align((uint64_t)cur, 8) > (uint64_t)cur)
        mo_wzeros(f, mo_align((uint64_t)cur, 8) - (uint64_t)cur);

    // Write export trie (bytes were built during layout, above).
    fseek(f, (long)export_off, SEEK_SET);
    if (export_data) {
        mo_wbuf(f, export_data, export_size);
        free(export_data);
    } else {
        fputc(0, f); // empty trie: single terminal node with 0 children
        fputc(0, f);
    }

    // Pad to codesig_off (16-byte-aligned start of the signature blob).
    cur = ftell(f);
    if (codesig_off > (uint64_t)cur) mo_wzeros(f, (size_t)(codesig_off - (uint64_t)cur));

    // Ad-hoc code sign: hash every 4KB page of the codesig_off bytes just
    // written (fflush first so the read-back below sees them) and lay the
    // SuperBlob/CodeDirectory down as the file's final bytes. Without this
    // the kernel's AMFI policy SIGKILLs the binary before main() ever runs
    // on Apple Silicon, even though nothing else about the file is wrong.
    fflush(f);
    uint8_t *sig = malloc(codesig_size);
    mo_codesign_sign(f, sig, codesig_off, "a.out", text_fileoff, text_vmsize, true);
    fseek(f, (long)codesig_off, SEEK_SET);
    mo_wbuf(f, sig, codesig_size);
    free(sig);

    fclose(f);
    chmod(s->out_path, 0755);
    free(mo_secs);
    return 0;
}
