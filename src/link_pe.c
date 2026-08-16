#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
// SPDX-License-Identifier: LGPL-2.1-or-later
// Native PE/COFF linker for rcc (Windows/MinGW).
#include "link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <sys/mman.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

// ---------------------------------------------------------------------------
// COFF / PE constants
// ---------------------------------------------------------------------------

#define IMAGE_FILE_MACHINE_AMD64  0x8664
#define IMAGE_FILE_MACHINE_ARM64  0xAA64

// COFF relocation types (x86-64)
#define IMAGE_REL_AMD64_ADDR64    1
#define IMAGE_REL_AMD64_ADDR32    2
#define IMAGE_REL_AMD64_ADDR32NB  3
#define IMAGE_REL_AMD64_REL32     4
#define IMAGE_REL_AMD64_REL32_1   5
#define IMAGE_REL_AMD64_REL32_2   6
#define IMAGE_REL_AMD64_REL32_3   7
#define IMAGE_REL_AMD64_REL32_4   8
#define IMAGE_REL_AMD64_REL32_5   9
#define IMAGE_REL_AMD64_SECTION   10
#define IMAGE_REL_AMD64_SECREL    11

// COFF relocation types (ARM64) — values per Microsoft PE/COFF spec
#define IMAGE_REL_ARM64_ADDR32           1
#define IMAGE_REL_ARM64_ADDR32NB         2
#define IMAGE_REL_ARM64_BRANCH26         3
#define IMAGE_REL_ARM64_PAGEBASE_REL21   4
#define IMAGE_REL_ARM64_PAGEOFFSET_12A   6
#define IMAGE_REL_ARM64_ADDR64           14

// Section flags
#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

// COFF characteristics
#define IMAGE_FILE_DLL                    0x2000

// Symbol storage class
#define IMAGE_SYM_CLASS_EXTERNAL         2
#define IMAGE_SYM_CLASS_STATIC           3
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL    105

// PE optional header magic
#define PE32PLUS_MAGIC 0x20b

// PE subsystem
#define IMAGE_SUBSYSTEM_WINDOWS_CUI  3

// DLL characteristics
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT  0x100
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x40

// Section alignment
#define PE_SECTION_ALIGN  0x1000
#define PE_FILE_ALIGN     0x200

// ---------------------------------------------------------------------------
// Helpers (local)
// ---------------------------------------------------------------------------

static uint16_t pe_r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t pe_r32le(const uint8_t *p) {
    return (uint32_t)pe_r16le(p) | ((uint32_t)pe_r16le(p + 2) << 16);
}
static void pe_w32le_m(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static void pe_w64le_m(uint8_t *p, uint64_t v) {
    pe_w32le_m(p, (uint32_t)v);
    pe_w32le_m(p + 4, (uint32_t)(v >> 32));
}
static void pe_w16le(FILE *f, uint16_t v) {
    fputc(v & 0xFF, f);
    fputc(v >> 8, f);
}
static void pe_w32le(FILE *f, uint32_t v) {
    pe_w16le(f, (uint16_t)v);
    pe_w16le(f, (uint16_t)(v >> 16));
}
static void pe_w64le(FILE *f, uint64_t v) {
    pe_w32le(f, (uint32_t)v);
    pe_w32le(f, (uint32_t)(v >> 32));
}
static void pe_wbuf(FILE *f, const void *b, size_t n) { fwrite(b, 1, n, f); }
static void pe_wzeros(FILE *f, size_t n) {
    static const uint8_t z[512] = {0};
    while (n > 512) {
        pe_wbuf(f, z, 512);
        n -= 512;
    }
    pe_wbuf(f, z, n);
}
static uint64_t pe_align_up(uint64_t v, uint64_t a) {
    return (v + a - 1) & ~(a - 1);
}

// Format into a fresh malloc'd buffer (owned by the caller).
static char *pe_xfmt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *out = malloc((size_t)n + 1);
    vsnprintf(out, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    return out;
}

// ---------------------------------------------------------------------------
// COFF → internal relocation type mapping
// ---------------------------------------------------------------------------
static int pe_map_reloc(uint16_t coff_type, uint16_t machine) {
    if (machine == IMAGE_FILE_MACHINE_AMD64) {
        switch (coff_type) {
        case IMAGE_REL_AMD64_ADDR64: return RL_ABS64;
        case IMAGE_REL_AMD64_ADDR32: return RL_ABS32;
        case IMAGE_REL_AMD64_REL32: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_1: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_2: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_3: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_4: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_5: return RL_PC32;
        case IMAGE_REL_AMD64_ADDR32NB: return RL_ADDR32NB;
        default: return -1;
        }
    } else if (machine == IMAGE_FILE_MACHINE_ARM64) {
        switch (coff_type) {
        case IMAGE_REL_ARM64_ADDR64: return RL_ABS64;
        case IMAGE_REL_ARM64_ADDR32: return RL_ABS32;
        case IMAGE_REL_ARM64_ADDR32NB: return RL_ADDR32NB;
        case IMAGE_REL_ARM64_BRANCH26: return RL_ARM64_B26;
        case IMAGE_REL_ARM64_PAGEBASE_REL21: return RL_ARM64_ADR_PG;
        case IMAGE_REL_ARM64_PAGEOFFSET_12A: return RL_ARM64_ADD_LO;
        default: return -1;
        }
    }
    return -1;
}

// COFF's REL32 family uses an implicit addend derived from the relocation
// type itself (unlike ELF's RELA, which carries an explicit addend field):
// IMAGE_REL_AMD64_REL32 means "value relative to the byte 4 past the
// relocation site" (the common case: a 4-byte disp32 field is the last
// operand bytes of the instruction). The _1.._5 variants exist for
// instructions with 1-5 more operand bytes trailing the disp32 (e.g. an
// immediate operand after a RIP-relative memory operand), shifting the
// reference point further past the relocation site. Our own PC32 apply
// path computes S - pc + A with pc = address of the disp32 field itself,
// so the negative distance must be supplied as the addend here.
static int64_t pe_pc32_addend(uint16_t coff_type, uint16_t machine) {
    if (machine == IMAGE_FILE_MACHINE_AMD64) {
        switch (coff_type) {
        case IMAGE_REL_AMD64_REL32: return -4;
        case IMAGE_REL_AMD64_REL32_1: return -5;
        case IMAGE_REL_AMD64_REL32_2: return -6;
        case IMAGE_REL_AMD64_REL32_3: return -7;
        case IMAGE_REL_AMD64_REL32_4: return -8;
        case IMAGE_REL_AMD64_REL32_5: return -9;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// COFF object file loader
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
    uint8_t *image;
#ifdef _WIN32
    image = malloc((size_t)st.st_size);
    if (!image) {
        close(fd);
        return -1;
    }
    if (read(fd, image, (size_t)st.st_size) != (ssize_t)st.st_size) {
        free(image);
        close(fd);
        return -1;
    }
    close(fd);
#else
    image = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (image == MAP_FAILED) return -1;
#endif

    // Check if ELF (shouldn't happen on Windows, but handle gracefully)
    if (st.st_size >= 4 && image[0] == 0x7f && image[1] == 'E' &&
        image[2] == 'L' && image[3] == 'F') {
        free(image);
        fprintf(stderr, "rcc: link: %s: ELF objects not supported by PE linker\n", path);
        return -1;
    }

    // COFF file header (20 bytes)
    if (st.st_size < 20) {
        free(image);
        return -1;
    }
    uint16_t machine = pe_r16le(image);
    uint16_t n_secs = pe_r16le(image + 2);
    uint32_t symtab_off = pe_r32le(image + 8);
    uint32_t n_syms = pe_r32le(image + 12);
    uint16_t opthdr_size = pe_r16le(image + 16);

    if (machine != IMAGE_FILE_MACHINE_AMD64 && machine != IMAGE_FILE_MACHINE_ARM64) {
        fprintf(stderr, "rcc: link: %s: unsupported machine 0x%x\n", path, machine);
        free(image);
        return -1;
    }

    // Section headers (40 bytes each, after 20-byte header + optional header)
    uint32_t sec_hdr_off = 20 + opthdr_size;
    int *out_sec_map = calloc((size_t)n_secs, sizeof(int));
    // Byte offset within the (possibly already non-empty, shared-by-name)
    // output section where THIS object's own section data begins -- a
    // later-loaded object's bytes are appended AFTER an earlier object's,
    // so its symbols' values (offsets within ITS OWN section, as stored
    // by the compiler) must be rebased by this amount to become correct
    // offsets within the merged output section. Relocation VAs already
    // get this treatment via `off` below; symbol values did not, which
    // silently pointed every symbol DEFINED in any object after the
    // first at the wrong (first object's) address.
    uint64_t *out_sec_off = calloc((size_t)n_secs, sizeof(uint64_t));

    // Snapshot every output section's pre-existing reloc count before
    // this object contributes any of its own: output sections merge by
    // name across every loaded object (out_sec_map[i] can resolve to a
    // section a PREVIOUS object already appended to), so `relocs[]` is a
    // shared, accumulating array. The symbol remap below must touch only
    // the relocations THIS object just added -- see there for why.
    int old_n_secs = s->n_secs;
    int *reloc_base = calloc((size_t)old_n_secs, sizeof(int));
    for (int i = 0; i < old_n_secs; i++) reloc_base[i] = s->secs[i].n_relocs;

    for (uint16_t i = 0; i < n_secs; i++) {
        const uint8_t *shdr = image + sec_hdr_off + (uint32_t)i * 40;
        char sname[9] = {0};
        memcpy(sname, shdr, 8);
        for (int j = 7; j >= 0 && (sname[j] == ' ' || sname[j] == '\0'); j--)
            sname[j] = '\0';

        uint32_t virtual_size = pe_r32le(shdr + 8);
        uint32_t raw_size = pe_r32le(shdr + 16);
        uint32_t raw_offset = pe_r32le(shdr + 20);
        uint32_t n_relocs = pe_r16le(shdr + 32);
        uint32_t reloc_off = pe_r32le(shdr + 24);
        uint32_t sec_flags = pe_r32le(shdr + 36);

        bool exec = (sec_flags & IMAGE_SCN_MEM_EXECUTE) != 0;
        bool write = (sec_flags & IMAGE_SCN_MEM_WRITE) != 0;
        bool is_bss = (sec_flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) != 0;

        int out_idx = link_find_or_create_sec(s, sname, true, write, exec,
                                              is_bss, false, 16);
        out_sec_map[i] = out_idx;

        if (!is_bss && raw_size > 0 && raw_offset > 0) {
            uint64_t off = link_sec_append(s, out_idx, image + raw_offset,
                                           raw_size, 16);
            out_sec_off[i] = off;

            for (uint32_t j = 0; j < n_relocs; j++) {
                const uint8_t *rp = image + reloc_off + j * 10;
                uint32_t r_va = pe_r32le(rp);
                uint32_t r_sym = pe_r32le(rp + 4);
                uint16_t r_type = pe_r16le(rp + 8);
                int rl = pe_map_reloc(r_type, machine);
                if (rl < 0) {
                    fprintf(stderr, "rcc: link: %s: unhandled COFF reloc type %u\n",
                            path, r_type);
                    continue;
                }
                link_add_reloc(s, out_idx, off + r_va, (uint32_t)rl,
                               (int)r_sym, pe_pc32_addend(r_type, machine));
            }
        } else if (is_bss && virtual_size > 0) {
            out_sec_off[i] = s->secs[out_idx].len;
            s->secs[out_idx].len += virtual_size;
        } else {
            out_sec_off[i] = s->secs[out_idx].len;
        }
    }

    // Symbol table (18 bytes per entry)
    const uint8_t *symtab = image + symtab_off;
    const uint8_t *strtab_base = symtab + (uint32_t)n_syms * 18;
    int *sym_map = calloc((size_t)n_syms, sizeof(int));

    for (uint32_t i = 0; i < n_syms;) {
        const uint8_t *se = symtab + (uint32_t)i * 18;
        char sym_name[256];
        if (se[0] == 0 && se[1] == 0 && se[2] == 0 && se[3] == 0) {
            uint32_t off = pe_r32le(se + 4);
            const char *strtab = (const char *)(strtab_base);
            snprintf(sym_name, sizeof(sym_name), "%s", strtab + off);
        } else {
            memcpy(sym_name, se, 8);
            sym_name[8] = '\0';
            for (int j = 7; j >= 0; j--) {
                if (sym_name[j] == '\0' || sym_name[j] == ' ') sym_name[j] = '\0';
                else
                    break;
            }
        }

        uint32_t value = pe_r32le(se + 8);
        int16_t sec_num = (int16_t)pe_r16le(se + 12);
        uint16_t sym_type = pe_r16le(se + 14);
        uint8_t storage_class = se[16];
        uint8_t num_aux = se[17];

        int bind, type, out_sec;
        if (sec_num <= 0) {
            bind = (storage_class == IMAGE_SYM_CLASS_WEAK_EXTERNAL) ? 2 : 1;
            // COFF: bit 0x20 in type field indicates function
            type = (sym_type & 0x20) ? 2 : 0;
            out_sec = -1;
        } else if (sec_num <= (int16_t)n_secs) {
            bind = (storage_class == IMAGE_SYM_CLASS_EXTERNAL) ? 1 : 0;
            type = 0;
            out_sec = out_sec_map[sec_num - 1];
        } else {
            sym_map[i] = -1;
            i += 1 + num_aux;
            continue;
        }

        uint64_t sym_value = (out_sec >= 0) ? value + out_sec_off[sec_num - 1] : 0;
        int sym_idx = link_add_sym(s, sym_name, out_sec, sym_value,
                                   0, bind, type, -1);
        sym_map[i] = sym_idx;
        i += 1 + num_aux;
    }

    // Re-map relocations: COFF sym index → LinkSym index. Scoped to just
    // the relocations THIS object added (sec->relocs[] is a shared,
    // accumulating array across every loaded object once output sections
    // merge by name) -- iterating and remapping the whole array here
    // would reinterpret an earlier object's already-resolved global sym
    // indices as raw COFF-local indices into THIS object's unrelated
    // sym_map/n_syms, corrupting cross-object relocations (e.g. main.o's
    // call to an external symbol defined in a separately-loaded bfn.o
    // silently resolving to whatever symbol happens to share that raw
    // index in bfn.o's own table -- observed as main() calling itself).
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        int start = (i < old_n_secs) ? reloc_base[i] : 0;
        for (int j = start; j < sec->n_relocs; j++) {
            int cs = sec->relocs[j].sym;
            if (cs >= 0 && cs < (int)n_syms)
                sec->relocs[j].sym = sym_map[cs];
        }
    }
    free(reloc_base);

    // Track object for cleanup
    LinkObj obj = {.path = strdup(path), .image = image, .image_size = (size_t)st.st_size};
    if (s->n_objs == s->cap_objs) {
        s->cap_objs = s->cap_objs ? s->cap_objs * 2 : 4;
        s->objs = realloc(s->objs, (size_t)s->cap_objs * sizeof(LinkObj));
    }
    s->objs[s->n_objs++] = obj;
    free(out_sec_map);
    free(out_sec_off);
    free(sym_map);
    return 0;
}

// ---------------------------------------------------------------------------
// PE executable writer
// ---------------------------------------------------------------------------

static uint64_t pe_symbol_address(LinkState *s, int idx) {
    LinkSym *sym = &s->syms[idx];
    if (sym->sec >= 0) return s->secs[sym->sec].addr + sym->value;
    return sym->value;
}

// ---------------------------------------------------------------------------
// DLL import resolution and .idata (import table) generation
// ---------------------------------------------------------------------------
//
// Our own COFF codegen emits a plain IMAGE_REL_AMD64_REL32 direct call to
// the imported symbol's name (e.g. "printf"), exactly like a normal mingw
// object file. Standard mingw import libraries satisfy that by providing a
// tiny code thunk *named* "printf" that does `jmp qword ptr [rip+disp]`
// through an Import Address Table (IAT) slot the loader fills in at load
// time. We synthesize the same thing directly: for each undefined strong
// symbol left after loading all objects, find which system DLL exports it,
// emit one .idata import descriptor per needed DLL, and redefine the
// symbol to point at a matching 6-byte trampoline in .text.

// Standard system DLLs searched (in order) for undefined symbols -- mirrors
// the default `-l` list mingw-w64-gcc links a normal executable against
// (msvcrt for the C runtime, then the core Win32 API libraries).
static const char *pe_default_dlls[] = {
    "msvcrt.dll",
    "kernel32.dll",
    "advapi32.dll",
    "user32.dll",
    "shell32.dll",
    NULL,
};

static uint32_t pe_rva_to_off(const uint8_t *img, uint32_t sec_hdr_off,
                              uint16_t n_secs, uint32_t rva) {
    for (uint16_t i = 0; i < n_secs; i++) {
        const uint8_t *sh = img + sec_hdr_off + (uint32_t)i * 40;
        uint32_t vsize = pe_r32le(sh + 8);
        uint32_t vaddr = pe_r32le(sh + 12);
        uint32_t rawsize = pe_r32le(sh + 16);
        uint32_t rawoff = pe_r32le(sh + 20);
        uint32_t span = vsize > rawsize ? vsize : rawsize;
        if (rva >= vaddr && rva < vaddr + span) return rawoff + (rva - vaddr);
    }
    return 0;
}

// Check which of `names[0..n)` are exported by the DLL at `path`, setting
// out[i] = true for each match found. Returns 0 if the file could be
// opened and parsed as a PE image (whether or not anything matched), -1 if
// it couldn't be opened/parsed at all (caller tries the next DLL).
static int pe_dll_check_exports(const char *path, const char **names, bool *out, int n) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 64) {
        fclose(f);
        return -1;
    }
    uint8_t *img = malloc((size_t)sz);
    if (!img || fread(img, 1, (size_t)sz, f) != (size_t)sz) {
        free(img);
        fclose(f);
        return -1;
    }
    fclose(f);
    int rc = -1;
    if (img[0] == 'M' && img[1] == 'Z') {
        uint32_t pe_off = pe_r32le(img + 0x3c);
        if (pe_off + 24 < (uint32_t)sz && !memcmp(img + pe_off, "PE\0\0", 4)) {
            uint16_t n_secs = pe_r16le(img + pe_off + 6);
            uint16_t opthdr_size = pe_r16le(img + pe_off + 20);
            uint32_t opt_off = pe_off + 24;
            uint16_t magic = pe_r16le(img + opt_off);
            uint32_t datadir_off = opt_off + (magic == PE32PLUS_MAGIC ? 112 : 96);
            uint32_t exp_rva = pe_r32le(img + datadir_off);
            uint32_t sec_hdr_off = opt_off + opthdr_size;
            rc = 0;
            if (exp_rva) {
                uint32_t exp_off = pe_rva_to_off(img, sec_hdr_off, n_secs, exp_rva);
                if (exp_off && exp_off + 40 <= (uint32_t)sz) {
                    uint32_t n_names = pe_r32le(img + exp_off + 24);
                    uint32_t names_rva = pe_r32le(img + exp_off + 32);
                    uint32_t names_off = pe_rva_to_off(img, sec_hdr_off, n_secs, names_rva);
                    for (uint32_t k = 0; k < n_names && names_off; k++) {
                        uint32_t name_rva = pe_r32le(img + names_off + k * 4);
                        uint32_t name_off = pe_rva_to_off(img, sec_hdr_off, n_secs, name_rva);
                        if (!name_off || name_off >= (uint32_t)sz) continue;
                        const char *ename = (const char *)(img + name_off);
                        for (int i = 0; i < n; i++)
                            if (!out[i] && !strcmp(ename, names[i])) out[i] = true;
                    }
                }
            }
        }
    }
    free(img);
    return rc;
}

