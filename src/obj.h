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
#define SEC_UNDEF      -1  // undefined / external symbol
#define SEC_TEXT        0
#define SEC_DATA        1
#define SEC_BSS         2
#define SEC_RODATA      3
#define SEC_INIT_ARRAY  4
#define SEC_FINI_ARRAY  5
#define SEC_NUM         6  // number of sections

// ---------------------------------------------------------------------------
// Symbol table
// ---------------------------------------------------------------------------
typedef enum { SB_LOCAL = 0,
               SB_GLOBAL = 1,
               SB_WEAK = 2 } SymBind;
typedef enum { ST_NOTYPE = 0,
               ST_OBJECT = 1,
               ST_FUNC = 2,
               ST_SECTION = 3 } SymType;

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
#define R_AARCH64_LD64_GOT_LO12_NC       309

// ELF x86-64 relocation types
#define R_X86_64_NONE       0
#define R_X86_64_64         1
#define R_X86_64_PC32       2
#define R_X86_64_GOT32      3
#define R_X86_64_PLT32      4
#define R_X86_64_32        10
#define R_X86_64_32S       11
#define R_X86_64_GOTPCREL   9
#define R_X86_64_PC64      24

// Fixup kinds (used internally by the assembler, defined in asm.c)
#define FIXUP_REL32      1
#define FIXUP_ABS64      2
#define FIXUP_ARM64_B26  3
#define FIXUP_ARM64_B19  4

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
    size_t bss_size; // .bss is zero-initialized; just track its size

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
};

void objfile_init(ObjFile *obj);
void objfile_free(ObjFile *obj);

// Add a symbol (returns its index).  Duplicate names with the same section
// and offset are merged.
int objfile_add_sym(ObjFile *obj, const char *name, int section,
                    uint64_t offset, uint64_t size, SymBind bind, SymType type);
int objfile_find_sym(ObjFile *obj, const char *name);

void objfile_add_reloc(ObjFile *obj, int section, uint64_t offset,
                       int sym_idx, uint32_t type, int64_t addend);

// Object file writers
int elf_write(ObjFile *obj, const char *path);
int macho_write(ObjFile *obj, const char *path);
int coff_write(ObjFile *obj, const char *path);

#endif // OBJ_H
