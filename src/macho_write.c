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
#define S_CSTRING_LITERALS    0x2
#define S_ATTR_SOME_INSTRUCTIONS  0x00000400
#define S_ATTR_PURE_INSTRUCTIONS  0x80000000

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
#define ARM64_RELOC_POINTER_TO_GOT   7
#define ARM64_RELOC_ADDEND           10

// Nlist types
#define N_UNDF  0x0
#define N_ABS   0x2
#define N_SECT  0xe
#define N_EXT   0x01
#define N_PEXT  0x10
#define NO_SECT 0
#define REFERENCE_FLAG_UNDEFINED_NON_LAZY 0
#define REFERENCE_FLAG_UNDEFINED_LAZY 1

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
        default: return ARM64_RELOC_UNSIGNED;
        }
    } else {
        switch (elf_type) {
        case R_X86_64_64: return X86_64_RELOC_UNSIGNED;
        case R_X86_64_PLT32: return X86_64_RELOC_BRANCH;
        case R_X86_64_PC32: return X86_64_RELOC_SIGNED;
        case R_X86_64_32S: return X86_64_RELOC_SIGNED;
        case R_X86_64_32: return X86_64_RELOC_UNSIGNED;
        default: return X86_64_RELOC_UNSIGNED;
        }
    }
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
        sym_map[i] = nsyms;
        Nlist *n = &nlist[nsyms++];
        n->strx = mstrtab_add(&mst, os->name);
        n->type = (os->section == SEC_UNDEF) ? N_UNDF : N_SECT;
        n->sect = (os->section == SEC_TEXT) ? 1 : (os->section == SEC_DATA) ? 2
            : (os->section == SEC_BSS)                                      ? 3
            : (os->section == SEC_RODATA)                                   ? 4
                                                                            : NO_SECT;
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
        n->sect = (os->section == SEC_TEXT) ? 1 : (os->section == SEC_DATA) ? 2
            : (os->section == SEC_BSS)                                      ? 3
            : (os->section == SEC_RODATA)                                   ? 4
                                                                            : NO_SECT;
        n->desc = 0;
        n->value = os->offset;
    }
    int nextdefsym = nsyms - iextdefsym;
    int iundefsym = nsyms;

    // Undefined (external) symbols
    for (int i = 0; i < obj->sym_count; i++) {
        ObjSym *os = &obj->syms[i];
        if (os->section != SEC_UNDEF) continue;
        sym_map[i] = nsyms;
        Nlist *n = &nlist[nsyms++];
        n->strx = mstrtab_add(&mst, os->name);
        n->type = N_UNDF | N_EXT;
        n->sect = NO_SECT;
        n->desc = (uint16_t)REFERENCE_FLAG_UNDEFINED_NON_LAZY;
        n->value = 0;
    }
    int nundefsym = nsyms - iundefsym;

    // -----------------------------------------------------------------------
    // Decide which sections to include
    // -----------------------------------------------------------------------
    // Mach-O section indices are 1-based.
    // We always emit at least __TEXT,__text.
    // __DATA,__data, __DATA,__bss, __TEXT,__const (rodata) as needed.
    // Mach-O .o has one segment "" (empty name) containing all sections.
    // Sections: 1=__text, 2=__data, 3=__bss, 4=__const (rodata)
    int nsections = 4;

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

    // Relocations follow section data
    uint64_t reloc_text_off = align(rodata_off + rodata_size, 4);
    uint32_t reloc_text_cnt = (uint32_t)obj->text_reloc_count;
    uint64_t reloc_data_off = reloc_text_off + reloc_text_cnt * 8;
    uint32_t reloc_data_cnt = (uint32_t)obj->data_reloc_count;
    uint64_t reloc_rod_off = reloc_data_off + reloc_data_cnt * 8;
    uint32_t reloc_rod_cnt = (uint32_t)obj->rodata_reloc_count;

    uint64_t symtab_off = align(reloc_rod_off + reloc_rod_cnt * 8, 8);
    uint32_t symtab_size = (uint32_t)nsyms * 16; // sizeof nlist_64 = 16
    uint64_t strtab_off = symtab_off + symtab_size;
    uint32_t strtab_size = (uint32_t)mst.len;

    // segment vm size = sum of all section sizes
    uint64_t seg_vmsize = text_size + data_size + obj->bss_size + rodata_size;
    // file size of segment = up to end of rodata
    uint64_t seg_filesize = (rodata_off + rodata_size) - text_off;

    // -----------------------------------------------------------------------
    // Write file
    // -----------------------------------------------------------------------
    FILE *f = fopen(path, "wb");
    if (!f) {
        free(sym_map);
        free(nlist);
        free(mst.data);
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

    // Relocations (Mach-O relocation_info: 8 bytes each)
    wzeros(f, reloc_text_off - (rodata_off + rodata_size));
    for (int i = 0; i < obj->text_reloc_count; i++) {
        ObjReloc *r = &obj->text_relocs[i];
        bool ext = r->sym_idx >= 0 && obj->syms[r->sym_idx].section == SEC_UNDEF;
        uint8_t mtype = elf_reloc_to_macho(r->type, is_arm64);
        bool pcrel = (r->type == R_X86_64_PC32 || r->type == R_X86_64_PLT32 ||
                      r->type == R_AARCH64_CALL26 || r->type == R_AARCH64_JUMP26 ||
                      r->type == R_AARCH64_ADR_PREL_PG_HI21 || r->type == R_AARCH64_ADR_GOT_PAGE);
        uint32_t sym_num;
        // ARM64 relocs must use r_extern=1 (symbol index), not section ordinals.
        // ld64 rejects r_extern=0 for branch/page/pcrel types.
        if (is_arm64) {
            ext = true;
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        } else if (!ext && r->sym_idx >= 0) {
            // Local relocation: symbolnum = section ordinal of target
            sym_num = obj_section_to_macho(obj->syms[r->sym_idx].section);
        } else {
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        }
        // r_address (4), then packed field (symbolnum:24, pcrel:1, length:2, ext:1, type:4)
        w32(f, (uint32_t)r->offset);
        uint32_t pack = (sym_num & 0xffffff) |
            ((uint32_t)pcrel << 24) |
            (2 << 25) |
            ((uint32_t)(ext ? 1 : 0) << 27) |
            ((uint32_t)mtype << 28);
        w32(f, pack);
    }
    for (int i = 0; i < obj->data_reloc_count; i++) {
        ObjReloc *r = &obj->data_relocs[i];
        bool ext = (bool)(r->sym_idx >= 0 && obj->syms[r->sym_idx].section == SEC_UNDEF);
        uint8_t mtype = elf_reloc_to_macho(r->type, is_arm64);
        uint32_t sym_num;
        if (is_arm64) {
            ext = true;
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        } else if (!ext && r->sym_idx >= 0) {
            sym_num = obj_section_to_macho(obj->syms[r->sym_idx].section);
        } else {
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        }
        w32(f, (uint32_t)r->offset);
        uint32_t pack = (sym_num & 0xffffff) | (3 << 25) |
            ((uint32_t)(ext ? 1 : 0) << 27) | ((uint32_t)mtype << 28);
        w32(f, pack);
    }
    for (int i = 0; i < obj->rodata_reloc_count; i++) {
        ObjReloc *r = &obj->rodata_relocs[i];
        bool ext = (bool)(r->sym_idx >= 0 && obj->syms[r->sym_idx].section == SEC_UNDEF);
        uint8_t mtype = elf_reloc_to_macho(r->type, is_arm64);
        uint32_t sym_num;
        if (is_arm64) {
            ext = true;
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        } else if (!ext && r->sym_idx >= 0) {
            sym_num = obj_section_to_macho(obj->syms[r->sym_idx].section);
        } else {
            sym_num = (uint32_t)(r->sym_idx >= 0 ? sym_map[r->sym_idx] : 0);
        }
        w32(f, (uint32_t)r->offset);
        uint32_t pack = (sym_num & 0xffffff) | (3 << 25) |
            ((uint32_t)(ext ? 1 : 0) << 27) | ((uint32_t)mtype << 28);
        w32(f, pack);
    }

    // Symbol table (nlist_64, 16 bytes each)
    wzeros(f, symtab_off - (reloc_rod_off + reloc_rod_cnt * 8));
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
    return 0;
}
#endif /* __APPLE__ */