// Locate a system DLL by name. Only meaningful when this file is itself
// compiled as a Windows binary (the only configuration link_pe() ever runs
// in -- see rcc_link()'s host dispatch), so system DLLs live under
// %SystemRoot%\System32.
static int pe_find_system_dll(const char *name, char *out_path, size_t out_sz) {
#ifdef _WIN32
    const char *root = getenv("SystemRoot");
    if (!root) root = getenv("windir");
    if (!root) root = "C:\\Windows";
    snprintf(out_path, out_sz, "%s\\System32\\%s", root, name);
    struct stat st;
    if (stat(out_path, &st) == 0) return 0;
#else
    (void)name;
    (void)out_path;
    (void)out_sz;
#endif
    return -1;
}

// A single 4-byte RVA field written into the .idata section's own byte
// buffer at build time as an offset-within-section placeholder; patched to
// an absolute RVA once the section's final address is known post-layout.
typedef struct {
    size_t off;
} PePatch;

// Scan for undefined strong (non-weak) symbols, resolve each against the
// system DLLs, and build a complete .idata import table plus one .text
// trampoline per resolved import. Returns 0 on success (including the
// trivial case of zero imports needed), -1 if any strong symbol could not
// be resolved against any known DLL (caller falls back to GCC's linker).
static int build_pe_imports(LinkState *s, int *idata_sec_out,
                            PePatch **patches_out, int *n_patches_out,
                            uint64_t *iat_off_out, uint64_t *iat_size_out,
                            uint64_t *import_dir_size_out,
                            const char ***imported_names_out, int *n_imported_out) {
    *idata_sec_out = -1;
    *patches_out = NULL;
    *n_patches_out = 0;
    *iat_off_out = 0;
    *iat_size_out = 0;
    *import_dir_size_out = 0;
    *imported_names_out = NULL;
    *n_imported_out = 0;
    // Collect distinct undefined strong symbol names.
    int n_undef = 0, cap_undef = 0;
    int *undef_idx = NULL;
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec >= 0 || sym->bind == 2 /* weak */ || !sym->name[0]) continue;
        if (n_undef == cap_undef) {
            cap_undef = cap_undef ? cap_undef * 2 : 16;
            undef_idx = realloc(undef_idx, (size_t)cap_undef * sizeof(int));
        }
        undef_idx[n_undef++] = i;
    }
    if (n_undef == 0) {
        free(undef_idx);
        return 0;
    }

    const char **names = malloc((size_t)n_undef * sizeof(char *));
    for (int i = 0; i < n_undef; i++) names[i] = s->syms[undef_idx[i]].name;

    // For each undefined symbol, which pe_default_dlls[] index resolves it
    // (-1 = unresolved).
    int *owner_dll = malloc((size_t)n_undef * sizeof(int));
    for (int i = 0; i < n_undef; i++) owner_dll[i] = -1;
    bool *found_this_dll = malloc((size_t)n_undef * sizeof(bool));

    bool any_dll_used[16] = {false};
    for (int d = 0; pe_default_dlls[d]; d++) {
        char dll_path[512];
        if (pe_find_system_dll(pe_default_dlls[d], dll_path, sizeof(dll_path)) != 0)
            continue;
        memset(found_this_dll, 0, (size_t)n_undef * sizeof(bool));
        if (pe_dll_check_exports(dll_path, names, found_this_dll, n_undef) != 0)
            continue;
        for (int i = 0; i < n_undef; i++) {
            if (owner_dll[i] < 0 && found_this_dll[i]) {
                owner_dll[i] = d;
                if (d < 16) any_dll_used[d] = true;
            }
        }
    }
    free(found_this_dll);

    bool all_resolved = true;
    for (int i = 0; i < n_undef; i++)
        if (owner_dll[i] < 0) all_resolved = false;
    if (!all_resolved) {
        free(names);
        free(owner_dll);
        free(undef_idx);
        return -1;
    }

    int idata_sec = link_find_or_create_sec(s, ".idata", true, true, false, false, false, 8);
    int text_sec = link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);

    // Layout the .idata contents as offsets-within-section first; patch to
    // absolute RVAs after link_layout() assigns the section's address.
    int n_dlls = 0;
    for (int d = 0; d < 16; d++)
        if (any_dll_used[d]) n_dlls++;

    PePatch *patches = malloc((size_t)(n_dlls * 3 + n_undef * 2) * sizeof(PePatch));
    int n_patches = 0;

    // Import Directory Table: (n_dlls + 1) * 20 bytes, zero-filled.
    size_t descriptors_off = link_sec_append(s, idata_sec, (const uint8_t *)"", 0, 8);
    uint8_t *zero20 = calloc((size_t)(n_dlls + 1) * 20, 1);
    link_sec_append(s, idata_sec, zero20, (size_t)(n_dlls + 1) * 20, 8);
    free(zero20);

    // DLL name strings.
    size_t *dll_name_off = calloc((size_t)16, sizeof(size_t));
    for (int d = 0; d < 16; d++) {
        if (!any_dll_used[d]) continue;
        dll_name_off[d] = link_sec_append(s, idata_sec,
                                          (const uint8_t *)pe_default_dlls[d], strlen(pe_default_dlls[d]) + 1, 1);
    }

    // Hint/name entries: one per import, 2-byte hint(0) + name + NUL,
    // padded to even length.
    size_t *hintname_off = malloc((size_t)n_undef * sizeof(size_t));
    for (int i = 0; i < n_undef; i++) {
        size_t namelen = strlen(names[i]);
        size_t entlen = 2 + namelen + 1;
        if (entlen & 1) entlen++;
        uint8_t *ent = calloc(entlen, 1);
        memcpy(ent + 2, names[i], namelen);
        hintname_off[i] = link_sec_append(s, idata_sec, ent, entlen, 2);
        free(ent);
    }

    // ILT then IAT, grouped per DLL, each terminated by an 8-byte zero
    // entry. iat_slot_off[i] is where this import's IAT pointer lives --
    // that is the address the loader overwrites with the real DLL function
    // pointer, and what our trampoline's indirect jump targets.
    size_t *ilt_start_off = calloc((size_t)16, sizeof(size_t));
    size_t *iat_start_off = calloc((size_t)16, sizeof(size_t));
    size_t *iat_slot_off = malloc((size_t)n_undef * sizeof(size_t));
    for (int d = 0; d < 16; d++) {
        if (!any_dll_used[d]) continue;
        ilt_start_off[d] = link_sec_append(s, idata_sec, (const uint8_t *)"", 0, 8);
        for (int i = 0; i < n_undef; i++) {
            if (owner_dll[i] != d) continue;
            uint8_t ent[8];
            pe_w64le_m(ent, hintname_off[i]); // placeholder: offset-within-section
            link_sec_append(s, idata_sec, ent, 8, 8);
            patches[n_patches++].off = s->secs[idata_sec].len - 8;
        }
        uint8_t zero8[8] = {0};
        link_sec_append(s, idata_sec, zero8, 8, 8);
    }
    for (int d = 0; d < 16; d++) {
        if (!any_dll_used[d]) continue;
        iat_start_off[d] = link_sec_append(s, idata_sec, (const uint8_t *)"", 0, 8);
        for (int i = 0; i < n_undef; i++) {
            if (owner_dll[i] != d) continue;
            uint8_t ent[8];
            pe_w64le_m(ent, hintname_off[i]); // placeholder: offset-within-section
            link_sec_append(s, idata_sec, ent, 8, 8);
            iat_slot_off[i] = s->secs[idata_sec].len - 8;
            patches[n_patches++].off = iat_slot_off[i];
        }
        uint8_t zero8[8] = {0};
        link_sec_append(s, idata_sec, zero8, 8, 8);
    }
    uint64_t iat_region_off = 0, iat_region_size = 0;
    for (int d = 0; d < 16; d++) {
        if (!any_dll_used[d]) continue;
        iat_region_off = iat_start_off[d];
        break;
    }
    iat_region_size = (uint64_t)s->secs[idata_sec].len - iat_region_off;

    // Fill the descriptor table now that every referenced offset is known.
    for (int d = 0, k = 0; d < 16; d++) {
        if (!any_dll_used[d]) continue;
        uint8_t *desc = s->secs[idata_sec].data + descriptors_off + (size_t)k * 20;
        pe_w32le_m(desc, (uint32_t)ilt_start_off[d]); // OriginalFirstThunk
        pe_w32le_m(desc + 4, 0); // TimeDateStamp
        pe_w32le_m(desc + 8, 0); // ForwarderChain
        pe_w32le_m(desc + 12, (uint32_t)dll_name_off[d]); // Name
        pe_w32le_m(desc + 16, (uint32_t)iat_start_off[d]); // FirstThunk
        patches[n_patches++].off = descriptors_off + (size_t)k * 20;
        patches[n_patches++].off = descriptors_off + (size_t)k * 20 + 12;
        patches[n_patches++].off = descriptors_off + (size_t)k * 20 + 16;
        k++;
    }

    // One 6-byte `jmp qword ptr [rip+disp32]` trampoline per import,
    // redefining the original undefined symbol to its address. The
    // relocation uses the same -4 addend convention as a normal COFF
    // REL32 call/jmp (see pe_pc32_addend).
    for (int i = 0; i < n_undef; i++) {
        uint8_t stub[6] = {0xFF, 0x25, 0, 0, 0, 0};
        size_t stub_off = link_sec_append(s, text_sec, stub, sizeof(stub), 16);
        char impname[300];
        snprintf(impname, sizeof(impname), "__imp_%s", names[i]);
        int imp_sym = link_add_sym(s, impname, idata_sec, (uint64_t)iat_slot_off[i],
                                   8, 1 /* global */, 1 /* object */, -1);
        link_add_reloc(s, text_sec, stub_off + 2, RL_PC32, imp_sym, -4);
        link_add_sym(s, names[i], text_sec, (uint64_t)stub_off, 0,
                     1 /* global */, 2 /* func */, -1);
    }

    *idata_sec_out = idata_sec;
    *patches_out = patches;
    *n_patches_out = n_patches;
    *iat_off_out = iat_region_off;
    *iat_size_out = iat_region_size;
    *import_dir_size_out = (uint64_t)(n_dlls + 1) * 20;
    *imported_names_out = names;
    *n_imported_out = n_undef;

    free(owner_dll);
    free(undef_idx);
    free(dll_name_off);
    free(hintname_off);
    free(ilt_start_off);
    free(iat_start_off);
    free(iat_slot_off);
    return 0;
}

