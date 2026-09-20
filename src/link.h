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
             const char *libs, bool opt_pie, bool opt_pic, bool opt_shared,
             bool opt_static, bool opt_export_dynamic);

// ---------------------------------------------------------------------------
// Linker-option classification, shared between main.c's native-vs-
// external-linker gate and link_elf.c's actual option handling so the
// two can never disagree about what the native ELF linker understands.
// ---------------------------------------------------------------------------

// Every -Wl,<opt>/-nodefaultlibs/-nostdlib/-r option this driver forwards
// to the linker, classified by link_parse_opts(). Fixed-size buffers so
// callers never need to free anything.
typedef struct {
    bool nodefaultlibs; // -nodefaultlibs
    bool nostdlib; // -nostdlib
    bool relocatable; // -r
    bool no_undefined; // -Wl,--no-undefined or -Wl,-z,defs
    bool muldefs; // -Wl,--allow-multiple-definition or -Wl,-z,muldefs
    bool bind_now; // -Wl,-z,now
    bool z_origin; // -Wl,-z,origin
    bool noexecstack; // -Wl,-z,noexecstack
    bool execstack; // -Wl,-z,execstack
    bool version_probe; // -Wl,-v: print a linker-version banner (muon/meson probe)
    bool have_soname;
    char soname[256]; // -Wl,-soname,<name> / -Wl,-h,<name> (last one wins)
    bool have_rpath;
    char rpath[1024]; // -Wl,-rpath,<dir> / -Wl,--rpath,<dir> (colon-joined, repeatable)
} LinkOpts;

// Scan every linker option in `libs` (bare -nodefaultlibs/-nostdlib/-r,
// and every comma sub-option of each -Wl,<a,b,c> group) and classify them
// into `*out`. `*out` is always fully populated. Returns true when every
// option found is one link_elf.c's native ELF linker knows how to honor;
// false when `libs` contains something outside that vocabulary (an
// unrecognized -Wl, sub-option, -Wl,-rpath with no value, ...), in which
// case main.c's driver must fall back to the external linker instead of
// attempting a native link -- `*out` must not be acted on in that case.
bool link_parse_opts(const char *libs, LinkOpts *out);

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
#define RL_ADDR32NB     18  // PE: IMAGE_REL_AMD64/ARM64_ADDR32NB (RVA-relative)

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

#define STB_LOCAL 0

typedef struct LinkSym LinkSym;
struct LinkSym {
    char *name; // symbol name (owned)
    int sec; // output section index, or -1 for undefined
    uint64_t value; // offset within section, or 0 for undefined
    uint64_t size; // symbol size
    int bind; // 0 local, 1 global, 2 weak
    int type; // 0 notype, 1 object, 2 func
    int visibility; // ELF st_other low bits: STV_DEFAULT/HIDDEN/...
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
    bool opt_shared;
    bool opt_export_dynamic;
    // -r: produce a partial-linked ET_REL object (link_elf_relocatable()
    // in link_elf.c) instead of an executable/shared object. Set from
    // `libs` by link_state_init() via link_parse_opts() -- needed before
    // any object is loaded, since it changes how elf_load_object()
    // records relocations (see its own comment).
    bool opt_relocatable;
    // -Wl,--allow-multiple-definition / -Wl,-z,muldefs: a second STRONG
    // definition of a global symbol keeps the first instead of failing
    // the link (see link_add_sym()).
    bool opt_muldefs;
    // ELF only: whether any loaded object requested (SHF_EXECINSTR on its
    // .note.GNU-stack section) or implicitly requires (section absent) an
    // executable stack.  Drives the output PT_GNU_STACK flags in link_elf.c.
    bool stack_note_exec;
    bool stack_note_missing;
    const char *libs; // -l and other linker flags

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
void link_state_init(LinkState *s, LinkArch arch, const char *out_path,
                     bool opt_static, bool opt_pie, bool opt_shared,
                     bool opt_export_dynamic, const char *libs);
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
// `image_base` is only meaningful for PE's RL_ADDR32NB (RVA-relative);
// ELF/Mach-O callers pass 0.
void link_apply_relocs(LinkState *s, uint64_t image_base);

// Architecture-specific relocation encoder (implemented in link.c).
void link_reloc_apply(LinkArch arch, LinkSec *sec, LinkReloc *r,
                      uint64_t sym_addr, uint64_t pc, uint64_t image_base);

// TODO Search a library name in the configured paths and load it.
// See arch-specific implementations
// int link_load_archive(LinkState *s, const char *name, const char *lib_paths);

// Format-specific object loader.  Implemented in the active link_*.c file.
int link_load_object(LinkState *s, const char *path);

// Backend entry points.
int link_elf(LinkState *s);
int link_macho(LinkState *s);
int link_pe(LinkState *s);

// Write a Windows import library (a `!<arch>\n` ar archive of COFF
// "short import" members, one per DLL export) at `implib_path`, derived
// from `dll_path`'s own PE export table (see -Wl,--out-implib).
// Windows/mingw-only (implemented in link_pe.c, only compiled for that
// target); returns 0 on success, -1 if `dll_path` has no usable export
// table.
int pe_write_out_implib(const char *dll_path, const char *implib_path);

#endif // LINK_H
