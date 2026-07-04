// SPDX-License-Identifier: LGPL-2.1-or-later
// Native linker for rcc.
// Links rcc-generated object files (and static archives) into executables
// without invoking an external toolchain.  Supports ELF64 (Linux),
// Mach-O 64 (macOS), and PE/COFF (Windows/MinGW).
#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Public driver API
// ---------------------------------------------------------------------------

// Link `n_objs` object files into an executable at `out_path`.
// `libs` is a string containing linker options copied from the command line
// (e.g. " -lm -lpthread -L/path -lfoo").  `opt_pie`/`opt_pic`/`opt_static`
// reflect the corresponding driver flags.
//
// Returns 0 on success, non-zero on error (an error message is printed to
// stderr).
int rcc_link(const char *out_path, char **obj_paths, int n_objs,
             const char *libs, bool opt_pie, bool opt_pic, bool opt_static);

// ---------------------------------------------------------------------------
// Shared relocation kinds used by all backends internally.
// Each backend converts its native relocation type to one of these.
// ---------------------------------------------------------------------------
#define RL_ABS64        1   // 64-bit absolute address
#define RL_ABS32        2   // 32-bit absolute address (signed)
#define RL_ABS32U       3   // 32-bit absolute address (unsigned)
#define RL_PC32         4   // 32-bit PC-relative
#define RL_PC32_PLT     5   // 32-bit PC-relative via PLT
#define RL_PC64         6   // 64-bit PC-relative
#define RL_GOTPCREL     7   // x86_64: R_X86_64_GOTPCREL
#define RL_TPOFF32      8   // x86_64: R_X86_64_TPOFF32
#define RL_ARM64_B26    9   // AArch64 unconditional branch 26-bit
#define RL_ARM64_ADR_PG 10  // AArch64 ADR_PREL_PG_HI21
#define RL_ARM64_ADD_LO 11  // AArch64 ADD_ABS_LO12_NC
#define RL_ARM64_GOT_PG 12  // AArch64 ADR_GOT_PAGE
#define RL_ARM64_GOT_LO 13  // AArch64 LD64_GOT_LO12_NC
#define RL_ARM64_TLSDESC_HI 14
#define RL_ARM64_TLSDESC_LO 15
#define RL_ARM64_TPREL_HI   16
#define RL_ARM64_TPREL_LO   17

// Architecture of the link session.
typedef enum {
    ARCH_X86_64,
    ARCH_AARCH64,
} LinkArch;

// ---------------------------------------------------------------------------
// Internal shared types (also exposed for backend use)
// ---------------------------------------------------------------------------

typedef struct LinkReloc LinkReloc;
struct LinkReloc {
    uint64_t offset; // offset within the output section
    uint32_t type; // RL_* kind
    int sym; // index into LinkState.syms
    int64_t addend;
};

typedef struct LinkSym LinkSym;
struct LinkSym {
    char *name; // symbol name (owned)
    int sec; // output section index, or -1 for undefined
    uint64_t value; // offset within section, or 0 for undefined
    uint64_t size; // symbol size
    int bind; // 0 local, 1 global, 2 weak
    int type; // 0 notype, 1 object, 2 func
    int src_obj; // source object index (for diagnostics)
    int hash_next; // next symbol in hash bucket (internal use)
    bool resolved; // true if this symbol has a final value
};

typedef struct LinkSec LinkSec;
struct LinkSec {
    char *name; // section name (owned)
    uint8_t *data; // section contents (owned, NULL for bss)
    size_t len; // size in file / memory for bss
    size_t cap; // allocated capacity of data
    size_t align; // alignment requirement (power of two)
    uint64_t addr; // virtual address after layout
    uint64_t fileoff; // file offset after layout
    bool alloc; // loaded into memory
    bool write; // writable
    bool exec; // executable
    bool is_bss; // zero-initialized
    bool is_tls; // thread-local
    LinkReloc *relocs; // relocations targeting this section
    int n_relocs;
    int cap_relocs;
};

typedef struct LinkObj LinkObj;
struct LinkObj {
    char *path; // source path
    uint8_t *image; // mmap/file image (owned)
    size_t image_size;
};

typedef struct LinkState LinkState;
struct LinkState {
    LinkArch arch;
    const char *out_path;
    bool opt_static;
    bool opt_pie;

    LinkSec *secs;
    int n_secs;
    int cap_secs;

    LinkSym *syms;
    int n_syms;
    int cap_syms;

    LinkObj *objs;
    int n_objs;
    int cap_objs;

    // Map from symbol name to LinkSym index.  Separate chaining is handled
    // by reusing the symbol table with next-in-bucket indices.
    int *sym_hash;
    int sym_hash_cap;
};

// ---------------------------------------------------------------------------
// Shared helpers (implemented in link.c)
// ---------------------------------------------------------------------------

void link_state_init(LinkState *s, LinkArch arch, const char *out_path,
                     bool opt_static, bool opt_pie);
void link_state_free(LinkState *s);

// Find or create an output section by name.  Returns section index.
int link_find_or_create_sec(LinkState *s, const char *name, bool alloc,
                            bool write, bool exec, bool is_bss, bool is_tls,
                            size_t align);

// Add data to the end of a section, returning the offset where it was placed.
uint64_t link_sec_append(LinkState *s, int sec_idx, const uint8_t *data,
                         size_t len, size_t align);

// Add a relocation targeting section `sec_idx`.
void link_add_reloc(LinkState *s, int sec_idx, uint64_t offset, uint32_t type,
                    int sym, int64_t addend);

// Symbol table.  Returns symbol index.  For global/weak symbols, if a symbol
// with the same name already exists, the existing index is returned and the
// new definition is merged according to ELF-like rules (strong overrides weak,
// defined overrides undefined, duplicate defined globals are an error).
int link_add_sym(LinkState *s, const char *name, int sec, uint64_t value,
                 uint64_t size, int bind, int type, int src_obj);
int link_find_sym(LinkState *s, const char *name);
LinkSym *link_get_sym(LinkState *s, int idx);

// Layout: assign addresses and file offsets.  `base` is the preferred load
// address (0x400000 for x86_64 ELF, 0x100000000 for AArch64).  Region
// boundaries are aligned to `page_align` (use 0 or 1 to disable).
int link_layout(LinkState *s, uint64_t base, uint64_t page_align);

// Apply all recorded relocations.  `s1->secs[].addr` must be valid.
void link_apply_relocs(LinkState *s);

// Architecture-specific relocation encoder (implemented in link.c).
void link_reloc_apply(LinkArch arch, LinkSec *sec, LinkReloc *r,
                      uint64_t sym_addr, uint64_t pc);

// Search a library name in the configured paths and load it.
int link_load_archive(LinkState *s, const char *name, const char *lib_paths);

// Format-specific object loader.  Implemented in the active link_*.c file.
int link_load_object(LinkState *s, const char *path);

// Backend entry points.
int link_elf(LinkState *s);
int link_macho(LinkState *s);
int link_pe(LinkState *s);

#endif // LINK_H
