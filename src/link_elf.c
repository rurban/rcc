// SPDX-License-Identifier: LGPL-2.1-or-later
// Native ELF64 linker for rcc.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

// ELF constants
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ET_REL 1
#define ET_EXEC 2
#define EM_X86_64 62
#define EM_AARCH64 183

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_INIT_ARRAY 14
#define SHT_FINI_ARRAY 15

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_TLS 0x400

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_TLS 6
#define STV_DEFAULT 0
#define SHN_UNDEF 0
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2

#define PT_LOAD 1
#define PT_INTERP 3
#define PT_DYNAMIC 2
#define PT_TLS 7
#define PF_X 1
#define PF_W 2
#define PF_R 4

#define R_X86_64_64 1
#define R_X86_64_PC32 2
#define R_X86_64_32 10
#define R_X86_64_32S 11
#define R_X86_64_GOTPCREL 9
#define R_X86_64_GOTPCRELX 41
#define R_X86_64_REX_GOTPCRELX 42
#define R_X86_64_PLT32 4
#define R_X86_64_TPOFF32 23
#define R_X86_64_PC64 24
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_GLOB_DAT 6

#define R_AARCH64_ABS64 257
#define R_AARCH64_ABS32 258
#define R_AARCH64_CALL26 283
#define R_AARCH64_JUMP26 282
#define R_AARCH64_ADR_PREL_PG_HI21 275
#define R_AARCH64_ADD_ABS_LO12_NC 277
#define R_AARCH64_LDST64_ABS_LO12_NC 286
#define R_AARCH64_LDST32_ABS_LO12_NC 285
#define R_AARCH64_LDST16_ABS_LO12_NC 284
#define R_AARCH64_LDST8_ABS_LO12_NC 278
#define R_AARCH64_ADR_GOT_PAGE 311
#define R_AARCH64_LD64_GOT_LO12_NC 312
#define R_AARCH64_TLSLE_ADD_TPREL_HI12 549
#define R_AARCH64_TLSLE_ADD_TPREL_LO12 550

// Dynamic tags
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_HASH 4
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DT_STRSZ 10
#define DT_SYMENT 11
#define DT_INIT 12
#define DT_FINI 13
#define DT_JMPREL 23
#define DT_PLTREL 20
#define DT_PLTRELSZ 2
#define DT_FLAGS 30
#define DT_FLAGS_1 0x6ffffffb
#define DF_BIND_NOW 0x8
#define DF_1_NOW 1

static uint8_t r8(const uint8_t *p) { return p[0]; }
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)r16le(p) | ((uint32_t)r16le(p + 2) << 16);
}
static uint64_t r64le(const uint8_t *p) {
    return (uint64_t)r32le(p) | ((uint64_t)r32le(p + 4) << 32);
}

static void w16le_m(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void w32le_m(uint8_t *p, uint32_t v) {
    w16le_m(p, (uint16_t)v);
    w16le_m(p + 2, (uint16_t)(v >> 16));
}
static void w64le_m(uint8_t *p, uint64_t v) {
    w32le_m(p, (uint32_t)v);
    w32le_m(p + 4, (uint32_t)(v >> 32));
}

static void w8(FILE *f, uint8_t v) { fputc(v, f); }
static void w16le(FILE *f, uint16_t v) {
    w8(f, v);
    w8(f, v >> 8);
}
static void w32le(FILE *f, uint32_t v) {
    w16le(f, v);
    w16le(f, v >> 16);
}
static void w64le(FILE *f, uint64_t v) {
    w32le(f, v);
    w32le(f, v >> 32);
}
static void wbuf(FILE *f, const void *b, size_t n) { fwrite(b, 1, n, f); }
static void wzeros(FILE *f, size_t n) {
    uint8_t z[64];
    memset(z, 0, sizeof(z));
    while (n >= 64) {
        fwrite(z, 1, 64, f);
        n -= 64;
    }
    if (n) fwrite(z, 1, n, f);
}

static uint64_t align_up(uint64_t v, uint64_t a) {
    return (v + a - 1) & ~(a - 1);
}

typedef struct {
    const uint8_t *image;
    size_t size;
    int is_mmap;
} ElfFile;

static int elf_open(const char *path, ElfFile *ef) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }
    size_t sz = (size_t)st.st_size;
    void *m = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (m == MAP_FAILED) return -1;
    ef->image = m;
    ef->size = sz;
    ef->is_mmap = 1;
    return 0;
}

static void elf_close(ElfFile *ef) {
    if (!ef->image) return;
    if (ef->is_mmap) munmap((void *)ef->image, ef->size);
    else
        free((void *)ef->image);
    ef->image = NULL;
}

static const char *shstr(const ElfFile *ef, uint64_t shstroff, uint32_t name) {
    if (shstroff + name >= ef->size) return "";
    return (const char *)ef->image + shstroff + name;
}

static int map_reloc_type(uint32_t elf_type, LinkArch arch) {
    if (arch == ARCH_X86_64) {
        switch (elf_type) {
        case R_X86_64_64: return RL_ABS64;
        case R_X86_64_PC32: return RL_PC32;
        case R_X86_64_PLT32: return RL_PC32_PLT;
        case R_X86_64_32: return RL_ABS32U;
        case R_X86_64_32S: return RL_ABS32;
        case R_X86_64_PC64: return RL_PC64;
        case R_X86_64_GOTPCREL:
        case R_X86_64_GOTPCRELX:
        case R_X86_64_REX_GOTPCRELX: return RL_GOTPCREL;
        case R_X86_64_TPOFF32: return RL_TPOFF32;
        }
    } else {
        switch (elf_type) {
        case R_AARCH64_ABS64: return RL_ABS64;
        case R_AARCH64_ABS32: return RL_ABS32U;
        case R_AARCH64_CALL26:
        case R_AARCH64_JUMP26: return RL_ARM64_B26;
        case R_AARCH64_ADR_PREL_PG_HI21: return RL_ARM64_ADR_PG;
        case R_AARCH64_ADD_ABS_LO12_NC:
        case R_AARCH64_LDST64_ABS_LO12_NC:
        case R_AARCH64_LDST32_ABS_LO12_NC:
        case R_AARCH64_LDST16_ABS_LO12_NC:
        case R_AARCH64_LDST8_ABS_LO12_NC: return RL_ARM64_ADD_LO;
        case R_AARCH64_ADR_GOT_PAGE: return RL_ARM64_GOT_PG;
        case R_AARCH64_LD64_GOT_LO12_NC: return RL_ARM64_GOT_LO;
        case R_AARCH64_TLSLE_ADD_TPREL_HI12: return RL_ARM64_TPREL_HI;
        case R_AARCH64_TLSLE_ADD_TPREL_LO12: return RL_ARM64_TPREL_LO;
        }
    }
    return 0;
}

