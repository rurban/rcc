// SPDX-License-Identifier: LGPL-2.1-or-later
// Write a Mach-O 64-bit relocatable object file (.o) from an ObjFile.
// Targets: macOS x86-64 and arm64.
#ifdef __APPLE__
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Mach-O constants
// ---------------------------------------------------------------------------
#define MH_MAGIC_64     0xfeedfacfU
#define MH_OBJECT       0x1
#define CPU_TYPE_X86_64     0x01000007
#define CPU_TYPE_ARM64      0x0100000c
#define CPU_SUBTYPE_ALL     0x00000003
#define CPU_SUBTYPE_ARM64_ALL 0x00000000

#define MH_SUBSECTIONS_VIA_SYMBOLS 0x2000

#define LC_SEGMENT_64   0x19
#define LC_SYMTAB       0x2
#define LC_DYSYMTAB     0xb
#define LC_BUILD_VERSION 0x32

#define PLATFORM_MACOS  1
#define MINOS_VERSION   0x000c0000 // macOS 12.0

// Section flags
#define S_REGULAR             0x0
#define S_ZEROFILL            0x1
#define S_ATTR_SOME_INSTRUCTIONS  0x00000400
#define S_ATTR_DEBUG          0x02000000
#define S_CSTRING_LITERALS    0x2
#define S_MOD_INIT_FUNC_POINTERS    0x9
#define S_ATTR_PURE_INSTRUCTIONS  0x80000000
#define S_THREAD_LOCAL_REGULAR            0x11
#define S_THREAD_LOCAL_VARIABLES          0x13
#define S_THREAD_LOCAL_VARIABLE_POINTERS  0x14

// Relocation types x86-64
#define X86_64_RELOC_UNSIGNED  0
#define X86_64_RELOC_SIGNED    1
#define X86_64_RELOC_BRANCH    2
#define X86_64_RELOC_GOT_LOAD  3
#define X86_64_RELOC_GOT       4
#define X86_64_RELOC_SUBTRACTOR 5
#define X86_64_RELOC_SIGNED_1  6
#define X86_64_RELOC_SIGNED_2  7
#define X86_64_RELOC_SIGNED_4  8
#define X86_64_RELOC_TLV       9

// Relocation types arm64
#define ARM64_RELOC_UNSIGNED         0
#define ARM64_RELOC_SUBTRACTOR       1
#define ARM64_RELOC_BRANCH26         2
#define ARM64_RELOC_PAGE21           3
#define ARM64_RELOC_PAGEOFF12        4
#define ARM64_RELOC_GOT_LOAD_PAGE21  5
#define ARM64_RELOC_GOT_LOAD_PAGEOFF12 6

// Darwin ARM64 TLS: General Dynamic model via TLV descriptors
#define ARM64_RELOC_TLVP_LOAD_PAGE21   8
#define ARM64_RELOC_TLVP_LOAD_PAGEOFF12 9
// Nlist types
#define N_UNDF  0x0
#define N_ABS   0x2
#define N_SECT  0xe
#define N_EXT   0x01
#define N_PEXT  0x10
#define NO_SECT 0
#define REFERENCE_FLAG_UNDEFINED_NON_LAZY 0
#define REFERENCE_FLAG_UNDEFINED_LAZY 1
// n_desc flags
#define N_WEAK_REF 0x0040 // undefined symbol is a weak reference (may be missing)
#define N_WEAK_DEF 0x0080 // defined symbol is a weak (coalesced) definition

// ---------------------------------------------------------------------------
// Write helpers (little-endian)
// ---------------------------------------------------------------------------
static void w8(FILE *f, uint8_t v) { fputc(v, f); }
static void w16(FILE *f, uint16_t v) {
    w8(f, (uint8_t)v);
    w8(f, v >> 8);
}
static void w32(FILE *f, uint32_t v) {
    w16(f, (uint16_t)v);
    w16(f, (uint16_t)(v >> 16));
}
static void w64(FILE *f, uint64_t v) {
    w32(f, (uint32_t)v);
    w32(f, (uint32_t)(v >> 32));
}
static void wbuf(FILE *f, const void *buf, size_t n) { fwrite(buf, 1, n, f); }
static void wzeros(FILE *f, uint64_t n) {
    uint8_t z[64];
    memset(z, 0, sizeof(z));
    while (n >= 64) {
        fwrite(z, 1, 64, f);
        n -= 64;
    }
    if (n) fwrite(z, 1, (size_t)n, f);
}
static uint64_t align(uint64_t x, uint64_t a) { return (x + a - 1) & ~(a - 1); }

// ---------------------------------------------------------------------------
// Mach-O string table
// ---------------------------------------------------------------------------
typedef struct {
    char *data;
    size_t len, cap;
} MachoStrtab;