static void pe_patch_idata(LinkState *s, int idata_sec, uint64_t image_base,
                           PePatch *patches, int n_patches) {
    if (idata_sec < 0) return;
    uint64_t sec_rva = s->secs[idata_sec].addr - image_base;
    uint8_t *data = s->secs[idata_sec].data;
    for (int i = 0; i < n_patches; i++) {
        uint32_t v = pe_r32le(data + patches[i].off);
        pe_w32le_m(data + patches[i].off, v + (uint32_t)sec_rva);
    }
    free(patches);
}

// ---------------------------------------------------------------------------
// -shared (.dll) output: PE export table (.edata)
// ---------------------------------------------------------------------------

// Build a DLL's export table. Real GNU ld's own default when a shared
// library has no symbol explicitly marked for export (no
// __declspec(dllexport) and no .def EXPORTS section -- rcc implements
// neither yet): export every global, defined symbol
// ("--export-all-symbols" is what binutils calls its own default in that
// case). Mirror that here. `excl_names`/`n_excl` are the C-level names
// build_pe_imports() redefined as trampolines into a system DLL -- those
// must never be re-exported under our own DLL's name. Returns the new
// section's index, or -1 if there is nothing to export.
static int build_pe_exports(LinkState *s, const char *dll_name,
                            const char **excl_names, int n_excl,
                            PePatch **patches_out, int *n_patches_out) {
    *patches_out = NULL;
    *n_patches_out = 0;
    int n_exp = 0, cap_exp = 0;
    int *exp_idx = NULL;
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec < 0 || sym->bind != 1 /* global */ || !sym->name[0]) continue;
        if (!strncmp(sym->name, "__imp_", 6)) continue;
        bool excluded = false;
        for (int k = 0; k < n_excl; k++)
            if (!strcmp(sym->name, excl_names[k])) {
                excluded = true;
                break;
            }
        if (excluded) continue;
        if (n_exp == cap_exp) {
            cap_exp = cap_exp ? cap_exp * 2 : 32;
            exp_idx = realloc(exp_idx, (size_t)cap_exp * sizeof(int));
        }
        exp_idx[n_exp++] = i;
    }
    if (n_exp == 0) {
        free(exp_idx);
        return -1;
    }
    // Sort alphabetically by name: AddressOfNames must be sorted so a
    // name-based binary search (real GetProcAddress, and any consumer
    // that mimics it) resolves correctly.
    for (int i = 0; i < n_exp; i++)
        for (int j = i + 1; j < n_exp; j++)
            if (strcmp(s->syms[exp_idx[j]].name, s->syms[exp_idx[i]].name) < 0) {
                int t = exp_idx[i];
                exp_idx[i] = exp_idx[j];
                exp_idx[j] = t;
            }

    int edata_sec = link_find_or_create_sec(s, ".edata", true, false, false, false, false, 4);

    // IMAGE_EXPORT_DIRECTORY (40 bytes). Its RVA fields (Name and the
    // three AddressOf* table pointers) are self-relative -- they point
    // within this same section -- so they use the same "offset-within-
    // section written now, patched by += the section's own final RVA
    // later" convention build_pe_imports() uses for .idata (see
    // pe_patch_idata(), reused verbatim below by the caller).
    size_t dir_off = link_sec_append(s, edata_sec, (const uint8_t *)"", 0, 4);
    uint8_t dir[40] = {0};
    link_sec_append(s, edata_sec, dir, sizeof(dir), 4);

    size_t dllname_off = link_sec_append(s, edata_sec, (const uint8_t *)dll_name,
                                         strlen(dll_name) + 1, 1);

    PePatch *patches = malloc((size_t)(4 + n_exp) * sizeof(PePatch));
    int n_patches = 0;

    // Export Address Table: one RVA per export. Unlike the fields above,
    // each entry's target lives in an arbitrary *other* section (.text/
    // .data/.bss) whose final address isn't known here either -- resolve
    // it with an ordinary deferred RL_ADDR32NB relocation against the
    // exported symbol instead, the same mechanism normal code/data
    // references use (see link_apply_relocs()'s later pass).
    size_t eat_off = link_sec_append(s, edata_sec, (const uint8_t *)"", 0, 4);
    uint8_t *zero_eat = calloc((size_t)n_exp, 4);
    link_sec_append(s, edata_sec, zero_eat, (size_t)n_exp * 4, 4);
    free(zero_eat);
    for (int i = 0; i < n_exp; i++)
        link_add_reloc(s, edata_sec, eat_off + (size_t)i * 4, RL_ADDR32NB, exp_idx[i], 0);

    // Export name strings, then the (alphabetically sorted) name-pointer
    // table and its parallel ordinal-index table (index into the EAT
    // above -- names and functions share the same sorted order here, so
    // ordinal[i] == i).
    size_t *name_str_off = malloc((size_t)n_exp * sizeof(size_t));
    for (int i = 0; i < n_exp; i++)
        name_str_off[i] = link_sec_append(s, edata_sec,
                                          (const uint8_t *)s->syms[exp_idx[i]].name,
                                          strlen(s->syms[exp_idx[i]].name) + 1, 1);

    size_t names_off = link_sec_append(s, edata_sec, (const uint8_t *)"", 0, 4);
    for (int i = 0; i < n_exp; i++) {
        uint8_t ent[4];
        pe_w32le_m(ent, (uint32_t)name_str_off[i]); // placeholder: offset-within-section
        link_sec_append(s, edata_sec, ent, 4, 4);
        patches[n_patches++].off = s->secs[edata_sec].len - 4;
    }
    free(name_str_off);

    size_t ords_off = link_sec_append(s, edata_sec, (const uint8_t *)"", 0, 2);
    for (int i = 0; i < n_exp; i++) {
        uint8_t ord[2];
        ord[0] = (uint8_t)i;
        ord[1] = (uint8_t)(i >> 8);
        link_sec_append(s, edata_sec, ord, 2, 2);
    }
    // Real linkers' consumers (notably binutils' pe_print_edata(), the
    // engine behind `objdump -p`'s export-table dump) flag the ordinal
    // table as "invalid" when its end lands EXACTLY on the Export
    // Directory's declared byte size (that check is `>=`, not `>` --
    // see CVE-2014-8502's fix). Since the data-directory size we report
    // is this section's exact final length and the ordinal table is the
    // last thing appended, pad a few trailing bytes so there is always
    // slack past it.
    link_sec_append(s, edata_sec, (const uint8_t *)"\0\0\0\0", 4, 1);

    // Fill in the directory now that every offset-within-.edata is known.
    uint8_t *d = s->secs[edata_sec].data + dir_off;
    pe_w32le_m(d + 12, (uint32_t)dllname_off); // Name
    pe_w32le_m(d + 16, 1); // Base (ordinal base)
    pe_w32le_m(d + 20, (uint32_t)n_exp); // NumberOfFunctions
    pe_w32le_m(d + 24, (uint32_t)n_exp); // NumberOfNames
    pe_w32le_m(d + 28, (uint32_t)eat_off); // AddressOfFunctions
    pe_w32le_m(d + 32, (uint32_t)names_off); // AddressOfNames
    pe_w32le_m(d + 36, (uint32_t)ords_off); // AddressOfNameOrdinals
    patches[n_patches++].off = dir_off + 12;
    patches[n_patches++].off = dir_off + 28;
    patches[n_patches++].off = dir_off + 32;
    patches[n_patches++].off = dir_off + 36;

    free(exp_idx);
    *patches_out = patches;
    *n_patches_out = n_patches;
    return edata_sec;
}

