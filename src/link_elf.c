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
#define ET_DYN 3
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
#define SHT_DYNSYM 11
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_GNU_verdef 0x6ffffffd
#define SHT_GNU_versym 0x6fffffff
#define SHT_GNU_verneed 0x6ffffffe

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
#define STT_FILE 4
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
#define PT_GNU_STACK 0x6474e551

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
#define R_X86_64_RELATIVE 8

#define R_AARCH64_ABS64 257
#define R_AARCH64_ABS32 258
#define R_AARCH64_PREL32 261
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
#define R_AARCH64_TLSLE_ADD_TPREL_LO12_NC 551
#define R_AARCH64_GLOB_DAT 1025
#define R_AARCH64_JUMP_SLOT 1026
#define R_AARCH64_RELATIVE 1027

// Dynamic tags
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_SONAME 14
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
#define DT_INIT_ARRAY 25
#define DT_FINI_ARRAY 26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_JMPREL 23
#define DT_PLTREL 20
#define DT_PLTRELSZ 2
#define DT_FLAGS 30
#define DT_FLAGS_1 0x6ffffffb
#define DF_BIND_NOW 0x8
#define DF_1_NOW 1
#define DT_VERSYM 0x6ffffff0
#define DT_VERNEED 0x6ffffffe
#define DT_VERNEEDNUM 0x6fffffff
#define VER_NDX_GLOBAL 1
#define VERSYM_HIDDEN 0x8000

static uint8_t r8(const uint8_t *p) { return p[0]; }
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)r16le(p) | ((uint32_t)r16le(p + 2) << 16);
}
// The classic ar/ranlib archive symbol table ("/" member) always stores
// its offset entries big-endian, independent of the target ELF's own
// byte order -- a fixed historical convention of the ar file format
// itself, not something to swap based on ELFDATA2LSB/MSB.
static uint32_t r32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
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
        case R_AARCH64_PREL32: return RL_PC32;
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
        case R_AARCH64_TLSLE_ADD_TPREL_LO12:
        case R_AARCH64_TLSLE_ADD_TPREL_LO12_NC: return RL_ARM64_TPREL_LO;
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
    // default: keep unknown allocatable sections
    return 0;
}