static void mstrtab_init(MachoStrtab *t) {
    t->cap = 256;
    t->data = malloc(t->cap);
    t->len = 0;
    // Mach-O string table starts with a space (index 0 = " ")
    t->data[t->len++] = ' ';
    t->data[t->len++] = '\0';
}

static uint32_t mstrtab_add(MachoStrtab *t, const char *s) {
    size_t n = strlen(s) + 1;
    if (t->len + n > t->cap) {
        while (t->cap < t->len + n) t->cap *= 2;
        t->data = realloc(t->data, t->cap);
    }
    uint32_t idx = (uint32_t)t->len;
    memcpy(t->data + t->len, s, n);
    t->len += n;
    return idx;
}

// Map obj section to Mach-O section ordinal (1-based)
static uint8_t obj_section_to_macho(int section) {
    switch (section) {
    case SEC_TEXT: return 1;
    case SEC_DATA: return 2;
    case SEC_BSS: return 3;
    case SEC_RODATA: return 4;
    case SEC_INIT_ARRAY: return 5;
    case SEC_TDATA: return 6;
    case SEC_THREAD_VARS: return 7;
    default: return 0;
    }
}

// ---------------------------------------------------------------------------
// Convert ELF reloc type → Mach-O reloc type (best-effort)
// ---------------------------------------------------------------------------
static uint8_t elf_reloc_to_macho(uint32_t elf_type, bool is_arm64) {
    if (is_arm64) {
        switch (elf_type) {
        case R_AARCH64_ABS64: return ARM64_RELOC_UNSIGNED;
        case R_AARCH64_CALL26: return ARM64_RELOC_BRANCH26;
        case R_AARCH64_JUMP26: return ARM64_RELOC_BRANCH26;
        case R_AARCH64_ADR_PREL_PG_HI21: return ARM64_RELOC_PAGE21;
        case R_AARCH64_ADD_ABS_LO12_NC: return ARM64_RELOC_PAGEOFF12;
        case R_AARCH64_ADR_GOT_PAGE: return ARM64_RELOC_GOT_LOAD_PAGE21;
        case R_AARCH64_LD64_GOT_LO12_NC: return ARM64_RELOC_GOT_LOAD_PAGEOFF12;
        case R_AARCH64_TLSLE_ADD_TPREL_HI12: return ARM64_RELOC_TLVP_LOAD_PAGE21;
        case R_AARCH64_TLSLE_ADD_TPREL_LO12: return ARM64_RELOC_TLVP_LOAD_PAGEOFF12;
        }
    } else {
        switch (elf_type) {
        case R_X86_64_64: return X86_64_RELOC_UNSIGNED;
        case R_X86_64_PLT32: return X86_64_RELOC_BRANCH;
        case R_X86_64_PC32: return X86_64_RELOC_SIGNED;
        case R_X86_64_32S: return X86_64_RELOC_SIGNED;
        case R_X86_64_32: return X86_64_RELOC_UNSIGNED;
        case R_X86_64_TPOFF32: return X86_64_RELOC_TLV;
        default: return X86_64_RELOC_UNSIGNED;
        }
    }
    return 0;
}
// ---------------------------------------------------------------------------
// Custom named sections from GAS `.section`/`.pushsection` with no built-in
// Mach-O slot above (e.g. the kernel's .altinstructions, .altinstr_replacement,
// __ex_table). elf_write.c/coff_write.c already handle these generically via
// obj->extra_secs; macho_write.c didn't.
// ---------------------------------------------------------------------------

// Executable content goes in __TEXT, everything else in __DATA — the same
// code/data split GAS's own section characteristics imply.
static const char *macho_segname_from_flags(uint32_t sh_flags) {
    return (sh_flags & SHF_EXECINSTR) ? "__TEXT" : "__DATA";
}

// Mach-O sectname is a fixed 16-byte field with no long-name escape (unlike
// COFF's "/<stroff>"); truncate rather than fail on an overlong GAS section
// name — real-world custom sections (.altinstructions, __ex_table, ...) fit.
static void macho_section_name(char sectname[16], const char *name) {
    memset(sectname, 0, 16);
    size_t n = strlen(name);
    if (n > 16) n = 16;
    memcpy(sectname, name, n);
}

// Map an ObjSym/ObjReloc section id (built-in SEC_TEXT..SEC_THREAD_VARS or a
// dynamic SEC_NUM+i custom section) to its 1-based Mach-O section ordinal.
// extra_ord[i] holds the ordinal assigned to obj->extra_secs[i]; computed
// once per macho_write() call since it depends on whether DWARF debug
// sections (-g) preceded the custom sections.
static uint8_t obj_section_ord(int section, const int *extra_ord) {
    if (section < SEC_NUM) return obj_section_to_macho(section);
    return (uint8_t)extra_ord[section - SEC_NUM];
}

