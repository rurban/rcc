// SPDX-License-Identifier: LGPL-2.1-or-later
// Object file emission: sections, symbols, relocations.
// Architecture-independent layer used by asm.c, elf_write.c, macho_write.c.
#ifndef OBJ_H
#define OBJ_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Growable byte buffer
// ---------------------------------------------------------------------------
typedef struct SecBuf SecBuf;
struct SecBuf {
    uint8_t *data;
    size_t len;
    size_t cap;
};

void secbuf_init(SecBuf *s);
void secbuf_free(SecBuf *s);
void secbuf_reserve(SecBuf *s, size_t extra);
size_t secbuf_emit8(SecBuf *s, uint8_t v);
size_t secbuf_emit16le(SecBuf *s, uint16_t v);
size_t secbuf_emit32le(SecBuf *s, uint32_t v);
size_t secbuf_emit64le(SecBuf *s, uint64_t v);
void secbuf_emitbuf(SecBuf *s, const void *buf, size_t n);
void secbuf_align(SecBuf *s, int align); // pad with zeros
// Patch a previously emitted 32-bit value at byte offset `off`
void secbuf_patch32le(SecBuf *s, size_t off, uint32_t v);
void secbuf_patch64le(SecBuf *s, size_t off, uint64_t v);

// ---------------------------------------------------------------------------
// Section indices (logical IDs, mapped to ELF/Mach-O section indices later)
// ---------------------------------------------------------------------------
#define SEC_UNDEF      -1 // undefined / external symbol
#define SEC_TEXT        0
#define SEC_DATA        1
#define SEC_BSS         2
#define SEC_RODATA      3
#define SEC_INIT_ARRAY  4
#define SEC_FINI_ARRAY  5
#define SEC_TDATA       6
#define SEC_THREAD_VARS  7
#define SEC_NUM         8 // number of sections

// ---------------------------------------------------------------------------
// Symbol table
// ---------------------------------------------------------------------------
typedef enum { SB_LOCAL = 0,
               SB_GLOBAL = 1,
               SB_WEAK = 2 } SymBind;
typedef enum { ST_NOTYPE = 0,
               ST_OBJECT = 1,
               ST_FUNC = 2,
               ST_SECTION = 3,
               ST_TLS = 6 } SymType;

typedef struct ObjSym ObjSym;
struct ObjSym {
    char *name;
    int section; // SEC_TEXT / SEC_DATA / SEC_BSS / SEC_UNDEF
    uint64_t offset; // byte offset within section (0 for SEC_UNDEF)
    uint64_t size;
    SymBind bind;
    SymType type;
};

// ---------------------------------------------------------------------------
// Relocation entry
// ---------------------------------------------------------------------------
typedef struct ObjReloc ObjReloc;
struct ObjReloc {
    uint64_t offset; // byte offset in section where reloc applies
    int sym_idx; // index into ObjFile.syms[]
    uint32_t type; // platform reloc type (R_AARCH64_* or R_X86_64_*)
    int64_t addend;
};

// ELF AArch64 relocation types
#define R_AARCH64_NONE                     0
#define R_AARCH64_ABS64                  257
#define R_AARCH64_ABS32                  258
#define R_AARCH64_PREL32                 261
#define R_AARCH64_JUMP26                 282
#define R_AARCH64_CALL26                 283
#define R_AARCH64_ADR_PREL_PG_HI21       275
#define R_AARCH64_ADD_ABS_LO12_NC        277
#define R_AARCH64_LDST64_ABS_LO12_NC     286
#define R_AARCH64_LDST32_ABS_LO12_NC     285
#define R_AARCH64_LDST16_ABS_LO12_NC     284
#define R_AARCH64_LDST8_ABS_LO12_NC      278
#define R_AARCH64_ADR_GOT_PAGE           311
#define R_AARCH64_LD64_GOT_LO12_NC       312
#define R_AARCH64_TLSLE_ADD_TPREL_HI12 549
#define R_AARCH64_TLSLE_ADD_TPREL_LO12 550

// ELF x86-64 relocation types
#define R_X86_64_NONE       0
#define R_X86_64_64         1
#define R_X86_64_PC32       2
#define R_X86_64_GOT32      3
#define R_X86_64_PLT32      4
#define R_X86_64_32        10
#define R_X86_64_32S       11
#define R_X86_64_TPOFF32   23
#define R_X86_64_GOTPCREL   9
#define R_X86_64_PC64      24

// Fixup kinds (used internally by the assembler, defined in asm.c)
#define FIXUP_REL32      1
#define FIXUP_ABS64      2
#define FIXUP_ARM64_B26  3
#define FIXUP_ARM64_B19  4

// ---------------------------------------------------------------------------
// Win64 SEH unwind info (x86-64 only). Captured during codegen, emitted by
// coff_write as .pdata (RUNTIME_FUNCTION) and .xdata (UNWIND_INFO). Without
// it RtlUnwindEx (used by longjmp and exceptions) cannot walk past a frame:
// Wine merely logs a warning, but real Windows crashes.
// ---------------------------------------------------------------------------
#define UWOP_PUSH_NONVOL 0 // OpInfo = register number; rsp += 8
#define UWOP_ALLOC_LARGE 1 // OpInfo=0: extra 16-bit slot = size/8; OpInfo=1: 2 slots = size
#define UWOP_ALLOC_SMALL 2 // OpInfo = size/8 - 1 (size 8..128)
#define UWOP_SET_FPREG   3 // establish frame pointer

