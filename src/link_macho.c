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

#define CPU_TYPE_ARM64    0x0100000C
#define CPU_TYPE_X86_64   0x01000007
#define CPU_SUBTYPE_ALL   0x00000003
#define CPU_SUBTYPE_ARM64_ALL 0x00000000

// Load commands
#define LC_SEGMENT_64     0x19
#define LC_SYMTAB         0x02
#define LC_DYSYMTAB       0x0B
#define LC_LOAD_DYLIB     0x0C
#define LC_MAIN           0x80000028
#define LC_BUILD_VERSION  0x32
#define LC_DYLD_INFO_ONLY 0x80000022

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

                // Map section name to output section
                char out_name[32];
                if (is_text) snprintf(out_name, sizeof(out_name), ".text");
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

                if (!is_bss && size > 0 && offset > 0) {
                    uint64_t base_off = link_sec_append(s, out_idx,
                                                        image + offset, (size_t)size, 16);
                    // Parse relocations
                    for (uint32_t k = 0; k < nreloc; k++) {
                        // Mach-O relocation entry: 8 bytes (int32 addr, uint32 sym+type)
                        const uint8_t *rp = image + reloff + k * 8;
                        int32_t r_addr = (int32_t)mo_r32(rp);
                        uint32_t r_symtype = mo_r32(rp + 4);
                        int r_sym = (int)(r_symtype & 0xFFFFFF);
                        uint32_t r_type = (r_symtype >> 24) & 0xFF;
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
                    s->secs[out_idx].len += (size_t)size;
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

                uint64_t sym_value = (out_sec >= 0) ? n_value : 0;
                int sym_idx = link_add_sym(s, name, out_sec, sym_value,
                                           0, bind, type, -1);
                sym_map[j] = sym_idx;
            }
        }
        lc += cmdsize;
    }

    // Re-map relocations from object symbol index to LinkSym index
    if (sym_map) {
        for (int i = 0; i < s->n_secs; i++) {
            LinkSec *sec = &s->secs[i];
            for (int j = 0; j < sec->n_relocs; j++) {
                int ms = sec->relocs[j].sym;
                if (ms >= 0 && ms < n_syms)
                    sec->relocs[j].sym = sym_map[ms];
            }
        }
        free(sym_map);
    }
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
    // Static linking and dylib (.dylib/-shared) output are not implemented --
    // only a dynamically-linked Mach-O executable (MH_EXECUTE) against
    // system dylibs.  Fall back to the external linker for both.
    if (s->opt_static || s->opt_shared) return -1;
    // KNOWN LIMITATION: no GOT table or PLT/lazy-binding stubs are built.
    // RL_ARM64_GOT_PG/RL_ARM64_GOT_LO and RL_GOTPCREL relocations against
    // *external* symbols (link_reloc_apply's "no real GOT at this level"
    // fallback in link.c) resolve as if the ADRP+LDR/ADRP+ADD sequence the
    // codegen emitted for GOT-relative addressing were instead a direct,
    // link-time-known address -- correct only when the symbol is DEFINED
    // in this same link (e.g. a local .rodata reference), and silently
    // wrong for anything genuinely external (e.g. libSystem's printf),
    // whose real address isn't known until dyld resolves it at load time.
    // A real fix needs the same machinery link_elf.c's .plt/.got.plt and
    // link_pe.c's IAT trampolines already have for their platforms: a
    // __DATA,__got section of real pointer slots plus, for calls,
    // PLT-style stub bodies dyld's lazy/non-lazy bind opcodes (currently
    // hardcoded empty in LC_DYLD_INFO_ONLY below) can bind into. Until
    // then this linker only actually works for programs whose every
    // external reference resolves through relocations we do apply
    // correctly (RL_ARM64_B26 direct branches, RL_ABS64/32 absolute
    // pointers) -- anything needing a real GOT/PLT slot silently emits a
    // corrupted call/load.
    // Create standard sections
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 8);
    link_find_or_create_sec(s, ".rodata", true, false, false, false, false, 8);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 8);
    // .init_array for ctors
    link_find_or_create_sec(s, ".init_array", true, true, false, false, false, 8);

    uint64_t base = 0x100000000ULL; // default ARM64 base

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
        if (sec->exec) {
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
        } else {
            mo_secs[n_mo].segname = "__DATA";
            mo_secs[n_mo].sectname = "__data";
        }
        mo_secs[n_mo].sec = sec;
        n_mo++;
    }

    uint64_t text_vmaddr = base + 0x4000; // after __PAGEZERO
    uint64_t text_vmsize = 0, data_vmsize = 0;
    {
        uint64_t cur_vm = text_vmaddr;
        for (int i = 0; i < n_mo; i++) {
            if (strcmp(mo_secs[i].segname, "__TEXT") != 0) continue;
            mo_secs[i].sec->addr = cur_vm;
            uint64_t sz = mo_align(mo_secs[i].sec->len, 16);
            cur_vm += sz;
            text_vmsize += sz;
        }
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

    // Apply relocations now that every section has its real address.
    link_apply_relocs(s, 0);

    // Entry point
    int entry_sym = link_find_sym(s, "_main");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "main");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "start");
    uint64_t entry_addr = 0;
    if (entry_sym >= 0) entry_addr = mo_symbol_address(s, entry_sym);

    // Identify undefined symbols for dynamic linking
    int n_undef = 0;
    for (int i = 0; i < s->n_syms; i++) {
        if (s->syms[i].sec < 0 && s->syms[i].name && s->syms[i].name[0])
            n_undef++;
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
    uint32_t lc_dylib = (uint32_t)mo_align(24 + strlen("/usr/lib/libSystem.B.dylib") + 1, 8);
    uint32_t lc_dyld_info = 48;
    // Symbol table: 1 null entry + undefined symbols
    uint32_t lc_symtab = 24;
    uint32_t lc_dysymtab = 80;
    uint32_t ncmds = 4; // PAGEZERO, TEXT, DATA, LINKEDIT
    ncmds += 1; // LC_DYLD_INFO_ONLY
    ncmds += 1; // LC_SYMTAB
    ncmds += 1; // LC_DYSYMTAB
    ncmds += 1; // LC_LOAD_DYLIB
    ncmds += 1; // LC_BUILD_VERSION
    ncmds += 1; // LC_MAIN
    uint32_t lc_pagezero = 72; // __PAGEZERO segment
    uint32_t lc_linkedit = 72; // __LINKEDIT segment

    uint32_t header_size = 32;
    // sizeofcmds (mach_header_64) covers every load command written below,
    // __PAGEZERO included -- it was missing here, so cmds_end/text_fileoff
    // (and therefore every file offset the __TEXT/__DATA section headers
    // and the "write section data"/"write symtab" cursors below rely on)
    // were 72 bytes short of where __PAGEZERO's own bytes actually land in
    // the file, corrupting the layout the same way the LC_LOAD_DYLIB size
    // mismatch did above.
    uint32_t total_lc = lc_pagezero + lc_linkedit + text_lc_size + data_lc_size;
    total_lc += lc_build_version + lc_main + lc_dylib + lc_dyld_info;
    total_lc += lc_symtab + lc_dysymtab;
    uint32_t cmds_end = header_size + total_lc;

    // Compute file offsets
    uint64_t fileoff = mo_align(cmds_end, 16);
    uint64_t text_fileoff = fileoff;
    uint64_t data_fileoff = mo_align(text_fileoff + text_vmsize, 16);
    uint64_t linkedit_fileoff = data_fileoff + data_vmsize;

    // Symbol table: 1 null + undef
    uint32_t nsyms = 1 + (uint32_t)n_undef;
    uint64_t symtab_off = linkedit_fileoff + 0; // inside __LINKEDIT
    uint64_t strtab_off = symtab_off + nsyms * 16;
    // String table: "\0" + undef names + dylib path
    size_t strtab_size = 1; // leading \0
    for (int i = 0; i < s->n_syms; i++) {
        if (s->syms[i].sec < 0 && s->syms[i].name && s->syms[i].name[0])
            strtab_size += strlen(s->syms[i].name) + 1;
    }
    const char *dylib_path = "/usr/lib/libSystem.B.dylib";
    strtab_size += strlen(dylib_path) + 1;
    strtab_size = mo_align(strtab_size, 8);
    uint64_t linkedit_end = strtab_off + strtab_size;

    // Open output file
    FILE *f = fopen(s->out_path, "wb");
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
    mo_w32(f, MH_EXECUTE);
    mo_w32(f, ncmds);
    mo_w32(f, total_lc);
    mo_w32(f, 0); // flags
    mo_w32(f, 0); // reserved

    // --- LC_SEGMENT_64: __PAGEZERO ---
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

    // --- LC_SEGMENT_64: __TEXT ---
    mo_w32(f, LC_SEGMENT_64);
    mo_w32(f, text_lc_size);
    mo_wbuf(f, "__TEXT\0\0\0\0\0\0\0\0\0\0", 16);
    mo_w64(f, text_vmaddr);
    mo_w64(f, text_vmsize);
    mo_w64(f, text_fileoff);
    mo_w64(f, text_vmsize);
    mo_w32(f, 7);
    mo_w32(f, 5); // maxprot=rwx, initprot=r-x
    mo_w32(f, nsects_text);
    mo_w32(f, 0);

    uint64_t cur_vm = text_vmaddr;
    uint64_t cur_fo = text_fileoff;
    for (int i = 0; i < n_mo; i++) {
        if (strcmp(mo_secs[i].segname, "__TEXT") != 0) continue;
        LinkSec *sec = mo_secs[i].sec;
        char sn[16] = {0}, sg[16] = {0};
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
    cur_vm = data_vmaddr;
    cur_fo = data_fileoff;
    mo_w32(f, LC_SEGMENT_64);
    mo_w32(f, data_lc_size);
    mo_wbuf(f, "__DATA\0\0\0\0\0\0\0\0\0\0", 16);
    uint64_t data_total_vmsize = 0;
    for (int i = 0; i < n_mo; i++)
        if (strcmp(mo_secs[i].segname, "__TEXT") != 0)
            data_total_vmsize += mo_align(mo_secs[i].sec->len, 16);
    mo_w64(f, data_vmaddr);
    mo_w64(f, data_total_vmsize);
    mo_w64(f, data_fileoff);
    mo_w64(f, data_total_vmsize);
    mo_w32(f, 7);
    mo_w32(f, 3); // maxprot=rwx, initprot=rw-
    mo_w32(f, nsects_data);
    mo_w32(f, 0);
    for (int i = 0; i < n_mo; i++) {
        if (strcmp(mo_secs[i].segname, "__TEXT") == 0) continue;
        LinkSec *sec = mo_secs[i].sec;
        char sn[16] = {0}, sg[16] = {0};
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

    // --- LC_SEGMENT_64: __LINKEDIT ---
    mo_w32(f, LC_SEGMENT_64);
    mo_w32(f, 72);
    mo_wbuf(f, "__LINKEDIT\0\0\0\0\0", 16);
    mo_w64(f, cur_vm);
    mo_w64(f, mo_align(linkedit_end - linkedit_fileoff, 0x4000));
    mo_w64(f, linkedit_fileoff);
    // filesize must equal exactly what gets written below (LINKEDIT's
    // content already ends at a strtab_size-rounded, page-independent
    // byte count -- no separate rounding here); rounding up to 16 claimed
    // 8 more bytes than were ever written, so dyld rejected the file as
    // truncated ("fileoff+filesize extends past the end of the file").
    mo_w64(f, linkedit_end - linkedit_fileoff);
    mo_w32(f, 7);
    mo_w32(f, 1); // maxprot=rwx, initprot=r--
    mo_w32(f, 0);
    mo_w32(f, 0);

    // --- LC_DYLD_INFO_ONLY ---
    mo_w32(f, LC_DYLD_INFO_ONLY);
    mo_w32(f, 48);
    for (int i = 0; i < 10; i++) mo_w32(f, 0); // all offsets/sizes = 0 (no bind/lazy/export)

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
    mo_w32(f, 1); // ilocalsym = 1 (skip null entry)
    mo_w32(f, (uint32_t)n_undef); // nlocalsym = undef externals
    mo_w32(f, 0);
    mo_w32(f, 0);
    mo_w32(f, 0); // iextdefsym, nextdefsym, iundefsym=0
    mo_w32(f, (uint32_t)n_undef); // nundefsym
    mo_w32(f, 0);
    mo_w32(f, 0); // toc, modtab
    mo_w32(f, 0);
    mo_w32(f, 0); // extrefsym, indirectsym
    mo_w32(f, 0);
    mo_w32(f, 0); // extrel, local
    mo_w32(f, 0);
    mo_w32(f, 0); // locrel, locrel count
    // dysymtab_command has exactly 18 uint32_t fields after cmd/cmdsize
    // (ilocalsym..nlocalsym, iextdefsym..nundefsym, tocoff..nmodtab,
    // extrefsymoff..nindirectsyms, extreloff..nlocrel); 14 are written
    // individually above, so this loop covers the last 4
    // (extreloff,nextrel,locreloff,nlocrel), not 5 -- the stray extra
    // zero word pushed every load command after LC_DYSYMTAB 4 bytes out
    // of place, which is what otool's "cmdsize not a multiple of 8"
    // report on the (now misaligned) LC_LOAD_DYLIB was actually seeing.
    for (int i = 0; i < 4; i++) mo_w32(f, 0);

    // --- LC_LOAD_DYLIB ---
    // dylib_padded must equal lc_dylib (computed earlier and baked into
    // cmds_end/text_fileoff): keeping this a single source of truth
    // instead of two independent computations is what the fix above
    // exists to guarantee.
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

    // --- LC_BUILD_VERSION ---
    mo_w32(f, LC_BUILD_VERSION);
    mo_w32(f, 24);
    mo_w32(f, PLATFORM_MACOS);
    mo_w32(f, 0x000E0000); // minos 14.0
    mo_w32(f, 0x000E0000); // sdk 14.0
    mo_w32(f, 0); // ntools = 0

    // --- LC_MAIN ---
    mo_w32(f, LC_MAIN);
    mo_w32(f, 24);
    mo_w64(f, entry_addr - base); // entry offset
    mo_w64(f, 0); // stack size (default)

    // Pad to section alignment
    uint64_t cur = ftell(f);
    if (text_fileoff > cur) mo_wzeros(f, (size_t)(text_fileoff - cur));

    // --- Write section data ---
    // sec->data is exactly sec->len bytes (unaligned); every offset this
    // file computes for the section *after* this one (and the fixed
    // relationship between mo_align(sec->len,16) steps and the section
    // headers written above) assumes each section occupies its
    // 16-byte-rounded length on disk. The previous code advanced cur_fo
    // by the rounded length but only ever wrote the unrounded sec->len
    // bytes, silently drifting ftell() behind cur_fo by the rounding slack
    // of every section so far -- so each later section's real file bytes
    // landed short of the offset its own header claims (observed as e.g.
    // __const's bytes ending up 8 bytes before the offset dyld/otool read
    // them at, showing up as all-zero padding instead of the real data).
    cur_fo = text_fileoff;
    for (int i = 0; i < n_mo; i++) {
        LinkSec *sec = mo_secs[i].sec;
        if (sec->is_bss) continue;
        mo_wbuf(f, sec->data, sec->len);
        uint64_t padded = mo_align(sec->len, 16);
        if (padded > sec->len) mo_wzeros(f, (size_t)(padded - sec->len));
        cur_fo += padded;
    }
    cur = ftell(f);
    if (symtab_off > (uint64_t)cur) mo_wzeros(f, (size_t)(symtab_off - (uint64_t)cur));

    // --- Write symbol table ---
    // Null entry (nlist_64 = 16 bytes)
    mo_w32(f, 0);
    fputc(0, f);
    fputc(0, f);
    fputc(0 & 0xFF, f);
    fputc((0 >> 8) & 0xFF, f);
    mo_w64(f, 0);
    // Undefined external symbols
    uint32_t str_off = 1; // skip leading \0
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
    for (int i = 0; i < s->n_syms; i++) {
        LinkSym *sym = &s->syms[i];
        if (sym->sec >= 0 || !sym->name || !sym->name[0]) continue;
        mo_wbuf(f, sym->name, strlen(sym->name) + 1);
    }
    mo_wbuf(f, dylib_path, strlen(dylib_path) + 1);
    cur = ftell(f);
    if (mo_align((uint64_t)cur, 8) > (uint64_t)cur)
        mo_wzeros(f, mo_align((uint64_t)cur, 8) - (uint64_t)cur);

    fclose(f);
    chmod(s->out_path, 0755);
    free(mo_secs);
    return 0;
}