// Write one Mach-O relocation_info (8 bytes) for a custom section's entry.
// Mirrors the ext/pcrel/sym_num logic already inlined per built-in section
// below, generalized with a type-driven pcrel/length so it also covers
// FIXUP_PCREL_DATA's ".long/.quad (label) - ." relocations (used by e.g. GAS
// .altinstructions-style idioms), which can be 4 or 8 bytes wide inside a
// single custom section, unlike the built-in sections which are uniformly
// one or the other.
static void write_macho_reloc(FILE *f, ObjReloc *r, ObjFile *obj,
                              const int *sym_map, bool is_arm64,
                              const int *extra_ord) {
    bool ext = r->sym_idx >= 0 && obj->syms[r->sym_idx].section == SEC_UNDEF;
    uint8_t mtype = elf_reloc_to_macho(r->type, is_arm64);
    bool pcrel;
    uint32_t length;
    if (is_arm64) {
        switch (r->type) {
        case R_AARCH64_CALL26:
        case R_AARCH64_JUMP26:
        case R_AARCH64_ADR_PREL_PG_HI21:
        case R_AARCH64_ADR_GOT_PAGE:
        case R_AARCH64_TLSLE_ADD_TPREL_HI12:
            pcrel = true;
            length = 2;
            break;
        default:
            pcrel = false;
            length = 3;
            break;
        }
    } else {
        switch (r->type) {
        case R_X86_64_PC32:
        case R_X86_64_PLT32:
            pcrel = true;
            length = 2;
            break;
        case R_X86_64_PC64:
            pcrel = true;
            length = 3;
            break;
        case R_X86_64_32:
        case R_X86_64_32S:
            pcrel = false;
            length = 2;
            break;
        default: // R_X86_64_64 and anything else
            pcrel = false;
            length = 3;
            break;
        }
    }
    uint32_t sym_num;
    if (is_arm64) {
        ext = true;
        sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
    } else if (!ext && r->sym_idx >= 0) {
        sym_num = obj_section_ord(obj->syms[r->sym_idx].section, extra_ord);
    } else {
        sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
    }
    w32(f, (uint32_t)r->offset);
    uint32_t pack = (sym_num & 0xffffff) |
        ((uint32_t)(pcrel ? 1 : 0) << 24) |
        (length << 25) |
        ((uint32_t)(ext ? 1 : 0) << 27) |
        ((uint32_t)mtype << 28);
    w32(f, pack);
}

