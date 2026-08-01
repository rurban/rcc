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
            s->secs[out_idx].len += virtual_size;
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

        uint64_t sym_value = (out_sec >= 0) ? value : 0;
        int sym_idx = link_add_sym(s, sym_name, out_sec, sym_value,
                                   0, bind, type, -1);
        sym_map[i] = sym_idx;
        i += 1 + num_aux;
    }

    // Re-map relocations: COFF sym index → LinkSym index
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        for (int j = 0; j < sec->n_relocs; j++) {
            int cs = sec->relocs[j].sym;
            if (cs >= 0 && cs < (int)n_syms)
                sec->relocs[j].sym = sym_map[cs];
        }
    }

    // Track object for cleanup
    LinkObj obj = {.path = strdup(path), .image = image, .image_size = (size_t)st.st_size};
    if (s->n_objs == s->cap_objs) {
        s->cap_objs = s->cap_objs ? s->cap_objs * 2 : 4;
        s->objs = realloc(s->objs, (size_t)s->cap_objs * sizeof(LinkObj));
    }
    s->objs[s->n_objs++] = obj;
    free(out_sec_map);
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
                            uint64_t *import_dir_size_out) {
    *idata_sec_out = -1;
    *patches_out = NULL;
    *n_patches_out = 0;
    *iat_off_out = 0;
    *iat_size_out = 0;
    *import_dir_size_out = 0;
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

    free(names);
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
        if (!sec->alloc) continue;
        addr = pe_align_up(addr, PE_SECTION_ALIGN);
        sec->addr = addr;
        addr += sec->len ? sec->len : 1; // reserve space even for empty sections
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
    // Static libgcc/libmingwex linking and DLL (.dll/-shared) output are not
    // implemented -- only a dynamically-linked .exe against system DLLs via
    // the synthesized CRT stub.  Fall back to the mingw toolchain for both.
    if (s->opt_static || s->opt_shared) return -1;
    // Create standard sections
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 16);
    link_find_or_create_sec(s, ".rdata", true, false, false, false, false, 16);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 16);

    // Find entry point
    int entry_sym = link_find_sym(s, "mainCRTStartup");
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
        // mainCRTStartup to call. Synthesize a minimal replacement: align
        // the stack per the Win64 ABI (process entry alignment is
        // unspecified), call main(), and hand its result to ExitProcess.
        // This runs and exits correctly but skips what a real CRT would
        // also do: real argv/environ parsing, atexit handlers, and C++
        // static initializers. main() is called with argc=0, argv=NULL
        // (safe zeroed defaults) rather than whatever garbage happened to
        // be in the entry registers.
        int text_sec = link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
        int exitprocess_sym = link_add_sym(s, "ExitProcess", -1, 0, 0, 1 /* global */, 2 /* func */, -1);
        uint8_t stub[] = {
            0x48,
            0x83,
            0xE4,
            0xF0, // and $-16, %rsp
            0x48,
            0x83,
            0xEC,
            0x20, // sub $0x20, %rsp
            0x31,
            0xC9, // xor %ecx, %ecx   (argc = 0)
            0x31,
            0xD2, // xor %edx, %edx   (argv = NULL)
            0xE8,
            0,
            0,
            0,
            0, // call main
            0x89,
            0xC1, // mov %eax, %ecx
            0xE8,
            0,
            0,
            0,
            0, // call ExitProcess
            0x90, // nop (unreachable)
        };
        uint64_t stub_off = link_sec_append(s, text_sec, stub, sizeof(stub), 16);
        link_add_reloc(s, text_sec, stub_off + 13, RL_PC32, main_sym, -4);
        link_add_reloc(s, text_sec, stub_off + 20, RL_PC32, exitprocess_sym, -4);
        entry_sym = link_add_sym(s, "_rcc_pe_start", text_sec, stub_off, sizeof(stub),
                                 1 /* global */, 2 /* func */, -1);
    }

    // Resolve external symbols against system DLLs and synthesize the
    // import table + trampolines before layout, since this may add new
    // sections/symbols that layout and relocation application must see.
    int idata_sec = -1, n_idata_patches = 0;
    PePatch *idata_patches = NULL;
    uint64_t iat_off = 0, iat_size = 0, import_dir_size = 0;
    if (build_pe_imports(s, &idata_sec, &idata_patches, &n_idata_patches,
                         &iat_off, &iat_size, &import_dir_size) != 0) {
        // A strong symbol we can't resolve against any known DLL (or an
        // exotic construct like TLS/weak imports we don't model yet).
        // Fall back to the mingw toolchain's own linker.
        return -1;
    }

    // Layout sections: one page per section (see pe_layout_sections), with
    // the first page reserved for the PE/COFF headers so .text starts at
    // RVA 0x1000, not RVA 0 (which would overlap the header region every
    // PE loader always maps there).
    uint64_t base = 0x140000000ULL;
    pe_layout_sections(s, base + PE_SECTION_ALIGN);

    // Fix up the import table's internal RVAs now that .idata has its
    // final address, then apply ordinary relocations (this also resolves
    // the trampolines' RIP-relative jumps into the IAT).
    pe_patch_idata(s, idata_sec, base, idata_patches, n_idata_patches);
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
    pe_w16le(f, 0x0022); // EXECUTABLE | LARGE_ADDRESS_AWARE

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
    pe_w32le(f, (uint32_t)(entry_addr - base)); // RVA of entry
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

    // Data directories (16 entries). Index 1 = Import Table, index 3 =
    // Exception Table (.pdata, if our own COFF codegen emitted Win64 SEH
    // unwind info for this object), index 5 = Base Relocation Table
    // (.reloc), index 12 = IAT (per IMAGE_DIRECTORY_ENTRY_IMPORT /
    // _EXCEPTION / _BASERELOC / _IAT).
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
    return 0;
}