// ---------------------------------------------------------------------------
// Main PE link entry point
// ---------------------------------------------------------------------------

// Lay out sections for a PE image: unlike ELF (which packs multiple small
// read-only/read-write sections into shared pages within one PT_LOAD
// segment), the Windows loader maps every PE section header as its own
// independently-protected region and expects each one's VirtualAddress to
// start on its own SectionAlignment (page) boundary -- real link.exe/
// lld-link output never packs .rdata/.xdata/.pdata etc. together the way
// ELF's link_layout() does. `base` already accounts for the one page
// reserved for the PE/COFF headers (the caller passes base + SECTION_ALIGN).
static void pe_layout_sections(LinkState *s, uint64_t base) {
    uint64_t addr = base;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        // Real linkers never emit a section header (let alone reserve VA
        // space) for a section nothing was ever appended to -- .data/
        // .rdata/.bss are created unconditionally up front and often end
        // up empty. Giving them a VA slot anyway (the pre-existing
        // "reserve space even for empty sections" behavior) leaves a
        // wasted gap in the address space between real sections that no
        // real PE ever has and that the section-header write pass below
        // skips right past anyway (see "if (!sec->alloc || sec->len ==
        // 0) continue;"), desyncing what the layout reserved from what
        // actually got written.
        if (!sec->alloc || sec->len == 0) continue;
        addr = pe_align_up(addr, PE_SECTION_ALIGN);
        sec->addr = addr;
        addr += sec->len;
    }
}

// IMAGE_REL_BASED_* relocation types used in .reloc block entries.
#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_DIR64    10

// Build the PE base relocation table (.reloc) from every RL_ABS64/RL_ABS32
// site recorded across all other sections, now that every section has its
// final layout address. Real linkers always pair DYNAMIC_BASE with a
// backing .reloc section (even one with very few entries) -- our own
// codegen uses RIP-relative addressing everywhere in practice, so this is
// typically empty, but must still exist structurally for any absolute-VA
// reference (e.g. a function pointer table) that does show up. Returns the
// new section's index, or -1 if there was nothing to relocate (in which
// case DYNAMIC_BASE must NOT be claimed, since there is no data to back
// it under system-enforced Mandatory ASLR).
static int build_pe_reloc(LinkState *s, uint64_t base) {
    // Collect (page RVA, offset-in-page) pairs for every ABS64/ABS32 site.
    int n_sites = 0, cap_sites = 0;
    uint32_t *site_rva = NULL;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || strcmp(sec->name, ".reloc") == 0) continue;
        for (int j = 0; j < sec->n_relocs; j++) {
            LinkReloc *r = &sec->relocs[j];
            if (r->type != RL_ABS64 && r->type != RL_ABS32 && r->type != RL_ABS32U) continue;
            if (n_sites == cap_sites) {
                cap_sites = cap_sites ? cap_sites * 2 : 16;
                site_rva = realloc(site_rva, (size_t)cap_sites * sizeof(uint32_t));
            }
            site_rva[n_sites++] = (uint32_t)(sec->addr + r->offset - base);
        }
    }
    if (n_sites == 0) {
        free(site_rva);
        return -1;
    }
    // Sort by RVA so entries group into contiguous per-page blocks.
    for (int i = 0; i < n_sites; i++)
        for (int j = i + 1; j < n_sites; j++)
            if (site_rva[j] < site_rva[i]) {
                uint32_t t = site_rva[i];
                site_rva[i] = site_rva[j];
                site_rva[j] = t;
            }

    int reloc_sec = link_find_or_create_sec(s, ".reloc", true, false, false, false, false, 4);
    int i = 0;
    while (i < n_sites) {
        uint32_t page = site_rva[i] & ~(uint32_t)0xfff;
        int j = i;
        while (j < n_sites && (site_rva[j] & ~(uint32_t)0xfff) == page) j++;
        int n_entries = j - i;
        bool pad = (n_entries & 1) != 0;
        uint32_t block_size = 8 + (uint32_t)(n_entries + (pad ? 1 : 0)) * 2;
        uint8_t *block = calloc(block_size, 1);
        pe_w32le_m(block, page);
        pe_w32le_m(block + 4, block_size);
        for (int k = 0; k < n_entries; k++) {
            uint16_t off_in_page = (uint16_t)(site_rva[i + k] - page);
            uint16_t entry = (uint16_t)((IMAGE_REL_BASED_DIR64 << 12) | off_in_page);
            block[8 + k * 2] = (uint8_t)entry;
            block[8 + k * 2 + 1] = (uint8_t)(entry >> 8);
        }
        // Padding entry (IMAGE_REL_BASED_ABSOLUTE = 0) stays zero-filled.
        link_sec_append(s, reloc_sec, block, block_size, 4);
        free(block);
        i = j;
    }
    free(site_rva);
    return reloc_sec;
}