// ---------------------------------------------------------------------------
// Main Mach-O writer
// ---------------------------------------------------------------------------
int macho_write(ObjFile *obj, const char *path) {
#ifdef __aarch64__
    bool is_arm64 = true;
    uint32_t cpu_type = CPU_TYPE_ARM64;
    uint32_t cpu_subtype = CPU_SUBTYPE_ARM64_ALL;
#else
    bool is_arm64 = false;
    uint32_t cpu_type = CPU_TYPE_X86_64;
    uint32_t cpu_subtype = CPU_SUBTYPE_ALL;
#endif

    // Sections: 1=__text, 2=__data, 3=__bss, 4=__const (rodata), 5=__mod_init_func,
    // 6=__thread_data, 7=__thread_vars. With -g, four __DWARF sections follow:
    // 8=__debug_line, 9=__debug_info, 10=__debug_abbrev, 11=__debug_aranges.
    // Custom obj->extra_secs[] sections (section id SEC_NUM+i) follow those.
    // Computed up front since the symbol table below needs extra_ord to place
    // symbols defined inside a custom section.
    bool has_debug = objfile_has_debug(obj);
    int extra_base_ord = 7 + (has_debug ? 4 : 0);
    int *extra_ord = NULL;
    if (obj->extra_sec_count > 0) {
        extra_ord = malloc((size_t)obj->extra_sec_count * sizeof(int));
        for (int i = 0; i < obj->extra_sec_count; i++)
            extra_ord[i] = extra_base_ord + i + 1;
    }

    // -----------------------------------------------------------------------
    // Build symbol table
    // -----------------------------------------------------------------------
    MachoStrtab mst;
    mstrtab_init(&mst);

    // Map obj symbol index → nlist index
    int *sym_map = calloc((size_t)(obj->sym_count + 1), sizeof(int));
    int nsyms = 0;

    // Symbol storage: (stridx, type, sect, desc, value)
    typedef struct {
        uint32_t strx;
        uint8_t type, sect;
        uint16_t desc;
        uint64_t value;
    } Nlist;
    Nlist *nlist = calloc((size_t)(obj->sym_count + 1), sizeof(Nlist));

    // Locals first (Mach-O requires locals before globals)
    for (int i = 0; i < obj->sym_count; i++) {
        ObjSym *os = &obj->syms[i];
        if (os->bind != SB_LOCAL) continue;
        if (os->section == SEC_UNDEF) continue; // undefined symbols go below
        sym_map[i] = nsyms;
        Nlist *n = &nlist[nsyms++];
        n->strx = mstrtab_add(&mst, os->name);
        n->type = (os->section == SEC_UNDEF) ? N_UNDF : N_SECT;
        n->sect = obj_section_ord(os->section, extra_ord);
        n->desc = 0;
        n->value = os->offset;
    }
    int ilocalsym = 0;
    int nlocalsym = nsyms;
    int iextdefsym = nsyms;

    // Defined globals
    for (int i = 0; i < obj->sym_count; i++) {
        ObjSym *os = &obj->syms[i];
        if (os->bind == SB_LOCAL || os->section == SEC_UNDEF) continue;
        sym_map[i] = nsyms;
        Nlist *n = &nlist[nsyms++];
        n->strx = mstrtab_add(&mst, os->name);
        n->type = N_SECT | N_EXT;
        n->sect = obj_section_ord(os->section, extra_ord);
        // A weak (coalesced) definition carries N_WEAK_DEF.
        n->desc = (os->bind == SB_WEAK) ? (uint16_t)N_WEAK_DEF : 0;
        n->value = os->offset;
    }
    int nextdefsym = nsyms - iextdefsym;
    int iundefsym = nsyms;

    // Undefined symbols (all SEC_UNDEF, both local and global)
    for (int i = 0; i < obj->sym_count; i++) {
        ObjSym *os = &obj->syms[i];
        if (os->section != SEC_UNDEF) continue;
        sym_map[i] = nsyms;
        Nlist *n = &nlist[nsyms++];
        n->strx = mstrtab_add(&mst, os->name);
        n->type = (os->bind == SB_LOCAL) ? N_UNDF : (N_UNDF | N_EXT);
        n->sect = NO_SECT;
        if (os->bind == SB_LOCAL) {
            n->desc = 0;
        } else {
            n->desc = (uint16_t)REFERENCE_FLAG_UNDEFINED_NON_LAZY;
            if (os->bind == SB_WEAK)
                n->desc |= (uint16_t)N_WEAK_REF;
        }
        n->value = 0;
    }
    int nundefsym = nsyms - iundefsym;

    // -----------------------------------------------------------------------
    // Decide which sections to include
    // -----------------------------------------------------------------------
    // Mach-O section indices are 1-based.
    // We always emit at least __TEXT,__text.
    // Mach-O .o has one segment "" (empty name) containing all sections.
    int nsections = extra_base_ord + obj->extra_sec_count;

    // -----------------------------------------------------------------------
    // Compute sizes / offsets
    // -----------------------------------------------------------------------
    // Mach-O header + load commands come first.
    // mach_header_64 = 32 bytes
    // LC_SEGMENT_64 = 72 + nsections*80
    // LC_SYMTAB = 24
    // LC_DYSYMTAB = 80
    uint32_t segment_lc_size = 72 + (uint32_t)nsections * 80;
    uint32_t symtab_lc_size = 24;
    uint32_t dysymtab_lc_size = 80;
    uint32_t build_version_lc_size = 24; // LC_BUILD_VERSION = 24 bytes
    uint32_t header_size = 32 + segment_lc_size + symtab_lc_size + dysymtab_lc_size + build_version_lc_size;

    uint64_t text_off = align(header_size, 16);
    uint64_t text_size = obj->text.len;
    uint64_t data_off = align(text_off + text_size, 8);
    uint64_t data_size = obj->data.len;
    uint64_t bss_off = align(data_off + data_size, 8); // no actual bytes
    uint64_t rodata_off = align(bss_off, 8); // bss uses no file space
    uint64_t rodata_size = obj->rodata.len;
    uint64_t init_off = align(rodata_off + rodata_size, 8);
    uint64_t init_size = obj->init_array.len;
    uint64_t tdata_off = align(init_off + init_size, 8);
    uint64_t tdata_size = obj->data_tls.len;
    uint64_t tvars_off = align(tdata_off + tdata_size, 8);
    uint64_t tvars_size = obj->thread_vars.len;

    // DWARF debug sections (-g). Byte-aligned, contiguous after __thread_vars.
    uint64_t dbgline_off = tvars_off + tvars_size;
    uint64_t dbgline_size = has_debug ? obj->debug_line_section.len : 0;
    uint64_t dbginfo_off = dbgline_off + dbgline_size;
    uint64_t dbginfo_size = has_debug ? obj->debug_info_section.len : 0;
    uint64_t dbgabbrev_off = dbginfo_off + dbginfo_size;
    uint64_t dbgabbrev_size = has_debug ? obj->debug_abbrev_section.len : 0;
    uint64_t dbgaranges_off = dbgabbrev_off + dbgabbrev_size;
    uint64_t dbgaranges_size = has_debug ? obj->debug_aranges_section.len : 0;
    uint64_t sections_end = has_debug ? dbgaranges_off + dbgaranges_size
                                      : tvars_off + tvars_size;

    // Custom obj->extra_secs[] sections, 8-byte aligned and contiguous after
    // the built-in/DWARF sections.
    uint64_t *extra_off = NULL;
    if (obj->extra_sec_count > 0)
        extra_off = malloc((size_t)obj->extra_sec_count * sizeof(uint64_t));
    uint64_t extra_end = sections_end;
    for (int i = 0; i < obj->extra_sec_count; i++) {
        extra_end = align(extra_end, 8);
        extra_off[i] = extra_end;
        extra_end += obj->extra_secs[i].buf.len;
    }

    // Relocations follow section data
    uint64_t reloc_text_off = align(extra_end, 4);
    uint32_t reloc_text_cnt = (uint32_t)obj->text_reloc_count;
    uint64_t reloc_data_off = reloc_text_off + reloc_text_cnt * 8;
    uint32_t reloc_data_cnt = (uint32_t)obj->data_reloc_count;
    uint64_t reloc_rod_off = reloc_data_off + reloc_data_cnt * 8;
    uint32_t reloc_rod_cnt = (uint32_t)obj->rodata_reloc_count;
    uint64_t reloc_tvars_off = reloc_rod_off + reloc_rod_cnt * 8;
    uint32_t reloc_tvars_cnt = (uint32_t)obj->thread_vars_reloc_count;
    uint64_t reloc_init_off = reloc_tvars_off + reloc_tvars_cnt * 8;
    uint32_t reloc_init_cnt = (uint32_t)obj->init_array_reloc_count;

    // DWARF address relocations: one UNSIGNED/quad each for __debug_line,
    // __debug_info and __debug_aranges, all against the __text section (ordinal 1).
    uint64_t reloc_dbgline_off = reloc_init_off + reloc_init_cnt * 8;
    uint32_t reloc_dbgline_cnt = has_debug && obj->debug_has_line_addr ? 1 : 0;
    uint64_t reloc_dbginfo_off = reloc_dbgline_off + reloc_dbgline_cnt * 8;
    uint32_t reloc_dbginfo_cnt = has_debug && obj->debug_has_low_pc ? 1 : 0;
    uint64_t reloc_dbgaranges_off = reloc_dbginfo_off + reloc_dbginfo_cnt * 8;
    uint32_t reloc_dbgaranges_cnt = has_debug && obj->debug_has_aranges_addr ? 1 : 0;

    // Custom sections' own relocations, one contiguous block per section.
    uint64_t *reloc_extra_off = NULL;
    if (obj->extra_sec_count > 0)
        reloc_extra_off = malloc((size_t)obj->extra_sec_count * sizeof(uint64_t));
    uint64_t reloc_extra_end = reloc_dbgaranges_off + reloc_dbgaranges_cnt * 8;
    for (int i = 0; i < obj->extra_sec_count; i++) {
        reloc_extra_off[i] = reloc_extra_end;
        reloc_extra_end += (uint64_t)obj->extra_secs[i].reloc_count * 8;
    }

    uint64_t symtab_off = align(reloc_extra_end, 8);
    uint32_t symtab_size = (uint32_t)nsyms * 16; // sizeof nlist_64 = 16
    uint64_t strtab_off = symtab_off + symtab_size;
    uint32_t strtab_size = (uint32_t)mst.len;

    // segment vm size = from text_off to end of the last section (debug and
    // custom sections included, when present), plus BSS VM allocation
    uint64_t seg_filesize = extra_end - text_off;

    uint64_t seg_vmsize = seg_filesize + obj->bss_size;

    // -----------------------------------------------------------------------
    // Write file
    // -----------------------------------------------------------------------
    mkdir_p(path);
    FILE *f = fopen(path, "wb");
    if (!f) {
        free(sym_map);
        free(nlist);
        free(mst.data);
        free(extra_ord);
        free(extra_off);
        free(reloc_extra_off);
        return -1;
    }

    // mach_header_64
    w32(f, MH_MAGIC_64);
    w32(f, cpu_type);
    w32(f, cpu_subtype);
    w32(f, MH_OBJECT);
    w32(f, 4); // ncmds (segment, build_version, symtab, dysymtab)
    w32(f, segment_lc_size + build_version_lc_size + symtab_lc_size + dysymtab_lc_size); // sizeofcmds
    w32(f, MH_SUBSECTIONS_VIA_SYMBOLS); // flags
    w32(f, 0); // reserved

    // LC_SEGMENT_64
    w32(f, LC_SEGMENT_64);
    w32(f, segment_lc_size);
    // segname (16 bytes, null-padded) = "" (empty for .o files)
    wzeros(f, 16);
    w64(f, 0); // vmaddr = 0
    w64(f, seg_vmsize);
    w64(f, text_off); // fileoff
    w64(f, seg_filesize);
    w32(f, 7); // maxprot  = rwx
    w32(f, 7); // initprot = rwx
    w32(f, (uint32_t)nsections);
    w32(f, 0); // flags

    // Section 1: __TEXT,__text
    {
        const char sn[16] = "__text";
        const char sg[16] = "__TEXT";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, text_size);
        w32(f, (uint32_t)text_off); // offset
        w32(f, 4); // align (2^4=16)
        w32(f, (uint32_t)reloc_text_off);
        w32(f, reloc_text_cnt);
        w32(f, S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS | S_REGULAR);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0); // reserved
    }
    // Section 2: __DATA,__data
    {
        const char sn[16] = "__data";
        const char sg[16] = "__DATA";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, data_size);
        w32(f, (uint32_t)data_off);
        w32(f, 3); // align (2^3=8)
        w32(f, (uint32_t)reloc_data_off);
        w32(f, reloc_data_cnt);
        w32(f, S_REGULAR);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }
    // Section 3: __DATA,__bss
    {
        const char sn[16] = "__bss";
        const char sg[16] = "__DATA";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, obj->bss_size);
        w32(f, (uint32_t)bss_off); // no file content
        w32(f, 3);
        w32(f, 0);
        w32(f, 0); // no relocs
        w32(f, S_ZEROFILL);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }
    // Section 4: __TEXT,__const (rodata)
    {
        const char sn[16] = "__const";
        const char sg[16] = "__TEXT";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, rodata_size);
        w32(f, (uint32_t)rodata_off);
        w32(f, 0);
        w32(f, (uint32_t)reloc_rod_off);
        w32(f, reloc_rod_cnt);
        w32(f, S_REGULAR);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }

    // Section 5: __DATA,__mod_init_func
    {
        const char sn[16] = "__mod_init_func";
        const char sg[16] = "__DATA";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, init_size);
        w32(f, (uint32_t)init_off);
        w32(f, 3); // align (2^3=8)
        w32(f, (uint32_t)reloc_init_off);
        w32(f, reloc_init_cnt);
        w32(f, S_MOD_INIT_FUNC_POINTERS);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }

    // Section 6: __DATA,__thread_data
    {
        const char sn[16] = "__thread_data";
        const char sg[16] = "__DATA";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, obj->data_tls.len);
        w32(f, (uint32_t)tdata_off); // offset
        w32(f, 3); // align (2^3=8)
        w32(f, 0); // reloc offset (unused for __thread_data)
        w32(f, 0); // nreloc = 0
        w32(f, S_THREAD_LOCAL_REGULAR);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }
    // Section 7: __DATA,__thread_vars
    {
        const char sn[16] = "__thread_vars";
        const char sg[16] = "__DATA";
        wbuf(f, sn, 16);
        wbuf(f, sg, 16);
        w64(f, 0); // addr
        w64(f, obj->thread_vars.len);
        w32(f, (uint32_t)tvars_off); // offset
        w32(f, 3); // align (2^3=8)
        w32(f, (uint32_t)reloc_tvars_off); // reloc offset
        w32(f, reloc_tvars_cnt);
        w32(f, S_THREAD_LOCAL_VARIABLES);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }

    // Sections 8-11: __DWARF debug sections (-g). All byte-aligned, marked
    // S_ATTR_DEBUG. Only the address-bearing ones carry a relocation.
    if (has_debug) {
        struct {
            const char *name;
            uint64_t size, off, reloff;
            uint32_t nreloc;
        } dbg[4] = {
            {"__debug_line", dbgline_size, dbgline_off, reloc_dbgline_off, reloc_dbgline_cnt},
            {"__debug_info", dbginfo_size, dbginfo_off, reloc_dbginfo_off, reloc_dbginfo_cnt},
            {"__debug_abbrev", dbgabbrev_size, dbgabbrev_off, 0, 0},
            {"__debug_aranges", dbgaranges_size, dbgaranges_off, reloc_dbgaranges_off, reloc_dbgaranges_cnt},
        };
        for (int i = 0; i < 4; i++) {
            char sn[16] = {0};
            const char sg[16] = "__DWARF";
            memcpy(sn, dbg[i].name, strlen(dbg[i].name));
            wbuf(f, sn, 16);
            wbuf(f, sg, 16);
            w64(f, 0); // addr (0, like every other rcc section; resolved via relocs)
            w64(f, dbg[i].size);
            w32(f, (uint32_t)dbg[i].off);
            w32(f, 0); // align 2^0 = 1
            w32(f, (uint32_t)(dbg[i].nreloc ? dbg[i].reloff : 0));
            w32(f, dbg[i].nreloc);
            w32(f, S_ATTR_DEBUG | S_REGULAR);
            w32(f, 0);
            w32(f, 0);
            w32(f, 0);
        }
    }

    // Custom obj->extra_secs[] sections (GAS .section/.pushsection names with
    // no built-in slot above), one per registered section.
    for (int i = 0; i < obj->extra_sec_count; i++) {
        ExtraSection *es = &obj->extra_secs[i];
        char sectname[16];
        macho_section_name(sectname, es->name);
        char segname[16] = {0};
        const char *sg = macho_segname_from_flags(es->sh_flags);
        memcpy(segname, sg, strlen(sg));
        wbuf(f, sectname, 16);
        wbuf(f, segname, 16);
        w64(f, 0); // addr
        w64(f, es->buf.len);
        w32(f, (uint32_t)extra_off[i]);
        w32(f, 3); // align (2^3=8)
        w32(f, es->reloc_count ? (uint32_t)reloc_extra_off[i] : 0);
        w32(f, (uint32_t)es->reloc_count);
        w32(f, (es->sh_flags & SHF_EXECINSTR) ? (S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS | S_REGULAR) : S_REGULAR);
        w32(f, 0);
        w32(f, 0);
        w32(f, 0);
    }

    // LC_BUILD_VERSION
    w32(f, LC_BUILD_VERSION);
    w32(f, build_version_lc_size);
    w32(f, PLATFORM_MACOS); // platform
    w32(f, MINOS_VERSION); // minos (12.0)
    w32(f, 0); // sdk (unspecified)
    w32(f, 0); // ntools

    // LC_SYMTAB
    w32(f, LC_SYMTAB);
    w32(f, symtab_lc_size);
    w32(f, (uint32_t)symtab_off);
    w32(f, (uint32_t)nsyms);
    w32(f, (uint32_t)strtab_off);
    w32(f, strtab_size);

    // LC_DYSYMTAB
    w32(f, LC_DYSYMTAB);
    w32(f, dysymtab_lc_size);
    w32(f, (uint32_t)ilocalsym);
    w32(f, (uint32_t)nlocalsym);
    w32(f, (uint32_t)iextdefsym);
    w32(f, (uint32_t)nextdefsym);
    w32(f, (uint32_t)iundefsym);
    w32(f, (uint32_t)nundefsym);
    wzeros(f, 48); // tocoff..modtaboff fields = 0

    // Section data
    wzeros(f, text_off - header_size);
    if (text_size) wbuf(f, obj->text.data, text_size);
    wzeros(f, data_off - (text_off + text_size));
    if (data_size) wbuf(f, obj->data.data, data_size);
    // bss: no bytes
    wzeros(f, rodata_off - (data_off + data_size));
    if (rodata_size) wbuf(f, obj->rodata.data, rodata_size);
    wzeros(f, init_off - (rodata_off + rodata_size));
    if (init_size) wbuf(f, obj->init_array.data, init_size);
    wzeros(f, tdata_off - (init_off + init_size));
    if (tdata_size) wbuf(f, obj->data_tls.data, tdata_size);
    wzeros(f, tvars_off - (tdata_off + tdata_size));
    if (tvars_size) wbuf(f, obj->thread_vars.data, tvars_size);
    // DWARF debug section data (contiguous, byte-aligned)
    if (has_debug) {
        if (dbgline_size) wbuf(f, obj->debug_line_section.data, dbgline_size);
        if (dbginfo_size) wbuf(f, obj->debug_info_section.data, dbginfo_size);
        if (dbgabbrev_size) wbuf(f, obj->debug_abbrev_section.data, dbgabbrev_size);
        if (dbgaranges_size) wbuf(f, obj->debug_aranges_section.data, dbgaranges_size);
    }
    // Custom obj->extra_secs[] section data (contiguous, 8-byte aligned)
    {
        uint64_t cursor = sections_end;
        for (int i = 0; i < obj->extra_sec_count; i++) {
            ExtraSection *es = &obj->extra_secs[i];
            wzeros(f, extra_off[i] - cursor);
            if (es->buf.len) wbuf(f, es->buf.data, es->buf.len);
            cursor = extra_off[i] + es->buf.len;
        }
        wzeros(f, reloc_text_off - cursor);
    }
    for (int i = 0; i < obj->text_reloc_count; i++) {
        ObjReloc *r = &obj->text_relocs[i];
        bool ext = r->sym_idx >= 0 && obj->syms[r->sym_idx].section == SEC_UNDEF;
        uint8_t mtype = elf_reloc_to_macho(r->type, is_arm64);
        bool pcrel = (r->type == R_X86_64_PC32 || r->type == R_X86_64_PLT32 ||
                      r->type == R_AARCH64_CALL26 || r->type == R_AARCH64_JUMP26 ||
                      r->type == R_AARCH64_ADR_PREL_PG_HI21 || r->type == R_AARCH64_ADR_GOT_PAGE ||
                      r->type == R_AARCH64_TLSLE_ADD_TPREL_HI12);
        // ld64 rejects r_extern=0 for branch/page/pcrel types.
        uint32_t sym_num;
        if (is_arm64) {
            ext = true;
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        } else if (!ext && r->sym_idx >= 0) {
            sym_num = obj_section_ord(obj->syms[r->sym_idx].section, extra_ord);
        } else {
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        }
        w32(f, (uint32_t)r->offset);
        uint32_t pack = (sym_num & 0xffffff) |
            ((uint32_t)pcrel << 24) |
            (2 << 25) |
            ((uint32_t)(ext ? 1 : 0) << 27) |
            ((uint32_t)mtype << 28);
        w32(f, pack);
    }
    for (int i = 0; i < obj->data_reloc_count; i++)
        write_macho_reloc(f, &obj->data_relocs[i], obj, sym_map, is_arm64, extra_ord);
    for (int i = 0; i < obj->rodata_reloc_count; i++)
        write_macho_reloc(f, &obj->rodata_relocs[i], obj, sym_map, is_arm64, extra_ord);

    // Thread vars relocs
    for (int i = 0; i < obj->thread_vars_reloc_count; i++)
        write_macho_reloc(f, &obj->thread_vars_relocs[i], obj, sym_map, is_arm64, extra_ord);

    // Init array relocs
    for (int i = 0; i < obj->init_array_reloc_count; i++)
        write_macho_reloc(f, &obj->init_array_relocs[i], obj, sym_map, is_arm64, extra_ord);

    // DWARF address relocations. Each is a non-scattered, non-pcrel, non-extern
    // UNSIGNED reloc of length 3 (8 bytes) against the __text section (ordinal 1);
    // r_type UNSIGNED is 0 for both x86-64 and arm64. The in-place placeholder is
    // 0, so the linker resolves it to the final address of __text.
    if (has_debug) {
        // r_symbolnum=1, pcrel=0, length=3 (bits 25-26), extern=0, type=UNSIGNED(0)
        const uint32_t dbg_pack = (1u & 0xffffff) | (3u << 25);
        struct {
            uint32_t addr, cnt;
        } dr[3] = {
            {(uint32_t)obj->debug_line_addr_off, reloc_dbgline_cnt},
            {(uint32_t)obj->debug_low_pc_offset, reloc_dbginfo_cnt},
            {(uint32_t)obj->debug_aranges_addr_off, reloc_dbgaranges_cnt},
        };
        for (int i = 0; i < 3; i++) {
            if (!dr[i].cnt)
                continue;
            w32(f, dr[i].addr);
            w32(f, dbg_pack);
        }
    }

    // Custom obj->extra_secs[] sections' own relocations, one contiguous
    // block per section (matches reloc_extra_off[] computed during layout).
    for (int i = 0; i < obj->extra_sec_count; i++) {
        ExtraSection *es = &obj->extra_secs[i];
        for (int j = 0; j < es->reloc_count; j++)
            write_macho_reloc(f, &es->relocs[j], obj, sym_map, is_arm64, extra_ord);
    }

    // Symbol table (nlist_64, 16 bytes each)
    wzeros(f, symtab_off - reloc_extra_end);
    for (int i = 0; i < nsyms; i++) {
        w32(f, nlist[i].strx);
        w8(f, nlist[i].type);
        w8(f, nlist[i].sect);
        w16(f, nlist[i].desc);
        w64(f, nlist[i].value);
    }

    // String table
    wbuf(f, mst.data, mst.len);

    fclose(f);
    free(sym_map);
    free(nlist);
    free(mst.data);
    free(extra_ord);
    free(extra_off);
    free(reloc_extra_off);
    return 0;
}
#endif /* __APPLE__ */