static int map_input_sec_to_output(const char *name, bool *alloc, bool *write,
                                   bool *exec, bool *bss, bool *tls) {
    *alloc = *write = *exec = *bss = *tls = false;
    if (strcmp(name, ".text") == 0 || strcmp(name, ".init") == 0 ||
        strcmp(name, ".fini") == 0) {
        *alloc = true;
        *exec = true;
        return 0;
    }
    if (strcmp(name, ".data") == 0) {
        *alloc = true;
        *write = true;
        return 0;
    }
    if (strcmp(name, ".rodata") == 0 || strcmp(name, ".rodata.*") == 0 ||
        strncmp(name, ".rodata.", 8) == 0) {
        *alloc = true;
        return 0;
    }
    if (strcmp(name, ".bss") == 0) {
        *alloc = true;
        *write = true;
        *bss = true;
        return 0;
    }
    if (strcmp(name, ".tdata") == 0) {
        *alloc = true;
        *write = true;
        *tls = true;
        return 0;
    }
    if (strcmp(name, ".tbss") == 0) {
        *alloc = true;
        *write = true;
        *bss = true;
        *tls = true;
        return 0;
    }
    if (strcmp(name, ".init_array") == 0) {
        *alloc = true;
        *write = true;
        return 0;
    }
    if (strcmp(name, ".fini_array") == 0) {
        *alloc = true;
        *write = true;
        return 0;
    }
    if (strncmp(name, ".debug", 6) == 0) return 0;
    if (strcmp(name, ".comment") == 0 || strcmp(name, ".note") == 0 ||
        strncmp(name, ".note.", 6) == 0) return 0;
    if (strcmp(name, ".eh_frame") == 0) {
        *alloc = true;
        return 0;
    }
    if (strcmp(name, ".gcc_except_table") == 0) {
        *alloc = true;
        return 0;
    }
    // default: non-allocated, skip
    return 1;
}

static size_t sec_alignment(const ElfFile *ef, uint64_t shoff, int idx) {
    const uint8_t *sh = ef->image + shoff + idx * 64;
    return (size_t)r64le(sh + 32);
}

static int elf_load_object(LinkState *s, const char *path);

int link_load_object(LinkState *s, const char *path) {
    return elf_load_object(s, path);
}