int link_pe(LinkState *s) {
    // Static libgcc/libmingwex linking is not implemented -- only a
    // dynamically-linked .exe (against system DLLs via the synthesized
    // CRT stub) or .dll (-shared, with an auto-exported .edata table).
    // Fall back to the mingw toolchain for static output.
    if (s->opt_static) return -1;
    // Create standard sections
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 16);
    link_find_or_create_sec(s, ".rdata", true, false, false, false, false, 16);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 16);

    // Find entry point. A DLL's is optional: AddressOfEntryPoint == 0
    // tells the loader to skip DLL_PROCESS_ATTACH/DETACH notifications
    // entirely, which is fine for a DLL that only exports plain
    // functions. If the user defined DllMain, its C signature already
    // matches the loader's Win64 calling convention exactly (rcx =
    // hinstDLL, rdx = fdwReason, r8 = lpvReserved, eax = BOOL return),
    // so it can be used directly with no synthesized stub -- unlike an
    // .exe's `main`, which needs argc/argv marshaled from the CRT.
    int entry_sym = -1;
    if (s->opt_shared) {
        entry_sym = link_find_sym(s, "DllMain");
    } else {
        entry_sym = link_find_sym(s, "mainCRTStartup");
        if (entry_sym < 0) {
            int main_sym = link_find_sym(s, "main");
            if (main_sym < 0) main_sym = link_find_sym(s, "_main");
            if (main_sym < 0) main_sym = link_find_sym(s, "WinMain");
            if (main_sym < 0) {
                // No known entry point at all: nothing we could run even with
                // a synthesized stub. Fall back to the mingw toolchain.
                return -1;
            }
            // We don't auto-load mingw's own CRT startup (crt2.o pulls in
            // libmingw32.a/libmingwex.a via full archive resolution, which
            // this linker doesn't implement yet), so there is no real
            // mainCRTStartup to call. Synthesize a minimal replacement that
            // still gets real argc/argv: call msvcrt's own __getmainargs
            // (exactly what a real mingw mainCRTStartup calls internally)
            // rather than passing argc=0/argv=NULL and silently breaking
            // every program that inspects its own arguments. Still skips
            // what a real CRT would also do beyond that: atexit handlers,
            // environ population, and C++ static initializers.
            //
            // int __cdecl __getmainargs(int *_Argc, char ***_Argv,
            //     char ***_Env, int _DoWildCard, _startupinfo *_StartInfo);
            // (_startupinfo is a single int, "newmode" -- zeroed, meaning
            // "don't change the floating-point/heap error mode").
            //
            // Stack layout after `sub rsp, 0x50` (80 bytes, 16-aligned):
            //   [rsp+0x00..0x1f]  32-byte shadow space (callee's, unused by us)
            //   [rsp+0x20]        5th arg slot: &startinfo
            //   [rsp+0x28]        padding
            //   [rsp+0x30]        argc  (out)
            //   [rsp+0x38]        argv  (out)
            //   [rsp+0x40]        env   (out, unused after the call)
            //   [rsp+0x48]        startinfo.newmode (in: 0, out: unused)
            int text_sec = link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
            int exitprocess_sym = link_add_sym(s, "ExitProcess", -1, 0, 0, 1 /* global */, 2 /* func */, -1);
            int getmainargs_sym = link_add_sym(s, "__getmainargs", -1, 0, 0, 1 /* global */, 2 /* func */, -1);
            uint8_t stub[] = {
                0x48,
                0x83,
                0xE4,
                0xF0, // and rsp, -16
                0x48,
                0x83,
                0xEC,
                0x50, // sub rsp, 0x50
                0xC7,
                0x44,
                0x24,
                0x48,
                0x00,
                0x00,
                0x00,
                0x00, // mov dword [rsp+0x48], 0  (newmode = 0)
                0x48,
                0x8D,
                0x4C,
                0x24,
                0x30, // lea rcx, [rsp+0x30]      (&argc)
                0x48,
                0x8D,
                0x54,
                0x24,
                0x38, // lea rdx, [rsp+0x38]      (&argv)
                0x4C,
                0x8D,
                0x44,
                0x24,
                0x40, // lea r8,  [rsp+0x40]      (&env)
                0x45,
                0x31,
                0xC9, // xor r9d, r9d                        (expand_wildcards = 0)
                0x48,
                0x8D,
                0x44,
                0x24,
                0x48, // lea rax, [rsp+0x48]      (&startinfo)
                0x48,
                0x89,
                0x44,
                0x24,
                0x20, // mov [rsp+0x20], rax      (5th arg on stack)
                0xE8,
                0,
                0,
                0,
                0, // call __getmainargs                  (disp32 @ +0x2d)
                0x8B,
                0x4C,
                0x24,
                0x30, // mov ecx, [rsp+0x30]            (argc)
                0x48,
                0x8B,
                0x54,
                0x24,
                0x38, // mov rdx, [rsp+0x38]      (argv)
                0xE8,
                0,
                0,
                0,
                0, // call main                           (disp32 @ +0x3b)
                0x89,
                0xC1, // mov ecx, eax
                0xE8,
                0,
                0,
                0,
                0, // call ExitProcess                    (disp32 @ +0x42)
            };
            uint64_t stub_off = link_sec_append(s, text_sec, stub, sizeof(stub), 16);
            link_add_reloc(s, text_sec, stub_off + 0x2d, RL_PC32, getmainargs_sym, -4);
            link_add_reloc(s, text_sec, stub_off + 0x3b, RL_PC32, main_sym, -4);
            link_add_reloc(s, text_sec, stub_off + 0x42, RL_PC32, exitprocess_sym, -4);
            entry_sym = link_add_sym(s, "_rcc_pe_start", text_sec, stub_off, sizeof(stub),
                                     1 /* global */, 2 /* func */, -1);
        }
    }

    // Resolve external symbols against system DLLs and synthesize the
    // import table + trampolines before layout, since this may add new
    // sections/symbols that layout and relocation application must see.
    int idata_sec = -1, n_idata_patches = 0;
    PePatch *idata_patches = NULL;
    uint64_t iat_off = 0, iat_size = 0, import_dir_size = 0;
    const char **imported_names = NULL;
    int n_imported = 0;
    if (build_pe_imports(s, &idata_sec, &idata_patches, &n_idata_patches,
                         &iat_off, &iat_size, &import_dir_size,
                         &imported_names, &n_imported) != 0) {
        // A strong symbol we can't resolve against any known DLL (or an
        // exotic construct like TLS/weak imports we don't model yet).
        // Fall back to the mingw toolchain's own linker.
        return -1;
    }

    // For -shared, build the export table (.edata) before layout too, for
    // the same reason: it may add a new section that layout must see.
    // The DLL's own advertised name is its output file's basename (what
    // a consumer's import table records and LoadLibrary searches for).
    int edata_sec = -1, n_edata_patches = 0;
    PePatch *edata_patches = NULL;
    if (s->opt_shared) {
        const char *base_name = strrchr(s->out_path, '/');
#if defined(_WIN32) || defined(__MINGW32__)
        const char *bslash = strrchr(s->out_path, '\\');
        if (bslash && (!base_name || bslash > base_name)) base_name = bslash;
#endif
        base_name = base_name ? base_name + 1 : s->out_path;
        edata_sec = build_pe_exports(s, base_name, imported_names, n_imported,
                                     &edata_patches, &n_edata_patches);
    }
    free(imported_names);

    // Layout sections: one page per section (see pe_layout_sections), with
    // the first page reserved for the PE/COFF headers so .text starts at
    // RVA 0x1000, not RVA 0 (which would overlap the header region every
    // PE loader always maps there). DLLs default to a different preferred
    // base than EXEs so a process that loads both at their preferred
    // addresses (the common case: no .reloc data, so no ASLR rebasing)
    // never collides.
    uint64_t base = s->opt_shared ? 0x180000000ULL : 0x140000000ULL;
    pe_layout_sections(s, base + PE_SECTION_ALIGN);

    // Fix up the import/export tables' internal (self-relative) RVAs now
    // that .idata/.edata have their final addresses, then apply ordinary
    // relocations (this also resolves the trampolines' RIP-relative jumps
    // into the IAT and the export table's per-symbol RL_ADDR32NB entries).
    pe_patch_idata(s, idata_sec, base, idata_patches, n_idata_patches);
    pe_patch_idata(s, edata_sec, base, edata_patches, n_edata_patches);
    link_apply_relocs(s, base);

    // Build the base relocation table from every ABS64/ABS32 site now
    // that section addresses are final, then re-run layout (idempotent
    // for every pre-existing section -- their addr/len are unchanged) so
    // the newly-appended .reloc section gets a real address too.
    int reloc_sec = build_pe_reloc(s, base);
    if (reloc_sec >= 0) pe_layout_sections(s, base + PE_SECTION_ALIGN);

    // Entry point
    uint64_t entry_addr = 0;
    if (entry_sym >= 0) entry_addr = pe_symbol_address(s, entry_sym);

    // Count output sections and compute image size
    int n_output_secs = 0;
    uint64_t image_end = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;
        n_output_secs++;
        uint64_t end = sec->addr + pe_align_up(sec->len, PE_SECTION_ALIGN);
        if (end > image_end) image_end = end;
    }
    uint64_t size_of_image = pe_align_up(image_end - base, PE_SECTION_ALIGN);

    // Header sizes
    uint32_t dos_stub_size = 64;
    // PE32+ optional header: 112 fixed bytes (through NumberOfRvaAndSizes)
    // plus 16 data directory entries * 8 bytes = 240 total.
    uint32_t opthdr_size = 112 + 16 * 8;
    uint32_t hdr_off = 64 + dos_stub_size + 4 + 20 + opthdr_size; // end of optional header
    uint32_t sec_hdr_total = (uint32_t)n_output_secs * 40;
    uint32_t hdr_total = hdr_off + sec_hdr_total;
    uint32_t hdr_file_size = (uint32_t)pe_align_up(hdr_total, PE_FILE_ALIGN);

    // Open output file
    FILE *f = fopen(s->out_path, "wb");
    if (!f) {
        fprintf(stderr, "rcc: link: cannot create %s: %s\n",
                s->out_path, strerror(errno));
        return -1;
    }

    // --- DOS Header ---
    static const uint8_t dos_stub[] =
        "This program cannot be run in DOS mode.\r\n$";
    pe_w16le(f, 0x5A4D); // MZ
    pe_w16le(f, 0x0090);
    pe_w16le(f, 0x0003); // size fields
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0x0004); // reloc, hdr paras
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0xFFFF);
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0x00B8); // SS:SP
    pe_w16le(f, 0x0000); // checksum
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0x0000); // IP:CS
    pe_w16le(f, 0x0040);
    pe_w16le(f, 0x0000); // reloc off, overlay
    for (int i = 0; i < 4; i++) pe_w16le(f, 0);
    pe_w16le(f, 0);
    pe_w16le(f, 0); // OEM
    for (int i = 0; i < 10; i++) pe_w16le(f, 0);
    pe_w32le(f, 64 + dos_stub_size); // e_lfanew
    pe_wbuf(f, dos_stub, sizeof(dos_stub) - 1);
    pe_wzeros(f, dos_stub_size - (sizeof(dos_stub) - 1));

    // --- PE Signature ---
    pe_wbuf(f, "PE\0\0", 4);

    // --- COFF Header ---
    uint16_t machine = IMAGE_FILE_MACHINE_AMD64;
    if (s->arch == ARCH_AARCH64) machine = IMAGE_FILE_MACHINE_ARM64;
    pe_w16le(f, machine);
    pe_w16le(f, (uint16_t)n_output_secs);
    pe_w32le(f, 0);
    pe_w32le(f, 0);
    pe_w32le(f, 0); // timestamp, symtab, nsyms
    pe_w16le(f, (uint16_t)opthdr_size); // sizeof optional header
    pe_w16le(f, (uint16_t)(0x0022 | (s->opt_shared ? IMAGE_FILE_DLL : 0))); // EXECUTABLE | LARGE_ADDRESS_AWARE [| DLL]

    // --- Optional Header (PE32+) ---
    uint32_t size_of_code = 0, size_of_init_data = 0, size_of_uninit_data = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;
        uint32_t raw = sec->is_bss ? (uint32_t)pe_align_up(sec->len, PE_SECTION_ALIGN)
                                   : (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
        if (sec->exec) size_of_code += raw;
        else if (sec->is_bss)
            size_of_uninit_data += raw;
        else
            size_of_init_data += raw;
    }
    pe_w16le(f, PE32PLUS_MAGIC);
    fputc(0, f);
    fputc(0, f); // linker ver
    pe_w32le(f, size_of_code);
    pe_w32le(f, size_of_init_data);
    pe_w32le(f, size_of_uninit_data);
    pe_w32le(f, entry_addr ? (uint32_t)(entry_addr - base) : 0); // RVA of entry (0 = none, valid for a DLL)
    pe_w32le(f, 0x1000); // base of code RVA
    pe_w64le(f, base);
    pe_w32le(f, PE_SECTION_ALIGN);
    pe_w32le(f, PE_FILE_ALIGN);
    pe_w16le(f, 6);
    pe_w16le(f, 0); // OS ver
    pe_w16le(f, 0);
    pe_w16le(f, 0); // image ver
    pe_w16le(f, 6);
    pe_w16le(f, 0); // subsystem ver
    pe_w32le(f, 0); // Win32Version
    pe_w32le(f, (uint32_t)size_of_image);
    pe_w32le(f, hdr_file_size); // SizeOfHeaders
    pe_w32le(f, 0); // CheckSum
    pe_w16le(f, IMAGE_SUBSYSTEM_WINDOWS_CUI);
    // DYNAMIC_BASE is only claimed when backed by a real .reloc section --
    // under system-enforced Mandatory ASLR, an image claiming ASLR support
    // with no relocation data to back it up can be rejected outright by
    // the loader ("not a valid application for this OS platform") instead
    // of just silently loading at the fixed preferred address.
    uint16_t dll_flags = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    if (reloc_sec >= 0) dll_flags |= IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    pe_w16le(f, dll_flags);
    pe_w64le(f, 0x100000);
    pe_w64le(f, 0x1000); // stack
    pe_w64le(f, 0x100000);
    pe_w64le(f, 0x1000); // heap
    pe_w32le(f, 0); // loader flags
    pe_w32le(f, 16); // number of directory entries

    // Data directories (16 entries). Index 0 = Export Table (.edata, DLL
    // output only), index 1 = Import Table, index 3 = Exception Table
    // (.pdata, if our own COFF codegen emitted Win64 SEH unwind info for
    // this object), index 5 = Base Relocation Table (.reloc), index 12 =
    // IAT (per IMAGE_DIRECTORY_ENTRY_EXPORT / _IMPORT / _EXCEPTION /
    // _BASERELOC / _IAT).
    int pdata_sec = -1;
    for (int i = 0; i < s->n_secs; i++) {
        if (s->secs[i].alloc && s->secs[i].len > 0 &&
            strcmp(s->secs[i].name, ".pdata") == 0) {
            pdata_sec = i;
            break;
        }
    }
    for (int i = 0; i < 16; i++) {
        uint32_t rva = 0, size = 0;
        if (idata_sec >= 0) {
            uint64_t sec_base = s->secs[idata_sec].addr - base;
            if (i == 1) {
                rva = (uint32_t)sec_base;
                size = (uint32_t)import_dir_size;
            } else if (i == 12) {
                rva = (uint32_t)(sec_base + iat_off);
                size = (uint32_t)iat_size;
            }
        }
        if (i == 0 && edata_sec >= 0) {
            rva = (uint32_t)(s->secs[edata_sec].addr - base);
            size = (uint32_t)s->secs[edata_sec].len;
        }
        if (i == 3 && pdata_sec >= 0) {
            rva = (uint32_t)(s->secs[pdata_sec].addr - base);
            size = (uint32_t)s->secs[pdata_sec].len;
        }
        if (i == 5 && reloc_sec >= 0) {
            rva = (uint32_t)(s->secs[reloc_sec].addr - base);
            size = (uint32_t)s->secs[reloc_sec].len;
        }
        pe_w32le(f, rva);
        pe_w32le(f, size);
    }

    // --- Section Headers ---
    // We need to know file offsets before writing; compute them in a first pass.
    // BSS sections have no file content (SizeOfRawData/PointerToRawData = 0,
    // matching link_load_object's own convention) and so must be skipped
    // here exactly like the "Write Section Data" pass below skips them --
    // otherwise this pass reserves phantom file space for bss that the
    // write pass never actually consumes, desyncing every later section's
    // real file position from what its own header claims.
    uint32_t *sec_file_offs = calloc((size_t)s->n_secs, sizeof(uint32_t));
    uint32_t cur_file_off = hdr_file_size;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0 || sec->is_bss) continue;
        sec_file_offs[i] = cur_file_off;
        cur_file_off += (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
    }

    uint32_t sec_idx = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;

        char sname[8] = {0};
        memcpy(sname, sec->name, 8);
        sname[7] = '\0';
        pe_wbuf(f, sname, 8);

        uint32_t virt_size = (uint32_t)pe_align_up(sec->len, PE_SECTION_ALIGN);
        uint32_t virt_addr = (uint32_t)(sec->addr - base);
        uint32_t raw_size = sec->is_bss ? 0 : (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
        uint32_t raw_off = sec->is_bss ? 0 : sec_file_offs[i];
        uint32_t flags = IMAGE_SCN_MEM_READ;
        if (sec->exec)
            flags |= IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
        else if (sec->is_bss)
            flags |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
        else
            flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;
        if (sec->write) flags |= IMAGE_SCN_MEM_WRITE;

        pe_w32le(f, virt_size);
        pe_w32le(f, virt_addr);
        pe_w32le(f, raw_size);
        pe_w32le(f, raw_off);
        pe_w32le(f, 0);
        pe_w32le(f, 0); // reloc ptr, linenum ptr
        pe_w16le(f, 0);
        pe_w16le(f, 0); // nrelocs, nlinenums
        pe_w32le(f, flags);
        sec_idx++;
    }
    free(sec_file_offs);

    // Pad to section alignment
    uint64_t ft = ftell(f);
    if ((uint64_t)hdr_file_size > ft)
        pe_wzeros(f, (size_t)((uint64_t)hdr_file_size - ft));

    // --- Write Section Data ---
    // Collect writeable sections sorted by address
    typedef struct {
        LinkSec *sec;
        uint32_t file_off;
    } Wsec;
    int n_ws = 0;
    Wsec *ws = NULL;
    cur_file_off = hdr_file_size;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->is_bss || sec->len == 0) continue;
        ws = realloc(ws, (size_t)(n_ws + 1) * sizeof(Wsec));
        ws[n_ws].sec = sec;
        ws[n_ws].file_off = cur_file_off;
        cur_file_off += (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
        n_ws++;
    }
    // Sort by file offset
    for (int i = 0; i < n_ws; i++)
        for (int j = i + 1; j < n_ws; j++)
            if (ws[j].file_off < ws[i].file_off) {
                Wsec t = ws[i];
                ws[i] = ws[j];
                ws[j] = t;
            }

    cur_file_off = hdr_file_size;
    for (int i = 0; i < n_ws; i++) {
        long real_pos = ftell(f);
        if (real_pos >= 0 && (uint64_t)ws[i].file_off > (uint64_t)real_pos)
            pe_wzeros(f, (size_t)((uint64_t)ws[i].file_off - (uint64_t)real_pos));
        pe_wbuf(f, ws[i].sec->data, ws[i].sec->len);
        cur_file_off = ws[i].file_off +
            (uint32_t)pe_align_up(ws[i].sec->len, PE_FILE_ALIGN);
    }
    // Pad the final section out to its full file-aligned raw size -- the
    // gap-fill above only covers bytes *between* sections, never the tail
    // of the last one, which SizeOfRawData still promises the loader.
    long final_pos = ftell(f);
    if (final_pos >= 0 && (uint64_t)cur_file_off > (uint64_t)final_pos)
        pe_wzeros(f, (size_t)((uint64_t)cur_file_off - (uint64_t)final_pos));
    free(ws);

    fclose(f);
    chmod(s->out_path, 0755);
    return 0;
}