typedef struct UnwindCode {
    uint8_t code_offset; // prolog offset just past the described instruction
    uint8_t op; // UWOP_* (stored in the low nibble on emit)
    uint8_t info; // OpInfo: register number or alloc-size code
    uint16_t extra[2]; // extra size slots for UWOP_ALLOC_LARGE
    int extra_count; // 0, 1, or 2
} UnwindCode;

typedef struct UnwindEntry {
    uint32_t func_start; // .text offset of function start
    uint32_t func_end; // .text offset one past the last byte
    uint8_t prolog_size; // bytes in prolog (through .seh_endprologue)
    uint8_t frame_register; // 0 = none, else nonvolatile register used as FP (for UNWIND_INFO)
    UnwindCode codes[16]; // captured in prolog order, reversed at emit time
    int code_count; // number of UnwindCode entries (not counting extra slots)
} UnwindEntry;

// ---------------------------------------------------------------------------
// Debug info entries (for -g / DWARF)
// ---------------------------------------------------------------------------
#define MAX_DEBUG_FILES 128
typedef struct {
    uint64_t text_offset; // offset in .text section
    uint32_t file_idx;
    uint32_t line;
} DebugLineEntry;

// ---------------------------------------------------------------------------
// Complete assembled object
// ---------------------------------------------------------------------------
typedef struct ObjFile ObjFile;
struct ObjFile {
    SecBuf text;
    SecBuf data;
    SecBuf rodata;
    SecBuf init_array;
    SecBuf fini_array;
    SecBuf data_tls; // .tdata — initialized TLS data
    size_t bss_size; // .bss is zero-initialized; just track its size
    SecBuf thread_vars; // __thread_vars — TLV descriptors
    ObjSym *syms;
    int sym_count;
    int sym_cap;

    ObjReloc *text_relocs;
    int text_reloc_count;
    int text_reloc_cap;

    ObjReloc *data_relocs;
    int data_reloc_count;
    int data_reloc_cap;

    ObjReloc *rodata_relocs;
    int rodata_reloc_count;
    int rodata_reloc_cap;

    ObjReloc *init_array_relocs;
    int init_array_reloc_count;
    int init_array_reloc_cap;

    ObjReloc *fini_array_relocs;
    int fini_array_reloc_count;
    int fini_array_reloc_cap;

    ObjReloc *data_tls_relocs;
    int data_tls_reloc_count;
    int data_tls_reloc_cap;


    ObjReloc *thread_vars_relocs;
    int thread_vars_reloc_count;
    int thread_vars_reloc_cap;
    // Win64 SEH unwind entries (one per emitted function); only populated on
    // _WIN32 x86-64 codegen, ignored by elf_write/macho_write.
    UnwindEntry *unwind;
    int unwind_count;
    int unwind_cap;
    // Debug info (for -g DWARF)
    char *debug_files[MAX_DEBUG_FILES];
    int debug_file_count;
    DebugLineEntry *debug_lines;
    int debug_line_count;
    int debug_line_cap;
    SecBuf debug_line_section; // serialized DWARF .debug_line
    SecBuf debug_info_section; // minimal DWARF .debug_info
    SecBuf debug_abbrev_section; // DWARF .debug_abbrev
    SecBuf debug_aranges_section; // DWARF .debug_aranges (address range lookup)
    size_t debug_low_pc_offset; // offset of DW_AT_low_pc in .debug_info (needs reloc)
    bool debug_has_low_pc; // true if DW_AT_low_pc needs R_X86_64_64 vs .text
    size_t debug_aranges_addr_off; // offset of address in .debug_aranges (needs reloc)
    bool debug_has_aranges_addr; // true if .debug_aranges address needs reloc
    size_t debug_line_addr_off; // offset of DW_LNE_set_address value in .debug_line
    bool debug_has_line_addr; // true if DW_LNE_set_address needs reloc
};

// Debug info helpers (for -g)
int objfile_add_debug_file(ObjFile *obj, char *filename);
void objfile_add_debug_line(ObjFile *obj, uint64_t text_offset, int file_idx, int line);
void objfile_flush_debug_line(ObjFile *obj, uint64_t text_end);
bool objfile_has_debug(ObjFile *obj);

void objfile_init(ObjFile *obj);
void objfile_free(ObjFile *obj);

// Add a symbol (returns its index).  Duplicate names with the same section
// and offset are merged.
int objfile_add_sym(ObjFile *obj, const char *name, int section,
                    uint64_t offset, uint64_t size, SymBind bind, SymType type);
int objfile_find_sym(ObjFile *obj, const char *name);

void objfile_add_reloc(ObjFile *obj, int section, uint64_t offset,
                       int sym_idx, uint32_t type, int64_t addend);

// Append a fresh zeroed Win64 unwind entry and return a pointer to it.
UnwindEntry *objfile_add_unwind(ObjFile *obj);

// Object file writers
int elf_write(ObjFile *obj, const char *path);
int macho_write(ObjFile *obj, const char *path);
int coff_write(ObjFile *obj, const char *path);

#endif // OBJ_H