static int elf_load_object(LinkState *s, const char *path) {
    ElfFile ef;
    if (elf_open(path, &ef) != 0) {
        fprintf(stderr, "rcc: link: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (ef.size < 64 || ef.image[0] != ELFMAG0 || ef.image[1] != ELFMAG1 ||
        ef.image[2] != ELFMAG2 || ef.image[3] != ELFMAG3 ||
        ef.image[4] != ELFCLASS64 || ef.image[5] != ELFDATA2LSB ||
        ef.image[6] != EV_CURRENT) {
        fprintf(stderr, "rcc: link: %s: not a valid ELF64 file\n", path);
        elf_close(&ef);
        return -1;
    }
    uint16_t e_type = r16le(ef.image + 16);
    uint16_t e_machine = r16le(ef.image + 18);
    uint64_t e_shoff = r64le(ef.image + 40);
    uint16_t e_shnum = r16le(ef.image + 60);
    uint16_t e_shstrndx = r16le(ef.image + 62);
    if (e_type != ET_REL) {
        fprintf(stderr, "rcc: link: %s: expected relocatable object\n", path);
        elf_close(&ef);
        return -1;
    }
    LinkArch expected = (e_machine == EM_AARCH64) ? ARCH_AARCH64 : ARCH_X86_64;
    if (expected != s->arch) {
        fprintf(stderr, "rcc: link: %s: architecture mismatch\n", path);
        elf_close(&ef);
        return -1;
    }

    const uint8_t *shstr_sh = ef.image + e_shoff + (uint64_t)e_shstrndx * 64;
    uint64_t shstroff = r64le(shstr_sh + 24);

    int *sec_map = calloc((size_t)e_shnum, sizeof(int));
    int *sym_map = calloc((size_t)e_shnum, sizeof(int));
    uint64_t *sec_base_off = calloc((size_t)e_shnum, sizeof(uint64_t));
    for (int i = 0; i < e_shnum; i++) sec_map[i] = -1;

    // First pass: create output sections for allocatable input sections.
    for (int i = 0; i < e_shnum; i++) {
        const uint8_t *sh = ef.image + e_shoff + (uint64_t)i * 64;
        uint32_t type = r32le(sh + 4);
        if (type == SHT_NULL) continue;
        const char *name = shstr(&ef, shstroff, r32le(sh));
        bool alloc, write, exec, bss, tls;
        int kind = map_input_sec_to_output(name, &alloc, &write, &exec, &bss, &tls);
        if (kind == 0) {
            uint64_t flags = r64le(sh + 8);
            if (flags & SHF_ALLOC) alloc = true;
            if (flags & SHF_WRITE) write = true;
            if (flags & SHF_EXECINSTR) exec = true;
            if (flags & SHF_TLS) tls = true;
            size_t align = sec_alignment(&ef, e_shoff, i);
            if (align < 1) align = 1;
            sec_map[i] = link_find_or_create_sec(s, name, alloc, write, exec, bss, tls, align);
        }
    }

    // Second pass: append section data and record base offsets.
    for (int i = 0; i < e_shnum; i++) {
        const uint8_t *sh = ef.image + e_shoff + (uint64_t)i * 64;
        uint32_t type = r32le(sh + 4);
        if (type == SHT_NULL) continue;
        uint64_t base_off = 0;
        int out_idx = sec_map[i];
        if (out_idx < 0) continue;
        uint64_t off = r64le(sh + 24);
        uint64_t size = r64le(sh + 32);
        if (s->secs[out_idx].is_bss) {
            base_off = link_sec_append(s, out_idx, NULL, (size_t)size, sec_alignment(&ef, e_shoff, i));
        } else if (size > 0) {
            base_off = link_sec_append(s, out_idx, ef.image + off, (size_t)size, sec_alignment(&ef, e_shoff, i));
        }
        sec_base_off[i] = base_off;
    }

    // Load symbol tables and relocations.
    for (int i = 0; i < e_shnum; i++) {
        const uint8_t *sh = ef.image + e_shoff + (uint64_t)i * 64;
        uint32_t type = r32le(sh + 4);
        uint32_t link = r32le(sh + 40);
        if (type == SHT_SYMTAB) {
            const uint8_t *sym_sh = ef.image + e_shoff + (uint64_t)link * 64;
            uint64_t stroff = r64le(sym_sh + 24);
            uint64_t sym_off = r64le(sh + 24);
            uint64_t sym_size = r64le(sh + 32);
            int nsyms = (int)(sym_size / 24);
            sym_map = realloc(sym_map, (size_t)nsyms * sizeof(int));
            for (int k = 0; k < nsyms; k++) {
                const uint8_t *sym = ef.image + sym_off + (uint64_t)k * 24;
                uint32_t name_off = r32le(sym);
                uint8_t info = r8(sym + 4);
                uint8_t other = r8(sym + 5);
                uint16_t shndx = r16le(sym + 6);
                uint64_t value = r64le(sym + 8);
                uint64_t symsize = r64le(sym + 16);
                const char *name = (const char *)ef.image + stroff + name_off;
                if (k == 0) {
                    sym_map[k] = -1;
                    continue;
                }
                int bind = (info >> 4);
                int stype = info & 0xf;
                int out_sec = -1;
                if (shndx == SHN_UNDEF) out_sec = -1;
                else if (shndx == SHN_ABS)
                    out_sec = -2;
                else if (shndx == SHN_COMMON)
                    out_sec = -3;
                else if (shndx < e_shnum)
                    out_sec = sec_map[shndx];
                else
                    out_sec = -1;
                (void)other;
                if (bind == STB_LOCAL && (stype == STT_SECTION || *name == '\0')) {
                    sym_map[k] = -1;
                    continue;
                }
                if (*name == '\0') name = "";
                int map_sec = out_sec;
                if (out_sec == -2 || out_sec == -3) map_sec = -1;
                // Adjust symbol value for position in merged output section.
                if (out_sec >= 0 && shndx < e_shnum) value += sec_base_off[shndx];
                int sym_idx = link_add_sym(s, name, map_sec, value, symsize,
                                           bind == STB_WEAK ? 2 : (bind == STB_GLOBAL ? 1 : 0),
                                           stype == STT_FUNC ? 2 : (stype == STT_OBJECT ? 1 : 0),
                                           s->n_objs);
                if (sym_idx < 0) {
                    free(sec_map);
                    free(sym_map);
                    elf_close(&ef);
                    return -1;
                }
                sym_map[k] = sym_idx;
            }
        }
    }

    // Third pass: relocations (adjust r_offset by section base offset).
    for (int i = 0; i < e_shnum; i++) {
        const uint8_t *sh = ef.image + e_shoff + (uint64_t)i * 64;
        uint32_t type = r32le(sh + 4);
        if (type != SHT_RELA && type != SHT_REL) continue;
        uint32_t info = r32le(sh + 44);
        int target_sec = (int)info;
        if (target_sec < 0 || target_sec >= e_shnum) continue;
        int out_idx = sec_map[target_sec];
        if (out_idx < 0) continue;
        uint64_t roff = r64le(sh + 24);
        uint32_t link = r32le(sh + 40);
        uint64_t ent_size = r64le(sh + 56);
        int nrel = (int)(r64le(sh + 32) / ent_size);
        const uint8_t *sym_sh = ef.image + e_shoff + (uint64_t)link * 64;
        uint64_t sym_off = r64le(sym_sh + 24);
        (void)sym_off;
        for (int k = 0; k < nrel; k++) {
            const uint8_t *rel = ef.image + roff + (uint64_t)k * ent_size;
            uint64_t r_offset = r64le(rel) + sec_base_off[target_sec];
            uint64_t r_info = r64le(rel + 8);
            uint32_t sym_idx = (uint32_t)(r_info >> 32);
            uint32_t r_type = (uint32_t)r_info;
            int64_t addend = (type == SHT_RELA) ? (int64_t)r64le(rel + 16) : 0;
            if (sym_idx == 0) continue;
            int mapped_sym = sym_map[sym_idx];
            if (mapped_sym < 0) continue;
            int rl_type = map_reloc_type(r_type, s->arch);
            if (rl_type == 0) {
                fprintf(stderr, "rcc: link: %s: unhandled reloc type %u\n", path, r_type);
                continue;
            }
            link_add_reloc(s, out_idx, r_offset, rl_type, mapped_sym, addend);
        }
    }

    // Track loaded object.
    if (s->n_objs == s->cap_objs) {
        s->cap_objs = s->cap_objs ? s->cap_objs * 2 : 8;
        s->objs = realloc(s->objs, (size_t)s->cap_objs * sizeof(LinkObj));
    }
    LinkObj *obj = &s->objs[s->n_objs++];
    obj->path = strdup(path);
    obj->image = NULL;
    obj->image_size = 0;
    free(sec_map);
    free(sym_map);
    free(sec_base_off);
    elf_close(&ef);
    return 0;
}

// ---------------------------------------------------------------------------
// Archive loading (GNU ar)
// ---------------------------------------------------------------------------

typedef struct {
    char name[16];
    uint64_t off;
    uint64_t size;
} ArMember;

static int load_archive(LinkState *s, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    size_t sz = (size_t)st.st_size;
    uint8_t *data = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (data == MAP_FAILED) return -1;
    if (sz < 8 || memcmp(data, "!<arch>\n", 8) != 0) {
        munmap(data, sz);
        return -1;
    }

    uint64_t off = 8;
    uint64_t symtab_off = 0, symtab_size = 0;
    while (off + 60 <= sz) {
        char hdr[60];
        memcpy(hdr, data + off, 60);
        char name[16];
        memcpy(name, hdr, 15);
        name[15] = '\0';
        uint64_t msize = (uint64_t)strtoull(hdr + 48, NULL, 10);
        if (strncmp(name, "/ ", 2) == 0 || strcmp(name, "/") == 0) {
            symtab_off = off + 60;
            symtab_size = msize;
        }
        off += 60 + msize;
        if (msize & 1) off++;
    }

    if (symtab_off == 0 || symtab_off + symtab_size > sz) {
        munmap(data, sz);
        return 0;
    }

    uint32_t nsym = r32le(data + symtab_off);
    uint8_t *used = calloc(nsym, 1);
    int changed = 1, round = 0;
    while (changed && round < 32) {
        changed = 0;
        round++;
        for (uint32_t i = 0; i < nsym; i++) {
            if (used[i]) continue;
            uint32_t moff = r32le(data + symtab_off + 4 + i * 4);
            const char *name = (const char *)data + symtab_off + 4 + nsym * 4 + i;
            while (name < (const char *)data + symtab_off + symtab_size && *name == '\0') name++;
            if ((const uint8_t *)name >= data + symtab_off + symtab_size) continue;
            const char *end = name;
            while (end < (const char *)data + symtab_off + symtab_size && *end) end++;
            char symname[256];
            size_t len = (size_t)(end - name);
            if (len >= sizeof(symname)) len = sizeof(symname) - 1;
            memcpy(symname, name, len);
            symname[len] = '\0';
            name = end;
            if (link_find_sym(s, symname) < 0) continue;
            off = 8;
            while (off + 60 <= sz) {
                char hdr[60];
                memcpy(hdr, data + off, 60);
                uint64_t msize = (uint64_t)strtoull(hdr + 48, NULL, 10);
                if (off + 60 == moff + 8) {
                    char tmp[] = "/tmp/rcc_link_ar_XXXXXX";
                    int tfd = mkstemp(tmp);
                    if (tfd >= 0) {
                        write(tfd, data + off + 60, (size_t)msize);
                        close(tfd);
                        elf_load_object(s, tmp);
                        unlink(tmp);
                        used[i] = 1;
                        changed = 1;
                    }
                    break;
                }
                off += 60 + msize;
                if (msize & 1) off++;
            }
        }
    }

    free(used);
    munmap(data, sz);
    return 0;
}

static int resolve_archives(LinkState *s) {
    (void)s;
    return 0;
}

// ---------------------------------------------------------------------------
// ELF executable writer
// ---------------------------------------------------------------------------

static void write_ehdr(FILE *f, uint16_t machine, uint64_t entry, uint64_t phoff,
                       uint16_t phnum) {
    uint8_t ident[16] = {
        0x7f, 'E', 'L', 'F', 2, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0};
    wbuf(f, ident, 16);
    w16le(f, ET_EXEC);
    w16le(f, machine);
    w32le(f, 1);
    w64le(f, entry);
    w64le(f, phoff);
    w64le(f, 0);
    w32le(f, 0);
    w16le(f, 64);
    w16le(f, 56);
    w16le(f, phnum);
    w16le(f, 64);
    w16le(f, 0);
    w16le(f, 0);
}

static void write_phdr(FILE *f, uint32_t type, uint32_t flags, uint64_t offset,
                       uint64_t vaddr, uint64_t paddr, uint64_t filesz,
                       uint64_t memsz, uint64_t align) {
    w32le(f, type);
    w32le(f, flags);
    w64le(f, offset);
    w64le(f, vaddr);
    w64le(f, paddr);
    w64le(f, filesz);
    w64le(f, memsz);
    w64le(f, align);
}

static uint32_t elf_hash(const char *name) {
    uint32_t h = 0, g;
    while (*name) {
        h = (h << 4) + (unsigned char)*name++;
        g = h & 0xf0000000;
        if (g) {
            h ^= g >> 24;
            h &= ~g;
        }
    }
    return h;
}

static uint64_t symbol_address(LinkState *s, int idx) {
    LinkSym *sym = &s->syms[idx];
    if (sym->sec < 0) return 0;
    return s->secs[sym->sec].addr + sym->value;
}

static void auto_dyn_ent(LinkSec *dyn, size_t *pos, uint64_t tag, uint64_t val) {
    uint8_t ent[16];
    w64le_m(ent, tag);
    w64le_m(ent + 8, val);
    memcpy(dyn->data + *pos, ent, 16);
    *pos += 16;
}

static int apply_dynamic_relocs(LinkState *s, const int *dyn_idx, const int *plt_idx,
                                const int *got_map, uint64_t got_addr,
                                uint64_t plt_addr) {
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        for (int j = 0; j < sec->n_relocs; j++) {
            LinkReloc *r = &sec->relocs[j];
            LinkSym *sym = &s->syms[r->sym];
            uint8_t *p = sec->data + r->offset;
            uint64_t pc = sec->addr + r->offset;
            int64_t A = r->addend;
            uint64_t S;

            switch (r->type) {
            case RL_ABS64:
                if (dyn_idx && dyn_idx[r->sym]) {
                    fprintf(stderr, "rcc: link: unsupported ABS64 reference to dynamic symbol '%s'\n",
                            sym->name);
                    return -1;
                }
                S = symbol_address(s, r->sym);
                w64le_m(p, r64le(p) + S + (uint64_t)A);
                break;
            case RL_ABS32:
                if (dyn_idx && dyn_idx[r->sym]) {
                    fprintf(stderr, "rcc: link: unsupported ABS32 reference to dynamic symbol '%s'\n",
                            sym->name);
                    return -1;
                }
                S = symbol_address(s, r->sym);
                w32le_m(p, (uint32_t)((int32_t)r32le(p) + (int32_t)A + (int64_t)S));
                break;
            case RL_ABS32U:
                if (dyn_idx && dyn_idx[r->sym]) {
                    fprintf(stderr, "rcc: link: unsupported ABS32U reference to dynamic symbol '%s'\n",
                            sym->name);
                    return -1;
                }
                S = symbol_address(s, r->sym);
                w32le_m(p, (uint32_t)(r32le(p) + (uint64_t)A + S));
                break;
            case RL_PC32:
            case RL_PC32_PLT:
                if (dyn_idx && dyn_idx[r->sym]) {
                    if (!plt_idx || plt_idx[r->sym] < 0) {
                        fprintf(stderr, "rcc: link: unsupported PC32 reference to dynamic data symbol '%s'\n",
                                sym->name);
                        return -1;
                    }
                    S = plt_addr + 16 + (uint64_t)plt_idx[r->sym] * 16;
                } else {
                    S = symbol_address(s, r->sym);
                }
                w32le_m(p, (uint32_t)((int32_t)r32le(p) + (int32_t)A + (int64_t)(S - pc)));
                break;
            case RL_PC64:
                if (dyn_idx && dyn_idx[r->sym]) {
                    fprintf(stderr, "rcc: link: unsupported PC64 reference to dynamic symbol '%s'\n",
                            sym->name);
                    return -1;
                }
                S = symbol_address(s, r->sym);
                w64le_m(p, r64le(p) + (uint64_t)A + S - pc);
                break;
            case RL_GOTPCREL: {
                if (!got_map) {
                    fprintf(stderr, "rcc: link: GOTPCREL without GOT\n");
                    return -1;
                }
                int slot = got_map[r->sym];
                if (slot < 0) {
                    fprintf(stderr, "rcc: link: no GOT slot for '%s'\n", sym->name);
                    return -1;
                }
                S = got_addr + (uint64_t)slot * 8;
                w32le_m(p, (uint32_t)((int32_t)r32le(p) + (int32_t)A + (int64_t)(S - pc)));
                break;
            }
            case RL_TPOFF32: {
                S = symbol_address(s, r->sym);
                w32le_m(p, (uint32_t)((int32_t)r32le(p) + (int32_t)A + (int64_t)S));
                break;
            }
            default:
                // Let the shared backend handle the rest (ARM64, etc.)
                link_reloc_apply(s->arch, sec, r, symbol_address(s, r->sym), pc);
                break;
            }
        }
    }
    return 0;
}