// ---------------------------------------------------------------------------
// -Wl,--out-implib: Windows import library generation
// ---------------------------------------------------------------------------

static void pe_w16le_buf(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

// Append `n` bytes to a growable buffer.
static void pe_buf_append_bytes(uint8_t **bufp, size_t *len, size_t *cap,
                                const void *data, size_t n) {
    if (*len + n > *cap) {
        size_t nc = *cap ? *cap * 2 : 256;
        while (nc < *len + n) nc *= 2;
        *bufp = realloc(*bufp, nc);
        *cap = nc;
    }
    memcpy(*bufp + *len, data, n);
    *len += n;
}

// Write one 60-byte ar member header. `name` may exceed 16 bytes: pass
// `longname_off >= 0` (an offset into the archive's "//" GNU extended
// filename table, already written) to use the `/<offset>` indirection
// form instead of the inline name field.
static void pe_ar_header(uint8_t **bufp, size_t *len, size_t *cap,
                         const char *name, long longname_off, size_t data_size) {
    char hdr[60];
    memset(hdr, ' ', sizeof(hdr));
    char namebuf[16];
    size_t nlen;
    if (longname_off >= 0) {
        nlen = (size_t)snprintf(namebuf, sizeof(namebuf), "/%ld", longname_off);
    } else {
        nlen = strlen(name);
        if (nlen > 16) nlen = 16;
        memcpy(namebuf, name, nlen);
    }
    memcpy(hdr, namebuf, nlen);
    char field[32];
    int n = snprintf(field, sizeof(field), "0");
    memcpy(hdr + 16, field, (size_t)n); // mtime
    memcpy(hdr + 28, field, (size_t)n); // uid
    memcpy(hdr + 34, field, (size_t)n); // gid
    n = snprintf(field, sizeof(field), "%o", 33188 /* 0100644: regular file, rw-r--r-- */);
    memcpy(hdr + 40, field, (size_t)n); // mode
    n = snprintf(field, sizeof(field), "%zu", data_size);
    memcpy(hdr + 48, field, (size_t)n); // size
    hdr[58] = 0x60;
    hdr[59] = 0x0a;
    pe_buf_append_bytes(bufp, len, cap, hdr, sizeof(hdr));
}

// ---------------------------------------------------------------------------
// Synthesize the GNU-ld "long form" import objects a real dlltool/
// `ld --out-implib` writes: three or more plain relocatable COFF .o
// members per DLL, built entirely from ordinary `.idata$2`.`.idata$7`
// chunk sections that ld's *own* default PE linker script concatenates
// into a real Import Directory Table + ILT + IAT + hint/name table at
// final-link time (see docs/pe-coff-import-library-format.md-equivalent
// knowledge captured here from disassembling a real reference archive:
// the PE-COFF spec's compact single-member-per-symbol "short import"
// format is also standardized and read by dumping tools, but this ld
// version's actual `.idata` construction for a *link* only reliably
// fires from these chunk objects -- verified empirically: a short-import
// archive still resolves symbol names (so the link succeeds) but silently
// produces no Import Directory Table entry at all, leaving the
// synthesized `jmp [rip+__imp_x]` thunk's IAT slot never bound by the
// loader). One "head" member owns the Import Directory Table entry
// (.idata$2) plus the empty `.idata$5`/`.idata$4` anchor chunks that
// ld's SORT(*) groups every other member's own $5/$4 slot after (by
// member name, hence the zero-padded numeric suffixes below); one
// "tail" member supplies the DLL name string and the ILT/IAT
// null-terminator entries; and one member per exported symbol supplies
// the `jmp` thunk plus that symbol's own ILT/IAT/hint-name slot.
// ---------------------------------------------------------------------------

typedef struct {
    uint32_t offset;
    int sym_index; // index into this object's own CoffMiniSym array
    uint16_t type; // IMAGE_REL_AMD64_ADDR32NB(3) or _REL32(4)
} CoffMiniReloc;

typedef struct {
    const char *name; // <=8 bytes; every section used here is
    uint32_t characteristics;
    const uint8_t *data; // section content, or NULL for zero-filled `size` bytes
    uint32_t size;
    CoffMiniReloc relocs[4];
    int n_relocs;
} CoffMiniSec;

typedef struct {
    const char *name;
    uint32_t value;
    int section; // 1-based index into this object's own section array; 0 = undefined external
    uint8_t storage_class; // IMAGE_SYM_CLASS_EXTERNAL(2) or _STATIC(3)
} CoffMiniSym;

#define COFF_IDATA_CHAR 0xC0300000u // MEM_WRITE | MEM_READ | ALIGN_4BYTES
#define COFF_TEXT_CHAR 0x60300020u  // CNT_CODE | MEM_EXECUTE | MEM_READ | ALIGN_4BYTES

// Serialize one minimal relocatable COFF object (no aux symbol table
// entries -- verified against a real dlltool-produced reference archive
// that these synthetic objects skip them entirely, unlike a normal
// compiled .o).
static uint8_t *build_coff_obj(CoffMiniSec *secs, int n_secs, CoffMiniSym *syms,
                               int n_syms, uint16_t machine, size_t *out_len) {
    uint8_t *out = NULL;
    size_t len = 0, cap = 0;

    uint8_t hdr[20] = {0};
    pe_w16le_buf(hdr + 0, machine);
    pe_w16le_buf(hdr + 2, (uint16_t)n_secs);
    pe_w16le_buf(hdr + 16, 0); // SizeOfOptionalHeader
    pe_w16le_buf(hdr + 18, 0x0004); // IMAGE_FILE_LINE_NUMS_STRIPPED (matches dlltool)
    pe_buf_append_bytes(&out, &len, &cap, hdr, sizeof(hdr));

    size_t *sechdr_off = malloc((size_t)n_secs * sizeof(size_t));
    for (int i = 0; i < n_secs; i++) {
        sechdr_off[i] = len;
        uint8_t sh[40] = {0};
        size_t nl = strlen(secs[i].name);
        memcpy(sh, secs[i].name, nl > 8 ? 8 : nl);
        pe_w32le_m(sh + 36, secs[i].characteristics);
        pe_buf_append_bytes(&out, &len, &cap, sh, sizeof(sh));
    }

    // Raw data immediately followed by that section's own relocations,
    // per section in order (matches the reference archive's own layout).
    for (int i = 0; i < n_secs; i++) {
        uint32_t raw_ptr = 0, reloc_ptr = 0;
        if (secs[i].size > 0) {
            raw_ptr = (uint32_t)len;
            if (secs[i].data) {
                pe_buf_append_bytes(&out, &len, &cap, secs[i].data, secs[i].size);
            } else {
                uint8_t *z = calloc(secs[i].size, 1);
                pe_buf_append_bytes(&out, &len, &cap, z, secs[i].size);
                free(z);
            }
        }
        if (secs[i].n_relocs > 0) {
            reloc_ptr = (uint32_t)len;
            for (int j = 0; j < secs[i].n_relocs; j++) {
                uint8_t r[10];
                pe_w32le_m(r + 0, secs[i].relocs[j].offset);
                pe_w32le_m(r + 4, (uint32_t)secs[i].relocs[j].sym_index);
                pe_w16le_buf(r + 8, secs[i].relocs[j].type);
                pe_buf_append_bytes(&out, &len, &cap, r, sizeof(r));
            }
        }
        uint8_t *sh = out + sechdr_off[i]; // re-fetch: `out` may have moved
        pe_w32le_m(sh + 16, secs[i].size);
        pe_w32le_m(sh + 20, raw_ptr);
        pe_w32le_m(sh + 24, reloc_ptr);
        pe_w16le_buf(sh + 32, (uint16_t)secs[i].n_relocs);
    }
    free(sechdr_off);

    // String table for symbol names > 8 bytes: build its content first
    // (in a scratch buffer, independent of `out`) so every symbol record
    // below can embed its final strtab offset directly.
    uint8_t *strtab = malloc(4);
    size_t strtab_len = 4; // reserve the 4-byte size prefix
    size_t strtab_cap = 4;
    uint32_t *name_stroff = malloc((size_t)n_syms * sizeof(uint32_t));
    for (int i = 0; i < n_syms; i++) {
        size_t nl = strlen(syms[i].name);
        if (nl <= 8) {
            name_stroff[i] = 0;
            continue;
        }
        name_stroff[i] = (uint32_t)strtab_len;
        pe_buf_append_bytes(&strtab, &strtab_len, &strtab_cap, syms[i].name, nl + 1);
    }
    pe_w32le_m(strtab, (uint32_t)strtab_len);

    uint32_t symtab_off = (uint32_t)len;
    for (int i = 0; i < n_syms; i++) {
        uint8_t sym[18] = {0};
        size_t nl = strlen(syms[i].name);
        if (nl <= 8) {
            memcpy(sym, syms[i].name, nl);
        } else {
            pe_w32le_m(sym + 4, name_stroff[i]);
        }
        pe_w32le_m(sym + 8, syms[i].value);
        pe_w16le_buf(sym + 12, (uint16_t)syms[i].section);
        pe_w16le_buf(sym + 14, 0); // Type
        sym[16] = syms[i].storage_class;
        sym[17] = 0; // NumberOfAuxSymbols
        pe_buf_append_bytes(&out, &len, &cap, sym, sizeof(sym));
    }
    free(name_stroff);
    pe_buf_append_bytes(&out, &len, &cap, strtab, strtab_len);
    free(strtab);

    pe_w32le_m(out + 8, symtab_off);
    pe_w32le_m(out + 12, (uint32_t)n_syms);

    *out_len = len;
    return out;
}

// Build the "head" member: owns the Import Directory Table entry
// (.idata$2) plus the empty `.idata$5`/`.idata$4` chunks every other
// member's own slot concatenates after.
static uint8_t *build_import_head_obj(const char *dllid, uint16_t machine, size_t *out_len) {
    char head_sym[320], iname_sym[320];
    snprintf(head_sym, sizeof(head_sym), "_head_%s", dllid);
    snprintf(iname_sym, sizeof(iname_sym), "%s_iname", dllid);

    CoffMiniSym syms[5] = {
        {".idata$2", 0, 1, 3},
        {".idata$5", 0, 2, 3},
        {".idata$4", 0, 3, 3},
        {head_sym, 0, 1, 2},
        {iname_sym, 0, 0, 2}, // undefined: resolved from the tail member
    };
    uint8_t dir[20] = {0};
    CoffMiniSec secs[3] = {
        {".idata$2", COFF_IDATA_CHAR, dir, 20, {{0, 2, 3}, {12, 4, 3}, {16, 1, 3}}, 3}, // OriginalFirstThunk/Name/FirstThunk
        {".idata$5", COFF_IDATA_CHAR, NULL, 0, {{0}}, 0},
        {".idata$4", COFF_IDATA_CHAR, NULL, 0, {{0}}, 0},
    };
    return build_coff_obj(secs, 3, syms, 5, machine, out_len);
}

// Build the "tail" member: DLL name string + ILT/IAT null-terminator
// entries.
static uint8_t *build_import_tail_obj(const char *dllid, const char *dll_name,
                                      uint16_t machine, size_t *out_len) {
    char iname_sym[320];
    snprintf(iname_sym, sizeof(iname_sym), "%s_iname", dllid);
    size_t namelen = strlen(dll_name) + 1;

    CoffMiniSym syms[4] = {
        {".idata$4", 0, 1, 3},
        {".idata$5", 0, 2, 3},
        {".idata$7", 0, 3, 3},
        {iname_sym, 0, 3, 2},
    };
    CoffMiniSec secs[3] = {
        {".idata$4", COFF_IDATA_CHAR, NULL, 8, {{0}}, 0},
        {".idata$5", COFF_IDATA_CHAR, NULL, 8, {{0}}, 0},
        {".idata$7", COFF_IDATA_CHAR, (const uint8_t *)dll_name, (uint32_t)namelen, {{0}}, 0},
    };
    return build_coff_obj(secs, 3, syms, 4, machine, out_len);
}

// Build one per-symbol member: a `jmp [rip+__imp_<sym>]` thunk (bound to
// `<sym>` itself, for a plain `extern` call site with no dllimport) plus
// that symbol's own ILT/IAT/hint-name slot, and a forcing reference to
// `_head_<dllid>` (an otherwise-unreferenced symbol -- this is what pulls
// the head member into the link once any exported symbol is needed).
static uint8_t *build_import_symbol_obj(const char *dllid, const char *sym_name,
                                        uint16_t hint, uint16_t machine, size_t *out_len) {
    char head_sym[320], imp_sym[300];
    snprintf(head_sym, sizeof(head_sym), "_head_%s", dllid);
    snprintf(imp_sym, sizeof(imp_sym), "__imp_%s", sym_name);

    CoffMiniSym syms[8] = {
        {".text", 0, 1, 3},
        {".idata$7", 0, 2, 3},
        {".idata$5", 0, 3, 3},
        {".idata$4", 0, 4, 3},
        {".idata$6", 0, 5, 3},
        {sym_name, 0, 1, 2},
        {imp_sym, 0, 3, 2},
        {head_sym, 0, 0, 2}, // undefined: forces the head member's pull-in
    };
    uint8_t stub[8] = {0xFF, 0x25, 0, 0, 0, 0, 0x90, 0x90}; // jmp *[rip+0]; nop; nop

    size_t symlen = strlen(sym_name);
    size_t hintname_len = 2 + symlen + 1;
    size_t hintname_padded = (hintname_len + 3) & ~(size_t)3; // 4-byte aligned, matches reference
    uint8_t *hintname = calloc(hintname_padded, 1);
    hintname[0] = (uint8_t)hint;
    hintname[1] = (uint8_t)(hint >> 8);
    memcpy(hintname + 2, sym_name, symlen + 1);

    CoffMiniSec secs[5] = {
        {".text", COFF_TEXT_CHAR, stub, 8, {{2, 2, 4}}, 1}, // REL32 -> .idata$5 (own IAT slot)
        {".idata$7", COFF_IDATA_CHAR, NULL, 4, {{0, 7, 3}}, 1}, // ADDR32NB -> _head_<dllid> (forcing ref)
        {".idata$5", COFF_IDATA_CHAR, NULL, 8, {{0, 4, 3}}, 1}, // ADDR32NB -> .idata$6 (hint/name)
        {".idata$4", COFF_IDATA_CHAR, NULL, 8, {{0, 4, 3}}, 1}, // ADDR32NB -> .idata$6 (hint/name)
        {".idata$6", COFF_IDATA_CHAR, hintname, (uint32_t)hintname_padded, {{0}}, 0},
    };
    uint8_t *r = build_coff_obj(secs, 5, syms, 8, machine, out_len);
    free(hintname);
    return r;
}

// Build one per-symbol member for a *data* export: same ILT/IAT/hint-name
// slot as build_import_symbol_obj(), but -- verified against a real
// dlltool-produced reference archive -- with NO `.text` thunk and NO
// plain `<sym>` alias defined anywhere. That absence is deliberate and
// load-bearing: a plain `extern int gcount;` reference (no dllimport)
// left genuinely undefined after every archive member is scanned, with
// only `__imp_gcount` resolvable, is exactly the shape GNU ld's PE
// "auto-import" pass looks for -- it rewrites each reference to load
// through `__imp_gcount` via a runtime pseudo-relocation patched in at
// process startup. Defining a `.text` jmp-thunk under the plain name
// (correct for a *function* import, wrong for data) would satisfy the
// reference directly instead and be read as raw machine-code bytes.
// `__nm_<sym>` (naming the hint/name entry) has no established consumer
// but is included for exact parity with the reference archive.
static uint8_t *build_import_data_obj(const char *dllid, const char *sym_name,
                                      uint16_t hint, uint16_t machine, size_t *out_len) {
    char head_sym[320], imp_sym[300], nm_sym[300];
    snprintf(head_sym, sizeof(head_sym), "_head_%s", dllid);
    snprintf(imp_sym, sizeof(imp_sym), "__imp_%s", sym_name);
    snprintf(nm_sym, sizeof(nm_sym), "__nm_%s", sym_name);

    CoffMiniSym syms[8] = {
        {".text", 0, 1, 3},
        {".idata$7", 0, 2, 3},
        {".idata$5", 0, 3, 3},
        {".idata$4", 0, 4, 3},
        {".idata$6", 0, 5, 3},
        {imp_sym, 0, 3, 2},
        {nm_sym, 0, 5, 2},
        {head_sym, 0, 0, 2}, // undefined: forces the head member's pull-in
    };

    size_t symlen = strlen(sym_name);
    size_t hintname_len = 2 + symlen + 1;
    size_t hintname_padded = (hintname_len + 3) & ~(size_t)3;
    uint8_t *hintname = calloc(hintname_padded, 1);
    hintname[0] = (uint8_t)hint;
    hintname[1] = (uint8_t)(hint >> 8);
    memcpy(hintname + 2, sym_name, symlen + 1);

    CoffMiniSec secs[5] = {
        {".text", COFF_TEXT_CHAR, NULL, 0, {{0}}, 0}, // deliberately empty
        {".idata$7", COFF_IDATA_CHAR, NULL, 4, {{0, 7, 3}}, 1}, // ADDR32NB -> _head_<dllid>
        {".idata$5", COFF_IDATA_CHAR, NULL, 8, {{0, 4, 3}}, 1}, // ADDR32NB -> .idata$6
        {".idata$4", COFF_IDATA_CHAR, NULL, 8, {{0, 4, 3}}, 1}, // ADDR32NB -> .idata$6
        {".idata$6", COFF_IDATA_CHAR, hintname, (uint32_t)hintname_padded, {{0}}, 0},
    };
    uint8_t *r = build_coff_obj(secs, 5, syms, 8, machine, out_len);
    free(hintname);
    return r;
}

// True if `rva` falls within a section whose Characteristics claims
// IMAGE_SCN_MEM_EXECUTE -- the code/data distinction PE's export
// directory itself doesn't record, used to decide which import-object
// shape (build_import_symbol_obj vs _data_obj) an export needs.
static bool pe_rva_is_exec(const uint8_t *img, uint32_t sec_hdr_off, uint16_t n_secs, uint32_t rva) {
    for (uint16_t i = 0; i < n_secs; i++) {
        const uint8_t *sh = img + sec_hdr_off + (uint32_t)i * 40;
        uint32_t vsize = pe_r32le(sh + 8);
        uint32_t vaddr = pe_r32le(sh + 12);
        if (rva >= vaddr && rva < vaddr + vsize)
            return (pe_r32le(sh + 36) & IMAGE_SCN_MEM_EXECUTE) != 0;
    }
    return true; // unknown: default to the function shape (the common case)
}

// Parse `dll_path`'s own PE export directory and emit `implib_path` as a
// standard ar archive of GNU-ld-native import-chunk COFF objects (see
// the block comment above) -- what a real dlltool/`ld --out-implib`
// produces, and what this ld reliably builds a complete Import Directory
// Table from at final-link time. Reading the export table back out of
// the just-linked DLL (rather than threading the export list through
// from build_pe_exports()) makes this work uniformly regardless of
// which linker actually produced the DLL -- our own native one, or a
// GCC/mingw-ld fallback. Returns 0 on success, -1 if `dll_path` couldn't
// be read/parsed as a PE image with an export table.
int pe_write_out_implib(const char *dll_path, const char *implib_path) {
    FILE *f = fopen(dll_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 64) {
        fclose(f);
        return -1;
    }
    uint8_t *img = malloc((size_t)sz);
    if (!img || fread(img, 1, (size_t)sz, f) != (size_t)sz) {
        free(img);
        fclose(f);
        return -1;
    }
    fclose(f);

    if (img[0] != 'M' || img[1] != 'Z') {
        free(img);
        return -1;
    }
    uint32_t pe_off = pe_r32le(img + 0x3c);
    if (pe_off + 24 >= (uint32_t)sz || memcmp(img + pe_off, "PE\0\0", 4) != 0) {
        free(img);
        return -1;
    }
    uint16_t machine = pe_r16le(img + pe_off + 4);
    uint16_t n_secs = pe_r16le(img + pe_off + 6);
    uint16_t opthdr_size = pe_r16le(img + pe_off + 20);
    uint32_t opt_off = pe_off + 24;
    if (opthdr_size < 2 || opt_off + 2 > (uint32_t)sz) {
        free(img);
        return -1;
    }
    uint16_t magic = pe_r16le(img + opt_off);
    uint32_t datadir_off = opt_off + (magic == PE32PLUS_MAGIC ? 112 : 96);
    uint32_t exp_rva = pe_r32le(img + datadir_off);
    uint32_t sec_hdr_off = opt_off + opthdr_size;
    if (!exp_rva) {
        free(img);
        return -1;
    }
    uint32_t exp_off = pe_rva_to_off(img, sec_hdr_off, n_secs, exp_rva);
    if (!exp_off || exp_off + 40 > (uint32_t)sz) {
        free(img);
        return -1;
    }

    uint32_t name_rva = pe_r32le(img + exp_off + 12);
    uint32_t name_off = pe_rva_to_off(img, sec_hdr_off, n_secs, name_rva);
    const char *slash = strrchr(dll_path, '/');
#if defined(_WIN32) || defined(__MINGW32__)
    const char *bslash = strrchr(dll_path, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;
#endif
    const char *dll_name = (name_off && name_off < (uint32_t)sz)
        ? (const char *)(img + name_off)
        : (slash ? slash + 1 : dll_path);
    // A safe-for-identifiers copy of the DLL name (dlltool's own
    // `_head_<id>`/`<id>_iname` convention: any character that can't
    // appear in a COFF/asm-style identifier, e.g. the "." before the
    // extension, becomes '_').
    char dllid[300];
    size_t dll_name_len = strlen(dll_name);
    size_t dllid_len = dll_name_len < sizeof(dllid) - 1 ? dll_name_len : sizeof(dllid) - 1;
    for (size_t i = 0; i < dllid_len; i++) {
        char c = dll_name[i];
        dllid[i] = (isalnum((unsigned char)c) || c == '_') ? c : '_';
    }
    dllid[dllid_len] = '\0';

    uint32_t n_names = pe_r32le(img + exp_off + 24);
    uint32_t names_rva = pe_r32le(img + exp_off + 32);
    uint32_t names_off = pe_rva_to_off(img, sec_hdr_off, n_secs, names_rva);
    uint32_t eat_rva = pe_r32le(img + exp_off + 28);
    uint32_t eat_off = pe_rva_to_off(img, sec_hdr_off, n_secs, eat_rva);
    if (!names_off || n_names == 0) {
        free(img);
        return -1;
    }

    // Build every member (head, one per export -- already alphabetically
    // sorted, since that's how build_pe_exports() wrote AddressOfNames --
    // then tail), strdup-ing each exported name so it outlives `img`.
    // `AddressOfNames`/`AddressOfFunctions` share the same sorted index
    // (build_pe_exports() built ordinal[i] == i), so is_data[k] can be
    // read straight off the matching EAT entry.
    int n_syms_exported = 0;
    char **exported = malloc((size_t)n_names * sizeof(char *));
    bool *is_data = malloc((size_t)n_names * sizeof(bool));
    for (uint32_t k = 0; k < n_names; k++) {
        uint32_t nrva = pe_r32le(img + names_off + k * 4);
        uint32_t noff = pe_rva_to_off(img, sec_hdr_off, n_secs, nrva);
        if (!noff || noff >= (uint32_t)sz) continue;
        exported[n_syms_exported] = strdup((const char *)(img + noff));
        uint32_t fn_rva = eat_off ? pe_r32le(img + eat_off + k * 4) : 0;
        is_data[n_syms_exported] = eat_off && !pe_rva_is_exec(img, sec_hdr_off, n_secs, fn_rva);
        n_syms_exported++;
    }
    free(img);

    if (n_syms_exported == 0) {
        free(exported);
        free(is_data);
        return -1;
    }

    int n_members = n_syms_exported + 2; // head + N symbols + tail
    uint8_t **member_data = malloc((size_t)n_members * sizeof(uint8_t *));
    size_t *member_len = malloc((size_t)n_members * sizeof(size_t));
    char **member_name = malloc((size_t)n_members * sizeof(char *));
    // Symbols this archive's own symbol table must expose so the linker
    // can (a) pull in the right member for `<sym>`/`__imp_<sym>` when the
    // main program references them, and (b) resolve the forcing/DLL-name
    // cross-references each member makes into the head/tail members.
    // A data export's member never defines the plain `<sym>` name (see
    // build_import_data_obj()), so its own archive_sym entries omit it.
    int n_archive_syms = 2 * n_syms_exported + 2;
    char **archive_sym = malloc((size_t)n_archive_syms * sizeof(char *));
    int *archive_sym_member = malloc((size_t)n_archive_syms * sizeof(int));
    int n_as = 0;

    member_data[0] = build_import_head_obj(dllid, machine, &member_len[0]);
    member_name[0] = pe_xfmt("%s_d%06d.o", dllid, 0);
    archive_sym[n_as] = pe_xfmt("_head_%s", dllid);
    archive_sym_member[n_as++] = 0;

    for (int i = 0; i < n_syms_exported; i++) {
        if (is_data[i])
            member_data[1 + i] = build_import_data_obj(dllid, exported[i], (uint16_t)i,
                                                       machine, &member_len[1 + i]);
        else
            member_data[1 + i] = build_import_symbol_obj(dllid, exported[i], (uint16_t)i,
                                                         machine, &member_len[1 + i]);
        member_name[1 + i] = pe_xfmt("%s_d%06d.o", dllid, i + 1);
        if (!is_data[i]) {
            archive_sym[n_as] = strdup(exported[i]);
            archive_sym_member[n_as++] = 1 + i;
        }
        archive_sym[n_as] = pe_xfmt("__imp_%s", exported[i]);
        archive_sym_member[n_as++] = 1 + i;
    }

    member_data[n_members - 1] = build_import_tail_obj(dllid, dll_name, machine,
                                                       &member_len[n_members - 1]);
    member_name[n_members - 1] = pe_xfmt("%s_d%06d.o", dllid, n_syms_exported + 1);
    archive_sym[n_as] = pe_xfmt("%s_iname", dllid);
    archive_sym_member[n_as++] = n_members - 1;

    for (int i = 0; i < n_syms_exported; i++) free(exported[i]);
    free(exported);
    free(is_data);


    // GNU extended filename table ("//"): every member name here easily
    // exceeds the ar header's 16-byte inline limit.
    uint8_t *longnames = NULL;
    size_t longnames_len = 0, longnames_cap = 0;
    long *member_longoff = malloc((size_t)n_members * sizeof(long));
    for (int i = 0; i < n_members; i++) {
        member_longoff[i] = (long)longnames_len;
        pe_buf_append_bytes(&longnames, &longnames_len, &longnames_cap, member_name[i],
                            strlen(member_name[i]));
        pe_buf_append_bytes(&longnames, &longnames_len, &longnames_cap, "/\n", 2);
    }
    size_t longnames_padded = longnames_len + (longnames_len & 1);

    // Symbol-table member ("/"): maps every archive-visible symbol name
    // to the byte offset of its member's ar header.
    size_t symtab_names_len = 0;
    for (int i = 0; i < n_as; i++) symtab_names_len += strlen(archive_sym[i]) + 1;
    size_t symtab_content_len = 4 + (size_t)n_as * 4 + symtab_names_len;
    size_t symtab_padded = symtab_content_len + (symtab_content_len & 1);

    uint32_t *member_off = malloc((size_t)n_members * sizeof(uint32_t));
    uint32_t cur = 8 + 60 + (uint32_t)symtab_padded + 60 + (uint32_t)longnames_padded;
    for (int i = 0; i < n_members; i++) {
        member_off[i] = cur;
        size_t padded = member_len[i] + (member_len[i] & 1);
        cur += (uint32_t)(60 + padded);
    }

    uint8_t *out = NULL;
    size_t out_len = 0, out_cap = 0;
    pe_buf_append_bytes(&out, &out_len, &out_cap, "!<arch>\n", 8);

    pe_ar_header(&out, &out_len, &out_cap, "/", -1, symtab_content_len);
    uint8_t cnt_be[4] = {
        (uint8_t)((uint32_t)n_as >> 24),
        (uint8_t)((uint32_t)n_as >> 16),
        (uint8_t)((uint32_t)n_as >> 8),
        (uint8_t)(uint32_t)n_as,
    };
    pe_buf_append_bytes(&out, &out_len, &out_cap, cnt_be, 4);
    for (int i = 0; i < n_as; i++) {
        uint32_t o = member_off[archive_sym_member[i]];
        uint8_t off_be[4] = {
            (uint8_t)(o >> 24),
            (uint8_t)(o >> 16),
            (uint8_t)(o >> 8),
            (uint8_t)o,
        };
        pe_buf_append_bytes(&out, &out_len, &out_cap, off_be, 4);
    }
    for (int i = 0; i < n_as; i++)
        pe_buf_append_bytes(&out, &out_len, &out_cap, archive_sym[i], strlen(archive_sym[i]) + 1);
    if (symtab_content_len & 1) {
        uint8_t pad = '\n';
        pe_buf_append_bytes(&out, &out_len, &out_cap, &pad, 1);
    }

    pe_ar_header(&out, &out_len, &out_cap, "//", -1, longnames_len);
    pe_buf_append_bytes(&out, &out_len, &out_cap, longnames, longnames_len);
    if (longnames_len & 1) {
        uint8_t pad = '\n';
        pe_buf_append_bytes(&out, &out_len, &out_cap, &pad, 1);
    }
    free(longnames);

    for (int i = 0; i < n_members; i++) {
        pe_ar_header(&out, &out_len, &out_cap, NULL, member_longoff[i], member_len[i]);
        pe_buf_append_bytes(&out, &out_len, &out_cap, member_data[i], member_len[i]);
        if (member_len[i] & 1) {
            uint8_t pad = '\n';
            pe_buf_append_bytes(&out, &out_len, &out_cap, &pad, 1);
        }
    }

    FILE *of = fopen(implib_path, "wb");
    int rc = -1;
    if (of) {
        rc = (fwrite(out, 1, out_len, of) == out_len) ? 0 : -1;
        fclose(of);
    }
    free(out);

    for (int i = 0; i < n_members; i++) {
        free(member_data[i]);
        free(member_name[i]);
    }
    for (int i = 0; i < n_as; i++) free(archive_sym[i]);
    free(member_data);
    free(member_len);
    free(member_name);
    free(member_longoff);
    free(member_off);
    free(archive_sym);
    free(archive_sym_member);
    return rc;
}