// Elf64_Shdr layout: sh_name(0,4) sh_type(4,4) sh_flags(8,8) sh_addr(16,8)
// sh_offset(24,8) sh_size(32,8) sh_link(40,4) sh_info(44,4)
// sh_addralign(48,8) sh_entsize(56,8) -- the field this input section's
// placement in the merged output section must actually align to is
// sh_addralign at offset 48, NOT sh_size at offset 32. Reading sh_size
// fed a completely unrelated value (the section's own byte length,
// almost never a power of 2) into align_up()'s power-of-2-only bitmask
// trick below, which for a non-power-of-2 "alignment" computes garbage
// instead of a round-up -- silently splicing a few zero-filled padding
// bytes into the middle of the merged section. For .init_array this
// planted zero-valued constructor-pointer slots that the CRT then
// calls unconditionally at startup, jumping to address 0 (SIGSEGV)
// before main() ever runs; for any other alloc section it corrupts
// data/code contents the same way, just less immediately visibly.
static size_t sec_alignment(const ElfFile *ef, uint64_t shoff, int idx) {
    const uint8_t *sh = ef->image + shoff + idx * 64;
    uint64_t a = r64le(sh + 48);
    return a ? (size_t)a : 1; // sh_addralign 0/1 both mean "no alignment constraint"
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
            {
                int *tmp = realloc(sym_map, (size_t)nsyms * sizeof(int));
                if (!tmp) {
                    free(sym_map);
                    fprintf(stderr, "rcc: out of memory\n");
                    exit(1);
                }
                sym_map = tmp;
            }
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
                if (bind == STB_LOCAL && stype == STT_SECTION) {
                    // Section symbols are unnamed but ARE valid relocation
                    // targets: "this input section's base address", with
                    // the actual offset carried by the relocation's own
                    // addend (e.g. gcc-emitted crt startup objects
                    // reference internal statics like "__wrap_main" this
                    // way instead of by name). Map to the merged output
                    // section at its base offset instead of dropping the
                    // symbol -- silently skipping it left any relocation
                    // against it unpatched, corrupting local address
                    // computations that depend on it (e.g. AArch64 crt1.o's
                    // ADRP+ADD sequence loading &__wrap_main to pass to
                    // __libc_start_main).
                    int sym_idx = link_add_sym(s, "", out_sec,
                                               out_sec >= 0 ? sec_base_off[shndx] : 0,
                                               0, 0, 0, s->n_objs);
                    if (sym_idx < 0) {
                        free(sec_map);
                        free(sym_map);
                        elf_close(&ef);
                        return -1;
                    }
                    sym_map[k] = sym_idx;
                    continue;
                }
                if (stype == STT_FILE) {
                    // Compiler-emitted ".file" debug metadata (the
                    // source filename, e.g. "foo.c") -- pure debug
                    // info, never a real relocation target. Its
                    // SHN_ABS section index maps to out_sec=-2, which
                    // gets folded to map_sec=-1 (undefined) below same
                    // as any other absolute symbol; left in, that
                    // makes it indistinguishable from a genuine
                    // undefined external reference by the time
                    // link_elf()'s "identify unresolved undefined
                    // symbols" pass runs, so it was getting synthesized
                    // a bogus DT_NEEDED/PLT entry and rejected by the
                    // runtime loader as "undefined symbol: foo.c".
                    sym_map[k] = -1;
                    continue;
                }
                if (bind == STB_LOCAL && *name == '\0') {
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
                // An unhandled relocation means the native linker cannot
                // produce a correct binary for this object. Fail the
                // native link so the driver falls back to the real
                // system linker (which handles every relocation type)
                // instead of silently emitting a broken output with the
                // relocation left unresolved (e.g. R_AARCH64_LDST128 on
                // the bundled libdfp.a's exception-flag globals produced
                // a binary that read garbage data and segfaulted).
                fprintf(stderr, "rcc: link: %s: unhandled reloc type %u\n", path, r_type);
                free(sec_map);
                free(sym_map);
                elf_close(&ef);
                return -1;
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

    // The classic ar symbol table ("/" member): a big-endian symbol
    // count, that many big-endian archive-member-header offsets (each
    // pointing at the ABSOLUTE FILE OFFSET -- including the leading
    // "!<arch>\n" -- of the header of the member defining that symbol),
    // then that many NUL-terminated symbol name strings packed back to
    // back in the same order. Precompute each name's start once, up
    // front, by walking the string table sequentially: strings don't
    // carry their own length or fixed stride, so finding the k-th one
    // requires having already found the (k-1)-th one's terminator.
    uint32_t nsym = r32be(data + symtab_off);
    const uint8_t *nametab = data + symtab_off + 4 + (size_t)nsym * 4;
    const uint8_t *nametab_end = data + symtab_off + symtab_size;
    const char **symnames = calloc(nsym, sizeof(char *));
    uint32_t *symoffs = calloc(nsym, sizeof(uint32_t));
    const uint8_t *cur = nametab;
    for (uint32_t i = 0; i < nsym && cur < nametab_end; i++) {
        symoffs[i] = r32be(data + symtab_off + 4 + i * 4);
        symnames[i] = (const char *)cur;
        while (cur < nametab_end && *cur) cur++;
        if (cur < nametab_end) cur++; // skip the NUL
    }

    uint8_t *used = calloc(nsym, 1);
    int changed = 1, round = 0;
    while (changed && round < 32) {
        changed = 0;
        round++;
        for (uint32_t i = 0; i < nsym; i++) {
            if (used[i] || !symnames[i]) continue;
            if (getenv("RCC_LINK_DEBUG") && strstr(symnames[i], "glbflags")) {
                int fs = link_find_sym(s, symnames[i]);
                fprintf(stderr, "DBG sym %d %s find=%d sec=%d\n", i, symnames[i], fs,
                        fs >= 0 ? s->syms[fs].sec : -99);
            }
            // link_find_sym returning >= 0 only means a symbol by this
            // name exists in the table AT ALL -- it says nothing about
            // whether it's still an outstanding, undefined reference.
            // Loading an earlier member in this SAME round routinely
            // *defines* other names this archive's own symbol index
            // also lists (a single .o commonly exports many symbols):
            // checking existence alone made every one of them look
            // "needed" again the moment the first pulled the member in,
            // reloading -- and thus redefining -- the identical member
            // for each additional name it happened to export, which
            // link_add_sym correctly rejects as a duplicate definition.
            // Gate on "referenced but not yet defined" (sec < 0)
            // instead, matching a real archive linker: once a name is
            // defined, every later index for that same name is done.
            int fsym = link_find_sym(s, symnames[i]);
            if (fsym < 0 || s->syms[fsym].sec >= 0) continue;
            // symoffs[i] is the target member header's absolute file
            // offset (including the archive-wide magic); off tracks the
            // same convention while scanning member headers below.
            off = 8;
            while (off + 60 <= sz) {
                char hdr[60];
                memcpy(hdr, data + off, 60);
                uint64_t msize = (uint64_t)strtoull(hdr + 48, NULL, 10);
                if (off == symoffs[i]) {
                    char tmp[] = "/tmp/rcc_link_ar_XXXXXX";
                    int tfd = mkstemp(tmp);
                    if (tfd >= 0) {
                        size_t written = write(tfd, data + off + 60, (size_t)msize);
                        close(tfd);
                        if (written != msize) {
                            perror("write rcc_link_ar");
                            abort();
                        }
                        if (getenv("RCC_LINK_DEBUG"))
                            fprintf(stderr, "DBG pulling member for sym %s (offs %u)\n", symnames[i], symoffs[i]);
                        if (elf_load_object(s, tmp) != 0) {
                            unlink(tmp);
                            free(used);
                            free(symnames);
                            free(symoffs);
                            munmap(data, sz);
                            return -1;
                        }
                        if (getenv("RCC_LINK_DEBUG"))
                            fprintf(stderr, "DBG loaded member for sym %s\n", symnames[i]);
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
    free(symoffs);
    free((void *)symnames);
    munmap(data, sz);
    return 0;
}

// A bare "lib<name>.so" -- what a real linker's -l<name> search looks
// for -- is frequently not a real, loadable ELF shared object at all:
// distro -dev packages commonly ship it as a GNU ld *linker script*
// (plain text, e.g. `GROUP ( /lib64/libm.so.6 AS_NEEDED (...) )`),
// meant only for link-time consumption by a real linker that knows how
// to follow it -- the *runtime* loader (ld.so) has no idea what a
// linker script is and rejects it outright ("invalid ELF header") the
// moment it tries to map a DT_NEEDED entry naming one. Rather than
// parsing linker scripts ourselves, just refuse to claim a name that
// isn't backed by a real, loadable ELF shared object: fall back to the
// mingw/gcc toolchain, whose own linker already knows how to follow
// them.
static bool is_real_elf_so(const char *path) {
    uint8_t hdr[4];
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    size_t n = fread(hdr, 1, 4, f);
    fclose(f);
    return n == 4 && hdr[0] == 0x7f && hdr[1] == 'E' && hdr[2] == 'L' && hdr[3] == 'F';
}

// Forward decl: full definition (with the standard-dirs search list)
// comes later in this file; resolve_archives() needs it earlier.
static int find_shared_lib(const char *libname, char *out_path, size_t out_sz,
                           char *out_soname, size_t out_soname_sz);

// Parse -l/-L flags from s->libs and pull in any static archive (.a) that
// resolves an -l<name> to a real file, using load_archive()'s existing
// GNU-ar symbol-table walker.  A shared object of the same name is
// preferred over a static archive when both exist (matching a real
// linker's -l search: .so before .a, unless -static), so an -l<name>
// with a matching .so is left alone here -- that gets a verified
// DT_NEEDED entry later in link_elf() itself, once .dynstr exists to
// hold the name.
static int resolve_archives(LinkState *s) {
    const char *lp = s->libs;
    while (lp && *lp) {
        while (*lp == ' ') lp++;
        if (!*lp) break;
        if (!strncmp(lp, "-l", 2) && lp[2] && lp[2] != ' ') {
            lp += 2;
            const char *end = lp;
            while (*end && *end != ' ') end++;
            size_t len = (size_t)(end - lp);
            if (len > 0 && len < 60) {
                char aname[64], soname[64];
                snprintf(aname, sizeof(aname), "lib%.*s.a", (int)len, lp);
                snprintf(soname, sizeof(soname), "lib%.*s.so", (int)len, lp);
                char apath[600], sopath[600];
                bool found_a = false, found_so = false;
                const char *dp = s->libs;
                while (dp && *dp && !(found_a && found_so)) {
                    while (*dp == ' ') dp++;
                    if (!*dp) break;
                    if (!strncmp(dp, "-L", 2) && dp[2] && dp[2] != ' ') {
                        dp += 2;
                        const char *dend = dp;
                        while (*dend && *dend != ' ') dend++;
                        if (!found_a) {
                            snprintf(apath, sizeof(apath), "%.*s/%s", (int)(dend - dp), dp, aname);
                            struct stat ast;
                            if (stat(apath, &ast) == 0) found_a = true;
                        }
                        if (!found_so) {
                            snprintf(sopath, sizeof(sopath), "%.*s/%s", (int)(dend - dp), dp, soname);
                            if (is_real_elf_so(sopath)) found_so = true;
                        }
                        dp = dend;
                    } else {
                        while (*dp && *dp != ' ') dp++;
                    }
                }
                if (!found_so) {
                    char stdpath[600];
                    if (find_shared_lib(soname, stdpath, sizeof(stdpath), NULL, 0) == 0)
                        found_so = true;
                }
                // Only pull the archive in when there's no .so for this
                // exact name to prefer instead (or we're statically
                // linking, where a .so wouldn't be usable anyway).
                if (found_a && (s->opt_static || !found_so)) {
                    if (load_archive(s, apath) != 0) return -1;
                }
            }
            lp = end;
        } else {
            while (*lp && *lp != ' ') lp++;
        }
    }

    // A .a given directly as a positional link input (not via -l<name>)
    // -- e.g. `rcc main.c libmath.a -o prog` -- was previously never
    // routed through load_archive() at all: the -l/-L scan above only
    // recognizes "-l"-prefixed tokens, so a bare archive path sat in
    // s->libs unread. Any symbol only that archive defines then stayed
    // permanently undefined, silently exported as an unresolvable
    // DT_NEEDED-style import (native linking still "succeeds", but the
    // resulting binary fails at load/run time with a dynamic-loader
    // "undefined symbol" error instead of a link-time one). Load every
    // bare *.a token here directly by its given path -- no name-based
    // search needed, the path is already exact.
    {
        const char *ap = s->libs;
        while (ap && *ap) {
            while (*ap == ' ') ap++;
            if (!*ap) break;
            const char *aend = ap;
            while (*aend && *aend != ' ') aend++;
            size_t alen = (size_t)(aend - ap);
            if (ap[0] != '-' && alen > 2 && alen < 600 &&
                strncmp(aend - 2, ".a", 2) == 0) {
                char apath[600];
                memcpy(apath, ap, alen);
                apath[alen] = '\0';
                if (load_archive(s, apath) != 0) return -1;
            }
            ap = aend;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// ELF executable writer
// ---------------------------------------------------------------------------

static void write_ehdr(FILE *f, uint16_t e_type, uint16_t machine, uint64_t entry, uint64_t phoff,
                       uint16_t phnum, uint64_t shoff, uint16_t shnum, uint16_t shstrndx) {
    uint8_t ident[16] = {
        0x7f, 'E', 'L', 'F', 2, 1, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0};
    wbuf(f, ident, 16);
    w16le(f, e_type);
    w16le(f, machine);
    w32le(f, 1);
    w64le(f, entry);
    w64le(f, phoff);
    w64le(f, shoff);
    w32le(f, 0);
    w16le(f, 64);
    w16le(f, 56);
    w16le(f, phnum);
    w16le(f, 64);
    w16le(f, shnum);
    w16le(f, shstrndx);
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
                // A dynamic (imported) symbol's real address isn't known
                // until ld.so resolves it at load time -- can't bake it
                // in here. Write just the addend; the R_X86_64_64/
                // R_AARCH64_ABS64 .rela.dyn entry emitted below (after
                // this function returns) tells ld.so to add the symbol's
                // resolved address on top at load time, same as a real
                // linker's copy relocation for a function-pointer-typed
                // global initialized from an external symbol (e.g.
                // `void (*fp)(void) = close;`).
                if (dyn_idx && dyn_idx[r->sym]) {
                    w64le_m(p, (uint64_t)A);
                    break;
                }
                S = symbol_address(s, r->sym);
                w64le_m(p, (r->addend ? 0 : r64le(p)) + S + (uint64_t)A);
                break;
            case RL_ABS32:
                if (dyn_idx && dyn_idx[r->sym]) {
                    w32le_m(p, (uint32_t)A);
                    break;
                }
                S = symbol_address(s, r->sym);
                w32le_m(p, (uint32_t)((int32_t)(r->addend ? 0 : r32le(p)) + (int32_t)A + (int64_t)S));
                break;
            case RL_ABS32U:
                if (dyn_idx && dyn_idx[r->sym]) {
                    w32le_m(p, (uint32_t)A);
                    break;
                }
                S = symbol_address(s, r->sym);
                w32le_m(p, (uint32_t)((r->addend ? 0 : r32le(p)) + (uint64_t)A + S));
                break;
            case RL_PC32:
            case RL_PC32_PLT:
                if (dyn_idx && dyn_idx[r->sym]) {
                    if (!plt_idx || plt_idx[r->sym] < 0) {
                        return -1;
                    }
                    S = plt_addr + 16 + (uint64_t)plt_idx[r->sym] * 16;
                } else {
                    S = symbol_address(s, r->sym);
                }
                w32le_m(p, (uint32_t)((int32_t)(r->addend ? 0 : r32le(p)) + (int32_t)A + (int64_t)(S - pc)));
                break;
            case RL_PC64:
                if (dyn_idx && dyn_idx[r->sym]) {
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
                w32le_m(p, (uint32_t)((int32_t)(r->addend ? 0 : r32le(p)) + (int32_t)A + (int64_t)(S - pc)));
                break;
            }
            case RL_TPOFF32: {
                LinkSym *tsym = &s->syms[r->sym];
                if (tsym->sec >= 0 && s->secs[tsym->sec].is_tls) {
                    uint64_t tbase = 0, tsize = 0;
                    for (int k = 0; k < s->n_secs; k++) {
                        if (s->secs[k].is_tls && s->secs[k].alloc) {
                            if (tbase == 0) tbase = s->secs[k].addr;
                            uint64_t end = s->secs[k].addr + s->secs[k].len;
                            if (end - tbase > tsize) tsize = end - tbase;
                        }
                    }
                    uint64_t offset = (s->secs[tsym->sec].addr - tbase) + tsym->value;
                    // x86-64 TLS LE: negative offset from TP
                    S = offset - tsize;
                } else {
                    S = symbol_address(s, r->sym);
                }
                w32le_m(p, (uint32_t)((int32_t)(r->addend ? 0 : r32le(p)) + (int32_t)A + (int64_t)S));
                break;
            }
            case RL_ARM64_TPREL_HI:
            case RL_ARM64_TPREL_LO: {
                // AArch64 local-exec TLS: unlike x86-64's negative,
                // TP-relative offset, AArch64 places the TLS block AFTER
                // a fixed 16-byte TCB header, so the offset is POSITIVE:
                // addr = TP + 16 + (var's offset within the merged block).
                LinkSym *tsym = &s->syms[r->sym];
                uint64_t tprel;
                if (tsym->sec >= 0 && s->secs[tsym->sec].is_tls) {
                    uint64_t tbase = 0;
                    for (int k = 0; k < s->n_secs; k++) {
                        if (s->secs[k].is_tls && s->secs[k].alloc) {
                            if (tbase == 0) tbase = s->secs[k].addr;
                        }
                    }
                    uint64_t offset = (s->secs[tsym->sec].addr - tbase) + tsym->value;
                    tprel = 16 + offset + (uint64_t)A;
                } else {
                    tprel = symbol_address(s, r->sym) + (uint64_t)A;
                }
                uint32_t ins = r32le(p);
                uint32_t imm12 = r->type == RL_ARM64_TPREL_HI
                    ? (uint32_t)((tprel >> 12) & 0xfff)
                    : (uint32_t)(tprel & 0xfff);
                ins = (ins & 0xffc003ff) | (imm12 << 10);
                w32le_m(p, ins);
                break;
            }
            case RL_ARM64_B26: {
                if (dyn_idx && dyn_idx[r->sym]) {
                    if (!plt_idx || plt_idx[r->sym] < 0) {
                        return -1;
                    }
                    S = plt_addr + 16 + (uint64_t)plt_idx[r->sym] * 16;
                } else {
                    S = symbol_address(s, r->sym);
                }
                uint32_t ins = r32le(p);
                int64_t delta = (int64_t)(S + (uint64_t)A - pc);
                delta >>= 2;
                ins = (ins & ~0x03ffffffu) | ((uint32_t)(delta & 0x03ffffffu));
                w32le_m(p, ins);
                break;
            }
            case RL_ARM64_ADR_PG:
                // RCC's codegen always computes external symbol addresses
                // directly (ADRP+ADD), never via GOT indirection on Linux.
                // That can't represent a true dynamic (undefined-until-load)
                // symbol's address, so fall back, matching RL_ABS64/32/32U.
                if (dyn_idx && dyn_idx[r->sym]) {
                    return -1;
                }
                S = symbol_address(s, r->sym);
                {
                    uint32_t ins = r32le(p);
                    int64_t delta = (int64_t)((S + (uint64_t)A) - (pc & ~(uint64_t)0xfff));
                    int64_t imm = delta >> 12;
                    ins = (ins & 0x9f00001f) |
                        ((uint32_t)(imm & 3) << 29) |
                        ((uint32_t)((imm >> 2) & 0x7ffff) << 5);
                    w32le_m(p, ins);
                }
                break;
            case RL_ARM64_ADD_LO:
                if (dyn_idx && dyn_idx[r->sym]) {
                    return -1;
                }
                S = symbol_address(s, r->sym);
                {
                    uint32_t ins = r32le(p);
                    uint64_t off = (S + (uint64_t)A) & 0xfff;
                    // ADD imm12 is unscaled, unlike LDR/STR's imm12.
                    ins = (ins & 0xffc003ff) | ((uint32_t)off << 10);
                    w32le_m(p, ins);
                }
                break;
            case RL_ARM64_GOT_PG:
            case RL_ARM64_GOT_LO: {
                if (!got_map) {
                    fprintf(stderr, "rcc: link: GOT-relative reloc without GOT\n");
                    return -1;
                }
                int slot = got_map[r->sym];
                if (slot < 0) {
                    fprintf(stderr, "rcc: link: no GOT slot for '%s'\n", sym->name);
                    return -1;
                }
                S = got_addr + (uint64_t)slot * 8;
                uint32_t ins = r32le(p);
                if (r->type == RL_ARM64_GOT_PG) {
                    int64_t delta = (int64_t)((S + (uint64_t)A) - (pc & ~(uint64_t)0xfff));
                    int64_t imm = delta >> 12;
                    ins = (ins & 0x9f00001f) |
                        ((uint32_t)(imm & 3) << 29) |
                        ((uint32_t)((imm >> 2) & 0x7ffff) << 5);
                } else {
                    uint64_t off = (S + (uint64_t)A) & 0xfff;
                    ins = (ins & 0xffc003ff) | ((uint32_t)(off >> 3) << 10);
                }
                w32le_m(p, ins);
                break;
            }
            default:
                // Let the shared backend handle the rest (ARM64, etc.)
                link_reloc_apply(s->arch, sec, r, symbol_address(s, r->sym), pc, 0);
                break;
            }
        }
    }
    return 0;
}

// Find a shared library by name in the standard, architecture-specific
// system library directories (never the bare /lib or /usr/lib: on a
// multilib system those commonly hold the *other* architecture's copy,
// and even when they don't, whatever they resolve "libname" to via a
// symlink is not reliably what the runtime loader's own search --
// governed by /etc/ld.so.cache, not by directory-list order here --
// will resolve the same bare name to). Returns 0 and fills out_path
// (the full path actually used) and out_soname (just the basename --
// what a DT_NEEDED entry carries) on success, -1 if nothing usable was
// found anywhere.
//
// A bare "lib<name>.so" is frequently not a real, loadable ELF shared
// object at all: distro -dev packages commonly ship it as a GNU ld
// *linker script* (plain text, e.g. `GROUP ( /lib64/libm.so.6
// AS_NEEDED (...) )`), meant only for link-time consumption by a real
// linker that knows how to follow it -- the *runtime* loader has no
// idea what a linker script is and rejects it outright ("invalid ELF
// header") the moment it tries to map a DT_NEEDED entry naming one.
// Rather than parsing linker scripts in general, special-case the one
// pattern every glibc install actually needs: if "lib<name>.so" itself
// isn't real ELF, retry "lib<name>.so.6" in the same directory -- the
// canonical glibc SONAME a real ld-linked binary ends up with for
// libm/libpthread/etc regardless (glibc's own -dev packages ship
// exactly this script-plus-real-so.6 pairing).
static int find_shared_lib(const char *libname, char *out_path, size_t out_sz,
                           char *out_soname, size_t out_soname_sz) {
    static const char *dirs[] = {
#ifdef __aarch64__
        "/usr/lib64",
        "/usr/lib/aarch64-linux-gnu",
        "/usr/aarch64-linux-gnu/lib",
        "/lib/aarch64-linux-gnu",
        "/lib64",
#else
        "/usr/lib64",
        "/usr/lib/x86_64-linux-gnu",
        "/lib/x86_64-linux-gnu",
        "/lib64",
#endif
        NULL,
    };
    for (int i = 0; dirs[i]; i++) {
        snprintf(out_path, out_sz, "%s/%s", dirs[i], libname);
        if (is_real_elf_so(out_path)) {
            if (out_soname) snprintf(out_soname, out_soname_sz, "%s", libname);
            return 0;
        }
        char versioned[600];
        snprintf(versioned, sizeof(versioned), "%s.6", out_path);
        if (is_real_elf_so(versioned)) {
            snprintf(out_path, out_sz, "%s", versioned);
            if (out_soname) snprintf(out_soname, out_soname_sz, "%s.6", libname);
            return 0;
        }
    }
    return -1;
}

// For each of the `n` requested symbol `names`, look up the *default*
// (non-hidden) versioned definition of that symbol in the shared library
// at `path`, via its .dynsym / .gnu.version (versym) / .gnu.version_d
// (verdef) sections. out[i] receives a malloc'd version-name string (e.g.
// "GLIBC_2.3.2") for symbols the library defines with version info, or
// NULL if the symbol isn't found or the library carries no version data
// for it (unversioned linking is used for those, unaffected).
//
// This matters because several glibc functions (pthread_cond_init,
// pthread_rwlock_init, etc.) have multiple ABI-incompatible symbol
// versions coexisting in one library for compatibility. An unversioned
// dynamic symbol reference can bind to whichever definition the loader's
// hash lookup happens to find first -- which is not guaranteed to be the
// current (default, "@@") one -- silently corrupting behavior instead of
// failing to link. Emitting proper VERNEED/VERSYM info pins each
// reference to the same version a normal ld-linked binary would use.
static void lookup_lib_symbol_versions(const char *path, const char **names,
                                       char **out, int n) {
    for (int i = 0; i < n; i++) out[i] = NULL;
    ElfFile ef;
    if (elf_open(path, &ef) != 0) return;
    if (ef.size < 64 || memcmp(ef.image, "\x7f"
                                         "ELF",
                               4) != 0) {
        elf_close(&ef);
        return;
    }
    uint64_t e_shoff = r64le(ef.image + 40);
    uint16_t e_shnum = r16le(ef.image + 60);
    if (!e_shoff || e_shnum == 0 || e_shoff + (uint64_t)e_shnum * 64 > ef.size) {
        elf_close(&ef);
        return;
    }

    int dynsym_idx = -1, versym_idx = -1, verdef_idx = -1;
    for (int i = 0; i < e_shnum; i++) {
        const uint8_t *sh = ef.image + e_shoff + (uint64_t)i * 64;
        uint32_t sh_type = r32le(sh + 4);
        if (sh_type == SHT_DYNSYM && dynsym_idx < 0) dynsym_idx = i;
        else if (sh_type == SHT_GNU_versym)
            versym_idx = i;
        else if (sh_type == SHT_GNU_verdef)
            verdef_idx = i;
    }
    if (dynsym_idx < 0 || versym_idx < 0 || verdef_idx < 0) {
        elf_close(&ef); // no version info in this library; leave all NULL
        return;
    }

    const uint8_t *dynsym_sh = ef.image + e_shoff + (uint64_t)dynsym_idx * 64;
    uint64_t dynsym_off = r64le(dynsym_sh + 24);
    uint64_t dynsym_size = r64le(dynsym_sh + 32);
    uint64_t dynsym_link = r32le(dynsym_sh + 40);
    uint64_t dynsym_entsz = r64le(dynsym_sh + 56);
    uint64_t n_syms = dynsym_entsz ? dynsym_size / dynsym_entsz : 0;

    const uint8_t *dynstr_sh = ef.image + e_shoff + dynsym_link * 64;
    uint64_t dynstr_off = r64le(dynstr_sh + 24);

    const uint8_t *versym_sh = ef.image + e_shoff + (uint64_t)versym_idx * 64;
    const uint8_t *versym_data = ef.image + r64le(versym_sh + 24);

    const uint8_t *verdef_sh = ef.image + e_shoff + (uint64_t)verdef_idx * 64;
    uint64_t verdef_off = r64le(verdef_sh + 24);
    uint32_t verdef_link = r32le(verdef_sh + 40);
    const uint8_t *verdefstr_sh = ef.image + e_shoff + (uint64_t)verdef_link * 64;
    uint64_t verdefstr_off = r64le(verdefstr_sh + 24);

    for (uint64_t si = 0; si < n_syms; si++) {
        const uint8_t *se = ef.image + dynsym_off + si * dynsym_entsz;
        uint32_t nm_off = r32le(se);
        uint16_t shndx = r16le(se + 6);
        if (shndx == SHN_UNDEF) continue; // not defined in this library
        uint8_t info = se[4];
        int bind = info >> 4;
        if (bind != STB_GLOBAL && bind != STB_WEAK) continue;
        const char *sname = (const char *)(ef.image + dynstr_off + nm_off);
        int match = -1;
        for (int i = 0; i < n; i++) {
            if (!out[i] && strcmp(sname, names[i]) == 0) {
                match = i;
                break;
            }
        }
        if (match < 0) continue;
        uint16_t vs = r16le(versym_data + si * 2);
        uint16_t vidx = vs & 0x7fff;
        bool hidden = (vs & VERSYM_HIDDEN) != 0;
        if (hidden || vidx < 2) continue; // not a default versioned definition
        // Walk the verdef chain to find the entry defining index `vidx`.
        uint64_t off = verdef_off;
        for (;;) {
            const uint8_t *vd = ef.image + off;
            uint16_t vd_ndx = r16le(vd + 4);
            uint16_t vd_cnt = r16le(vd + 6);
            uint32_t vd_aux = r32le(vd + 12);
            uint32_t vd_next = r32le(vd + 16);
            if (vd_ndx == vidx && vd_cnt > 0) {
                const uint8_t *vda = vd + vd_aux;
                uint32_t vda_name = r32le(vda);
                out[match] = strdup((const char *)(ef.image + verdefstr_off + vda_name));
                break;
            }
            if (vd_next == 0) break;
            off += vd_next;
        }
    }
    elf_close(&ef);
}

// Mark found[i]=true for every name in names[0..n) that the ELF shared
// object at `path` defines: a .dynsym entry that is not SHN_UNDEF and
// binds global or weak. Returns true iff the file was a readable ELF
// whose .dynsym could actually be scanned -- so the caller can tell the
// difference between "this library does not define the symbol" (real
// evidence of absence) and "this library could not be inspected" (no
// evidence either way). Unlike lookup_lib_symbol_versions() above this
// needs only .dynsym, so it works against version-less libraries
// (libgcc_s, a bare *.so input) too.
static bool so_mark_defined(const char *path, const char **names,
                            bool *found, int n) {
    ElfFile ef;
    if (elf_open(path, &ef) != 0) return false;
    if (ef.size < 64 || memcmp(ef.image, "\x7f"
                                         "ELF",
                               4) != 0) {
        elf_close(&ef);
        return false;
    }
    uint64_t e_shoff = r64le(ef.image + 40);
    uint16_t e_shnum = r16le(ef.image + 60);
    if (!e_shoff || e_shnum == 0 || e_shoff + (uint64_t)e_shnum * 64 > ef.size) {
        elf_close(&ef);
        return false;
    }
    int dynsym_idx = -1;
    for (int i = 0; i < e_shnum; i++) {
        const uint8_t *sh = ef.image + e_shoff + (uint64_t)i * 64;
        if (r32le(sh + 4) == SHT_DYNSYM) {
            dynsym_idx = i;
            break;
        }
    }
    if (dynsym_idx < 0) {
        elf_close(&ef);
        return false;
    }
    const uint8_t *dynsym_sh = ef.image + e_shoff + (uint64_t)dynsym_idx * 64;
    uint64_t sym_off = r64le(dynsym_sh + 24);
    uint64_t sym_size = r64le(dynsym_sh + 32);
    uint64_t sym_link = r32le(dynsym_sh + 40);
    uint64_t sym_entsz = r64le(dynsym_sh + 56);
    if (!sym_entsz || sym_off + sym_size > ef.size || sym_link >= e_shnum) {
        elf_close(&ef);
        return false;
    }
    const uint8_t *dynstr_sh = ef.image + e_shoff + sym_link * 64;
    uint64_t dynstr_off = r64le(dynstr_sh + 24);
    uint64_t n_syms = sym_size / sym_entsz;
    for (uint64_t si = 0; si < n_syms; si++) {
        const uint8_t *se = ef.image + sym_off + si * sym_entsz;
        if (r16le(se + 6) == SHN_UNDEF) continue; // imported, not defined here
        uint8_t bind = se[4] >> 4;
        if (bind != STB_GLOBAL && bind != STB_WEAK) continue;
        const char *sname = (const char *)(ef.image + dynstr_off + r32le(se));
        for (int i = 0; i < n; i++) {
            if (!found[i] && strcmp(sname, names[i]) == 0) {
                found[i] = true;
                break;
            }
        }
    }
    elf_close(&ef);
    return true;
}

// Extract DT_SONAME from an arbitrary ELF shared object by walking its
// PROGRAM headers (PT_DYNAMIC) directly, the same way ld.so itself finds
// it at load time -- unlike lookup_lib_symbol_versions() above, this
// works even against a .so this linker itself produced (which currently
// carries no section headers at all: PT_DYNAMIC/program headers are the
// only structure ld.so ever actually needs). Returns false (caller falls
// back to the file's own basename, matching how ld.so treats a
// soname-less object) if the file has no PT_DYNAMIC or no DT_SONAME tag.
static bool read_soname(const char *path, char *out, size_t out_sz) {
    ElfFile ef;
    if (elf_open(path, &ef) != 0) return false;
    if (ef.size < 64 || memcmp(ef.image, "\x7f"
                                         "ELF",
                               4) != 0) {
        elf_close(&ef);
        return false;
    }
    uint64_t e_phoff = r64le(ef.image + 32);
    uint16_t e_phnum = r16le(ef.image + 56);
    if (!e_phoff || e_phnum == 0) {
        elf_close(&ef);
        return false;
    }

    typedef struct {
        uint64_t vaddr, offset, filesz;
    } LoadSeg;
    LoadSeg loads[16];
    int n_loads = 0;
    uint64_t dyn_off = 0, dyn_filesz = 0;
    bool have_dyn = false;
    for (int i = 0; i < e_phnum; i++) {
        uint64_t ph_off = e_phoff + (uint64_t)i * 56;
        if (ph_off + 56 > ef.size) break;
        const uint8_t *ph = ef.image + ph_off;
        uint32_t p_type = r32le(ph);
        if (p_type == PT_DYNAMIC) {
            dyn_off = r64le(ph + 8); // p_offset
            dyn_filesz = r64le(ph + 32); // p_filesz
            have_dyn = true;
        } else if (p_type == PT_LOAD && n_loads < 16) {
            loads[n_loads].offset = r64le(ph + 8);
            loads[n_loads].vaddr = r64le(ph + 16);
            loads[n_loads].filesz = r64le(ph + 32);
            n_loads++;
        }
    }
    if (!have_dyn || dyn_off + dyn_filesz > ef.size) {
        elf_close(&ef);
        return false;
    }

    uint64_t strtab_vaddr = 0, soname_val = 0;
    bool have_strtab = false, have_soname = false;
    for (uint64_t off = dyn_off; off + 16 <= dyn_off + dyn_filesz; off += 16) {
        uint64_t tag = r64le(ef.image + off);
        uint64_t val = r64le(ef.image + off + 8);
        if (tag == DT_NULL) break;
        if (tag == DT_STRTAB) {
            strtab_vaddr = val;
            have_strtab = true;
        } else if (tag == DT_SONAME) {
            soname_val = val;
            have_soname = true;
        }
    }
    if (!have_strtab || !have_soname) {
        elf_close(&ef);
        return false;
    }

    uint64_t strtab_off = 0;
    bool mapped = false;
    for (int i = 0; i < n_loads; i++) {
        if (strtab_vaddr >= loads[i].vaddr && strtab_vaddr < loads[i].vaddr + loads[i].filesz) {
            strtab_off = loads[i].offset + (strtab_vaddr - loads[i].vaddr);
            mapped = true;
            break;
        }
    }
    if (!mapped) {
        elf_close(&ef);
        return false;
    }

    uint64_t name_off = strtab_off + soname_val;
    if (name_off >= ef.size) {
        elf_close(&ef);
        return false;
    }
    size_t maxlen = (size_t)(ef.size - name_off);
    if (maxlen >= out_sz) maxlen = out_sz - 1;
    size_t n = strnlen((const char *)(ef.image + name_off), maxlen);
    bool ok = n > 0 && n < out_sz;
    if (ok) {
        memcpy(out, ef.image + name_off, n);
        out[n] = '\0';
    }
    elf_close(&ef);
    return ok;
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
#ifdef __aarch64__
        "/usr/lib64",
        "/usr/lib/aarch64-linux-gnu",
        "/usr/aarch64-linux-gnu/lib",
        "/lib/aarch64-linux-gnu",
        "/lib64",
#else
        "/usr/lib64",
        "/usr/lib/x86_64-linux-gnu",
        "/lib/x86_64-linux-gnu",
        "/lib64",
#endif
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

// Whether `sec` should get a real section-header-table entry. Every
// alloc'd section qualifies except .gnu.version/.gnu.version_r when no
// version info was ever found for this link (n_verneed_versions == 0):
// both are unconditionally created via link_find_or_create_sec() up
// front, but only ever populated with real bytes inside the
// `n_verneed_versions > 0` branch below, matching the equally-gated
// DT_VERSYM/DT_VERNEED dynamic tags. Declaring a *populated-looking*
// SHT_GNU_versym header for an EMPTY section corrupts the 1:1
// versym-per-dynsym-entry invariant BFD enforces: readelf/ld reject the
// object outright ("invalid version offset 1 (max 0)") the moment
// .dynsym has any real entries but .gnu.version has zero.
static bool sec_wants_shdr(const LinkSec *sec, int n_verneed_versions) {
    if (n_verneed_versions == 0 &&
        (strcmp(sec->name, ".gnu.version") == 0 || strcmp(sec->name, ".gnu.version_r") == 0))
        return false;
    return true;
}

int link_elf(LinkState *s) {
    // Static + shared is nonsensical (there's no "statically linked
    // shared object" concept); refuse rather than guess which one wins.
    if (s->opt_static && s->opt_shared) return -1;
    if (resolve_archives(s) != 0) return -1;

    // Ensure required sections exist.
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".rodata", true, false, false, false, false, 1);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 8);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 8);

    // Load C runtime startup files when producing a standalone executable
    // (crt1.o's _start -> __libc_start_main chain). A shared object needs
    // none of this: it has no entry point of its own, and DT_INIT_ARRAY
    // (set up below from any .init_array this link produced) is how its
    // constructors run, the same way crt1.o would have wired them for an
    // executable via .init_array too.
    if (!s->opt_static && !s->opt_shared) {
        if (load_crt_files(s) != 0) return -1;
    }

    int entry_sym = -1;
    if (!s->opt_shared) {
        entry_sym = link_find_sym(s, "_start");
        if (entry_sym < 0) {
            // No _start: need C runtime.  Fall back to external linker.
            return -1;
        }
    }

    // Synthesize __start_<name>/__stop_<name> boundary symbols for any
    // section whose name is a valid C identifier -- matches real GNU ld's
    // automatic behavior (ld.bfd/ld.gold/lld all do this), used by source
    // like `extern char __start_foo[]; extern char __stop_foo[];`
    // bracketing data placed via `__attribute__((section("foo")))` (e.g.
    // a GC's "const heap" boundary markers, a linker-collected registry
    // array). All objects are already loaded at this point (link_elf()
    // is only reached after every link_load_object() call in rcc_link()
    // returns), so `sec->len` is final -- no later pass grows a
    // user-object-derived section's *file* content further, only the
    // handful of backend-created sections (.dynstr etc.) referenced by
    // name explicitly below, never through this generic name match.
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec >= 0 || !sym->name) continue;
        bool is_start = !strncmp(sym->name, "__start_", 8);
        bool is_stop = !is_start && !strncmp(sym->name, "__stop_", 7);
        if (!is_start && !is_stop) continue;
        const char *secname = sym->name + (is_start ? 8 : 7);
        if (!*secname) continue;
        int target = -1;
        for (int j = 0; j < s->n_secs; j++) {
            if (!strcmp(s->secs[j].name, secname)) {
                target = j;
                break;
            }
        }
        if (target < 0) continue;
        sym->sec = target;
        sym->value = is_start ? 0 : s->secs[target].len;
        sym->resolved = true;
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
                {
                    int *tmp = realloc(dyn_syms, (size_t)cap_dyn * sizeof(int));
                    if (!tmp) {
                        free(dyn_syms);
                        fprintf(stderr, "rcc: out of memory\n");
                        exit(1);
                    }
                    dyn_syms = tmp;
                }
            }
            dyn_syms[n_dyn] = i;
            dyn_idx[i] = n_dyn + 1; // dynsym index, 1-based (0 is null)
            n_dyn++;
        }
    }

    // A shared object always needs .dynamic/.dynsym/PT_DYNAMIC
    // infrastructure to be a discoverable, loadable shared object at
    // all -- regardless of whether it happens to import anything.
    bool do_dynamic = (n_dyn > 0 && !s->opt_static) || s->opt_shared;
    if (n_dyn > 0 && !do_dynamic) {
        // Unsupported dynamic linking configuration; fall back.
        free(dyn_syms);
        free(dyn_idx);
        return -1;
    }

    // For -shared, also collect every globally-visible symbol *defined*
    // in this link (not just the undefined ones above) to export via
    // .dynsym with a real address -- so other objects, or a dlopen()
    // caller, can actually resolve this library's own functions/data.
    // Exported entries are appended to .dynsym after the imported ones
    // (dynsym index n_dyn+1 .. n_dyn+n_exp); dyn_idx[] intentionally
    // stays untouched by them since dyn_idx marks *imports* specifically
    // (apply_dynamic_relocs uses it to route a reference through the
    // PLT/GOT instead of resolving it directly).
    //
    // -rdynamic (opt_export_dynamic) requests the identical treatment for
    // an ordinary executable: real ld's --export-dynamic/-E adds every
    // global symbol to .dynsym so a later dlopen()'d shared object (e.g.
    // bash's loadable builtins) can dlsym()/resolve back into the main
    // program's own symbols, which a plain executable's .dynsym would
    // otherwise omit (it normally holds only the imports needed to bind
    // against libraries, not the executable's own definitions).
    int n_exp = 0, cap_exp = 0;
    int *exp_syms = NULL;
    if (s->opt_shared || s->opt_export_dynamic) {
        for (int i = 0; i < s->n_syms; i++) {
            LinkSym *sym = &s->syms[i];
            if (sym->sec >= 0 && sym->bind != STB_LOCAL && sym->name && sym->name[0]) {
                if (n_exp == cap_exp) {
                    cap_exp = cap_exp ? cap_exp * 2 : 16;
                    {
                        int *tmp = realloc(exp_syms, (size_t)cap_exp * sizeof(int));
                        if (!tmp) {
                            free(exp_syms);
                            fprintf(stderr, "rcc: out of memory\n");
                            exit(1);
                        }
                        exp_syms = tmp;
                    }
                }
                exp_syms[n_exp++] = i;
            }
        }
    }

    int interp_sec = -1, dynamic_sec = -1, dynsym_sec = -1, dynstr_sec = -1;
    int hash_sec = -1, plt_sec = -1, gotplt_sec = -1, relaplt_sec = -1, reladyn_sec = -1;
    int versym_sec = -1, verneed_sec = -1, n_verneed_versions = 0;
    int *dyn_kind = NULL;
    int *plt_idx = NULL;
    int *got_map = NULL;
    int *dynstr_off = NULL;
    int n_func_dyn = 0;
    int n_reladyn = 0;
    int n_relative = 0;
    int n_absdyn = 0;
    int soname_off = 0;
    int libc_off = 0;
    int n_needed = 1;
    int needed_offs[16];
    // Filesystem paths of the DT_NEEDED shared objects this link resolves,
    // captured as they are discovered so the undefined-symbol gate below
    // can scan their .dynsym for genuine unresolved references.
    char scan_paths[32][600];
    int n_scan = 0;

    if (do_dynamic) {
        if (!s->opt_shared)
            interp_sec = link_find_or_create_sec(s, ".interp", true, false, false, false, false, 1);
        dynamic_sec = link_find_or_create_sec(s, ".dynamic", true, false, false, false, false, 8);
        dynsym_sec = link_find_or_create_sec(s, ".dynsym", true, false, false, false, false, 8);
        dynstr_sec = link_find_or_create_sec(s, ".dynstr", true, false, false, false, false, 1);
        hash_sec = link_find_or_create_sec(s, ".hash", true, false, false, false, false, 8);
        plt_sec = link_find_or_create_sec(s, ".plt", true, false, true, false, false, 16);
        gotplt_sec = link_find_or_create_sec(s, ".got.plt", true, true, false, false, false, 8);
        relaplt_sec = link_find_or_create_sec(s, ".rela.plt", true, false, false, false, false, 8);
        reladyn_sec = link_find_or_create_sec(s, ".rela.dyn", true, false, false, false, false, 8);
        versym_sec = link_find_or_create_sec(s, ".gnu.version", true, false, false, false, false, 2);
        verneed_sec = link_find_or_create_sec(s, ".gnu.version_r", true, false, false, false, false, 4);

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

        // .interp: only executables need to tell the kernel which
        // dynamic linker to invoke on exec(); a shared object is always
        // loaded *by* one (or by dlopen()), never exec()'d directly.
        if (!s->opt_shared) {
            const char *interp = s->arch == ARCH_AARCH64 ? "/lib/ld-linux-aarch64.so.1"
                                                         : "/lib64/ld-linux-x86-64.so.2";
            link_sec_append(s, interp_sec, (const uint8_t *)interp, strlen(interp) + 1, 1);
        }

        // .dynstr: start with a null byte, then needed library names.
        uint8_t nul = 0;
        link_sec_append(s, dynstr_sec, &nul, 1, 1);
        libc_off = (int)link_sec_append(s, dynstr_sec,
                                        (const uint8_t *)"libc.so.6", 10, 1);

        // Add libgcc_s.so.1 for compiler-rt functions (__udivti3, etc.)
        int libgcc_off = (int)link_sec_append(s, dynstr_sec,
                                              (const uint8_t *)"libgcc_s.so.1", 14, 1);

        // Add libm.so.6 unconditionally, matching libgcc_s above: RCC's
        // codegen emits a genuine external call for math.h functions
        // (fabs/sqrt/pow/...) rather than inlining them to native FP
        // instructions the way GCC/Clang do for the simple ones -- so a
        // program merely #including <math.h> and calling e.g. fabs()
        // needs a real libm symbol at load time even though an
        // equivalent GCC-compiled binary wouldn't reference libm at all
        // (its fabs() call never survives past codegen). glibc >= 2.34
        // ships libm.so.6 as an empty compatibility stub -- loading it
        // when unused costs nothing beyond one extra DT_NEEDED entry.
        int libm_off = (int)link_sec_append(s, dynstr_sec,
                                            (const uint8_t *)"libm.so.6", 10, 1);

        // DT_SONAME: the name other objects' DT_NEEDED entries record
        // when they link against this library -- what the runtime loader
        // actually looks up at their load time, taking precedence over
        // whatever path this file happened to be found at. Real linkers
        // only emit one when passed -soname explicitly; lacking that
        // flag here, fall back to this output file's own basename (e.g.
        // "libfoo.so"), which is what every DT_NEEDED consumer will look
        // for anyway since that's the name they pass to -l.
        if (s->opt_shared) {
            const char *base = strrchr(s->out_path, '/');
            base = base ? base + 1 : s->out_path;
            soname_off = (int)link_sec_append(s, dynstr_sec,
                                              (const uint8_t *)base, strlen(base) + 1, 1);
        }

        // Parse -l flags for additional DT_NEEDED entries.  Collect -L
        // search dirs first (order matters: -L dirs are searched before
        // the standard system dirs, same as a real linker) so each -l
        // name can be verified against a real file instead of assumed.

        const char *user_dirs[16];
        int n_user_dirs = 0;
        {
            const char *dp = s->libs;
            while (dp && *dp) {
                while (*dp == ' ') dp++;
                if (!*dp) break;
                if (!strncmp(dp, "-L", 2) && dp[2] && dp[2] != ' ') {
                    dp += 2;
                    const char *dend = dp;
                    while (*dend && *dend != ' ') dend++;
                    if (n_user_dirs < 16) {
                        char *d = malloc((size_t)(dend - dp) + 1);
                        memcpy(d, dp, (size_t)(dend - dp));
                        d[dend - dp] = '\0';
                        user_dirs[n_user_dirs++] = d;
                    }
                    dp = dend;
                } else {
                    while (*dp && *dp != ' ') dp++;
                }
            }
        }

        n_needed = 3;
        needed_offs[0] = libc_off;
        needed_offs[1] = libgcc_off;
        needed_offs[2] = libm_off;
        bool lib_lookup_failed = false;
        const char *lp = s->libs;
        while (lp && *lp) {
            while (*lp == ' ') lp++;
            if (!*lp) break;
            if (!strncmp(lp, "-l", 2) && lp[2] && lp[2] != ' ') {
                lp += 2;
                const char *end = lp;
                while (*end && *end != ' ') end++;
                size_t len = (size_t)(end - lp);
                // Skip libs integrated into libc on modern glibc.
                if (len == 7 && !memcmp(lp, "pthread", 7)) {
                    lp = end;
                    continue;
                }
                if (len > 0 && len < 60 && n_needed < 16) {
                    char soname[64];
                    snprintf(soname, sizeof(soname), "lib%.*s.so", (int)len, lp);
                    // A DT_NEEDED entry is a promise the dynamic loader
                    // will be able to find this file at run time -- verify
                    // it actually exists (checking -L dirs first, then the
                    // standard system dirs) instead of assuming every
                    // -l<name> names some shared library we've never
                    // looked for. An unverified guess here previously
                    // produced executables that "linked" successfully but
                    // failed at startup with a missing-library error the
                    // moment the loader actually went looking for it.
                    char found_path[600];
                    char resolved_soname[64];
                    snprintf(resolved_soname, sizeof(resolved_soname), "%s", soname);
                    bool found = false;
                    for (int di = 0; di < n_user_dirs && !found; di++) {
                        snprintf(found_path, sizeof(found_path), "%s/%s", user_dirs[di], soname);
                        if (is_real_elf_so(found_path)) {
                            found = true;
                        } else {
                            char versioned[600];
                            snprintf(versioned, sizeof(versioned), "%s.6", found_path);
                            if (is_real_elf_so(versioned)) {
                                snprintf(resolved_soname, sizeof(resolved_soname), "%s.6", soname);
                                found = true;
                            }
                        }
                    }
                    if (!found && find_shared_lib(soname, found_path, sizeof(found_path), resolved_soname, sizeof(resolved_soname)) == 0)
                        found = true;
                    if (!found) {
                        // No .so anywhere -- but resolve_archives() above
                        // already pulled in a matching .a for this exact
                        // name if one existed in an -L dir, satisfying
                        // whatever symbols it needed to statically. If
                        // that's what happened, this -l flag needs no
                        // DT_NEEDED entry at all (nothing dynamic to load).
                        char aname[64], apath[600];
                        snprintf(aname, sizeof(aname), "lib%.*s.a", (int)len, lp);
                        bool found_a = false;
                        for (int di = 0; di < n_user_dirs && !found_a; di++) {
                            snprintf(apath, sizeof(apath), "%s/%s", user_dirs[di], aname);
                            struct stat ast;
                            if (stat(apath, &ast) == 0) found_a = true;
                        }
                        if (found_a) {
                            lp = end;
                            continue;
                        }
                        // Neither a real .so nor a real .a for this name:
                        // we can't back the promise a DT_NEEDED entry
                        // makes. Fall back to the mingw/gcc toolchain,
                        // which does real -L/-l search.
                        lib_lookup_failed = true;
                        break;
                    }
                    if (n_scan < 32) {
                        snprintf(scan_paths[n_scan], sizeof(scan_paths[0]), "%s", found_path);
                        n_scan++;
                    }
                    needed_offs[n_needed] = (int)link_sec_append(s, dynstr_sec,
                                                                 (const uint8_t *)resolved_soname, strlen(resolved_soname) + 1, 1);
                    n_needed++;
                }
                lp = end;
            } else {
                while (*lp && *lp != ' ') lp++;
            }
        }
        for (int di = 0; di < n_user_dirs; di++) free((void *)user_dirs[di]);
        if (lib_lookup_failed) {
            free(dyn_syms);
            free(dyn_idx);
            return -1;
        }

        // A .so given directly as a positional link input (not via
        // -l<name>) -- e.g. `rcc main.c libsqlite3.so -o prog` -- was
        // previously invisible to DT_NEEDED generation entirely: the -l
        // scan above only recognizes "-l"-prefixed tokens. Any function
        // this linker resolved through a PLT slot (any symbol undefined
        // here is always treated as dynamically importable, above) then
        // had no DT_NEEDED entry naming the library that actually
        // defines it, and ld.so aborted at startup with "undefined
        // symbol" the moment it tried to bind the (unfulfillable) PLT
        // relocation -- the link itself "succeeded" with a broken
        // binary instead of either working or falling back. Verify each
        // bare *.so token is really an ELF shared object, read its real
        // DT_SONAME (falling back to its basename, matching every
        // DT_NEEDED consumer's own lookup key when no soname was
        // recorded), and add a DT_NEEDED entry for it -- the exact same
        // entry a real `ld prog.o /path/to/libsqlite3.so` would produce.
        const char *sp = s->libs;
        while (sp && *sp) {
            while (*sp == ' ') sp++;
            if (!*sp) break;
            const char *send = sp;
            while (*send && *send != ' ') send++;
            size_t slen = (size_t)(send - sp);
            if (sp[0] != '-' && slen > 3 && slen < 600 &&
                strncmp(send - 3, ".so", 3) == 0 && n_needed < 16) {
                char spath[600];
                memcpy(spath, sp, slen);
                spath[slen] = '\0';
                if (is_real_elf_so(spath)) {
                    char resolved_soname[64];
                    if (!read_soname(spath, resolved_soname, sizeof(resolved_soname))) {
                        const char *base = strrchr(spath, '/');
                        base = base ? base + 1 : spath;
                        snprintf(resolved_soname, sizeof(resolved_soname), "%s", base);
                    }
                    if (n_scan < 32) {
                        snprintf(scan_paths[n_scan], sizeof(scan_paths[0]), "%s", spath);
                        n_scan++;
                    }
                    needed_offs[n_needed] = (int)link_sec_append(s, dynstr_sec,
                                                                 (const uint8_t *)resolved_soname, strlen(resolved_soname) + 1, 1);
                    n_needed++;
                }
            }
            sp = send;
        }

        // Genuine-undefined-symbol gate (dynamic executables only).
        //
        // Every non-weak symbol left undefined at this point was collected
        // above as a dynamic import and routed through a PLT/GOT slot,
        // deferred to the runtime loader -- so a program referencing a
        // function that lives in *no* linked library nonetheless "links"
        // and only dies at exec with "symbol lookup error". That turns an
        // autoconf-style feature probe (which decides a function exists iff
        // the test program links) into a false positive -- e.g. detecting
        // OpenBSD's pledge() on glibc. A real `ld` rejects such a symbol at
        // link time instead.
        //
        // Prove absence rather than assume it: scan the .dynsym of every
        // DT_NEEDED library this link actually resolved (libc / libm /
        // libgcc_s plus each -l and bare-*.so input). A symbol defined by
        // none of the libraries we could *read* is a genuine unresolved
        // reference; hand the whole link to the system compiler (return -1),
        // which does the authoritative crt/-L/-l/static-libgcc search and
        // then either completes the link or emits the real, nonzero-exit
        // "undefined reference" diagnostic. Only libraries we successfully
        // open count as evidence, so an unreadable or unresolved library
        // never fabricates a false error -- that case still defers exactly
        // as before. Shared objects (-shared) are exempt: unresolved
        // imports are legal and bound when the .so is loaded into a process.
        if (!s->opt_shared && n_dyn > 0) {
            const char **names = malloc((size_t)n_dyn * sizeof(char *));
            bool *found = calloc((size_t)n_dyn, sizeof(bool));
            if (names && found) {
                for (int k = 0; k < n_dyn; k++)
                    names[k] = s->syms[dyn_syms[k]].name;
                bool scanned_any = false;
                // The three libraries this linker adds to DT_NEEDED
                // unconditionally (see above); resolve their real paths.
                const char *core[3] = {"libc.so.6", "libgcc_s.so.1", "libm.so.6"};
                for (int c = 0; c < 3; c++) {
                    char cpath[600];
                    if (find_shared_lib(core[c], cpath, sizeof(cpath), NULL, 0) == 0)
                        scanned_any |= so_mark_defined(cpath, names, found, n_dyn);
                }
                for (int p = 0; p < n_scan; p++)
                    scanned_any |= so_mark_defined(scan_paths[p], names, found, n_dyn);
                int missing = -1;
                if (scanned_any) {
                    for (int k = 0; k < n_dyn; k++)
                        if (!found[k]) {
                            missing = k;
                            break;
                        }
                }
                if (missing >= 0) {
                    if (getenv("RCC_LINK_DEBUG"))
                        fprintf(stderr,
                                "rcc: LINK_DEBUG undefined reference to '%s': "
                                "provided by no linked library; deferring to system linker\n",
                                names[missing]);
                    free(names);
                    free(found);
                    free(dyn_syms);
                    free(dyn_idx);
                    return -1;
                }
            }
            free(names);
            free(found);
        }

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

        // Exported (locally-defined, globally-visible) symbols: appended
        // after the imports at dynsym index n_dyn+1..n_dyn+n_exp. Real
        // section index doesn't matter for runtime resolution (this
        // linker never emits a section header table at all, matching its
        // existing ET_EXEC output; ld.so/dlsym() only look at st_value +
        // the library's own load bias) -- 1 just needs to be anything
        // other than SHN_UNDEF/SHN_ABS/SHN_COMMON so ld.so treats the
        // symbol as defined here. st_value is only known after layout;
        // patched in place below once addresses are final.
        for (int k = 0; k < n_exp; k++) {
            LinkSym *sym = &s->syms[exp_syms[k]];
            int off = (int)link_sec_append(s, dynstr_sec, (const uint8_t *)sym->name,
                                           strlen(sym->name) + 1, 1);
            uint8_t ent[24];
            w32le_m(ent, (uint32_t)off);
            ent[4] = (uint8_t)(((sym->bind == 2 ? STB_WEAK : STB_GLOBAL) << 4) |
                               (sym->type == 2 ? STT_FUNC : (sym->type == 1 ? STT_OBJECT : STT_NOTYPE)));
            ent[5] = STV_DEFAULT;
            w16le_m(ent + 6, 1);
            w64le_m(ent + 8, 0); // st_value: patched after layout
            w64le_m(ent + 16, sym->size);
            link_sec_append(s, dynsym_sec, ent, 24, 8);
        }

        // Symbol versioning: pin each imported dynamic symbol to the same
        // default (non-hidden) glibc version a normal ld-linked binary
        // would bind to. Without this, unversioned lookups can silently
        // resolve to an old ABI-incompatible compat symbol (e.g.
        // pthread_cond_init@GLIBC_2.2.5 instead of @GLIBC_2.3.2) --
        // functions succeed individually but disagree on internal layout.
        char **dyn_versions = calloc((size_t)n_dyn, sizeof(char *));
        if (n_dyn > 0) {
            const char **names = malloc((size_t)n_dyn * sizeof(char *));
            for (int k = 0; k < n_dyn; k++)
                names[k] = s->syms[dyn_syms[k]].name;
            char libc_path[512];
            if (find_shared_lib("libc.so.6", libc_path, sizeof(libc_path), NULL, 0) == 0)
                lookup_lib_symbol_versions(libc_path, names, dyn_versions, n_dyn);
            free(names);
        }
        // Assign a VERSYM index (>= 2) to each distinct version name seen,
        // in first-seen order, and record them for the Verneed/Vernaux
        // section. dyn_verndx[k] mirrors dyn_versions[k] as an index.
        char *verneed_names[64];
        int *dyn_verndx = calloc((size_t)n_dyn, sizeof(int));
        for (int k = 0; k < n_dyn; k++) {
            if (!dyn_versions[k]) continue;
            int found = -1;
            for (int v = 0; v < n_verneed_versions; v++)
                if (strcmp(verneed_names[v], dyn_versions[k]) == 0) {
                    found = v;
                    break;
                }
            if (found < 0 && n_verneed_versions < 64) {
                found = n_verneed_versions++;
                verneed_names[found] = dyn_versions[k];
            }
            dyn_verndx[k] = found >= 0 ? found + 2 : 0;
        }
        if (n_verneed_versions > 0) {
            // .gnu.version: parallel to .dynsym, null entry first.
            uint8_t vs0[2] = {0, 0};
            link_sec_append(s, versym_sec, vs0, 2, 2);
            for (int k = 0; k < n_dyn; k++) {
                uint16_t vidx = dyn_verndx[k] > 0 ? (uint16_t)dyn_verndx[k] : VER_NDX_GLOBAL;
                uint8_t vb[2];
                w16le_m(vb, vidx);
                link_sec_append(s, versym_sec, vb, 2, 2);
            }
            // .gnu.version is indexed 1:1 with .dynsym -- it must cover
            // every entry there, imports AND (for -shared) the appended
            // exports, or a reader walking past the imports' n_dyn count
            // finds garbage past the array's real end (observed as
            // "<corrupt>" version info on exported symbols via
            // objdump/readelf, and ld.so itself either mis-binding or
            // crashing while resolving them). VER_NDX_GLOBAL (1) is the
            // standard "defined here, no specific version" index.
            for (int k = 0; k < n_exp; k++) {
                uint8_t vb[2];
                w16le_m(vb, VER_NDX_GLOBAL);
                link_sec_append(s, versym_sec, vb, 2, 2);
            }
            // .gnu.version_r: one Verneed record for libc.so.6 with one
            // Vernaux entry per distinct required version.
            uint8_t vnbuf[16];
            w16le_m(vnbuf, 1); // vn_version
            w16le_m(vnbuf + 2, (uint16_t)n_verneed_versions); // vn_cnt
            w32le_m(vnbuf + 4, (uint32_t)libc_off); // vn_file
            w32le_m(vnbuf + 8, 16); // vn_aux: first Vernaux right after this record
            w32le_m(vnbuf + 12, 0); // vn_next: only one Verneed record
            link_sec_append(s, verneed_sec, vnbuf, 16, 4);
            for (int v = 0; v < n_verneed_versions; v++) {
                int name_off = (int)link_sec_append(s, dynstr_sec,
                                                    (const uint8_t *)verneed_names[v], strlen(verneed_names[v]) + 1, 1);
                uint8_t vabuf[16];
                w32le_m(vabuf, elf_hash(verneed_names[v])); // vna_hash
                w16le_m(vabuf + 4, 0); // vna_flags
                w16le_m(vabuf + 6, (uint16_t)(v + 2)); // vna_other (VERSYM index)
                w32le_m(vabuf + 8, (uint32_t)name_off); // vna_name
                w32le_m(vabuf + 12, v + 1 < n_verneed_versions ? 16 : 0); // vna_next
                link_sec_append(s, verneed_sec, vabuf, 16, 4);
            }
        }
        for (int k = 0; k < n_dyn; k++) free(dyn_versions[k]);
        free(dyn_versions);
        free(dyn_verndx);

        // .hash: covers both imported (indices 1..n_dyn) and, for
        // -shared, exported (indices n_dyn+1..n_dyn+n_exp) symbols --
        // anything a consumer might look up by name via ld.so/dlsym().
        uint32_t nchain = (uint32_t)(n_dyn + n_exp + 1);
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
        for (int k = 0; k < n_exp; k++) {
            uint32_t idx = (uint32_t)(n_dyn + 1 + k);
            const char *name = s->syms[exp_syms[k]].name;
            uint32_t h = elf_hash(name) % nbucket;
            chain[idx] = buckets[h];
            buckets[h] = idx;
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
                        r->type == RL_GOTPCREL || r->type == RL_ARM64_B26 ||
                        r->type == RL_ARM64_GOT_PG) {
                        // Only actual functions get PLT entries.
                        if (s->syms[si].type == STT_FUNC || r->type == RL_PC32_PLT ||
                            r->type == RL_ARM64_B26)
                            dyn_kind[si] = 1; // function reference
                    } else if (r->type == RL_ABS64 || r->type == RL_ABS32 ||
                               r->type == RL_ABS32U) {
                        // A symbol commonly has BOTH kinds of reference --
                        // e.g. a libc function called directly somewhere
                        // (PC32/PLT) and ALSO stored in a function-pointer
                        // table elsewhere (ABS64/32/32U), as sqlite3's VFS
                        // method tables do throughout. Since these two
                        // loops run over every relocation in link order
                        // with no guaranteed ordering between a symbol's
                        // call sites and its address-taken sites, an
                        // ABS64/32/32U hit seen *after* a PC32/PLT hit
                        // must not downgrade dyn_kind back to "data
                        // reference" -- that silently drops the PLT slot
                        // apply_dynamic_relocs() requires for the call
                        // site, which then fails link_elf() outright
                        // (return -1, forcing a GCC fallback that doesn't
                        // even reach this problem). The reverse direction
                        // needs no such guard: the PC32/PLT branch above
                        // always sets dyn_kind[si]=1 unconditionally, so a
                        // function reference seen after a data reference
                        // still correctly wins.
                        if (dyn_kind[si] != 1) dyn_kind[si] = 2; // data reference
                    }
                } else if ((r->type == RL_GOTPCREL || r->type == RL_ARM64_GOT_PG) &&
                           got_map[si] < 0) {
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
                if (!dyn_idx[si] && (r->type == RL_GOTPCREL || r->type == RL_ARM64_GOT_PG) &&
                    got_map[si] == 3 + n_dyn) {
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
        // ABS64/32/32U relocations against an *imported* (dynamic)
        // symbol -- e.g. `void (*fp)(void) = close;`, a function
        // pointer variable statically initialized from an external
        // symbol -- can't be resolved at link time either: emit a real
        // R_X86_64_64/R_AARCH64_ABS64 .rela.dyn entry naming the symbol,
        // which ld.so resolves and writes at load time (apply_dynamic_
        // relocs() above already left just the addend in place for
        // these). Unlike n_relative/n_extra_got below, this isn't
        // -shared-specific: a dynamically-linked *executable* doing the
        // same thing hits the identical "address not known until load
        // time" problem.
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            for (int j = 0; j < sec->n_relocs; j++) {
                LinkReloc *r = &sec->relocs[j];
                if (dyn_idx[r->sym] &&
                    (r->type == RL_ABS64 || r->type == RL_ABS32 || r->type == RL_ABS32U))
                    n_absdyn++;
            }
        }
        // -shared additionally needs one R_*_RELATIVE .rela.dyn entry per
        // ABS64/32/32U relocation against a *locally* defined symbol:
        // codegen bakes in a link-time address assuming base 0 (this
        // link uses base=0 for -shared, see below), but the real runtime
        // address is base 0 + whatever load bias ld.so/dlopen() picks --
        // ld.so applies that bias for us at load time via this reloc,
        // exactly the way it does for PIE executables. PC-relative/
        // GOT-relative code references need no such fixup: they're
        // position-independent by construction.
        if (s->opt_shared) {
            for (int i = 0; i < s->n_secs; i++) {
                LinkSec *sec = &s->secs[i];
                for (int j = 0; j < sec->n_relocs; j++) {
                    LinkReloc *r = &sec->relocs[j];
                    if (!dyn_idx[r->sym] &&
                        (r->type == RL_ABS64 || r->type == RL_ABS32 || r->type == RL_ABS32U))
                        n_relative++;
                }
            }
        }
        // Extra GOT slots (n_extra_got, above) hold a base-0 absolute
        // address for a locally-resolved GOT-relative reference too --
        // same fixup requirement as n_relative, just via a different
        // write path (got->data directly, not sec->relocs[]).
        n_reladyn = n_dyn - n_func_dyn + n_relative + n_absdyn + (s->opt_shared ? n_extra_got : 0);

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
        int n_dynent = 5 + (n_reladyn > 0 ? 3 : 0) + (n_func_dyn > 0 ? 3 : 0) + n_needed + 3 + 4 + 3 + (s->opt_shared ? 1 : 0);
        uint8_t *dyn_placeholder = calloc((size_t)n_dynent * 16, 1);
        link_sec_append(s, dynamic_sec, dyn_placeholder, (size_t)n_dynent * 16, 8);
        free(dyn_placeholder);
    }

    // Layout.  Use page-aligned region boundaries so each PT_LOAD segment
    // starts at a valid file offset / virtual address pair.
    // -shared uses base 0: a real shared object's own p_vaddr values are
    // offsets from whatever load address ld.so/dlopen() actually picks
    // (there is no fixed "preferred" address the way an ET_EXEC has),
    // and R_*_RELATIVE relocs (see n_relative above) are computed
    // assuming exactly this base -- ld.so adds its real load bias to
    // them at load time.
    uint64_t base = 0x400000ULL;
    if (s->opt_shared) base = 0;
    else if (s->opt_pie)
        base = 0x10000ULL;
    // Fix BSS section length from symbol sizes.
    {
        int bss_sec = link_find_or_create_sec(s, ".bss", true, true, false, true, false, 8);
        uint64_t max_bss = 0;
        for (int i = 0; i < s->n_syms; i++) {
            LinkSym *sym = &s->syms[i];
            if (sym->sec == bss_sec) {
                uint64_t end = sym->value + sym->size;
                if (end > max_bss) max_bss = end;
                if (getenv("RCC_LINK_DEBUG") && strstr(sym->name, "glbflags"))
                    fprintf(stderr, "DBG bss sym %s sec=%d value=%llu size=%llu\n", sym->name, sym->sec,
                            (unsigned long long)sym->value, (unsigned long long)sym->size);
            }
        }
        if (getenv("RCC_LINK_DEBUG"))
            fprintf(stderr, "DBG bss_sec=%d len=%zu max_bss=%llu\n", bss_sec, s->secs[bss_sec].len,
                    (unsigned long long)max_bss);
        if (max_bss > s->secs[bss_sec].len)
            s->secs[bss_sec].len = (size_t)max_bss;
    }

    if (link_layout(s, base, 0x1000) != 0) {
        free(dyn_syms);
        free(dyn_idx);
        free(dyn_kind);
        free(plt_idx);
        free(got_map);
        free(dynstr_off);
        return -1;
    }


    // Compute page-aligned header size for address shifting.
    {
        int phnum = 3;
        if (do_dynamic) {
            phnum += 1; // PT_DYNAMIC
            if (!s->opt_shared) phnum += 1; // PT_INTERP
        }
        for (int i = 0; i < s->n_secs; i++)
            if (s->secs[i].is_tls) {
                phnum++;
                break;
            }
        uint64_t file_off = align_up(64 + (uint64_t)phnum * 56, 0x1000);
        // Shift vaddrs and fileoffs by page-aligned amount.
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            if (!sec->alloc) continue;
            sec->addr += file_off;
            if (!sec->is_bss) sec->fileoff += file_off;
        }
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

        // Patch each exported symbol's dynsym entry with its real,
        // final address (st_value at byte offset 8 within its 24-byte
        // nlist entry, appended at dynsym index n_dyn+1..n_dyn+n_exp
        // when .dynsym was built, before layout knew any addresses).
        LinkSec *dynsym = &s->secs[dynsym_sec];
        for (int k = 0; k < n_exp; k++) {
            uint8_t *ent = dynsym->data + (size_t)(1 + n_dyn + k) * 24;
            w64le_m(ent + 8, symbol_address(s, exp_syms[k]));
        }

        // PLT entries.
        if (s->arch == ARCH_AARCH64) {
            // PLT0 (functionally dead: DF_BIND_NOW means ld.so resolves all
            // GOT.PLT slots eagerly before _start runs, so this is never
            // actually executed -- but keep a plausible skeleton).
            //   stp x16, x30, [sp, #-16]!
            //   adrp x16, GOT+16
            //   ldr x17, [x16, #:lo12:GOT+16]
            //   br x17
            w32le_m(plt->data + 0, 0xa9bf7bf0u);
            uint64_t got2_addr = got_addr + 16;
            uint64_t plt0_pc = plt_addr;
            int64_t d0 = (int64_t)(got2_addr - (plt0_pc & ~(uint64_t)0xfff));
            int64_t imm0 = d0 >> 12;
            w32le_m(plt->data + 4, 0x90000010u | ((uint32_t)(imm0 & 3) << 29) | ((uint32_t)((imm0 >> 2) & 0x7ffff) << 5));
            w32le_m(plt->data + 8, 0xf9400211u | (((uint32_t)(got2_addr & 0xfff) >> 3) << 10));
            w32le_m(plt->data + 12, 0xd61f0220u);
        } else {
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
        }

        int relaplt_pos = 0;
        int reladyn_pos = 0;
        for (int k = 0; k < n_dyn; k++) {
            int si = dyn_syms[k];
            int dynsym_index = k + 1;
            int got_slot = got_map[si];
            uint64_t slot_addr = got_addr + (uint64_t)got_slot * 8;
            if (plt_idx[si] >= 0) {
                int fidx = plt_idx[si];
                uint64_t entry_addr = plt_addr + 16 + (uint64_t)fidx * 16;
                uint8_t *p = plt->data + 16 + (size_t)fidx * 16;
                if (s->arch == ARCH_AARCH64) {
                    // adrp x16, GOT[slot]; ldr x17, [x16, #lo12]; add x16, x16, #lo12; br x17
                    int64_t d = (int64_t)(slot_addr - (entry_addr & ~(uint64_t)0xfff));
                    int64_t imm = d >> 12;
                    w32le_m(p + 0, 0x90000010u | ((uint32_t)(imm & 3) << 29) | ((uint32_t)((imm >> 2) & 0x7ffff) << 5));
                    uint32_t lo12 = (uint32_t)(slot_addr & 0xfff);
                    w32le_m(p + 4, 0xf9400211u | ((lo12 >> 3) << 10));
                    w32le_m(p + 8, 0x91000210u | (lo12 << 10));
                    w32le_m(p + 12, 0xd61f0220u);
                } else {
                    int32_t disp_got = (int32_t)(slot_addr - (entry_addr + 6));
                    int32_t disp_plt0 = (int32_t)(plt_addr - (entry_addr + 11));
                    p[0] = 0xff;
                    p[1] = 0x25;
                    w32le_m(p + 2, (uint32_t)disp_got);
                    p[6] = 0x68;
                    w32le_m(p + 7, (uint32_t)fidx);
                    p[11] = 0xe9;
                    w32le_m(p + 12, (uint32_t)disp_plt0);
                }

                uint8_t *rp = relaplt->data + (size_t)relaplt_pos * 24;
                w64le_m(rp, slot_addr);
                uint32_t jump_slot = s->arch == ARCH_AARCH64 ? R_AARCH64_JUMP_SLOT : R_X86_64_JUMP_SLOT;
                w64le_m(rp + 8, ((uint64_t)dynsym_index << 32) | jump_slot);
                w64le_m(rp + 16, 0);
                relaplt_pos++;
            } else {
                // GLOB_DAT relocation for data-like dynamic symbols.
                uint8_t *rp = reladyn->data + (size_t)reladyn_pos * 24;
                w64le_m(rp, slot_addr);
                uint32_t glob_dat = s->arch == ARCH_AARCH64 ? R_AARCH64_GLOB_DAT : R_X86_64_GLOB_DAT;
                w64le_m(rp + 8, ((uint64_t)dynsym_index << 32) | glob_dat);
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

        // R_X86_64_64/R_AARCH64_ABS64 entries for ABS64/32/32U relocs
        // against *imported* symbols (n_absdyn, counted above) -- not
        // -shared-specific, applies to any do_dynamic link. Unlike
        // RELATIVE entries this carries a real symbol index: ld.so
        // resolves the named symbol and adds it to r_addend itself.
        {
            uint32_t abs_type = s->arch == ARCH_AARCH64 ? R_AARCH64_ABS64 : R_X86_64_64;
            for (int i = 0; i < s->n_secs; i++) {
                LinkSec *sec = &s->secs[i];
                for (int j = 0; j < sec->n_relocs; j++) {
                    LinkReloc *r = &sec->relocs[j];
                    if (!dyn_idx[r->sym] ||
                        (r->type != RL_ABS64 && r->type != RL_ABS32 && r->type != RL_ABS32U))
                        continue;
                    uint8_t *rp = reladyn->data + (size_t)reladyn_pos * 24;
                    w64le_m(rp, sec->addr + r->offset);
                    w64le_m(rp + 8, ((uint64_t)dyn_idx[r->sym] << 32) | abs_type);
                    w64le_m(rp + 16, (uint64_t)r->addend);
                    reladyn_pos++;
                }
            }
        }

        // R_*_RELATIVE entries for -shared: one per ABS64/32/32U reloc
        // against a locally-defined symbol (n_relative, counted above).
        // apply_dynamic_relocs() already wrote the base-0 link-time value
        // into the data at r->offset (symbol_address() returns an
        // offset from base=0 for -shared, see above) -- read it back as
        // the addend ld.so will add its real load bias to at load time.
        if (s->opt_shared) {
            uint32_t relative = s->arch == ARCH_AARCH64 ? R_AARCH64_RELATIVE : R_X86_64_RELATIVE;
            for (int i = 0; i < s->n_secs; i++) {
                LinkSec *sec = &s->secs[i];
                for (int j = 0; j < sec->n_relocs; j++) {
                    LinkReloc *r = &sec->relocs[j];
                    if (dyn_idx[r->sym] ||
                        (r->type != RL_ABS64 && r->type != RL_ABS32 && r->type != RL_ABS32U))
                        continue;
                    uint8_t *p = sec->data + r->offset;
                    uint64_t addend = r->type == RL_ABS64 ? r64le(p)
                        : r->type == RL_ABS32             ? (uint64_t)(int64_t)(int32_t)r32le(p)
                                                          : (uint64_t)r32le(p);
                    uint8_t *rp = reladyn->data + (size_t)reladyn_pos * 24;
                    w64le_m(rp, sec->addr + r->offset);
                    w64le_m(rp + 8, relative);
                    w64le_m(rp + 16, addend);
                    reladyn_pos++;
                }
            }

            // Same fixup for the "extra" GOT slots filled above for
            // locally-resolved GOT-relative references (e.g. a global
            // variable accessed via GOT-indirect load, standard -fPIC
            // codegen even for symbols defined in this same object):
            // each slot holds symbol_address()'s base-0 value, which
            // ld.so must still bias by the real load address, or code
            // dereferencing the GOT slot's *content* (not the slot
            // itself) follows a pointer into whatever used to be mapped
            // at that offset in the pre-ASLR address space.
            for (int i = 0; i < s->n_syms; i++) {
                int slot = got_map[i];
                if (slot < 0 || dyn_idx[i]) continue;
                if (slot < 3 + n_dyn) continue; // reserved/dynamic slots
                uint8_t *rp = reladyn->data + (size_t)reladyn_pos * 24;
                w64le_m(rp, got_addr + (uint64_t)slot * 8);
                w64le_m(rp + 8, relative);
                w64le_m(rp + 16, symbol_address(s, i));
                reladyn_pos++;
            }
        }

        // Build .dynamic section after layout so addresses are known.
        // The .dynamic section was pre-allocated before layout; patch it now.
        size_t dpos = 0;
        LinkSec *dyn = &s->secs[dynamic_sec];
        auto_dyn_ent(dyn, &dpos, DT_HASH, s->secs[hash_sec].addr);
        auto_dyn_ent(dyn, &dpos, DT_STRTAB, s->secs[dynstr_sec].addr);
        auto_dyn_ent(dyn, &dpos, DT_SYMTAB, s->secs[dynsym_sec].addr);
        auto_dyn_ent(dyn, &dpos, DT_STRSZ, s->secs[dynstr_sec].len);
        auto_dyn_ent(dyn, &dpos, DT_SYMENT, 24);
        if (s->opt_shared)
            auto_dyn_ent(dyn, &dpos, DT_SONAME, (uint64_t)soname_off);
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
        // Emit DT_INIT_ARRAY / DT_FINI_ARRAY if sections exist.
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            if (!sec->alloc || sec->len == 0) continue;
            if (strcmp(sec->name, ".init_array") == 0) {
                auto_dyn_ent(dyn, &dpos, DT_INIT_ARRAY, sec->addr);
                auto_dyn_ent(dyn, &dpos, DT_INIT_ARRAYSZ, sec->len);
            }
            if (strcmp(sec->name, ".fini_array") == 0) {
                auto_dyn_ent(dyn, &dpos, DT_FINI_ARRAY, sec->addr);
                auto_dyn_ent(dyn, &dpos, DT_FINI_ARRAYSZ, sec->len);
            }
        }
        for (int k = 0; k < n_needed; k++)
            auto_dyn_ent(dyn, &dpos, DT_NEEDED, (uint64_t)needed_offs[k]);
        if (n_verneed_versions > 0) {
            auto_dyn_ent(dyn, &dpos, DT_VERSYM, s->secs[versym_sec].addr);
            auto_dyn_ent(dyn, &dpos, DT_VERNEED, s->secs[verneed_sec].addr);
            auto_dyn_ent(dyn, &dpos, DT_VERNEEDNUM, 1);
        }
        auto_dyn_ent(dyn, &dpos, DT_FLAGS, DF_BIND_NOW);
        auto_dyn_ent(dyn, &dpos, DT_FLAGS_1, DF_1_NOW);
        auto_dyn_ent(dyn, &dpos, DT_NULL, 0);
    } else {
        // Static link: apply relocations normally.
        link_apply_relocs(s, 0);
    }

    // Build program headers.
    uint64_t text_off = 0, text_addr = 0, text_filesz = 0, text_memsz = 0;
    uint64_t rodata_off = 0, rodata_addr = 0, rodata_filesz = 0, rodata_memsz = 0;
    uint64_t data_off = 0, data_addr = 0, data_filesz = 0, data_memsz = 0;
    bool have_text = false, have_rodata = false, have_data = false;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc) continue;
        // .tbss occupies no bytes in any PT_LOAD segment: its storage only
        // exists per-thread via PT_TLS's memsz. .tdata, however, is real
        // file-backed data that physically lives inside the RW LOAD
        // segment (PT_TLS just points at the same bytes as a template),
        // so it must be counted here or ld.so's static TLS copy reads
        // demand-zeroed (unmapped-from-file) memory instead of the image.
        if (sec->is_tls && sec->is_bss) continue;
        if (sec->exec) {
            if (!have_text) {
                have_text = true;
                text_off = 0;
                text_addr = base;
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
        phnum += 1; // PT_DYNAMIC
        if (!s->opt_shared) phnum += 1; // PT_INTERP
    }
    phnum++; // PT_GNU_STACK
    {
        bool htls = false;
        for (int i = 0; i < s->n_secs; i++)
            if (s->secs[i].alloc && s->secs[i].is_tls) htls = true;
        if (htls) phnum++;
    }

    uint64_t ehdr_size = 64;
    uint64_t phdr_size = phnum * 56;
    uint64_t file_off = align_up(ehdr_size + phdr_size, 0x1000);

    // Section fileoffs and vaddrs already shifted above.
    // First LOAD covers offset 0 with text_off=0, text_addr=base.

    // A shared object has no entry point of its own (dlopen()/DT_NEEDED
    // never jump into it directly -- only DT_INIT_ARRAY constructors run,
    // driven by ld.so, not e_entry); leave it 0, matching what a real
    // `ld -shared` output's ELF header carries.
    uint64_t entry_addr = s->opt_shared ? 0 : base;
    if (entry_sym >= 0) {
        entry_addr = s->secs[s->syms[entry_sym].sec].addr + s->syms[entry_sym].value;
    }

    // Build a real ELF section header table (SHT) + .shstrtab so the
    // output is consumable by BFD-based tools -- readelf, objdump, and
    // critically GNU ld itself when this file is later used as a link
    // input (e.g. `gcc prog.c this.so`). This linker's own consumer
    // (ld.so/dlopen) never needed one -- it walks PT_LOAD/PT_DYNAMIC
    // exclusively -- so it was omitted entirely until now; BFD's ELF
    // backend unconditionally rejects any object with e_shoff==0/
    // e_shnum==0 as unrecognized ("file in wrong format"), even though
    // the kernel/ld.so loader has no such requirement. Non-alloc'd
    // sections (a stray .comment/.note from a system crt*.o) are
    // deliberately left out: this linker's own layout pass never
    // assigns them a real fileoff/addr, so describing one here would
    // be describing bytes that were never actually written.
    int n_shdr_secs = 0;
    for (int i = 0; i < s->n_secs; i++)
        if (s->secs[i].alloc && sec_wants_shdr(&s->secs[i], n_verneed_versions)) n_shdr_secs++;
    uint16_t shnum = (uint16_t)(1 + n_shdr_secs + 1); // NULL + real secs + .shstrtab
    uint16_t shstrndx = (uint16_t)(shnum - 1);

    int *sec_to_shidx = malloc((size_t)s->n_secs * sizeof(int));
    for (int i = 0; i < s->n_secs; i++) sec_to_shidx[i] = 0;
    size_t shstrtab_cap = 64, shstrtab_len = 1;
    char *shstrtab_buf = malloc(shstrtab_cap);
    shstrtab_buf[0] = '\0'; // index 0: the NULL section's empty name
    uint32_t *shdr_name_off = malloc((size_t)(n_shdr_secs > 0 ? n_shdr_secs : 1) * sizeof(uint32_t));
    {
        int k = 0;
        for (int i = 0; i < s->n_secs; i++) {
            if (!s->secs[i].alloc || !sec_wants_shdr(&s->secs[i], n_verneed_versions)) continue;
            sec_to_shidx[i] = k + 1; // header index 0 is NULL
            const char *nm = s->secs[i].name;
            size_t nl = strlen(nm) + 1;
            if (shstrtab_len + nl > shstrtab_cap) {
                while (shstrtab_len + nl > shstrtab_cap) shstrtab_cap *= 2;
                {
                    char *tmp = realloc(shstrtab_buf, shstrtab_cap);
                    if (!tmp) {
                        free(shstrtab_buf);
                        fprintf(stderr, "rcc: out of memory\n");
                        exit(1);
                    }
                    shstrtab_buf = tmp;
                }
            }
            shdr_name_off[k] = (uint32_t)shstrtab_len;
            memcpy(shstrtab_buf + shstrtab_len, nm, nl);
            shstrtab_len += nl;
            k++;
        }
    }
    uint32_t shstrtab_self_name_off = (uint32_t)shstrtab_len;
    {
        const char *nm = ".shstrtab";
        size_t nl = strlen(nm) + 1;
        if (shstrtab_len + nl > shstrtab_cap) {
            while (shstrtab_len + nl > shstrtab_cap) shstrtab_cap *= 2;
            {
                char *tmp = realloc(shstrtab_buf, shstrtab_cap);
                if (!tmp) {
                    free(shstrtab_buf);
                    fprintf(stderr, "rcc: out of memory\n");
                    exit(1);
                }
                shstrtab_buf = tmp;
            }
        }
        memcpy(shstrtab_buf + shstrtab_len, nm, nl);
        shstrtab_len += nl;
    }

    // Where .shstrtab's bytes and the header array will physically land:
    // right after every other section's real file data -- the exact
    // same `written` position the data-write loop below independently
    // arrives at, computed here ahead of time since write_ehdr() (which
    // needs the final e_shoff) runs before that loop.
    uint64_t shstrtab_file_off = file_off;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->is_bss || sec->len == 0) continue;
        uint64_t end = sec->fileoff + sec->len;
        if (end > shstrtab_file_off) shstrtab_file_off = end;
    }
    uint64_t shoff = align_up(shstrtab_file_off + shstrtab_len, 8);

    FILE *f = fopen(s->out_path, "wb");
    if (!f) {
        fprintf(stderr, "rcc: link: cannot create %s: %s\n", s->out_path, strerror(errno));
        free(sec_to_shidx);
        free(shdr_name_off);
        free(shstrtab_buf);
        free(dyn_syms);
        free(dyn_idx);
        free(dyn_kind);
        free(plt_idx);
        free(got_map);
        free(dynstr_off);
        return -1;
    }

    uint16_t machine = (s->arch == ARCH_AARCH64) ? EM_AARCH64 : EM_X86_64;
    write_ehdr(f, s->opt_shared ? ET_DYN : ET_EXEC, machine, entry_addr, ehdr_size, (uint16_t)phnum,
               shoff, shnum, shstrndx);

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
    // PT_TLS: thread-local storage segment
    {
        uint64_t tls_off = 0, tls_addr = 0, tls_filesz = 0, tls_memsz = 0;
        bool have_tls = false;
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            if (!sec->alloc || !sec->is_tls) continue;
            if (!have_tls) {
                have_tls = true;
                tls_off = sec->fileoff;
                tls_addr = sec->addr;
            }
            if (sec->is_bss) {
                uint64_t end = sec->addr + sec->len - tls_addr;
                if (end > tls_memsz) tls_memsz = end;
            } else {
                uint64_t end = sec->fileoff + sec->len - tls_off;
                if (end > tls_filesz) tls_filesz = end;
                if (end > tls_memsz) tls_memsz = end;
            }
        }
        if (have_tls) {
            write_phdr(f, PT_TLS, PF_R, tls_off, tls_addr, tls_addr, tls_filesz, tls_memsz, 1);
            cur += 56;
        }
    }
    if (do_dynamic && !s->opt_shared) {
        LinkSec *interp = &s->secs[interp_sec];
        write_phdr(f, PT_INTERP, PF_R, interp->fileoff, interp->addr,
                   interp->addr, interp->len, interp->len, 1);
        cur += 56;
    }
    if (do_dynamic) {
        LinkSec *dyn = &s->secs[dynamic_sec];
        write_phdr(f, PT_DYNAMIC, PF_R, dyn->fileoff, dyn->addr,
                   dyn->addr, dyn->len, dyn->len, 8);
        cur += 56;
    }
    // glibc's ld.so refuses to dlopen()/load-as-a-dependency any shared
    // object whose PT_GNU_STACK requests an executable stack ("cannot
    // enable executable stack as shared object requires") -- a
    // hardening check that doesn't apply to ET_EXEC (the kernel's own
    // loader doesn't enforce it). PF_X here exists for GNU nested-
    // function trampolines, which -shared doesn't support emitting
    // executable-stack code into anyway; keep it only for executables.
    write_phdr(f, PT_GNU_STACK, PF_R | PF_W | (s->opt_shared ? 0 : PF_X), 0, 0, 0, 0, 0, 0x10);
    cur += 56;
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
        {
            WriteSec *tmp = realloc(wsecs, (size_t)(n_wsecs + 1) * sizeof(WriteSec));
            if (!tmp) {
                free(wsecs);
                fprintf(stderr, "rcc: out of memory\n");
                exit(1);
            }
            wsecs = tmp;
        }
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

    // .shstrtab bytes, then the section header array -- both physically
    // land exactly where shoff/e_shstrndx above assumed they would.
    if (written < shstrtab_file_off) wzeros(f, shstrtab_file_off - written);
    wbuf(f, shstrtab_buf, shstrtab_len);
    written = shstrtab_file_off + shstrtab_len;
    if (shoff > written) wzeros(f, shoff - written);

    // Section header 0: the mandatory all-zero NULL entry.
    {
        uint8_t z[64] = {0};
        wbuf(f, z, 64);
    }
    {
        int k = 0;
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            if (!sec->alloc || !sec_wants_shdr(sec, n_verneed_versions)) continue;
            uint32_t sh_type = sec->is_bss ? SHT_NOBITS : SHT_PROGBITS;
            uint32_t sh_link = 0, sh_info = 0;
            uint64_t sh_entsize = 0;
            // This linker only ever creates a fixed, known set of
            // dynamic-linking section names (see the do_dynamic block
            // above); anything else -- .text/.data/.rodata/.bss/
            // .init_array/user .o sections passed through by
            // map_input_sec_to_output() -- is plain SHT_PROGBITS/
            // NOBITS, already the default set above.
            if (strcmp(sec->name, ".dynsym") == 0) {
                sh_type = SHT_DYNSYM;
                sh_link = (uint32_t)sec_to_shidx[dynstr_sec];
                sh_info = 1; // first non-local symbol: every entry we emit is global/weak
                sh_entsize = 24;
            } else if (strcmp(sec->name, ".dynstr") == 0) {
                sh_type = SHT_STRTAB;
            } else if (strcmp(sec->name, ".hash") == 0) {
                sh_type = SHT_HASH;
                sh_link = (uint32_t)sec_to_shidx[dynsym_sec];
                sh_entsize = 4;
            } else if (strcmp(sec->name, ".dynamic") == 0) {
                sh_type = SHT_DYNAMIC;
                sh_link = (uint32_t)sec_to_shidx[dynstr_sec];
                sh_entsize = 16;
            } else if (strcmp(sec->name, ".rela.plt") == 0 ||
                       strcmp(sec->name, ".rela.dyn") == 0) {
                sh_type = SHT_RELA;
                sh_link = (uint32_t)sec_to_shidx[dynsym_sec];
                sh_entsize = 24;
            } else if (strcmp(sec->name, ".gnu.version") == 0) {
                sh_type = SHT_GNU_versym;
                sh_link = (uint32_t)sec_to_shidx[dynsym_sec];
                sh_entsize = 2;
            } else if (strcmp(sec->name, ".gnu.version_r") == 0) {
                sh_type = SHT_GNU_verneed;
                sh_link = (uint32_t)sec_to_shidx[dynstr_sec];
                sh_info = n_verneed_versions > 0 ? 1 : 0; // one Verneed record (libc.so.6)
            } else if (strcmp(sec->name, ".init_array") == 0) {
                sh_type = SHT_INIT_ARRAY;
            }
            uint32_t sh_flags = SHF_ALLOC;
            if (sec->write) sh_flags |= SHF_WRITE;
            if (sec->exec) sh_flags |= SHF_EXECINSTR;
            if (sec->is_tls) sh_flags |= SHF_TLS;

            uint8_t sh[64];
            memset(sh, 0, sizeof(sh));
            w32le_m(sh + 0, shdr_name_off[k]);
            w32le_m(sh + 4, sh_type);
            w64le_m(sh + 8, sh_flags);
            w64le_m(sh + 16, sec->addr);
            w64le_m(sh + 24, sec->fileoff);
            w64le_m(sh + 32, sec->len);
            w32le_m(sh + 40, sh_link);
            w32le_m(sh + 44, sh_info);
            w64le_m(sh + 48, (uint64_t)(sec->align ? sec->align : 1));
            w64le_m(sh + 56, sh_entsize);
            wbuf(f, sh, 64);
            k++;
        }
    }
    // .shstrtab's own section header, last (index shstrndx).
    {
        uint8_t sh[64];
        memset(sh, 0, sizeof(sh));
        w32le_m(sh + 0, shstrtab_self_name_off);
        w32le_m(sh + 4, SHT_STRTAB);
        w64le_m(sh + 24, shstrtab_file_off);
        w64le_m(sh + 32, shstrtab_len);
        w64le_m(sh + 48, 1);
        wbuf(f, sh, 64);
    }
    free(sec_to_shidx);
    free(shdr_name_off);
    free(shstrtab_buf);

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