static int try_load_crt(LinkState *s, const char *dir, const char *file) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, file);
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return elf_load_object(s, path);
}

static int load_crt_files(LinkState *s) {
    static const char *dirs[] = {
        "/usr/lib64",
        "/usr/lib/x86_64-linux-gnu",
        "/lib/x86_64-linux-gnu",
        "/lib64",
        NULL,
    };
    const char *crt_dir = NULL;
    for (int i = 0; dirs[i]; i++) {
        if (try_load_crt(s, dirs[i], "crt1.o") == 0) {
            crt_dir = dirs[i];
            break;
        }
    }
    if (!crt_dir) {
        fprintf(stderr, "rcc: link: cannot find crt1.o\n");
        return -1;
    }
    if (try_load_crt(s, crt_dir, "crti.o") != 0) {
        fprintf(stderr, "rcc: link: cannot find crti.o\n");
        return -1;
    }
    if (try_load_crt(s, crt_dir, "crtn.o") != 0) {
        fprintf(stderr, "rcc: link: cannot find crtn.o\n");
        return -1;
    }
    return 0;
}

int link_elf(LinkState *s) {
    if (resolve_archives(s) != 0) return -1;

    // Ensure required sections exist.
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".rodata", true, false, false, false, false, 1);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 8);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 8);

    // Load C runtime startup files on Linux x86_64 when not linking statically.
    if (!s->opt_static && s->arch == ARCH_X86_64) {
        if (load_crt_files(s) != 0) return -1;
    }

    int entry_sym = link_find_sym(s, "_start");
    if (entry_sym < 0) {
        // No _start: need C runtime.  Fall back to external linker.
        return -1;
    }

    // Identify unresolved undefined symbols (excluding weak refs).
    int n_dyn = 0;
    int cap_dyn = 0;
    int *dyn_syms = NULL;
    int *dyn_idx = calloc((size_t)s->n_syms, sizeof(int));
    if (!dyn_idx) return -1;
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec < 0 && sym->bind != 2 && sym->name && sym->name[0]) {
            if (n_dyn == cap_dyn) {
                cap_dyn = cap_dyn ? cap_dyn * 2 : 16;
                dyn_syms = realloc(dyn_syms, (size_t)cap_dyn * sizeof(int));
            }
            dyn_syms[n_dyn] = i;
            dyn_idx[i] = n_dyn + 1; // dynsym index, 1-based (0 is null)
            n_dyn++;
        }
    }

    bool do_dynamic = n_dyn > 0 && !s->opt_static && s->arch == ARCH_X86_64;
    if (n_dyn > 0 && !do_dynamic) {
        // Unsupported dynamic linking configuration; fall back.
        free(dyn_syms);
        free(dyn_idx);
        return -1;
    }

    int interp_sec = -1, dynamic_sec = -1, dynsym_sec = -1, dynstr_sec = -1;
    int hash_sec = -1, plt_sec = -1, gotplt_sec = -1, relaplt_sec = -1, reladyn_sec = -1;
    int *dyn_kind = NULL;
    int *plt_idx = NULL;
    int *got_map = NULL;
    int *dynstr_off = NULL;
    int n_func_dyn = 0;
    int n_reladyn = 0;
    int libc_off = 0;

    if (do_dynamic) {
        interp_sec = link_find_or_create_sec(s, ".interp", true, false, false, false, false, 1);
        dynamic_sec = link_find_or_create_sec(s, ".dynamic", true, false, false, false, false, 8);
        dynsym_sec = link_find_or_create_sec(s, ".dynsym", true, false, false, false, false, 8);
        dynstr_sec = link_find_or_create_sec(s, ".dynstr", true, false, false, false, false, 1);
        hash_sec = link_find_or_create_sec(s, ".hash", true, false, false, false, false, 8);
        plt_sec = link_find_or_create_sec(s, ".plt", true, false, true, false, false, 16);
        gotplt_sec = link_find_or_create_sec(s, ".got.plt", true, true, false, false, false, 8);
        relaplt_sec = link_find_or_create_sec(s, ".rela.plt", true, false, false, false, false, 8);
        reladyn_sec = link_find_or_create_sec(s, ".rela.dyn", true, false, false, false, false, 8);

        // Define the global offset table symbol before scanning unresolved symbols.
        int got_sym = link_find_sym(s, "_GLOBAL_OFFSET_TABLE_");
        if (got_sym < 0) {
            got_sym = link_add_sym(s, "_GLOBAL_OFFSET_TABLE_", gotplt_sec, 0, 8,
                                   STB_GLOBAL, STT_NOTYPE, -1);
        } else {
            s->syms[got_sym].sec = gotplt_sec;
            s->syms[got_sym].value = 0;
            s->syms[got_sym].resolved = true;
        }
        // Remove _GLOBAL_OFFSET_TABLE_ from dynamic symbols if present.
        for (int k = 0; k < n_dyn; k++) {
            if (dyn_syms[k] == got_sym) {
                dyn_idx[got_sym] = 0;
                n_dyn--;
                memmove(&dyn_syms[k], &dyn_syms[k + 1], (size_t)(n_dyn - k) * sizeof(int));
                for (int m = k; m < n_dyn; m++)
                    dyn_idx[dyn_syms[m]] = m + 1;
                break;
            }
        }

        // .interp
        const char *interp = "/lib64/ld-linux-x86-64.so.2";
        link_sec_append(s, interp_sec, (const uint8_t *)interp, strlen(interp) + 1, 1);

        // .dynstr: start with a null byte, then needed library names.
        uint8_t nul = 0;
        link_sec_append(s, dynstr_sec, &nul, 1, 1);
        libc_off = (int)link_sec_append(s, dynstr_sec,
                                        (const uint8_t *)"libc.so.6", 10, 1);

        // .dynsym: null entry first.
        uint8_t zero24[24] = {0};
        link_sec_append(s, dynsym_sec, zero24, 24, 8);
        dynstr_off = malloc((size_t)n_dyn * sizeof(int));
        for (int k = 0; k < n_dyn; k++) {
            LinkSym *sym = &s->syms[dyn_syms[k]];
            int off = (int)link_sec_append(s, dynstr_sec, (const uint8_t *)sym->name,
                                           strlen(sym->name) + 1, 1);
            dynstr_off[k] = off;
            uint8_t ent[24];
            w32le_m(ent, (uint32_t)off);
            ent[4] = (uint8_t)(((sym->bind == 2 ? STB_WEAK : STB_GLOBAL) << 4) |
                               (sym->type == 2 ? STT_FUNC : (sym->type == 1 ? STT_OBJECT : STT_NOTYPE)));
            ent[5] = STV_DEFAULT;
            w16le_m(ent + 6, SHN_UNDEF);
            w64le_m(ent + 8, 0);
            w64le_m(ent + 16, 0);
            link_sec_append(s, dynsym_sec, ent, 24, 8);
        }

        // .hash
        uint32_t nchain = (uint32_t)(n_dyn + 1);
        uint32_t nbucket = 1;
        while (nbucket < nchain) nbucket <<= 1;
        uint32_t *buckets = calloc(nbucket, sizeof(uint32_t));
        uint32_t *chain = calloc(nchain, sizeof(uint32_t));
        for (int k = 1; k <= n_dyn; k++) {
            const char *name = s->syms[dyn_syms[k - 1]].name;
            uint32_t h = elf_hash(name) % nbucket;
            chain[k] = buckets[h];
            buckets[h] = k;
        }
        uint8_t htmp[4];
        w32le_m(htmp, nbucket);
        link_sec_append(s, hash_sec, htmp, 4, 4);
        w32le_m(htmp, nchain);
        link_sec_append(s, hash_sec, htmp, 4, 4);
        for (uint32_t i = 0; i < nbucket; i++) {
            w32le_m(htmp, buckets[i]);
            link_sec_append(s, hash_sec, htmp, 4, 4);
        }
        for (uint32_t i = 0; i < nchain; i++) {
            w32le_m(htmp, chain[i]);
            link_sec_append(s, hash_sec, htmp, 4, 4);
        }
        free(buckets);
        free(chain);

        // Determine dynamic symbol use (function vs data) and GOT/PLT needs.
        dyn_kind = calloc((size_t)s->n_syms, sizeof(int));
        plt_idx = malloc((size_t)s->n_syms * sizeof(int));
        got_map = malloc((size_t)s->n_syms * sizeof(int));
        for (int i = 0; i < s->n_syms; i++) {
            plt_idx[i] = -1;
            got_map[i] = -1;
        }
        // Every dynamic symbol gets a GOT slot at index 2 + dynsym_index.
        for (int k = 0; k < n_dyn; k++) {
            got_map[dyn_syms[k]] = 2 + (k + 1);
        }

        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            for (int j = 0; j < sec->n_relocs; j++) {
                LinkReloc *r = &sec->relocs[j];
                int si = r->sym;
                if (dyn_idx[si]) {
                    if (r->type == RL_PC32 || r->type == RL_PC32_PLT ||
                        r->type == RL_GOTPCREL) {
                        dyn_kind[si] = 1; // function reference
                    } else if (r->type == RL_ABS64 || r->type == RL_ABS32 ||
                               r->type == RL_ABS32U) {
                        dyn_kind[si] = 2; // data reference
                    }
                } else if (r->type == RL_GOTPCREL && got_map[si] < 0) {
                    // Defined or weak symbol referenced through GOT.
                    got_map[si] = 3 + n_dyn; // placeholder; counted below
                }
            }
        }

        // Count additional GOT slots for non-dynamic GOTPCREL targets.
        int n_extra_got = 0;
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            for (int j = 0; j < sec->n_relocs; j++) {
                LinkReloc *r = &sec->relocs[j];
                int si = r->sym;
                if (!dyn_idx[si] && r->type == RL_GOTPCREL && got_map[si] == 3 + n_dyn) {
                    got_map[si] = 3 + n_dyn + n_extra_got;
                    n_extra_got++;
                }
            }
        }

        // Assign PLT indices to function-like dynamic symbols.
        for (int k = 0; k < n_dyn; k++) {
            int si = dyn_syms[k];
            if (dyn_kind[si] == 1 ||
                (dyn_kind[si] == 0 && s->syms[si].type == STT_FUNC)) {
                plt_idx[si] = n_func_dyn++;
            }
        }
        n_reladyn = n_dyn - n_func_dyn;

        // Allocate .got.plt, .plt, .rela.plt, .rela.dyn.
        int total_got_slots = 3 + n_dyn + n_extra_got;
        size_t got_size = (size_t)total_got_slots * 8;
        uint8_t *got_init = calloc(got_size, 1);
        link_sec_append(s, gotplt_sec, got_init, got_size, 8);
        free(got_init);

        size_t plt_size = 16 + (size_t)n_func_dyn * 16;
        uint8_t *plt_init = calloc(plt_size, 1);
        link_sec_append(s, plt_sec, plt_init, plt_size, 16);
        free(plt_init);

        if (n_func_dyn > 0) {
            size_t relaplt_size = (size_t)n_func_dyn * 24;
            uint8_t *relaplt_init = calloc(relaplt_size, 1);
            link_sec_append(s, relaplt_sec, relaplt_init, relaplt_size, 8);
            free(relaplt_init);
        }
        if (n_reladyn > 0) {
            size_t reladyn_size = (size_t)n_reladyn * 24;
            uint8_t *reladyn_init = calloc(reladyn_size, 1);
            link_sec_append(s, reladyn_sec, reladyn_init, reladyn_size, 8);
            free(reladyn_init);
        }

        // Pre-allocate .dynamic entries so layout reserves the correct size.
        int n_dynent = 6 + (n_reladyn > 0 ? 3 : 0) + (n_func_dyn > 0 ? 3 : 0) + 4;
        uint8_t *dyn_placeholder = calloc((size_t)n_dynent * 16, 1);
        link_sec_append(s, dynamic_sec, dyn_placeholder, (size_t)n_dynent * 16, 8);
        free(dyn_placeholder);
    }

    // Layout.  Use page-aligned region boundaries so each PT_LOAD segment
    // starts at a valid file offset / virtual address pair.
    uint64_t base = 0x400000ULL;
    if (s->opt_pie) base = 0x10000ULL;
    if (link_layout(s, base, 0x1000) != 0) {
        free(dyn_syms);
        free(dyn_idx);
        free(dyn_kind);
        free(plt_idx);
        free(got_map);
        free(dynstr_off);
        return -1;
    }

    if (do_dynamic) {
        uint64_t got_addr = s->secs[gotplt_sec].addr;
        uint64_t plt_addr = s->secs[plt_sec].addr;
        LinkSec *got = &s->secs[gotplt_sec];
        LinkSec *plt = &s->secs[plt_sec];
        LinkSec *relaplt = &s->secs[relaplt_sec];
        LinkSec *reladyn = &s->secs[reladyn_sec];

        // GOT initial contents.
        w64le_m(got->data + 0, s->secs[dynamic_sec].addr);
        w64le_m(got->data + 8, 0);
        w64le_m(got->data + 16, 0);
        // Dynamic symbol slots: leave zero for the dynamic linker.
        // Extra GOT slots for defined/weak symbols.
        for (int i = 0; i < s->n_syms; i++) {
            int slot = got_map[i];
            if (slot < 0) continue;
            if (dyn_idx[i]) continue; // resolved by dynamic linker
            uint64_t addr = symbol_address(s, i);
            w64le_m(got->data + (size_t)slot * 8, addr);
        }

        // PLT entries.
        // PLT0: push GOTPLT+8(%rip); jmp *GOTPLT+16(%rip); nop
        int32_t disp_push = (int32_t)((got_addr + 8) - (plt_addr + 6));
        int32_t disp_jmp0 = (int32_t)((got_addr + 16) - (plt_addr + 12));
        plt->data[0] = 0xff;
        plt->data[1] = 0x35;
        w32le_m(plt->data + 2, (uint32_t)disp_push);
        plt->data[6] = 0xff;
        plt->data[7] = 0x25;
        w32le_m(plt->data + 8, (uint32_t)disp_jmp0);
        plt->data[12] = 0x0f;
        plt->data[13] = 0x1f;
        plt->data[14] = 0x40;
        plt->data[15] = 0x00;

        int relaplt_pos = 0;
        int reladyn_pos = 0;
        for (int k = 0; k < n_dyn; k++) {
            int si = dyn_syms[k];
            int dynsym_index = k + 1;
            int got_slot = got_map[si];
            if (plt_idx[si] >= 0) {
                int fidx = plt_idx[si];
                uint64_t entry_addr = plt_addr + 16 + (uint64_t)fidx * 16;
                int32_t disp_got = (int32_t)((got_addr + (uint64_t)got_slot * 8) - (entry_addr + 6));
                int32_t disp_plt0 = (int32_t)(plt_addr - (entry_addr + 11));
                uint8_t *p = plt->data + 16 + (size_t)fidx * 16;
                p[0] = 0xff;
                p[1] = 0x25;
                w32le_m(p + 2, (uint32_t)disp_got);
                p[6] = 0x68;
                w32le_m(p + 7, (uint32_t)fidx);
                p[11] = 0xe9;
                w32le_m(p + 12, (uint32_t)disp_plt0);

                uint8_t *rp = relaplt->data + (size_t)relaplt_pos * 24;
                w64le_m(rp, got_addr + (uint64_t)got_slot * 8);
                w64le_m(rp + 8, ((uint64_t)dynsym_index << 32) | R_X86_64_JUMP_SLOT);
                w64le_m(rp + 16, 0);
                relaplt_pos++;
            } else {
                // GLOB_DAT relocation for data-like dynamic symbols.
                uint8_t *rp = reladyn->data + (size_t)reladyn_pos * 24;
                w64le_m(rp, got_addr + (uint64_t)got_slot * 8);
                w64le_m(rp + 8, ((uint64_t)dynsym_index << 32) | R_X86_64_GLOB_DAT);
                w64le_m(rp + 16, 0);
                reladyn_pos++;
            }
        }

        // Apply relocations using PLT/GOT.
        if (apply_dynamic_relocs(s, dyn_idx, plt_idx, got_map, got_addr, plt_addr) != 0) {
            free(dyn_syms);
            free(dyn_idx);
            free(dyn_kind);
            free(plt_idx);
            free(got_map);
            free(dynstr_off);
            return -1;
        }

        // Build .dynamic section after layout so addresses are known.
        // The .dynamic section was pre-allocated before layout; patch it now.
        uint8_t ent[16];
        size_t dpos = 0;
        LinkSec *dyn = &s->secs[dynamic_sec];
        auto_dyn_ent(dyn, &dpos, DT_HASH, s->secs[hash_sec].addr);
        auto_dyn_ent(dyn, &dpos, DT_STRTAB, s->secs[dynstr_sec].addr);
        auto_dyn_ent(dyn, &dpos, DT_SYMTAB, s->secs[dynsym_sec].addr);
        auto_dyn_ent(dyn, &dpos, DT_STRSZ, s->secs[dynstr_sec].len);
        auto_dyn_ent(dyn, &dpos, DT_SYMENT, 24);
        if (n_reladyn > 0) {
            auto_dyn_ent(dyn, &dpos, DT_RELA, s->secs[reladyn_sec].addr);
            auto_dyn_ent(dyn, &dpos, DT_RELASZ, (uint64_t)n_reladyn * 24);
            auto_dyn_ent(dyn, &dpos, DT_RELAENT, 24);
        }
        if (n_func_dyn > 0) {
            auto_dyn_ent(dyn, &dpos, DT_JMPREL, s->secs[relaplt_sec].addr);
            auto_dyn_ent(dyn, &dpos, DT_PLTRELSZ, (uint64_t)n_func_dyn * 24);
            auto_dyn_ent(dyn, &dpos, DT_PLTREL, DT_RELA);
        }
        auto_dyn_ent(dyn, &dpos, DT_NEEDED, (uint64_t)libc_off);
        auto_dyn_ent(dyn, &dpos, DT_FLAGS, DF_BIND_NOW);
        auto_dyn_ent(dyn, &dpos, DT_FLAGS_1, DF_1_NOW);
        auto_dyn_ent(dyn, &dpos, DT_NULL, 0);
    } else {
        // Static link: apply relocations normally.
        link_apply_relocs(s);
    }

    // Build program headers.
    uint64_t text_off = 0, text_addr = 0, text_filesz = 0, text_memsz = 0;
    uint64_t rodata_off = 0, rodata_addr = 0, rodata_filesz = 0, rodata_memsz = 0;
    uint64_t data_off = 0, data_addr = 0, data_filesz = 0, data_memsz = 0;
    bool have_text = false, have_rodata = false, have_data = false;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc) continue;
        if (sec->exec) {
            if (!have_text) {
                have_text = true;
                text_off = sec->fileoff;
                text_addr = sec->addr;
            }
            text_filesz = sec->fileoff + sec->len - text_off;
            text_memsz = text_filesz;
        } else if (!sec->write) {
            if (!have_rodata) {
                have_rodata = true;
                rodata_off = sec->fileoff;
                rodata_addr = sec->addr;
            }
            rodata_filesz = sec->fileoff + sec->len - rodata_off;
            rodata_memsz = rodata_filesz;
        } else {
            if (!have_data) {
                have_data = true;
                data_off = sec->fileoff;
                data_addr = sec->addr;
            }
            if (sec->is_bss) {
                if (sec->len > 0) {
                    uint64_t bss_end = sec->addr + sec->len;
                    if (bss_end - data_addr > data_memsz)
                        data_memsz = bss_end - data_addr;
                }
            } else {
                uint64_t end = sec->fileoff + sec->len - data_off;
                if (end > data_filesz) data_filesz = end;
                if (end > data_memsz) data_memsz = end;
            }
        }
    }

    uint64_t phnum = 0;
    if (text_filesz) phnum++;
    if (rodata_filesz) phnum++;
    if (data_filesz || data_memsz) phnum++;
    if (do_dynamic) {
        phnum += 2; // PT_INTERP and PT_DYNAMIC
    }
    if (phnum == 0) phnum = 1;

    uint64_t ehdr_size = 64;
    uint64_t phdr_size = phnum * 56;
    uint64_t file_off = align_up(ehdr_size + phdr_size, 0x1000);

    // Shift all non-bss section file offsets by the page-aligned header size.
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->is_bss) {
            sec->fileoff = 0;
            continue;
        }
        sec->fileoff += file_off;
    }
    if (have_text) text_off += file_off;
    if (have_rodata) rodata_off += file_off;
    if (have_data) data_off += file_off;

    uint64_t entry_addr = base;
    if (entry_sym >= 0) {
        entry_addr = s->secs[s->syms[entry_sym].sec].addr + s->syms[entry_sym].value;
    }

    FILE *f = fopen(s->out_path, "wb");
    if (!f) {
        fprintf(stderr, "rcc: link: cannot create %s: %s\n", s->out_path, strerror(errno));
        free(dyn_syms);
        free(dyn_idx);
        free(dyn_kind);
        free(plt_idx);
        free(got_map);
        free(dynstr_off);
        return -1;
    }

    uint16_t machine = (s->arch == ARCH_AARCH64) ? EM_AARCH64 : EM_X86_64;
    write_ehdr(f, machine, entry_addr, ehdr_size, (uint16_t)phnum);

    // Write program headers.
    uint64_t cur = ehdr_size;
    if (text_filesz) {
        write_phdr(f, PT_LOAD, PF_R | PF_X, text_off, text_addr, text_addr,
                   text_filesz, text_memsz, 0x1000);
        cur += 56;
    }
    if (rodata_filesz) {
        write_phdr(f, PT_LOAD, PF_R, rodata_off,
                   rodata_addr, rodata_addr, rodata_filesz, rodata_memsz, 0x1000);
        cur += 56;
    }
    if (data_filesz || data_memsz) {
        write_phdr(f, PT_LOAD, PF_R | PF_W, data_off,
                   data_addr, data_addr, data_filesz, data_memsz, 0x1000);
        cur += 56;
    }
    if (do_dynamic) {
        LinkSec *interp = &s->secs[interp_sec];
        write_phdr(f, PT_INTERP, PF_R, interp->fileoff, interp->addr,
                   interp->addr, interp->len, interp->len, 1);
        cur += 56;
        LinkSec *dyn = &s->secs[dynamic_sec];
        write_phdr(f, PT_DYNAMIC, PF_R, dyn->fileoff, dyn->addr,
                   dyn->addr, dyn->len, dyn->len, 8);
        cur += 56;
    }
    wzeros(f, file_off - cur);

    // Write section data in file-offset order so later sections never
    // overwrite earlier ones.
    typedef struct {
        LinkSec *sec;
    } WriteSec;
    WriteSec *wsecs = NULL;
    int n_wsecs = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->is_bss || sec->len == 0) continue;
        wsecs = realloc(wsecs, (size_t)(n_wsecs + 1) * sizeof(WriteSec));
        wsecs[n_wsecs++].sec = sec;
    }
    for (int i = 0; i < n_wsecs; i++) {
        for (int j = i + 1; j < n_wsecs; j++) {
            if (wsecs[j].sec->fileoff < wsecs[i].sec->fileoff) {
                WriteSec t = wsecs[i];
                wsecs[i] = wsecs[j];
                wsecs[j] = t;
            }
        }
    }
    uint64_t written = file_off;
    for (int i = 0; i < n_wsecs; i++) {
        LinkSec *sec = wsecs[i].sec;
        if (sec->fileoff > written) wzeros(f, sec->fileoff - written);
        wbuf(f, sec->data, sec->len);
        written = sec->fileoff + sec->len;
    }
    free(wsecs);
    fclose(f);
    chmod(s->out_path, 0755);

    free(dyn_syms);
    free(dyn_idx);
    free(dyn_kind);
    free(plt_idx);
    free(got_map);
    free(dynstr_off);
    return 0;
}
