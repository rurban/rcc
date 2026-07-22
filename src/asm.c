// SPDX-License-Identifier: LGPL-2.1-or-later
// Built-in assembler: parse rcc-generated .s text → ObjFile → ELF/Mach-O.
// Handles the exact subset of ARM64 and x86-64 assembly that rcc emits.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
// Auto-detect target architecture from host (must be before rcc.h or standalone)
#if defined(__aarch64__) && !defined(ARCH_ARM64)
#define ARCH_ARM64
#endif
#include "asm.h"
#include "obj.h"
#include "arm64_enc.h"
#include "x86_enc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
typedef struct {
    ObjFile *obj;
    int cur_sec; // current section: SEC_TEXT / SEC_DATA / SEC_BSS / SEC_RODATA / ...
    int lineno;
    const char *filename;

    // .pushsection/.popsection stack (GAS section switches nest a handful
    // of levels deep at most — the kernel's exception-table/fixup macros
    // never push more than one).
    int sec_stack[32];
    int sec_stack_depth;

    // Backpatching: forward label references
    // (max 512 pending fixups at once; sufficient for compiler output)
    struct Fixup {
        size_t patch_off; // byte offset in section buffer
        int section;
        char label[128];
        char label2[128]; // FIXUP_LABELDIFF only: patch = offset(label2) - offset(label)
        char label3[128]; // FIXUP_SKIP_MAXDIFF only: C in max(0,(A-B)-(C-D))
        char label4[128]; // FIXUP_SKIP_MAXDIFF only: D in max(0,(A-B)-(C-D))
        int kind; // FIXUP_ARM64_B26, FIXUP_ARM64_B19, FIXUP_REL32, FIXUP_LABELDIFF,
        // FIXUP_SKIP_MAXDIFF
        int size; // FIXUP_LABELDIFF only: patch field width in bytes (1/2/4/8)
        int fill_byte; // FIXUP_SKIP_MAXDIFF only: byte value to pad with
        int64_t addend;
    } fixups[512];
    int nfixups;

    // Local label map (for forward references: .Lxxx → offset)
    struct LocalSym {
        char name[128];
        int section;
        size_t offset;
        int sym_idx; // objfile symbol table index for this exact occurrence
        // (numeric labels like "1:" reuse the name, so lookups
        // by symbol *name* alone can't tell occurrences apart —
        // needed to reference the *right* one from another
        // section, e.g. `.long 1b - .` in __ex_table)
    } locals[2048];
    int nlocals;

    // GAS macro definitions (.macro/.endm), expanded by asm_macro_pass()
    // before the main line loop ever sees them. Kernel headers use this
    // (with .irp/.ifc/.set/.if) to compute e.g. which GP register an
    // exception-table entry's faulting instruction used.
    struct AsmMacro {
        char name[64];
        char params[8][32];
        int nparams;
        char **body; // owned copies of each raw body line
        int nbody;
    } macros[16];
    int nmacros;

    // Assembler-time integer variables (.set NAME, EXPR — GAS macro-time
    // scratch state, e.g. `.set .Lfound, 0` / `.set .Lfound, .Lfound+1`;
    // distinct from real object-file symbols).
    struct AsmVar {
        char name[64];
        int64_t value;
    } vars[64];
    int nvars;
} AsmState;

static void asm_error(AsmState *as, const char *msg) {
    fprintf(stderr, "%s:%d: asm error: %s\n",
            as->filename ? as->filename : "?", as->lineno, msg);
}

// Current section buffer
static SecBuf *cur_sec_buf(AsmState *as) {
    SecBuf *b = objfile_section_buf(as->obj, as->cur_sec);
    return b ? b : &as->obj->data; // BSS/unregistered handled specially
}

static size_t cur_off(AsmState *as) {
    if (as->cur_sec == SEC_BSS) return as->obj->bss_size;
    return cur_sec_buf(as)->len;
}

// ---------------------------------------------------------------------------
// Symbol management
// ---------------------------------------------------------------------------
static void define_label(AsmState *as, const char *name, bool is_global, bool is_weak,
                         bool is_func) {
    int sec = as->cur_sec;
    size_t off = cur_off(as);

    // Check if already in symbol table (global declaration first, then definition)
    int idx = objfile_find_sym(as->obj, name);
    if (idx < 0) {
        SymBind bind = is_weak ? SB_WEAK : (is_global ? SB_GLOBAL : SB_LOCAL);
        SymType type = is_func ? ST_FUNC : ST_OBJECT;
        idx = objfile_add_sym(as->obj, name, sec, off, 0, bind, type);
    } else {
        as->obj->syms[idx].section = sec;
        as->obj->syms[idx].offset = off;
        if (is_global && as->obj->syms[idx].bind == SB_LOCAL)
            as->obj->syms[idx].bind = is_weak ? SB_WEAK : SB_GLOBAL;
        if (is_func) as->obj->syms[idx].type = ST_FUNC;
    }

    // GAS numeric local labels ("1:") reuse the same name every time they
    // recur, so the shared symbol-table slot above only ever holds the
    // *latest* occurrence's value — fine for same-section jmp/jcc, which
    // resolve by (name, position) via the locals[] scan below, but useless
    // for a cross-section reference (e.g. `.long 1b - .` in the kernel's
    // __ex_table, referencing a label back in .text), which needs a real
    // relocation against a symbol that stays pinned to *this* occurrence.
    // Give every numeric-label definition its own private, uniquely-named
    // symbol for that purpose.
    bool is_numeric_label = *name != '\0';
    for (const char *p = name; *p; p++)
        if (!isdigit((unsigned char)*p)) {
            is_numeric_label = false;
            break;
        }
    int occurrence_idx = idx;
    if (is_numeric_label) {
        static int numeric_label_seq;
        char uniq[64];
        snprintf(uniq, sizeof(uniq), ".Lrcc_num%d.%d", atoi(name), numeric_label_seq++);
        occurrence_idx = objfile_add_sym(as->obj, uniq, sec, off, 0, SB_LOCAL, ST_NOTYPE);
    }

    // Also record as local sym for backpatching
    if (as->nlocals < 2047) {
        struct LocalSym *ls = &as->locals[as->nlocals++];
        strncpy(ls->name, name, sizeof(ls->name) - 1);
        ls->name[sizeof(ls->name) - 1] = '\0';
        ls->section = sec;
        ls->offset = off;
        ls->sym_idx = occurrence_idx;
    }

    // Resolve any pending fixups for this label
    for (int i = 0; i < as->nfixups; i++) {
        struct Fixup *fx = &as->fixups[i];
        // FIXUP_LABELDIFF (GAS "A - B" label-difference, e.g. ALTERNATIVE()'s
        // alt_rlen) and FIXUP_SKIP_MAXDIFF (the alt_rlen/alt_slen padding
        // computation) are both resolved once at the end of assemble_inline
        // once all their labels are known, not here — their `label`/etc.
        // fields are ordinary labels that may legitimately live in a
        // different section than whichever one happens to be defined
        // first, which isn't the "cross-section jump target" case below at
        // all (and FIXUP_SKIP_MAXDIFF's `label` isn't even a branch target
        // to patch — it's just the first operand of a length expression).
        if (fx->kind == FIXUP_LABELDIFF || fx->kind == FIXUP_SKIP_MAXDIFF) continue;
        if (strcmp(fx->label, name) != 0) continue;
        if (fx->section != sec) {
            // Cross-section jump/call target — e.g. a `jmp`/`jcc` inside
            // the kernel's throwaway .altinstr_aux-style out-of-line
            // section, branching back to the real function body in
            // .text. A same-section byte-patch can't express this (the
            // two sections may end up placed anywhere relative to each
            // other); only the linker can, via a real relocation — same
            // idea as the `(label) - .` case in the .long/.byte/...
            // directive handler.
#ifndef ARCH_ARM64
            if (fx->kind == FIXUP_REL32) {
                objfile_add_reloc(as->obj, fx->section, fx->patch_off,
                                  occurrence_idx, R_X86_64_PC32, fx->addend - 4);
            } else
#endif
                asm_error(as, "cross-section fixup");
            as->fixups[i] = as->fixups[--as->nfixups];
            i--;
            continue;
        }
        SecBuf *buf = cur_sec_buf(as);
        int64_t target = (int64_t)off + fx->addend;
        int64_t pc = (int64_t)fx->patch_off;
        switch (fx->kind) {
        case FIXUP_ARM64_B26: {
            int32_t delta = (int32_t)((target - pc) / 4);
            uint32_t old;
            memcpy(&old, buf->data + fx->patch_off, 4);
            old = (old & ~0x03ffffffu) | ((uint32_t)delta & 0x03ffffffu);
            secbuf_patch32le(buf, fx->patch_off, old);
            break;
        }
        case FIXUP_ARM64_B19: {
            int32_t delta = (int32_t)((target - pc) / 4);
            uint32_t old;
            memcpy(&old, buf->data + fx->patch_off, 4);
            old = (old & ~(0x7ffff << 5)) | (((uint32_t)delta & 0x7ffff) << 5);
            secbuf_patch32le(buf, fx->patch_off, old);
            break;
        }
        case FIXUP_REL32: {
            // 32-bit PC-relative: target - (patch_off + 4)
            int32_t delta = (int32_t)(target - (pc + 4));
            secbuf_patch32le(buf, fx->patch_off, (uint32_t)delta);
            break;
        }
        default: break;
        }
        // Remove this fixup
        as->fixups[i] = as->fixups[--as->nfixups];
        i--;
    }
}

// Add a fixup for a forward-referenced label
static void add_fixup(AsmState *as, size_t patch_off, int section,
                      const char *label, int kind, int64_t addend) {
    if (as->nfixups >= 511) {
        asm_error(as, "too many fixups");
        return;
    }
    struct Fixup *fx = &as->fixups[as->nfixups++];
    fx->patch_off = patch_off;
    fx->section = section;
    strncpy(fx->label, label, sizeof(fx->label) - 1);
    fx->label[sizeof(fx->label) - 1] = '\0';
    fx->kind = kind;
    fx->addend = addend;
}

// Add a fixup for "label2 - label" (GAS's label-difference idiom, e.g.
// the kernel's ALTERNATIVE() macro computing an instruction-block length
// as "775f-774f"): resolved once BOTH labels are defined, which for a
// well-formed input always happens by the end of this same assemble_inline
// call even when one or both are still forward references right now.
static void add_labeldiff_fixup(AsmState *as, size_t patch_off, int section,
                                const char *label, const char *label2, int size) {
    if (as->nfixups >= 511) {
        asm_error(as, "too many fixups");
        return;
    }
    struct Fixup *fx = &as->fixups[as->nfixups++];
    fx->patch_off = patch_off;
    fx->section = section;
    strncpy(fx->label, label, sizeof(fx->label) - 1);
    fx->label[sizeof(fx->label) - 1] = '\0';
    strncpy(fx->label2, label2, sizeof(fx->label2) - 1);
    fx->label2[sizeof(fx->label2) - 1] = '\0';
    fx->kind = FIXUP_LABELDIFF;
    fx->size = size;
    fx->addend = 0;
}

// Add a deferred FIXUP_SKIP_MAXDIFF: at end-of-buffer resolution, once all
// four labels are known, insert max(0,(offset(a)-offset(b))-(offset(c)-
// offset(d))) bytes of `fill` at patch_off, shifting everything already
// recorded after it in this section (see the resolution site for why this
// self-contained shift is safe rather than needing a full second pass).
// `locals_mark` is a snapshot of as->nlocals at the moment this .skip is
// seen — the boundary between labels defined *before* it (like the OLDINSTR
// "772:" that sits, numerically, at the exact same not-yet-inserted offset
// as the .skip itself, and must NOT move) and labels defined *after* it
// (like "773:", at that same numeric offset right now, that MUST move once
// the padding bytes actually go in). Offset alone can't tell those two
// apart when nothing has been inserted yet; chronological order (locals[]
// and fixups[] are both append-only, so array index == chronological
// order) can.
static void add_skip_maxdiff_fixup(AsmState *as, size_t patch_off, int section,
                                   const char *a, const char *b, const char *c,
                                   const char *d, int fill, int locals_mark) {
    if (as->nfixups >= 511) {
        asm_error(as, "too many fixups");
        return;
    }
    struct Fixup *fx = &as->fixups[as->nfixups++];
    fx->patch_off = patch_off;
    fx->section = section;
    strncpy(fx->label, a, sizeof(fx->label) - 1);
    fx->label[sizeof(fx->label) - 1] = '\0';
    strncpy(fx->label2, b, sizeof(fx->label2) - 1);
    fx->label2[sizeof(fx->label2) - 1] = '\0';
    strncpy(fx->label3, c, sizeof(fx->label3) - 1);
    fx->label3[sizeof(fx->label3) - 1] = '\0';
    strncpy(fx->label4, d, sizeof(fx->label4) - 1);
    fx->label4[sizeof(fx->label4) - 1] = '\0';
    fx->kind = FIXUP_SKIP_MAXDIFF;
    fx->fill_byte = fill;
    fx->addend = (int64_t)locals_mark;
}

// Look up a label offset (returns -1 if not found)
static int64_t lookup_local(AsmState *as, const char *name, int *sec_out) {
    for (int i = as->nlocals - 1; i >= 0; i--) {
        if (strcmp(as->locals[i].name, name) == 0) {
            if (sec_out) *sec_out = as->locals[i].section;
            return (int64_t)as->locals[i].offset;
        }
    }
    return -1;
}

// Nearest-prior ("Nb") resolution of a local label to its own private
// symbol-table entry, for use in relocations that must survive a
// cross-section reference (see define_label's occurrence_idx). Returns -1
// if not found — in particular for a forward ("Nf") reference, which
// define_label hasn't created an occurrence for yet.
static int lookup_local_sym(AsmState *as, const char *name, int *sec_out) {
    for (int i = as->nlocals - 1; i >= 0; i--) {
        if (strcmp(as->locals[i].name, name) == 0) {
            if (sec_out) *sec_out = as->locals[i].section;
            return as->locals[i].sym_idx;
        }
    }
    return -1;
}

// If `tok` is a GAS numeric local label reference ("1b"/"1f" — one or more
// digits plus a trailing b/f), strip the direction suffix in place so it
// matches the bare name define_label() records ("1"). No-op otherwise.
static void strip_local_label_suffix(char *tok) {
    size_t len = strlen(tok);
    if (len < 2 || (tok[len - 1] != 'b' && tok[len - 1] != 'f')) return;
    for (size_t i = 0; i < len - 1; i++)
        if (!isdigit((unsigned char)tok[i])) return;
    tok[len - 1] = '\0';
}

// Ensure a symbol is in the object's symbol table (for extern refs in relocs)
static int ensure_sym(AsmState *as, const char *name) {
    int idx = objfile_find_sym(as->obj, name);
    if (idx < 0)
        idx = objfile_add_sym(as->obj, name, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
    return idx;
}

// ---------------------------------------------------------------------------
// Text parsing helpers
// ---------------------------------------------------------------------------
static char *skip_ws(char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static char *trim_end(char *p) {
    int len = (int)strlen(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t' || p[len - 1] == '\n' || p[len - 1] == '\r')) {
        p[--len] = '\0';
    }
    return p;
}

// Match GAS's label-difference idiom "A - B" where both sides are bare
// label references (no other operators) — e.g. the kernel's ALTERNATIVE()
// macro computing an instruction-block length as "772b-771b" or
// "775f-774f" for a .byte/.long/.quad field. On success, NUL-terminates
// and returns the two operand substrings in place within `val`; on
// failure (anything fancier — parens, multiple operators, a trailing
// "- ." PC-relative reference, ...) leaves `val` untouched and returns
// false so the caller falls through to its existing handling.
static bool try_parse_label_diff(char *val, char **lbl_a, char **lbl_b) {
    char *dash = strchr(val, '-');
    if (!dash) return false;
    char *a = skip_ws(val);
    char *aend = dash;
    while (aend > a && isspace((unsigned char)aend[-1])) aend--;
    if (aend == a) return false;
    *aend = '\0';
    char *b = skip_ws(dash + 1);
    trim_end(b);
    if (!*a || !*b) return false;
    for (char *p = a; *p; p++)
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '.') return false;
    for (char *p = b; *p; p++)
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '.') return false;
    *lbl_a = a;
    *lbl_b = b;
    return true;
}

// Match the one fixed ".skip" shape the kernel's ALTERNATIVE() macro
// generates for replacement-vs-original padding:
//   -(((A-B)-(C-D)) > 0) * ((A-B)-(C-D)),<fill>
// i.e. max(0,(A-B)-(C-D)) bytes of <fill> — where A/B (the replacement's
// length) are typically forward references not yet defined at this point
// in the token stream (they live in a *later* .pushsection'd
// .altinstr_replacement block), so real evaluation has to wait. Rather
// than parse the arithmetic generally, this pulls out the (up to) four
// label-like tokens appearing before the final comma and the trailing
// fill-byte token; anything that doesn't fit — a different operator
// shape, a plain numeric count, more or fewer than four labels — falls
// through to the caller's existing plain-integer handling.
static bool try_parse_skip_maxdiff(const char *args, char lbl[4][128], int *fill_out) {
    const char *comma = strrchr(args, ',');
    if (!comma) return false;
    char fillbuf[64];
    const char *fp = skip_ws((char *)comma + 1);
    size_t flen = strlen(fp);
    if (flen == 0 || flen >= sizeof(fillbuf)) return false;
    strncpy(fillbuf, fp, sizeof(fillbuf) - 1);
    fillbuf[sizeof(fillbuf) - 1] = '\0';
    trim_end(fillbuf);
    char *fend;
    long fill = strtol(fillbuf, &fend, 0);
    if (*fend) return false;

    int n = 0;
    const char *p = args;
    while (*p && p < comma && n < 4) {
        if (isalnum((unsigned char)*p) || *p == '_' || *p == '.') {
            const char *start = p;
            while (isalnum((unsigned char)*p) || *p == '_' || *p == '.') p++;
            size_t len = (size_t)(p - start);
            if (len >= 128) return false;
            memcpy(lbl[n], start, len);
            lbl[n][len] = '\0';
            n++;
        } else {
            p++;
        }
    }
    if (n != 4) return false;
    *fill_out = (int)fill;
    return true;
}

// Parse a comma-separated operand list. Returns count, fills ops[].
// Each ops[i] points into a copy of the operand string (null-terminated).
static int split_operands(char *line, char **ops, int max_ops) {
    int n = 0;
    char *p = line;
    while (*p && n < max_ops) {
        p = skip_ws(p);
        if (!*p) break;
        ops[n++] = p;
        // Find next comma, but skip over brackets and quoted strings
        int depth = 0;
        while (*p) {
            if (*p == '[' || *p == '(') depth++;
            else if (*p == ']' || *p == ')')
                depth--;
            else if (*p == ',' && depth == 0) {
                *p++ = '\0';
                break;
            } else if (*p == '#' || *p == '$') { /* skip */
            }
            p++;
        }
    }
    // Trim each operand
    for (int i = 0; i < n; i++) {
        ops[i] = skip_ws(ops[i]);
        trim_end(ops[i]);
    }
    return n;
}

#ifdef ARCH_ARM64
// ---------------------------------------------------------------------------
// ARM64 register parsing
// ---------------------------------------------------------------------------
static int parse_arm64_reg(const char *s, bool *is32) {
    if (!s) return -1;
    s = skip_ws((char *)s);
    bool w = false;
    if (s[0] == 'x')
        w = false;
    else if (s[0] == 'w')
        w = true;
    else if (strcmp(s, "sp") == 0) {
        if (is32) *is32 = false;
        return 31;
    } else if (strcmp(s, "xzr") == 0) {
        if (is32) *is32 = false;
        return 31;
    } else if (strcmp(s, "wzr") == 0) {
        if (is32) *is32 = true;
        return 31;
    } else if (s[0] == 'd') {
        if (is32) *is32 = false;
        return atoi(s + 1);
    } else if (s[0] == 's') {
        if (is32) *is32 = true;
        return atoi(s + 1);
    } else
        return -1;
    if (is32) *is32 = w;
    if (strcmp(s + 1, "zr") == 0) return 31;
    if (strcmp(s + 1, "29") == 0) return 29;
    if (strcmp(s + 1, "30") == 0) return 30;
    return atoi(s + 1);
}

// Parse #imm or imm (no #) (arm64 only)
static int64_t parse_imm(const char *s) {
    if (!s) return 0;
    s = skip_ws((char *)s);
    if (*s == '#') s++;
    if (*s == '-') return -(int64_t)strtoull(s + 1, NULL, 0);
    return (int64_t)strtoull(s, NULL, 0);
}

// Parse ARM64 memory operand like "[x29, #-8]" or "[sp, #16]!" or "[sp], #16"
// Returns base register, fills *imm, *preindex, *postindex
static int parse_arm64_mem(const char *s, int64_t *imm, bool *pre, bool *post,
                           int *rn2) {
    if (!s) return -1;
    s = skip_ws((char *)s);
    *imm = 0;
    *pre = false;
    *post = false;
    if (rn2) *rn2 = -1;

    // Detect post-index: "[base], #off"
    char buf[256];
    strncpy(buf, s, 255);
    buf[255] = 0;
    char *close = strchr(buf, ']');
    if (close) {
        char *after = skip_ws(close + 1);
        if (*after == ',') {
            *post = true;
            *imm = parse_imm(skip_ws(after + 1));
        }
        if (*(close - 1) == '!') {
            *pre = true;
            *(close - 1) = 0;
        } else
            *close = 0;
    }
    char *inner = buf;
    if (*inner == '[') inner++;
    inner = skip_ws(inner);

    // Split base, optionally offset
    char *comma = strchr(inner, ',');
    if (comma) {
        *comma = 0;
        char *off_s = skip_ws(comma + 1);
        if (*off_s == 'x' || *off_s == 'w') {
            // Register offset
            if (rn2) *rn2 = parse_arm64_reg(off_s, NULL);
        } else if (!*post) {
            *imm = parse_imm(off_s);
        }
    }
    return parse_arm64_reg(inner, NULL);
}

// Extract relocation type from ":lo12:sym", ":got:sym", ":got_lo12:sym"
// Returns pointer to just the symbol name (modifies buf), sets *rel_type
static const char *parse_arm64_sym_reloc(const char *s, uint32_t *rel_type) {
    *rel_type = 0;
    if (s[0] == ':') {
        if (strncmp(s, ":lo12:", 6) == 0) {
            *rel_type = R_AARCH64_ADD_ABS_LO12_NC;
            return s + 6;
        }
        if (strncmp(s, ":got:", 5) == 0) {
            *rel_type = R_AARCH64_ADR_GOT_PAGE;
            return s + 5;
        }
        if (strncmp(s, ":got_lo12:", 10) == 0) {
            *rel_type = R_AARCH64_LD64_GOT_LO12_NC;
            return s + 10;
        }
    }
    // Darwin: sym@PAGE or sym@PAGEOFF
    char *at = strrchr(s, '@');
    if (at) {
        static char tmp[256];
        strncpy(tmp, s, sizeof(tmp) - 1);
        at = strrchr(tmp, '@');
        if (at) {
            *at = 0;
            if (strcmp(at + 1, "PAGE") == 0 || strcmp(at + 1, "GOTPAGE") == 0)
                *rel_type = R_AARCH64_ADR_PREL_PG_HI21;
            else if (strcmp(at + 1, "PAGEOFF") == 0 || strcmp(at + 1, "GOTPAGEOFF") == 0)
                *rel_type = R_AARCH64_ADD_ABS_LO12_NC;
            return tmp;
        }
    }
    return s;
}
#endif

#ifndef ARCH_ARM64
// ---------------------------------------------------------------------------
// x86-64 register parsing (AT&T names)
// ---------------------------------------------------------------------------
static X86Reg parse_x86_reg64(const char *s) {
    if (!s || *s != '%') return X86_NOREG;
    s++;
    if (!strcmp(s, "rax") || !strcmp(s, "eax") || !strcmp(s, "ax") || !strcmp(s, "al")) return X86_RAX;
    if (!strcmp(s, "rcx") || !strcmp(s, "ecx") || !strcmp(s, "cx") || !strcmp(s, "cl")) return X86_RCX;
    if (!strcmp(s, "rdx") || !strcmp(s, "edx") || !strcmp(s, "dx") || !strcmp(s, "dl")) return X86_RDX;
    if (!strcmp(s, "rbx") || !strcmp(s, "ebx") || !strcmp(s, "bx") || !strcmp(s, "bl")) return X86_RBX;
    if (!strcmp(s, "rsp") || !strcmp(s, "esp") || !strcmp(s, "sp") || !strcmp(s, "spl")) return X86_RSP;
    if (!strcmp(s, "rbp") || !strcmp(s, "ebp") || !strcmp(s, "bp") || !strcmp(s, "bpl")) return X86_RBP;
    if (!strcmp(s, "rsi") || !strcmp(s, "esi") || !strcmp(s, "si") || !strcmp(s, "sil")) return X86_RSI;
    if (!strcmp(s, "rdi") || !strcmp(s, "edi") || !strcmp(s, "di") || !strcmp(s, "dil")) return X86_RDI;
    if (!strcmp(s, "r8") || !strcmp(s, "r8d") || !strcmp(s, "r8w") || !strcmp(s, "r8b")) return X86_R8;
    if (!strcmp(s, "r9") || !strcmp(s, "r9d") || !strcmp(s, "r9w") || !strcmp(s, "r9b")) return X86_R9;
    if (!strcmp(s, "r10") || !strcmp(s, "r10d") || !strcmp(s, "r10w") || !strcmp(s, "r10b")) return X86_R10;
    if (!strcmp(s, "r11") || !strcmp(s, "r11d") || !strcmp(s, "r11w") || !strcmp(s, "r11b")) return X86_R11;
    if (!strcmp(s, "r12") || !strcmp(s, "r12d") || !strcmp(s, "r12w") || !strcmp(s, "r12b")) return X86_R12;
    if (!strcmp(s, "r13") || !strcmp(s, "r13d") || !strcmp(s, "r13w") || !strcmp(s, "r13b")) return X86_R13;
    if (!strcmp(s, "r14") || !strcmp(s, "r14d") || !strcmp(s, "r14w") || !strcmp(s, "r14b")) return X86_R14;
    if (!strcmp(s, "r15") || !strcmp(s, "r15w") || !strcmp(s, "r15b") || !strcmp(s, "r15")) return X86_R15;
    return X86_NOREG;
}

// Parse "%xmmN" -> X86_XMM0+N.  parse_x86_reg64 only knows GP registers.
static X86XmmReg parse_x86_xmm(const char *s) {
    if (!s || *s != '%') return X86_XMM0;
    s++;
    if (strncmp(s, "xmm", 3) != 0) return X86_XMM0;
    return (X86XmmReg)(X86_XMM0 + atoi(s + 3));
}

static int reg_size_x86(const char *s) {
    if (!s || *s != '%') return 8;
    s++;
    if (s[0] == 'r' && isdigit((unsigned char)s[1])) {
        // r8..r15: check suffix (e.g. r10d→4, r10w→2, r10b→1, r10→8)
        char *end = (char *)(s + 1);
        while (isdigit((unsigned char)*end)) end++;
        if (*end == 'd') return 4;
        if (*end == 'w') return 2;
        if (*end == 'b') return 1;
        return 8;
    }
    int n = strlen(s);
    if (n >= 2) {
        char last2 = s[n - 1];
        char first = s[0];
        if (first == 'r' && (s[1] == 'a' || s[1] == 'b' || s[1] == 'c' || s[1] == 'd' || s[1] == 's' || s[1] == 'd')) return 8;
        if (first == 'e') return 4;
        if (last2 == 'l' || last2 == 'h') return 1;
        if (last2 == 'x' && n == 2) return 2;
        if (last2 == 'p' || last2 == 'i') return 2; // sp, bp, si, di
    }
    return 8;
}

// Parse AT&T memory operand: disp(%base, %index, scale) or (%base)
// Returns true on success
static bool parse_x86_mem(const char *s, X86Mem *m) {
    m->base = X86_NOREG;
    m->index = X86_NOREG;
    m->scale = 1;
    m->disp = 0;

    char buf[256];
    strncpy(buf, s, 255);
    buf[255] = 0;
    char *paren = strchr(buf, '(');
    if (paren) {
        *paren = 0;
        if (buf[0]) {
            // Parse displacement (may be a symbol or integer)
            char *disp_s = skip_ws(buf);
            if (disp_s[0] == '-' || isdigit((unsigned char)disp_s[0]))
                m->disp = strtoll(disp_s, NULL, 0);
            // Symbol displacement handled separately
        }
        char *inner = paren + 1;
        char *close = strchr(inner, ')');
        if (close) *close = 0;

        char *parts[3] = {NULL, NULL, NULL};
        int np = 0;
        char *p = inner;
        while (*p && np < 3) {
            parts[np++] = skip_ws(p);
            char *c = strchr(p, ',');
            if (!c) break;
            *c = 0;
            p = c + 1;
        }
        if (np > 0) m->base = parse_x86_reg64(skip_ws(parts[0]));
        if (np > 1) m->index = parse_x86_reg64(skip_ws(parts[1]));
        if (np > 2) m->scale = (int)strtol(parts[2], NULL, 10);
        return true;
    }
    return false;
}
#endif // !ARM64

// ---------------------------------------------------------------------------
// Directive handling
// ---------------------------------------------------------------------------
// Parse `NAME[, "FLAGS"[, @TYPE[, ENTSIZE]]]` — the GAS .section/.pushsection
// argument grammar — and resolve/create the named section. Returns -1 if
// `args` has no usable name (caller should leave the current section alone).
static int parse_named_section(AsmState *as, const char *args) {
    char argbuf[300];
    strncpy(argbuf, args, sizeof(argbuf) - 1);
    argbuf[sizeof(argbuf) - 1] = 0;

    char *name = strtok(argbuf, ",");
    if (!name) return -1;
    name = skip_ws(name);
    trim_end(name);
    size_t nlen = strlen(name);
    if (nlen >= 2 && name[0] == '"' && name[nlen - 1] == '"') {
        name[nlen - 1] = '\0';
        name++;
    }
    if (!*name) return -1;

    uint32_t flags = 0;
    char *flagtok = strtok(NULL, ",");
    if (flagtok) {
        flagtok = skip_ws(flagtok);
        trim_end(flagtok);
        for (char *p = flagtok; *p; p++) {
            switch (*p) {
            case 'a': flags |= SHF_ALLOC; break;
            case 'w': flags |= SHF_WRITE; break;
            case 'x': flags |= SHF_EXECINSTR; break;
            case 'M': flags |= SHF_MERGE; break;
            case 'S': flags |= SHF_STRINGS; break;
            case 'T': flags |= SHF_TLS; break;
            default: break; // 'G' (group), 'o' (link-order), quotes: not needed here
            }
        }
    }
    strtok(NULL, ","); // @progbits/%progbits — always emitted as SHT_PROGBITS
    char *entsz_tok = strtok(NULL, ",");
    uint32_t entsize = 0;
    if (entsz_tok) {
        entsz_tok = skip_ws(entsz_tok);
        trim_end(entsz_tok);
        entsize = (uint32_t)strtol(entsz_tok, NULL, 0);
    }
    return objfile_find_or_add_section(as->obj, name, flags, entsize);
}

static void handle_directive(AsmState *as, const char *dir, char *args) {
    args = skip_ws(args);
    trim_end(args);

    if (!strcmp(dir, "text") || !strcmp(dir, "section__TEXT,__text") ||
        (strncmp(dir, "text", 4) == 0)) {
        as->cur_sec = SEC_TEXT;
    } else if (!strcmp(dir, "data") || !strcmp(dir, "section__DATA,__data")) {
        as->cur_sec = SEC_DATA;
    } else if (!strcmp(dir, "bss")) {
        as->cur_sec = SEC_BSS;
    } else if (!strcmp(dir, "rodata") || !strcmp(dir, "section.rodata")) {
        as->cur_sec = SEC_RODATA;
    } else if (!strncmp(dir, "section", 7)) {
        // .section .note.GNU-stack or similar — check for specific sections
        if (strstr(args, ".rodata") || strstr(args, "__const"))
            as->cur_sec = SEC_RODATA;
        else if (strstr(args, ".data"))
            as->cur_sec = SEC_DATA;
        else if (strstr(args, ".bss"))
            as->cur_sec = SEC_BSS;
        else if (strstr(args, ".text"))
            as->cur_sec = SEC_TEXT;
        else if (strstr(args, ".note.GNU-stack") || strstr(args, ".note.gnu"))
            ; // marker section elf_write always emits on its own; ignore
        else {
            int sec = parse_named_section(as, args);
            if (sec >= 0) as->cur_sec = sec;
        }
    } else if (!strcmp(dir, "pushsection")) {
        // .pushsection NAME[, "FLAGS"[, @TYPE[, ENTSIZE]]] — save the
        // current section and switch to (or create) the named one. Used by
        // e.g. the kernel's _ASM_EXTABLE macros to build __ex_table
        // entries without corrupting the surrounding .text stream.
        if (as->sec_stack_depth < (int)(sizeof(as->sec_stack) / sizeof(as->sec_stack[0])))
            as->sec_stack[as->sec_stack_depth++] = as->cur_sec;
        else
            asm_error(as, "pushsection stack overflow");
        if (strstr(args, ".rodata") || strstr(args, "__const"))
            as->cur_sec = SEC_RODATA;
        else if (strstr(args, ".data"))
            as->cur_sec = SEC_DATA;
        else if (strstr(args, ".bss"))
            as->cur_sec = SEC_BSS;
        else if (strstr(args, ".text"))
            as->cur_sec = SEC_TEXT;
        else {
            int sec = parse_named_section(as, args);
            if (sec >= 0) as->cur_sec = sec;
        }
    } else if (!strcmp(dir, "popsection")) {
        if (as->sec_stack_depth > 0)
            as->cur_sec = as->sec_stack[--as->sec_stack_depth];
        else
            asm_error(as, "popsection without pushsection");
    } else if (!strcmp(dir, "globl") || !strcmp(dir, "global")) {
        // Mark symbol as global (may not be defined yet)
        char *sym = args;
        trim_end(sym);
        int idx = objfile_find_sym(as->obj, sym);
        if (idx < 0)
            idx = objfile_add_sym(as->obj, sym, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
        else
            as->obj->syms[idx].bind = SB_GLOBAL;
    } else if (!strcmp(dir, "weak")) {
        char *sym = args;
        trim_end(sym);
        int idx = objfile_find_sym(as->obj, sym);
        if (idx < 0)
            objfile_add_sym(as->obj, sym, SEC_UNDEF, 0, 0, SB_WEAK, ST_NOTYPE);
        else
            as->obj->syms[idx].bind = SB_WEAK;
    } else if (!strcmp(dir, "type")) {
        // .type sym, @function / .type sym, %function
        char *sym = strtok(args, ",");
        if (!sym) return;
        char *kind = strtok(NULL, "");
        if (!kind) return;
        kind = skip_ws(kind);
        bool is_func = strstr(kind, "function") != NULL;
        int idx = objfile_find_sym(as->obj, sym);
        if (idx >= 0 && is_func) as->obj->syms[idx].type = ST_FUNC;
    } else if (!strcmp(dir, "size")) {
        // .size sym, .-sym (update symbol size)
        char *sym = strtok(args, ",");
        if (!sym) return;
        trim_end(sym);
        int idx = objfile_find_sym(as->obj, sym);
        if (idx >= 0 && as->obj->syms[idx].section == as->cur_sec) {
            size_t now = cur_off(as);
            as->obj->syms[idx].size = now - as->obj->syms[idx].offset;
        }
    } else if (!strcmp(dir, "set") || !strcmp(dir, "equiv")) {
        // .set alias, target
        char *alias = strtok(args, ",");
        if (!alias) return;
        char *target = strtok(NULL, "");
        if (!target) return;
        alias = skip_ws(alias);
        trim_end(alias);
        target = skip_ws(target);
        trim_end(target);
        // Create an alias symbol that points to the same location as target
        int tidx = ensure_sym(as, target);
        int aidx = objfile_find_sym(as->obj, alias);
        if (aidx < 0) {
            aidx = objfile_add_sym(as->obj, alias,
                                   as->obj->syms[tidx].section,
                                   as->obj->syms[tidx].offset,
                                   0, SB_GLOBAL, ST_NOTYPE);
        }
        (void)aidx;
    } else if (!strcmp(dir, "balign") || !strcmp(dir, "align") ||
               !strcmp(dir, "p2align")) {
        int a = atoi(args);
        if (!strcmp(dir, "p2align")) a = 1 << a;
        if (a > 1) {
            if (as->cur_sec == SEC_BSS) {
                size_t rem = as->obj->bss_size % (size_t)a;
                if (rem) as->obj->bss_size += (size_t)a - rem;
            } else {
                secbuf_align(cur_sec_buf(as), a);
            }
        }
    } else if (!strcmp(dir, "byte") || !strcmp(dir, "2byte") ||
               !strcmp(dir, "4byte") || !strcmp(dir, "hword") ||
               !strcmp(dir, "word") || !strcmp(dir, "long") ||
               !strcmp(dir, "quad") || !strcmp(dir, "octa") ||
               !strcmp(dir, "8byte")) {
        // Data emission
        int sz = (!strcmp(dir, "byte")) ? 1 : (!strcmp(dir, "2byte") || !strcmp(dir, "hword") || !strcmp(dir, "word")) ? 2
            : (!strcmp(dir, "4byte") || !strcmp(dir, "long"))                                                          ? 4
                                                                                                                       : 8;

        SecBuf *buf = cur_sec_buf(as);
        // May have multiple comma-separated values
        char *val = strtok(args, ",");
        while (val) {
            val = skip_ws(val);
            trim_end(val);

            // "(SYM) - ." / "SYM - ." — PC-relative-to-this-field, GAS's
            // idiom for a cross-section table entry pointing back at a
            // label (e.g. the kernel's `.long (1b) - .` in _ASM_EXTABLE).
            // "." here is exactly the position being written, so unlike
            // the ABS64 case below this always needs a real relocation —
            // the two sections may end up placed anywhere relative to each
            // other, only the linker knows the final addresses.
            size_t vlen = strlen(val);
            bool pc_rel_here = false;
            if (vlen >= 3) {
                size_t e = vlen;
                while (e > 0 && isspace((unsigned char)val[e - 1])) e--;
                if (e > 0 && val[e - 1] == '.') {
                    e--;
                    while (e > 0 && isspace((unsigned char)val[e - 1])) e--;
                    if (e > 0 && val[e - 1] == '-') {
                        e--;
                        while (e > 0 && isspace((unsigned char)val[e - 1])) e--;
                        val[e] = '\0';
                        vlen = e;
                        pc_rel_here = true;
                    }
                }
            }
            if (pc_rel_here && (sz == 4 || sz == 8)) {
                char *sym = skip_ws(val);
                trim_end(sym);
                size_t slen = strlen(sym);
                if (slen >= 2 && sym[0] == '(' && sym[slen - 1] == ')') {
                    sym[slen - 1] = '\0';
                    sym++;
                    trim_end(sym);
                    sym = skip_ws(sym);
                }
                // Optional "+ addend"/"- addend" (e.g. jump_label.h's
                // __jump_table entry: ".quad key + branch - .", where
                // "branch" is a small 0/1 constant to fold into the
                // relocation's addend, not part of the symbol name).
                int64_t addend = 0;
                char *plus = strchr(sym, '+');
                char *minus = strchr(sym, '-');
                if (plus) {
                    addend = strtoll(plus, NULL, 0);
                    *plus = '\0';
                    trim_end(sym);
                } else if (minus) {
                    addend = strtoll(minus, NULL, 0);
                    *minus = '\0';
                    trim_end(sym);
                }
                strip_local_label_suffix(sym);
                int sec_of_sym = 0;
                int sidx = lookup_local_sym(as, sym, &sec_of_sym);
                if (sidx < 0) sidx = ensure_sym(as, sym);
                if (sz == 4) {
                    size_t off = secbuf_emit32le(buf, 0);
                    objfile_add_reloc(as->obj, as->cur_sec, off, sidx, R_X86_64_PC32, addend);
                } else {
                    size_t off = secbuf_emit64le(buf, 0);
                    objfile_add_reloc(as->obj, as->cur_sec, off, sidx, R_X86_64_PC64, addend);
                }
                val = strtok(NULL, ",");
                continue;
            }

            // "LABEL2 - LABEL1" — GAS's label-difference idiom (the
            // kernel's ALTERNATIVE() macro: alt_slen/alt_total_slen/
            // alt_rlen compute an instruction-block's length this way,
            // e.g. "772b-771b"). Unlike "SYM - .", both operands are
            // ordinary labels — if both are already defined (backward
            // references, the common case since these length macros are
            // used right after the labelled instructions they measure)
            // the difference is a plain compile-time constant; if either
            // is still a forward reference, defer via a fixup resolved
            // once assemble_inline finishes (both labels are always
            // defined by the end of the same asm block for this idiom).
            {
                char *lbl_a, *lbl_b;
                char valcopy[256];
                strncpy(valcopy, val, sizeof(valcopy) - 1);
                valcopy[sizeof(valcopy) - 1] = '\0';
                if (try_parse_label_diff(valcopy, &lbl_a, &lbl_b)) {
                    strip_local_label_suffix(lbl_a);
                    strip_local_label_suffix(lbl_b);
                    // Always defer, even though both labels are usually
                    // already-defined backward references at this point
                    // (the common case, e.g. "772b-771b" right after the
                    // instructions it measures) — a FIXUP_SKIP_MAXDIFF
                    // elsewhere in this same buffer (e.g. the padding
                    // between "772:" and "773:") can still retroactively
                    // move either label's offset via skip_insert_shift,
                    // and that shift is only visible to fixups resolved
                    // *after* it runs, not to a value already baked in
                    // eagerly here.
                    size_t off = 0;
                    switch (sz) {
                    case 1: off = secbuf_emit8(buf, 0); break;
                    case 2: off = secbuf_emit16le(buf, 0); break;
                    case 4: off = secbuf_emit32le(buf, 0); break;
                    case 8: off = secbuf_emit64le(buf, 0); break;
                    }
                    add_labeldiff_fixup(as, off, as->cur_sec, lbl_a, lbl_b, sz);
                    val = strtok(NULL, ",");
                    continue;
                }
            }

            // Check if it's a symbol reference (for relocation)
            bool is_sym = val[0] == '.' || isalpha((unsigned char)val[0]) || val[0] == '_';
            if (is_sym && sz == 8) {
                // Emit an absolute 64-bit relocation
                int64_t addend = 0;
                char symname[256];
                strncpy(symname, val, 255);
                // Check for +/-addend
                char *plus = strchr(symname, '+');
                char *minus = strchr(symname, '-');
                if (plus) {
                    addend = strtoll(plus, NULL, 0);
                    *plus = 0;
                } else if (minus) {
                    addend = strtoll(minus, NULL, 0);
                    *minus = 0;
                }
                size_t off = secbuf_emit64le(buf, (uint64_t)addend);
                int sidx = ensure_sym(as, symname);
                objfile_add_reloc(as->obj, as->cur_sec, off, sidx,
                                  R_AARCH64_ABS64, addend);
            } else if (!is_sym) {
                int64_t v = strtoll(val, NULL, 0);
                switch (sz) {
                case 1: secbuf_emit8(buf, (uint8_t)v); break;
                case 2: secbuf_emit16le(buf, (uint16_t)v); break;
                case 4: secbuf_emit32le(buf, (uint32_t)v); break;
                case 8: secbuf_emit64le(buf, (uint64_t)v); break;
                }
            }
            val = strtok(NULL, ",");
        }
    } else if (!strcmp(dir, "zero") || !strcmp(dir, "skip") || !strcmp(dir, "space")) {
        char mlbl[4][128];
        int mfill;
        if (!strcmp(dir, "skip") && try_parse_skip_maxdiff(args, mlbl, &mfill)) {
            for (int i = 0; i < 4; i++) strip_local_label_suffix(mlbl[i]);
            add_skip_maxdiff_fixup(as, cur_off(as), as->cur_sec, mlbl[0], mlbl[1],
                                   mlbl[2], mlbl[3], mfill, as->nlocals);
            return;
        }
        int n = atoi(args);
        if (as->cur_sec == SEC_BSS) {
            as->obj->bss_size += (size_t)n;
        } else {
            SecBuf *buf = cur_sec_buf(as);
            secbuf_reserve(buf, (size_t)n);
            memset(buf->data + buf->len, 0, (size_t)n);
            buf->len += (size_t)n;
        }
    } else if (!strcmp(dir, "ascii") || !strcmp(dir, "asciz") || !strcmp(dir, "string")) {
        SecBuf *buf = cur_sec_buf(as);
        // Find the content between quotes
        char *p = strchr(args, '"');
        if (!p) return;
        p++;
        while (*p && *p != '"') {
            if (*p == '\\') {
                p++;
                switch (*p) {
                case 'n': secbuf_emit8(buf, '\n'); break;
                case 't': secbuf_emit8(buf, '\t'); break;
                case 'r': secbuf_emit8(buf, '\r'); break;
                case '0': secbuf_emit8(buf, 0); break;
                case '\\': secbuf_emit8(buf, '\\'); break;
                case '"': secbuf_emit8(buf, '"'); break;
                default: secbuf_emit8(buf, (uint8_t)*p); break;
                }
            } else {
                secbuf_emit8(buf, (uint8_t)*p);
            }
            p++;
        }
        if (!strcmp(dir, "asciz") || !strcmp(dir, "string"))
            secbuf_emit8(buf, 0); // null terminator
    } else if (!strcmp(dir, "file") || !strcmp(dir, "loc") ||
               !strcmp(dir, "ident")) {
        // Debug info — ignore in binary output
    } else if (!strcmp(dir, "note") || !strncmp(dir, "note.", 5)) {
        // Ignore note sections
    }
    // Other directives (weak_reference, weak_definition, etc.) ignored
}


#ifdef ARCH_ARM64
// ---------------------------------------------------------------------------
// ARM64 instruction encoding dispatch
// ---------------------------------------------------------------------------

// Map condition string → Arm64Cond
static Arm64Cond parse_arm64_cond(const char *s) {
    if (!strcmp(s, "eq")) return ARM64_EQ;
    if (!strcmp(s, "ne")) return ARM64_NE;
    if (!strcmp(s, "cs") || !strcmp(s, "hs")) return ARM64_CS;
    if (!strcmp(s, "cc") || !strcmp(s, "lo")) return ARM64_CC;
    if (!strcmp(s, "mi")) return ARM64_MI;
    if (!strcmp(s, "pl")) return ARM64_PL;
    if (!strcmp(s, "vs")) return ARM64_VS;
    if (!strcmp(s, "vc")) return ARM64_VC;
    if (!strcmp(s, "hi")) return ARM64_HI;
    if (!strcmp(s, "ls")) return ARM64_LS;
    if (!strcmp(s, "ge")) return ARM64_GE;
    if (!strcmp(s, "lt")) return ARM64_LT;
    if (!strcmp(s, "gt")) return ARM64_GT;
    if (!strcmp(s, "le")) return ARM64_LE;
    if (!strcmp(s, "al")) return ARM64_AL;
    return ARM64_AL;
}

// Encode a branch and handle relocation/backpatch
static void emit_arm64_branch(AsmState *as, uint32_t insn_base,
                              bool is_bl, bool is_cond, const char *target) {
    SecBuf *buf = cur_sec_buf(as);
    size_t off = buf->len;

    // Try local label first
    int sec = 0;
    int64_t toff = lookup_local(as, target, &sec);
    if (toff >= 0 && sec == as->cur_sec) {
        // Resolve now
        int32_t delta = (int32_t)((toff - (int64_t)off) / 4);
        if (is_cond)
            insn_base |= ((uint32_t)delta & 0x7ffff) << 5;
        else
            insn_base |= (uint32_t)delta & 0x3ffffff;
        secbuf_emit32le(buf, insn_base);
    } else {
        // Try global symbol
        int sidx = objfile_find_sym(as->obj, target);
        bool is_global_def = sidx >= 0 && as->obj->syms[sidx].section != SEC_UNDEF;

        secbuf_emit32le(buf, insn_base); // placeholder

        if (!is_global_def) {
            // Either forward local or external symbol → need fixup or reloc
            int idx = objfile_find_sym(as->obj, target);
            if (idx >= 0 && as->obj->syms[idx].section == SEC_UNDEF) {
                // External symbol → reloc
                uint32_t rtype = is_bl ? R_AARCH64_CALL26 : R_AARCH64_JUMP26;
                objfile_add_reloc(as->obj, as->cur_sec, off, idx, rtype, 0);
            } else {
                // Forward local label → fixup
                add_fixup(as, off, as->cur_sec, target,
                          is_cond ? FIXUP_ARM64_B19 : FIXUP_ARM64_B26, 0);
            }
        } else {
            // Already-defined global in text: patch directly
            int64_t tgt_off = (int64_t)as->obj->syms[sidx].offset;
            int32_t delta = (int32_t)((tgt_off - (int64_t)off) / 4);
            if (is_cond)
                insn_base |= ((uint32_t)delta & 0x7ffff) << 5;
            else
                insn_base |= (uint32_t)delta & 0x3ffffff;
            secbuf_patch32le(buf, off, insn_base);
        }
    }
}

// Encode ARM64 instruction line (mnemonic already separated, ops = operand string)
static bool encode_arm64(AsmState *as, const char *mnem, char *ops_str) {
    char ops_buf[512];
    strncpy(ops_buf, ops_str, 511);
    ops_buf[511] = 0;
    char *ops[6];
    int nops = split_operands(ops_buf, ops, 6);
    SecBuf *buf = cur_sec_buf(as);

    // GAS numeric local labels ("1:") are referenced as "1b"/"1f" — see the
    // matching comment in encode_x86(). emit_arm64_branch() below (used by
    // b/bl/b.cond/cbz/cbnz and friends) looks up its target directly against
    // defined label names, so the direction suffix must be stripped here too,
    // once, before any branch target is resolved.
    for (int _i = 0; _i < nops; _i++)
        strip_local_label_suffix(ops[_i]);

// Strip mnemonic suffix for condition codes in B.cond
// mnem is like "b.eq", "b.ne", etc. Already split at first '.'

// Helper macros
#define AREG(n)  parse_arm64_reg(ops[n], NULL)
#define AREG32(n, w) parse_arm64_reg(ops[n], w)
#define IMM(n)  parse_imm(ops[n])
#define SF(r)   ((r) < 32 ? 1 : 0) // always 1 for x-regs in context

    bool is32_0 = false, is32_1 = false;
    int r0 = (nops > 0) ? parse_arm64_reg(ops[0], &is32_0) : -1;
    int r1 = (nops > 1) ? parse_arm64_reg(ops[1], &is32_1) : -1;
    int r2 = (nops > 2) ? parse_arm64_reg(ops[2], NULL) : -1;
    int sf = is32_0 ? 0 : 1;

    if (!strcmp(mnem, "nop")) {
        arm64_nop(buf);
        return true;
    }
    if (!strcmp(mnem, "ret")) {
        int rn = (nops > 0 && r0 >= 0) ? r0 : 30;
        arm64_ret(buf, rn);
        return true;
    }
    if (!strcmp(mnem, "br")) {
        arm64_br(buf, r0);
        return true;
    }
    if (!strcmp(mnem, "blr")) {
        arm64_blr(buf, r0);
        return true;
    }

    if (!strcmp(mnem, "bl")) {
        char *t = (nops > 0) ? ops[0] : ""; // target label
        uint32_t base = 0x94000000u;
        emit_arm64_branch(as, base, true, false, t);
        return true;
    }
    if (!strcmp(mnem, "b")) {
        if (nops > 0) {
            uint32_t base = 0x14000000u;
            emit_arm64_branch(as, base, false, false, ops[0]);
        }
        return true;
    }
    // B.cond: mnemonic is "b" and condition is in the rest after '.'
    // We handle it by checking if ops[0] is a label and mnem has '.'
    // (The caller should detect "b.XX" and pass mnem="b" and condition separately)
    // Here mnem might be "b.eq" etc.
    if (mnem[0] == 'b' && mnem[1] == '.') {
        Arm64Cond cond = parse_arm64_cond(mnem + 2);
        uint32_t base = 0x54000000u | cond;
        emit_arm64_branch(as, base, false, true, ops[0]);
        return true;
    }

    // CBZ / CBNZ
    if (!strcmp(mnem, "cbz") || !strcmp(mnem, "cbnz")) {
        bool nz = !strcmp(mnem, "cbnz");
        int rt = r0;
        char *lbl = (nops > 1) ? ops[1] : "";
        uint32_t base = (sf ? 0xb4000000u : 0x34000000u) | (nz ? 0x01000000u : 0) | rt;
        emit_arm64_branch(as, base, false, true, lbl);
        return true;
    }

    // MOV (register/immediate/memory)
    if (!strcmp(mnem, "mov") && nops >= 2) {
        // Memory source: mov rd, [rn] → ldr rd, [rn]
        if (r0 >= 0 && r1 < 0 && ops[1][0] == '[') {
            int base, rn2 = -1;
            int64_t imm = 0;
            bool pre = false, post = false;
            base = parse_arm64_mem(ops[1], &imm, &pre, &post, &rn2);
            if (base >= 0) {
                int sz = sf ? 3 : 2;
                if (rn2 >= 0) {
                    // Register offset: ldr rt, [rn, rm]
                    arm64_ldr_reg(buf, sz, r0, base, rn2, false, 0);
                } else if (post) {
                    arm64_ldr_imm(buf, sf, r0, base, (int32_t)imm, true);
                } else if (pre) {
                    arm64_ldr_imm(buf, sf, r0, base, (int32_t)imm, true);
                } else {
                    // Simple: ldr rt, [rn] or ldur rt, [rn, #imm]
                    if (imm == 0)
                        arm64_ldr_uoff(buf, sz, r0, base, 0);
                    else
                        arm64_ldur(buf, sf, r0, base, (int32_t)imm);
                }
                return true;
            }
        }
        // Memory destination: mov [rn], rs → str rs, [rn]
        if (r0 < 0 && r1 >= 0 && ops[0][0] == '[') {
            int base, rn2 = -1;
            int64_t imm = 0;
            bool pre = false, post = false;
            base = parse_arm64_mem(ops[0], &imm, &pre, &post, &rn2);
            if (base >= 0) {
                int sz = sf ? 3 : 2;
                if (rn2 >= 0) {
                    arm64_str_reg(buf, sz, r1, base, rn2, false, 0);
                } else if (post) {
                    arm64_str_imm(buf, sf, r1, base, (int32_t)imm, true);
                } else if (pre) {
                    arm64_str_imm(buf, sf, r1, base, (int32_t)imm, true);
                } else {
                    if (imm == 0)
                        arm64_str_uoff(buf, sz, r1, base, 0);
                    else
                        arm64_stur(buf, sf, r1, base, (int32_t)imm);
                }
                return true;
            }
        }
        if (ops[1][0] == '#' || isdigit((unsigned char)ops[1][0]) || ops[1][0] == '-') {
            // Immediate: emit movz (and possibly movk if > 16 bits)
            uint64_t val = (uint64_t)(int64_t)IMM(1);
            arm64_movz(buf, sf, r0, (uint16_t)(val & 0xffff), 0);
            if (val >> 16) {
                int sh = 16;
                uint64_t v = val >> 16;
                while (v && sh <= (sf ? 48 : 16)) {
                    arm64_movk(buf, sf, r0, (uint16_t)(v & 0xffff), sh);
                    v >>= 16;
                    sh += 16;
                }
            }
            return true;
        }
        // Register: MOV rd, rn → ORR rd, xzr, rn
        if (r0 >= 0 && r1 >= 0) {
            arm64_orr_reg(buf, sf, r0, 31, r1, ARM64_LSL, 0);
            return true;
        }
        // Fall through to unknown
    }

    if (!strcmp(mnem, "movz")) {
        // movz reg, #imm [, lsl #shift]
        int64_t imm = IMM(1);
        int shift = 0;
        if (nops > 2) {
            char *p = ops[2];
            while (*p && !isdigit((unsigned char)*p)) p++;
            shift = atoi(p);
        }
        arm64_movz(buf, sf, r0, (uint16_t)imm, shift);
        return true;
    }
    if (!strcmp(mnem, "movk")) {
        int64_t imm = IMM(1);
        int shift = 0;
        if (nops > 2) {
            char *p = ops[2];
            while (*p && !isdigit((unsigned char)*p)) p++;
            shift = atoi(p);
        }
        arm64_movk(buf, sf, r0, (uint16_t)imm, shift);
        return true;
    }
    if (!strcmp(mnem, "mvn")) {
        arm64_mvn(buf, sf, r0, r1, ARM64_LSL, 0);
        return true;
    }

    // ADD / SUB
    if (!strcmp(mnem, "add") || !strcmp(mnem, "adds") ||
        !strcmp(mnem, "sub") || !strcmp(mnem, "subs")) {
        bool is_sub = (mnem[0] == 's' && mnem[1] == 'u');
        bool set_flags = (mnem[2] == 's' || (is_sub && mnem[3] == 's'));
        if (nops >= 3) {
            char *o2 = ops[2];
            bool is_imm = (o2[0] == '#' || isdigit((unsigned char)o2[0]));
            // Check for symbol reference (:lo12:sym)
            if (!is_imm && o2[0] == ':') {
                uint32_t rtype;
                const char *sym = parse_arm64_sym_reloc(o2, &rtype);
                size_t off = buf->len;
                arm64_add_imm(buf, sf, r0, r1, 0, 0);
                int sidx = ensure_sym(as, sym);
                objfile_add_reloc(as->obj, as->cur_sec, off, sidx, rtype, 0);
                return true;
            }
            if (is_imm) {
                int64_t imm = IMM(2);
                int shift = 0;
                if (nops > 3 && strstr(ops[3], "lsl")) {
                    char *p = ops[3];
                    while (*p && !isdigit((unsigned char)*p)) p++;
                    shift = atoi(p) / 12; // 0 or 1 (for shift12)
                }
                if (is_sub) {
                    if (set_flags) {
                        arm64_subs_imm(buf, sf, r0, r1, (int32_t)imm, shift);
                    } else {
                        arm64_sub_imm(buf, sf, r0, r1, (int32_t)imm, shift);
                    }
                } else {
                    if (set_flags) {
                        arm64_adds_imm(buf, sf, r0, r1, (int32_t)imm, shift);
                    } else {
                        arm64_add_imm(buf, sf, r0, r1, (int32_t)imm, shift);
                    }
                }
            } else {
                // Register operand
                if (is_sub) {
                    if (set_flags)
                        arm64_subs_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
                    else
                        arm64_sub_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
                } else {
                    if (set_flags)
                        arm64_adds_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
                    else
                        arm64_add_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
                }
            }
        } else {
            // 2-operand form: add/sub rd, rn  (rd = rd +/- rn)
            if (is_sub)
                arm64_sub_reg(buf, sf, r0, r0, r1, ARM64_LSL, 0);
            else
                arm64_add_reg(buf, sf, r0, r0, r1, ARM64_LSL, 0);
        }
        return true;
    }

    // CMP → SUBS xzr, rn, operand
    if (!strcmp(mnem, "cmp") || !strcmp(mnem, "cmn")) {
        bool is_cmn = !strcmp(mnem, "cmn");
        if (nops >= 2) {
            bool is_imm = (ops[1][0] == '#' || isdigit((unsigned char)ops[1][0]));
            if (is_imm) {
                int64_t imm = IMM(1);
                if (is_cmn) {
                    arm64_adds_imm(buf, sf, 31, r0, (int32_t)imm, 0);
                } else {
                    arm64_subs_imm(buf, sf, 31, r0, (int32_t)imm, 0);
                }
            }
        }
        return true;
    }

    // MUL, SDIV, UDIV, SMULL, UMULL, SMULH, UMULH
    if (!strcmp(mnem, "mul")) {
        arm64_mul(buf, sf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "sdiv")) {
        arm64_sdiv(buf, sf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "udiv")) {
        arm64_udiv(buf, sf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "smull")) {
        arm64_smull(buf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "umull")) {
        arm64_umull(buf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "smulh")) {
        arm64_smulh(buf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "umulh")) {
        arm64_umulh(buf, r0, r1, r2);
        return true;
    }

    // Logic (register or immediate)
    if (!strcmp(mnem, "and")) {
        bool is_imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (is_imm)
            arm64_and_imm(buf, sf, r0, r1, (uint64_t)IMM(2));
        else
            arm64_and_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
        return true;
    }
    if (!strcmp(mnem, "orr")) {
        bool is_imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (is_imm)
            arm64_orr_imm(buf, sf, r0, r1, (uint64_t)IMM(2));
        else
            arm64_orr_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
        return true;
    }
    if (!strcmp(mnem, "eor")) {
        bool is_imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (is_imm)
            arm64_eor_imm(buf, sf, r0, r1, (uint64_t)IMM(2));
        else
            arm64_eor_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
        return true;
    }
    if (!strcmp(mnem, "bic")) {
        arm64_bic_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
        return true;
    }
    if (!strcmp(mnem, "ands")) {
        bool is_imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (is_imm)
            arm64_ands_imm(buf, sf, r0, r1, (uint64_t)IMM(2));
        else
            arm64_ands_reg(buf, sf, r0, r1, r2, ARM64_LSL, 0);
        return true;
    }
    // TST → ANDS xzr, rn, rm
    if (!strcmp(mnem, "tst")) {
        arm64_ands_reg(buf, sf, 31, r0, r1, ARM64_LSL, 0);
        return true;
    }

    // Shifts
    if (!strcmp(mnem, "lsl")) {
        bool imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (imm)
            arm64_lsl_imm(buf, sf, r0, r1, (int)IMM(2));
        else
            arm64_lsl_reg(buf, sf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "lsr")) {
        bool imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (imm)
            arm64_lsr_imm(buf, sf, r0, r1, (int)IMM(2));
        else
            arm64_lsr_reg(buf, sf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "asr")) {
        bool imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (imm)
            arm64_asr_imm(buf, sf, r0, r1, (int)IMM(2));
        else
            arm64_asr_reg(buf, sf, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "ror")) {
        bool imm = (nops > 2 && (ops[2][0] == '#' || isdigit((unsigned char)ops[2][0])));
        if (imm) {
            // ROR immediate → EXTR rd, rn, rn, #shift
            int shift = (int)IMM(2);
            arm64_extr(buf, sf, r0, r1, r1, shift);
        } else {
            arm64_ror_reg(buf, sf, r0, r1, r2);
        }
        return true;
    }

    // CLZ, CLS, RBIT, REV
    if (!strcmp(mnem, "clz")) {
        arm64_clz(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "cls")) {
        arm64_cls(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "rbit")) {
        arm64_rbit(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "rev")) {
        arm64_rev(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "rev16")) {
        arm64_rev16(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "rev32")) {
        arm64_rev32(buf, r0, r1);
        return true;
    }

    // Extend
    if (!strcmp(mnem, "sxtb")) {
        arm64_sxtb(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "sxth")) {
        arm64_sxth(buf, sf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "sxtw")) {
        arm64_sxtw(buf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "uxtb")) {
        arm64_uxtb(buf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "uxth")) {
        arm64_uxth(buf, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "ubfx")) {
        arm64_ubfx(buf, sf, r0, r1, (int)IMM(2), (int)IMM(3) + 1);
        return true;
    }

    // NEG / NEGS
    if (!strcmp(mnem, "neg") || !strcmp(mnem, "negs")) {
        arm64_neg(buf, sf, r0, r1);
        return true;
    }

    // Conditional select
    if (!strcmp(mnem, "csel")) {
        arm64_csel(buf, sf, r0, r1, r2, parse_arm64_cond(ops[3]));
        return true;
    }
    if (!strcmp(mnem, "csinc")) {
        arm64_csinc(buf, sf, r0, r1, r2, parse_arm64_cond(ops[3]));
        return true;
    }
    if (!strcmp(mnem, "csneg")) {
        arm64_csneg(buf, sf, r0, r1, r2, parse_arm64_cond(ops[3]));
        return true;
    }
    if (!strcmp(mnem, "cset")) {
        arm64_cset(buf, sf, r0, parse_arm64_cond(ops[1]));
        return true;
    }
    if (!strcmp(mnem, "cneg")) {
        arm64_cneg(buf, sf, r0, r1, parse_arm64_cond(ops[2]));
        return true;
    }

    // ADRP
    if (!strcmp(mnem, "adrp")) {
        char *sym = ops[1];
        uint32_t rtype = R_AARCH64_ADR_PREL_PG_HI21;
        const char *sname = parse_arm64_sym_reloc(sym, &rtype);
        if (rtype == 0) rtype = R_AARCH64_ADR_PREL_PG_HI21;
        size_t off = buf->len;
        arm64_adrp(buf, r0, 0);
        int sidx = ensure_sym(as, sname);
        objfile_add_reloc(as->obj, as->cur_sec, off, sidx, rtype, 0);
        return true;
    }
    if (!strcmp(mnem, "adr")) {
        size_t off = buf->len;
        arm64_adr(buf, r0, 0);
        int sidx = ensure_sym(as, ops[1]);
        objfile_add_reloc(as->obj, as->cur_sec, off, sidx, R_AARCH64_ADR_PREL_PG_HI21, 0);
        return true;
    }

    // Load/Store
    if (!strncmp(mnem, "ldr", 3) || !strncmp(mnem, "str", 3) ||
        !strncmp(mnem, "ldp", 3) || !strncmp(mnem, "stp", 3) ||
        !strncmp(mnem, "ldur", 4) || !strncmp(mnem, "stur", 4) ||
        !strncmp(mnem, "ldx", 3) || !strncmp(mnem, "stx", 3) ||
        !strncmp(mnem, "lda", 3) || !strncmp(mnem, "stl", 3)) {
        // Parse memory operand (last one for store, second for load)
        // LDR/LDUR/LDRB/LDRH rt, [rn, #imm]
        // STR/STUR/STRB/STRH rt, [rn, #imm]
        bool is_load = (mnem[0] == 'l');
        bool is_pair = (!strncmp(mnem, "ldp", 3) || !strncmp(mnem, "stp", 3));
        bool is_ldur = (!strncmp(mnem, "ldur", 4) || !strncmp(mnem, "stur", 4));
        bool is_byte = strstr(mnem, "b") != NULL && !strstr(mnem, "bl");
        bool is_half = strstr(mnem, "h") != NULL;
        bool is_sw = strstr(mnem, "sw") != NULL; // ldrsw
        bool is_exc = (mnem[0] == 'l' && mnem[1] == 'd' && mnem[2] == 'x') || (mnem[0] == 's' && mnem[1] == 't' && mnem[2] == 'x');
        bool is_acq = strstr(mnem, "lda") != NULL;
        bool is_rel = strstr(mnem, "stl") != NULL;

        if (is_pair) {
            // LDP/STP rt1, rt2, [rn, #imm]!  or  [rn], #imm
            int rt1 = r0, rt2 = r1;
            int64_t imm = 0;
            bool pre = false, post = false;
            int rn = parse_arm64_mem(ops[2], &imm, &pre, &post, NULL);
            int32_t imm7 = (int32_t)(imm / (sf ? 8 : 4));
            if (is_load) {
                arm64_ldp(buf, sf, rt1, rt2, rn, imm7, pre, post);
            } else {
                arm64_stp(buf, sf, rt1, rt2, rn, imm7, pre, post);
            }
            return true;
        }

        int rt = r0;
        int memop_idx = is_load ? 1 : (is_pair ? 2 : 1);
        int64_t imm = 0;
        bool pre = false, post = false;
        int rn2 = -1;
        int rn = parse_arm64_mem(ops[memop_idx], &imm, &pre, &post, &rn2);

        if (is_exc) {
            if (is_load) {
                if (is_byte) {
                    arm64_ldxrb(buf, rt, rn);
                } else if (is_half) {
                    arm64_ldxrh(buf, rt, rn);
                } else {
                    arm64_ldxr(buf, sf, rt, rn);
                }
            } else {
                int rs = r1; // status register for stxr
                // stxr rs, rt, [rn]
                if (is_byte) {
                    arm64_stxrb(buf, rs, rt, rn);
                } else if (is_half) {
                    arm64_stxrh(buf, rs, rt, rn);
                } else {
                    arm64_stxr(buf, sf, rs, rt, rn);
                }
            }
            return true;
        }

        if (is_acq || is_rel) {
            if (is_load) {
                if (is_byte) {
                    arm64_ldarb(buf, rt, rn);
                } else if (is_half) {
                    arm64_ldarh(buf, rt, rn);
                } else {
                    arm64_ldar(buf, sf, rt, rn);
                }
            } else {
                if (is_byte) {
                    arm64_stlrb(buf, rt, rn);
                } else if (is_half) {
                    arm64_stlrh(buf, rt, rn);
                } else {
                    arm64_stlr(buf, sf, rt, rn);
                }
            }
            return true;
        }

        // Register offset?
        if (rn2 >= 0) {
            int sz = is_byte ? 0 : is_half ? 1
                : sf                       ? 3
                                           : 2;
            if (is_load) {
                arm64_ldr_reg(buf, sz, rt, rn, rn2, false, 0);
            } else {
                arm64_str_reg(buf, sz, rt, rn, rn2, false, 0);
            }
            return true;
        }

        if (is_ldur) {
            if (is_load) {
                arm64_ldur(buf, sf, rt, rn, (int32_t)imm);
            } else {
                arm64_stur(buf, sf, rt, rn, (int32_t)imm);
            }
            return true;
        }

        // Check for symbol reference (LDR rt, =sym or LDR rt, [rn, :got_lo12:sym])
        if (ops[memop_idx][0] == ':' || (ops[memop_idx][0] == '[' && strstr(ops[memop_idx], ":"))) {
            // Memory with relocation (already parsed rn above, but addend is reloc)
            char *rel_part = strstr(ops[memop_idx], ":");
            uint32_t rtype = 0;
            const char *sym = rel_part ? parse_arm64_sym_reloc(rel_part, &rtype) : NULL;
            int sz = is_byte ? 0 : is_half ? 1
                : sf                       ? 3
                                           : 2;
            size_t off = buf->len;
            if (is_load) {
                arm64_ldr_uoff(buf, sz, rt, rn, 0);
            } else {
                arm64_str_uoff(buf, sz, rt, rn, 0);
            }
            if (sym && rtype) {
                int sidx = ensure_sym(as, sym);
                objfile_add_reloc(as->obj, as->cur_sec, off, sidx, rtype, 0);
            }
            return true;
        }

        // Standard immediate offset
        int32_t simm = (int32_t)imm;
        if (is_byte) {
            uint32_t uoff = simm >= 0 ? (uint32_t)simm : 0;
            if (is_load) {
                if (simm >= 0)
                    arm64_ldrb_uoff(buf, rt, rn, uoff);
                else
                    arm64_ldrb_imm(buf, rt, rn, simm);
            } else {
                if (simm >= 0)
                    arm64_strb_uoff(buf, rt, rn, uoff);
                else
                    arm64_strb_imm(buf, rt, rn, simm);
            }
        } else if (is_half) {
            uint32_t uoff = simm >= 0 ? (uint32_t)(simm / 2) : 0;
            if (is_load) {
                if (simm >= 0)
                    arm64_ldrh_uoff(buf, rt, rn, uoff);
                else
                    arm64_ldrh_imm(buf, rt, rn, simm);
            } else {
                if (simm >= 0)
                    arm64_strh_uoff(buf, rt, rn, uoff);
                else
                    arm64_strh_imm(buf, rt, rn, simm);
            }
        } else if (is_sw) {
            uint32_t uoff = simm >= 0 ? (uint32_t)(simm / 4) : 0;
            if (simm >= 0)
                arm64_ldrsw_uoff(buf, rt, rn, uoff);
            else
                arm64_ldrsw_imm(buf, rt, rn, simm);
        } else {
            int sz = sf ? 3 : 2;
            uint32_t uoff = simm >= 0 ? (uint32_t)(simm / (sf ? 8 : 4)) : 0;
            if (pre || post) {
                if (is_load)
                    arm64_ldr_imm(buf, sf, rt, rn, simm, pre);
                else
                    arm64_str_imm(buf, sf, rt, rn, simm, pre);
            } else if (simm >= 0) {
                if (is_load)
                    arm64_ldr_uoff(buf, sz, rt, rn, uoff);
                else
                    arm64_str_uoff(buf, sz, rt, rn, uoff);
            } else {
                if (is_load)
                    arm64_ldur(buf, sf, rt, rn, simm);
                else
                    arm64_stur(buf, sf, rt, rn, simm);
            }
        }
        return true;
    }

    // DMB / DSB / ISB
    if (!strcmp(mnem, "dmb")) {
        arm64_dmb(buf, 0xb);
        return true;
    }
    if (!strcmp(mnem, "dsb")) {
        arm64_dsb(buf, 0xb);
        return true;
    }
    if (!strcmp(mnem, "isb")) {
        arm64_isb(buf);
        return true;
    }

    // PRFM
    if (!strcmp(mnem, "prfm")) {
        arm64_nop(buf);
        return true;
    } // simplification

    // FP: FMOV, FADD, FSUB, FMUL, FDIV, FCMP, FCVT, SCVTF, UCVTF, FCVTZS, FCVTZU, FNEG
    if (!strcmp(mnem, "fmov")) {
        if (nops == 2) {
            // Could be fmov Xd, Sn (FP→GP) or fmov Sd, Xn (GP→FP) or fmov Sd, Sn
            bool d_fp = (ops[0][0] == 'd' || ops[0][0] == 's');
            bool s_fp = (ops[1][0] == 'd' || ops[1][0] == 's');
            bool d_gp = (ops[0][0] == 'x' || ops[0][0] == 'w');
            bool s_gp = (ops[1][0] == 'x' || ops[1][0] == 'w');
            bool is_dbl = (ops[0][0] == 'd' || ops[1][0] == 'd' || ops[0][0] == 'x');
            int ftype = is_dbl ? 1 : 0;
            if (d_gp && s_fp)
                arm64_fmov_f2i(buf, is_dbl ? 1 : 0, r0, r1);
            else if (d_fp && s_gp)
                arm64_fmov_i2f(buf, is_dbl ? 1 : 0, r0, r1);
            else { // FP to FP
                // Use FMOV (register) encoding
                secbuf_emit32le(buf, 0x1e204000u | ((uint32_t)ftype << 22) | ((uint32_t)r1 << 5) | (uint32_t)r0);
            }
        }
        return true;
    }
    if (!strcmp(mnem, "fadd")) {
        bool db = (ops[0][0] == 'd');
        arm64_fadd(buf, db ? 1 : 0, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "fsub")) {
        bool db = (ops[0][0] == 'd');
        arm64_fsub(buf, db ? 1 : 0, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "fmul")) {
        bool db = (ops[0][0] == 'd');
        arm64_fmul(buf, db ? 1 : 0, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "fdiv")) {
        bool db = (ops[0][0] == 'd');
        arm64_fdiv(buf, db ? 1 : 0, r0, r1, r2);
        return true;
    }
    if (!strcmp(mnem, "fneg")) {
        bool db = (ops[0][0] == 'd');
        arm64_fneg(buf, db ? 1 : 0, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "fcmp")) {
        bool db = (ops[0][0] == 'd');
        arm64_fcmp(buf, db ? 1 : 0, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "scvtf")) {
        bool src64 = (ops[1][0] == 'x');
        bool dstd = (ops[0][0] == 'd');
        arm64_scvtf(buf, src64 ? 1 : 0, dstd ? 1 : 0, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "ucvtf")) {
        bool src64 = (ops[1][0] == 'x');
        bool dstd = (ops[0][0] == 'd');
        arm64_ucvtf(buf, src64 ? 1 : 0, dstd ? 1 : 0, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "fcvtzs")) {
        bool dst64 = (ops[0][0] == 'x');
        bool srcd = (ops[1][0] == 'd');
        arm64_fcvtzs(buf, dst64 ? 1 : 0, srcd ? 1 : 0, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "fcvtzu")) {
        bool dst64 = (ops[0][0] == 'x');
        bool srcd = (ops[1][0] == 'd');
        arm64_fcvtzu(buf, dst64 ? 1 : 0, srcd ? 1 : 0, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "fcvt")) {
        // FCVT Dd, Sn or FCVT Sd, Dn
        bool dst_d = ops[0][0] == 'd';
        bool src_d = ops[1][0] == 'd';
        int ftype = src_d ? 1 : 0;
        int opc = dst_d ? 1 : 0;
        arm64_fcvt(buf, opc, ftype, r0, r1);
        return true;
    }
    if (!strcmp(mnem, "cnt")) {
        arm64_nop(buf);
        return true;
    } // vector cnt, simplify
    if (!strcmp(mnem, "addv")) {
        arm64_nop(buf);
        return true;
    } // vector addv
    if (!strcmp(mnem, "ins")) {
        arm64_nop(buf);
        return true;
    } // vector insert
    // System instructions (DMB, DSB, ISB)
    if (!strcmp(mnem, "dmb")) {
        int opt = (nops > 0 && !strcmp(ops[0], "sy")) ? 0xf : 0xb; // sy=0xf, ish=0xb default
        arm64_dmb(buf, opt);
        return true;
    }
    if (!strcmp(mnem, "dsb")) {
        int opt = (nops > 0 && !strcmp(ops[0], "sy")) ? 0xf : 0xb;
        arm64_dsb(buf, opt);
        return true;
    }
    if (!strcmp(mnem, "isb")) {
        arm64_isb(buf);
        return true;
    }
    // MRS / MSR (system register access)
    if (!strcmp(mnem, "mrs")) {
        int rt = r0;
        char *regname = (nops > 1) ? ops[1] : "";
        uint32_t sys_reg = 0;
        if (!strcmp(regname, "fpcr")) sys_reg = 0xDA20u; // op0=3,op1=3,CRn=4,CRm=4,op2=0 → 0b11_011_0100_0100_000
        else if (!strcmp(regname, "fpsr"))
            sys_reg = 0xDA21u; // op0=3,op1=3,CRn=4,CRm=4,op2=1 → 0b11_011_0100_0100_001
        else if (!strcmp(regname, "nzcv"))
            sys_reg = 0xDA10u; // op0=3,op1=3,CRn=4,CRm=2,op2=0
        arm64_mrs(buf, rt, sys_reg);
        return true;
    }
    if (!strcmp(mnem, "msr")) {
        char *regname = ops[0];
        int rt = (nops > 1) ? parse_arm64_reg(ops[1], NULL) : -1;
        uint32_t sys_reg = 0;
        if (!strcmp(regname, "fpcr")) sys_reg = 0xDA20u;
        else if (!strcmp(regname, "fpsr"))
            sys_reg = 0xDA21u; // op0=3,op1=3,CRn=4,CRm=4,op2=1
        else if (!strcmp(regname, "nzcv"))
            sys_reg = 0xDA10u;
        arm64_msr(buf, sys_reg, rt);
        return true;
    }

    // Unknown — emit NOP as fallback (silent; validator already handles this)
    arm64_nop(buf);
    return true;

#undef AREG
#undef AREG32
#undef IMM
#undef SF
}
#endif // ARM64

#ifndef ARCH_ARM64
// ---------------------------------------------------------------------------
// x86-64 instruction encoding dispatch
// ---------------------------------------------------------------------------

// Returns 0 if no explicit AT&T size suffix, else 1/2/4/8.
// "sub" ends with 'b' but that's the mnemonic, not a suffix.
// We maintain a list of known base names that don't carry size suffixes.
static int suffix_size(const char *mnem) {
    static const char *no_sfx[] = {
        "sub", "add", "and", "or", "xor", "not", "neg", "inc", "dec",
        "mul", "div", "imul", "idiv", "cmp", "test", "mov", "lea",
        "shl", "shr", "sal", "sar", "rol", "ror", "rcl", "rcr",
        "push", "pop", "call", "ret", "jmp", "nop", "xchg",
        "bsf", "bsr", "popcnt", "lzcnt", "tzcnt", "bswap",
        "movabs", "lock", "rep", "repe", "repne", "cld", "mfence",
        "rdfsbase", "rdgsbase", "wrfsbase", "wrgsbase",
        NULL};
    for (int i = 0; no_sfx[i]; i++)
        if (!strcmp(mnem, no_sfx[i])) return 0;
    int n = (int)strlen(mnem);
    if (n < 2) return 8;
    char last = mnem[n - 1];
    if (last == 'q') return 8;
    if (last == 'l') return 4;
    if (last == 'w') return 2;
    if (last == 'b') return 1;
    return 8;
}

// Operand helpers for x86 encoding — macros to avoid nested functions
#define X86_R(i)    ((i) < nops ? parse_x86_reg64(ops[i]) : X86_NOREG)
#define X86_IMM(i)  ((i) < nops ? (ops[i][0]=='$' ? strtoll(ops[i]+1,NULL,0) : strtoll(ops[i],NULL,0)) : (int64_t)0)
#define X86_ISREG(i) ((i)<nops && ops[i][0]=='%')
#define X86_ISIMM(i) ((i)<nops && ops[i][0]=='$')
#define X86_ISMEM(i) ((i)<nops && strchr(ops[i],'(')!=NULL)
#define X86_ISSYM(i) ((i)<nops && ops[i][0]!='%' && ops[i][0]!='$' && \
                      (isalpha((unsigned char)ops[i][0])||ops[i][0]=='_'||ops[i][0]=='.'||ops[i][0]=='-'))
static X86Mem x86_get_mem(char **ops, int nops, int i) {
    X86Mem m = {X86_NOREG, X86_NOREG, 1, 0};
    if (i < nops) parse_x86_mem(ops[i], &m);
    return m;
}
#define X86_M(i)    x86_get_mem(ops, nops, (i))

static bool encode_x86(AsmState *as, const char *mnem, char *ops_str) {
    char ops_buf[512];
    strncpy(ops_buf, ops_str, 511);
    ops_buf[511] = 0;
    char *ops[6];
    int nops = split_operands(ops_buf, ops, 6);
    // Not always .text: a .pushsection'd non-.text region (e.g. the kernel's
    // ALTERNATIVE() macro's .altinstr_replacement, holding real replacement
    // instructions, not just data directives) needs instructions encoded
    // into whichever section is actually current.
    SecBuf *buf = cur_sec_buf(as);

    // GAS numeric local labels ("1:") are referenced as "1b" (nearest prior)
    // or "1f" (nearest next) — a digit-name plus a direction suffix that is
    // never part of the defined symbol name itself (define_label records
    // just "1"). Strip a trailing b/f off any purely-numeric operand here,
    // once, so every lookup_local()/add_fixup() call below — for jmp/jcc/
    // call/lea and friends — resolves against the name it was actually
    // defined under instead of silently missing and falling through to a
    // dangling forward fixup that's never patched.
    for (int _i = 0; _i < nops; _i++)
        strip_local_label_suffix(ops[_i]);

    // Determine operand size from mnemonic suffix (0 = derive from operand)
    int sz = suffix_size(mnem);

// Shorten operand access
#define R(i)      X86_R(i)
#define IMM(i)    X86_IMM(i)
#define M(i)      X86_M(i)
#define is_reg(i) X86_ISREG(i)
#define is_imm(i) X86_ISIMM(i)
#define is_mem(i) X86_ISMEM(i)
#define is_sym(i) X86_ISSYM(i)

    // If no explicit suffix (sz==0), derive size from register operands
    if (sz == 0) {
        sz = 8; // default to 64-bit
        for (int _i = 0; _i < nops; _i++) {
            if (X86_ISREG(_i)) {
                int rsz = reg_size_x86(ops[_i]);
                if (rsz != 8) {
                    sz = rsz;
                    break;
                } // prefer explicit sub-64-bit size
                sz = rsz; // keep 8 but may be overridden
                break;
            }
        }
    }

    if (!strcmp(mnem, "nop")) {
        x86_nop(buf);
        return true;
    }
    if (!strcmp(mnem, "ret") || !strcmp(mnem, "retq")) {
        x86_ret(buf);
        return true;
    }
    if (!strcmp(mnem, "leave") || !strcmp(mnem, "leaveq")) {
        x86_leave(buf);
        return true;
    }
    if (!strcmp(mnem, "cld")) {
        x86_cld(buf);
        return true;
    }
    if (!strcmp(mnem, "mfence")) {
        x86_mfence(buf);
        return true;
    }
    if (!strcmp(mnem, "lfence")) {
        x86_lfence(buf);
        return true;
    }
    if (!strcmp(mnem, "sfence")) {
        x86_sfence(buf);
        return true;
    }
    if (!strcmp(mnem, "rdtsc")) {
        x86_rdtsc(buf);
        return true;
    }
    if (!strcmp(mnem, "rdtscp")) {
        x86_rdtscp(buf);
        return true;
    }
    if (!strcmp(mnem, "clac")) {
        x86_clac(buf);
        return true;
    }
    if (!strcmp(mnem, "stac")) {
        x86_stac(buf);
        return true;
    }
    if (!strcmp(mnem, "iretq") || !strcmp(mnem, "iret")) {
        x86_iretq(buf);
        return true;
    }
    if (!strcmp(mnem, "invpcid")) {
        x86_invpcid(buf, R(1), M(0));
        return true;
    }
    if (!strcmp(mnem, "rdfsbase")) {
        x86_rdfsbase(buf, sz, R(0));
        return true;
    }
    if (!strcmp(mnem, "rdgsbase")) {
        x86_rdgsbase(buf, sz, R(0));
        return true;
    }
    if (!strcmp(mnem, "wrfsbase")) {
        x86_wrfsbase(buf, sz, R(0));
        return true;
    }
    if (!strcmp(mnem, "wrgsbase")) {
        x86_wrgsbase(buf, sz, R(0));
        return true;
    }
    if (!strcmp(mnem, "ud2") || !strcmp(mnem, "ud2a")) {
        x86_ud2(buf);
        return true;
    }
    if (!strcmp(mnem, "cdq")) {
        x86_cdq(buf);
        return true;
    }
    if (!strcmp(mnem, "cqo")) {
        x86_cqo(buf);
        return true;
    }

    // PUSH/POP
    if (!strncmp(mnem, "push", 4)) {
        if (is_imm(0))
            x86_push_imm(buf, (int32_t)IMM(0));
        else
            x86_push(buf, R(0));
        return true;
    }
    if (!strncmp(mnem, "pop", 3)) {
        x86_pop(buf, R(0));
        return true;
    }

    // String instructions (movs/stos/cmps/scas + b/w/l/q size suffix):
    // no operands, implicit rsi/rdi/rcx, typically rep-prefixed (e.g.
    // copy_user_generic's "rep movsb", generic memset/memcpy fallbacks).
    // Must be checked by exact name before the generic "mov" dispatch
    // below: "movsl" (32-bit string move) textually collides with that
    // dispatch's movslq-style sign-extend detection (both contain "sl").
    if (nops == 0) {
        if (!strcmp(mnem, "movsb")) {
            x86_movs(buf, 1);
            return true;
        }
        if (!strcmp(mnem, "movsw")) {
            x86_movs(buf, 2);
            return true;
        }
        if (!strcmp(mnem, "movsl")) {
            x86_movs(buf, 4);
            return true;
        }
        if (!strcmp(mnem, "movsq")) {
            x86_movs(buf, 8);
            return true;
        }
        if (!strcmp(mnem, "stosb")) {
            x86_stos(buf, 1);
            return true;
        }
        if (!strcmp(mnem, "stosw")) {
            x86_stos(buf, 2);
            return true;
        }
        if (!strcmp(mnem, "stosl")) {
            x86_stos(buf, 4);
            return true;
        }
        if (!strcmp(mnem, "stosq")) {
            x86_stos(buf, 8);
            return true;
        }
        if (!strcmp(mnem, "cmpsb")) {
            x86_cmps(buf, 1);
            return true;
        }
        if (!strcmp(mnem, "cmpsw")) {
            x86_cmps(buf, 2);
            return true;
        }
        if (!strcmp(mnem, "cmpsl")) {
            x86_cmps(buf, 4);
            return true;
        }
        if (!strcmp(mnem, "cmpsq")) {
            x86_cmps(buf, 8);
            return true;
        }
        if (!strcmp(mnem, "scasb")) {
            x86_scas(buf, 1);
            return true;
        }
        if (!strcmp(mnem, "scasw")) {
            x86_scas(buf, 2);
            return true;
        }
        if (!strcmp(mnem, "scasl")) {
            x86_scas(buf, 4);
            return true;
        }
        if (!strcmp(mnem, "scasq")) {
            x86_scas(buf, 8);
            return true;
        }
    }

    // MOV variants (but not the SSE scalar moves movsd/movss, which are
    // handled by their dedicated encoders below).
    if (!strncmp(mnem, "mov", 3) && strcmp(mnem, "movsd") && strcmp(mnem, "movss")) {
        // AT&T: src, dst
        bool is_movabs = strstr(mnem, "abs") != NULL;
        bool is_movsx = strstr(mnem, "sx") != NULL || strstr(mnem, "sl") != NULL;
        bool is_movzx = strstr(mnem, "zb") != NULL || strstr(mnem, "zw") != NULL || strstr(mnem, "zl") != NULL;
        bool is_movs = !is_movsx && !is_movzx && (strstr(mnem, "sbl") || strstr(mnem, "sbq") || strstr(mnem, "swl") || strstr(mnem, "swq") || strstr(mnem, "slq"));

        int src_sz = sz, dst_sz = sz;
        if (strstr(mnem, "bl") || strstr(mnem, "bq"))
            src_sz = 1;
        else if (strstr(mnem, "wl") || strstr(mnem, "wq"))
            src_sz = 2;
        else if (strstr(mnem, "lq"))
            src_sz = 4;

        if (is_movabs) {
            x86_movabs(buf, R(1), (uint64_t)IMM(0));
            return true;
        }
        if (is_movs || is_movsx) {
            // MOVSBL, MOVSBQ, etc.
            if (is_reg(0) && is_reg(1))
                x86_movsx(buf, dst_sz, src_sz, R(1), R(0));
            else if (is_mem(0))
                x86_movsx_rm(buf, dst_sz, src_sz, R(1), M(0));
            return true;
        }
        if (is_movzx) {
            if (is_reg(0) && is_reg(1))
                x86_movzx(buf, dst_sz, src_sz, R(1), R(0));
            else if (is_mem(0))
                x86_movzx_rm(buf, dst_sz, src_sz, R(1), M(0));
            return true;
        }
        // Regular MOV: src, dst (AT&T order)
        // AT&T: movq src, dst
        if (is_imm(0) && is_reg(1)) {
            x86_mov_ri(buf, sz, R(1), IMM(0));
            return true;
        }
        if (is_imm(0) && is_mem(1)) {
            x86_mov_mi(buf, sz, M(1), (int32_t)IMM(0));
            return true;
        }
        if (is_reg(0) && is_reg(1)) {
            x86_mov_rr(buf, sz, R(1), R(0));
            return true;
        }
        if (is_reg(0) && is_mem(1)) {
            x86_mov_mr(buf, sz, M(1), R(0));
            return true;
        }
        if (is_mem(0) && is_reg(1)) {
            x86_mov_rm(buf, sz, R(1), M(0));
            return true;
        }
        if (is_reg(0) && is_sym(1)) {
            // movq %reg, sym (absolute address)
            // For now, treat as reg→mem with symbol offset (not fully correct for PIC)
        }
        return true;
    }

    // LEA
    if (!strncmp(mnem, "lea", 3)) {
        if (is_mem(0) && is_reg(1))
            x86_lea(buf, sz, R(1), M(0));
        return true;
    }

// ADD / SUB / AND / OR / XOR
// AT&T order is src, dst — ops[0]=src, ops[1]=dst. Besides the
// reg-dest forms, two memory-destination forms must go through the
// dedicated mem-destination encoders, not fn_rm (which loads FROM memory
// into a register — the opposite direction):
// - reg source ("addl %eax, mem", kernel atomics like arch_atomic_add)
//   goes through fn_mr.
// - immediate source ("addl $0, mem", e.g. x86_64's __smp_mb() —
//   "lock addl $0,-4(%rsp)" — used by every smp_mb()/wq_has_sleeper()
//   call in the kernel) goes through fn_mi.
// Without these cases the memory-destination forms matched no branch at
// all and silently encoded nothing while still reporting success.
#define ALU_OP(name, fn_rr, fn_ri, fn_rm, fn_mr, fn_mi) \
    if (!strncmp(mnem, name, strlen(name))) { \
        if (is_imm(0)&&is_reg(1)) { fn_ri(buf,sz,R(1),(int32_t)IMM(0)); } \
        else if (is_reg(0)&&is_reg(1)) { fn_rr(buf,sz,R(1),R(0)); } \
        else if (is_mem(0)&&is_reg(1)) { fn_rm(buf,sz,R(1),M(0)); } \
        else if (is_reg(0)&&is_mem(1)) { fn_mr(buf,sz,M(1),R(0)); } \
        else if (is_imm(0)&&is_mem(1)) { fn_mi(buf,sz,M(1),(int32_t)IMM(0)); } \
        return true; \
    }
    ALU_OP("add", x86_add_rr, x86_add_ri, x86_add_rm, x86_add_mr, x86_add_mi)
    ALU_OP("sub", x86_sub_rr, x86_sub_ri, x86_sub_rm, x86_sub_mr, x86_sub_mi)
    ALU_OP("and", x86_and_rr, x86_and_ri, x86_and_rm, x86_and_mr, x86_and_mi)
    ALU_OP("or", x86_or_rr, x86_or_ri, x86_or_rm, x86_or_mr, x86_or_mi)
    ALU_OP("xor", x86_xor_rr, x86_xor_ri, x86_xor_rm, x86_xor_mr, x86_xor_mi)
    ALU_OP("adc", x86_adc_rr, x86_adc_ri, x86_adc_rm, x86_adc_mr, x86_adc_mi)
    ALU_OP("sbb", x86_sbb_rr, x86_sbb_ri, x86_sbb_rm, x86_sbb_mr, x86_sbb_mi)
#undef ALU_OP

    // MUL (F6/F7 group /4): implicit RDX:RAX = RAX * r/m. Excludes the SSE
    // mulss/mulsd/mulps/mulpd mnemonics (4th char 's' or 'p'), handled
    // separately below.
    if (!strncmp(mnem, "mul", 3) && mnem[3] != 's' && mnem[3] != 'p') {
        if (is_mem(0))
            x86_mul_m(buf, sz, M(0));
        else
            x86_mul_r(buf, sz, R(0));
        return true;
    }

    // IMUL
    if (!strncmp(mnem, "imul", 4)) {
        if (nops == 2 && is_reg(0) && is_reg(1))
            x86_imul_rr(buf, sz, R(1), R(0));
        else if (nops == 3 && is_imm(2))
            x86_imul_rri(buf, sz, R(1), R(0), (int32_t)IMM(2));
        else if (nops == 1)
            x86_imul_r(buf, sz, R(0));
        return true;
    }
    if (!strncmp(mnem, "idiv", 4)) {
        x86_idiv_r(buf, sz, R(0));
        return true;
    }
    if (!strncmp(mnem, "div", 3)) {
        x86_div_r(buf, sz, R(0));
        return true;
    }

    // NEG / NOT
    if (!strncmp(mnem, "neg", 3)) {
        if (is_mem(0))
            x86_neg_m(buf, sz, M(0));
        else
            x86_neg_r(buf, sz, R(0));
        return true;
    }
    if (!strncmp(mnem, "not", 3)) {
        if (is_mem(0))
            x86_not_m(buf, sz, M(0));
        else
            x86_not_r(buf, sz, R(0));
        return true;
    }
    if (!strncmp(mnem, "inc", 3)) {
        if (is_mem(0))
            x86_inc_m(buf, sz, M(0));
        else
            x86_inc_r(buf, sz, R(0));
        return true;
    }
    if (!strncmp(mnem, "dec", 3)) {
        if (is_mem(0))
            x86_dec_m(buf, sz, M(0));
        else
            x86_dec_r(buf, sz, R(0));
        return true;
    }

    // SHIFTS (AT&T: count, r/m)
    if (!strncmp(mnem, "shl", 3) || !strncmp(mnem, "sal", 3)) {
        if (is_imm(0))
            x86_shl_ri(buf, sz, R(1), (uint8_t)IMM(0));
        else
            x86_shl_rcl(buf, sz, R(1));
        return true;
    }
    if (!strncmp(mnem, "shr", 3)) {
        if (is_imm(0))
            x86_shr_ri(buf, sz, R(1), (uint8_t)IMM(0));
        else
            x86_shr_rcl(buf, sz, R(1));
        return true;
    }
    if (!strncmp(mnem, "sar", 3)) {
        if (is_imm(0))
            x86_sar_ri(buf, sz, R(1), (uint8_t)IMM(0));
        else
            x86_sar_rcl(buf, sz, R(1));
        return true;
    }
    if (!strncmp(mnem, "ror", 3)) {
        x86_ror_ri(buf, sz, R(1), (uint8_t)IMM(0));
        return true;
    }
    if (!strncmp(mnem, "rol", 3)) {
        x86_rol_ri(buf, sz, R(1), (uint8_t)IMM(0));
        return true;
    }

    // CMP / TEST (but not CMPXCHG[8B/16B], handled separately below since
    // it shares the "cmp" prefix but is a completely different opcode)
    if (!strncmp(mnem, "cmp", 3) && strncmp(mnem, "cmpxchg", 7)) {
        if (is_imm(0) && is_reg(1))
            x86_cmp_ri(buf, sz, R(1), (int32_t)IMM(0));
        else if (is_reg(0) && is_reg(1))
            x86_cmp_rr(buf, sz, R(1), R(0));
        else if (is_mem(0) && is_reg(1))
            x86_cmp_rm(buf, sz, R(1), M(0));
        else if (is_reg(0) && is_mem(1))
            x86_cmp_mr(buf, sz, M(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "test", 4)) {
        if (is_imm(0) && is_reg(1))
            x86_test_ri(buf, sz, R(1), (int32_t)IMM(0));
        else if (is_reg(0) && is_reg(1))
            x86_test_rr(buf, sz, R(1), R(0));
        return true;
    }

    // SETcc
    if (!strncmp(mnem, "set", 3)) {
        const char *cc_s = mnem + 3;
        X86Cond cc = X86_E;
        if (!strcmp(cc_s, "e") || !strcmp(cc_s, "z"))
            cc = X86_E;
        else if (!strcmp(cc_s, "ne") || !strcmp(cc_s, "nz"))
            cc = X86_NE;
        else if (!strcmp(cc_s, "l") || !strcmp(cc_s, "nge"))
            cc = X86_L;
        else if (!strcmp(cc_s, "le") || !strcmp(cc_s, "ng"))
            cc = X86_LE;
        else if (!strcmp(cc_s, "g") || !strcmp(cc_s, "nle"))
            cc = X86_G;
        else if (!strcmp(cc_s, "ge") || !strcmp(cc_s, "nl"))
            cc = X86_GE;
        else if (!strcmp(cc_s, "b") || !strcmp(cc_s, "c") || !strcmp(cc_s, "nae"))
            cc = X86_B;
        else if (!strcmp(cc_s, "be") || !strcmp(cc_s, "na"))
            cc = X86_BE;
        else if (!strcmp(cc_s, "a") || !strcmp(cc_s, "nbe"))
            cc = X86_A;
        else if (!strcmp(cc_s, "ae") || !strcmp(cc_s, "nc"))
            cc = X86_AE;
        else if (!strcmp(cc_s, "s"))
            cc = X86_S;
        else if (!strcmp(cc_s, "ns"))
            cc = X86_NS;
        else if (!strcmp(cc_s, "p"))
            cc = X86_P;
        else if (!strcmp(cc_s, "np") || !strcmp(cc_s, "po"))
            cc = X86_NP;
        x86_setcc(buf, cc, R(0));
        return true;
    }

    // CMOVcc
    if (!strncmp(mnem, "cmov", 4)) {
        const char *cc_s = mnem + 4;
        X86Cond cc = X86_NE;
        if (!strcmp(cc_s, "nz") || !strcmp(cc_s, "ne"))
            cc = X86_NE;
        else if (!strcmp(cc_s, "z") || !strcmp(cc_s, "e"))
            cc = X86_E;
        // others...
        if (is_reg(0) && is_reg(1))
            x86_cmovcc(buf, sz, cc, R(1), R(0));
        return true;
    }

    // JMP and Jcc
    if (!strcmp(mnem, "jmp") || !strcmp(mnem, "jmpq")) {
        if (is_reg(0)) {
            x86_jmp_r(buf, R(0));
            return true;
        }
        // Label-based jump
        char *lbl = ops[0];
        size_t off = buf->len;
        x86_jmp_rel32(buf, 0); // placeholder
        int sec = 0;
        int64_t toff = lookup_local(as, lbl, &sec);
        if (toff >= 0 && sec == as->cur_sec) {
            int32_t delta = (int32_t)(toff - (int64_t)(off + 5));
            secbuf_patch32le(buf, off + 1, (uint32_t)delta);
        } else {
            add_fixup(as, off + 1, as->cur_sec, lbl, FIXUP_REL32, 0);
        }
        return true;
    }

    // Jcc
    if (mnem[0] == 'j' && strlen(mnem) <= 6) {
        const char *cc_s = mnem + 1;
        X86Cond cc = X86_E;
        if (!strcmp(cc_s, "e") || !strcmp(cc_s, "z"))
            cc = X86_E;
        else if (!strcmp(cc_s, "ne") || !strcmp(cc_s, "nz"))
            cc = X86_NE;
        else if (!strcmp(cc_s, "l") || !strcmp(cc_s, "nge"))
            cc = X86_L;
        else if (!strcmp(cc_s, "le") || !strcmp(cc_s, "ng"))
            cc = X86_LE;
        else if (!strcmp(cc_s, "g") || !strcmp(cc_s, "nle"))
            cc = X86_G;
        else if (!strcmp(cc_s, "ge") || !strcmp(cc_s, "nl"))
            cc = X86_GE;
        else if (!strcmp(cc_s, "b") || !strcmp(cc_s, "c") || !strcmp(cc_s, "nae"))
            cc = X86_B;
        else if (!strcmp(cc_s, "be") || !strcmp(cc_s, "na"))
            cc = X86_BE;
        else if (!strcmp(cc_s, "a") || !strcmp(cc_s, "nbe"))
            cc = X86_A;
        else if (!strcmp(cc_s, "ae") || !strcmp(cc_s, "nc"))
            cc = X86_AE;
        else if (!strcmp(cc_s, "s"))
            cc = X86_S;
        else if (!strcmp(cc_s, "ns"))
            cc = X86_NS;
        else if (!strcmp(cc_s, "p"))
            cc = X86_P;
        else if (!strcmp(cc_s, "np") || !strcmp(cc_s, "po"))
            cc = X86_NP;
        else if (!strcmp(cc_s, "mp")) { // jmp → already handled
            x86_jmp_rel32(buf, 0);
            return true;
        }
        char *lbl = ops[0];
        size_t off = buf->len;
        x86_jcc_rel32(buf, cc, 0); // emits 0F 8X imm32 (6 bytes total)
        int sec = 0;
        int64_t toff = lookup_local(as, lbl, &sec);
        if (toff >= 0 && sec == as->cur_sec) {
            int32_t delta = (int32_t)(toff - (int64_t)(off + 6));
            secbuf_patch32le(buf, off + 2, (uint32_t)delta);
        } else {
            add_fixup(as, off + 2, as->cur_sec, lbl, FIXUP_REL32, 0);
        }
        return true;
    }

    // CALL
    if (!strcmp(mnem, "call") || !strcmp(mnem, "callq")) {
        if (is_reg(0)) {
            x86_call_r(buf, R(0));
            return true;
        }
        char *lbl = ops[0];
        size_t off = buf->len;
        x86_call_rel32(buf, 0);
        int sidx = ensure_sym(as, lbl);
        // Check if local label
        int sec = 0;
        int64_t toff = lookup_local(as, lbl, &sec);
        if (toff >= 0 && sec == as->cur_sec) {
            int32_t delta = (int32_t)(toff - (int64_t)(off + 5));
            secbuf_patch32le(buf, off + 1, (uint32_t)delta);
        } else {
            // External function → PLT32 reloc
            objfile_add_reloc(as->obj, as->cur_sec, off + 1, sidx,
                              R_X86_64_PLT32, -4);
        }
        return true;
    }

    // BSF/BSR/POPCNT/LZCNT/TZCNT
    if (!strncmp(mnem, "bsf", 3)) {
        x86_bsf(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "bsr", 3)) {
        x86_bsr(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "popcnt", 6)) {
        x86_popcnt(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "lzcnt", 5)) {
        x86_lzcnt(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "tzcnt", 5)) {
        x86_tzcnt(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "bswap", 5)) {
        x86_bswap(buf, sz, R(0));
        return true;
    }
    if (!strncmp(mnem, "xchg", 4)) {
        x86_xchg_rr(buf, sz, R(0), R(1));
        return true;
    }

    // BT/BTS/BTR/BTC r/m, r — kernel bitops (arch_set_bit/clear_bit/
    // change_bit's non-constant-index path) compile to these against a
    // memory operand; AT&T order is "bts %bit_index_reg, %mem_or_reg".
    if (!strncmp(mnem, "bts", 3)) {
        if (is_mem(1)) x86_bts_mr(buf, sz, M(1), R(0));
        else
            x86_bts_rr(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "btr", 3)) {
        if (is_mem(1)) x86_btr_mr(buf, sz, M(1), R(0));
        else
            x86_btr_rr(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "btc", 3)) {
        if (is_mem(1)) x86_btc_mr(buf, sz, M(1), R(0));
        else
            x86_btc_rr(buf, sz, R(1), R(0));
        return true;
    }
    if (!strncmp(mnem, "bt", 2)) {
        if (is_mem(1)) x86_bt_mr(buf, sz, M(1), R(0));
        else
            x86_bt_rr(buf, sz, R(1), R(0));
        return true;
    }
    // XADD r/m, r — this_cpu_add_return/atomic_fetch_add-style ops.
    if (!strncmp(mnem, "xadd", 4)) {
        if (is_mem(1)) x86_xadd_mr(buf, sz, M(1), R(0));
        else
            x86_xadd_rr(buf, sz, R(1), R(0));
        return true;
    }
    // CMPXCHG r/m, r — atomic_cmpxchg/try_cmpxchg and friends.
    if (!strncmp(mnem, "cmpxchg", 7) && strncmp(mnem, "cmpxchg8b", 9) &&
        strncmp(mnem, "cmpxchg16b", 10)) {
        if (is_mem(1)) x86_cmpxchg_mr(buf, sz, M(1), R(0));
        else
            x86_cmpxchg_rr(buf, sz, R(1), R(0));
        return true;
    }

    // Prefixes ("lock foo %1,%0", "rep movsb", ...): mnem/ops_str splitting
    // upstream only cuts at the first whitespace, so the prefix and the
    // instruction it modifies land in the SAME call — mnem is the prefix
    // ("lock") and ops_str is the rest of the line starting with the real
    // mnemonic ("xaddl %0, %1"), not an operand list. Emitting only the
    // prefix byte and returning here silently drops that instruction
    // entirely, leaving a dangling prefix byte the CPU decodes as garbage
    // against whatever follows — this broke every lock-prefixed atomic op
    // (lock xadd/cmpxchg/bts/btr/btc/...) used throughout kernel atomics.
    // Re-split ops_str into its own mnemonic + operands and encode that.
    // Segment-override prefixes ("ds clflush %0", "ds wrmsr", ...): the
    // kernel deliberately prepends a redundant segment prefix to some
    // instructions to pad them to a fixed patchable length for
    // ALTERNATIVE(); same "prefix mnem, real mnem lands in one call" shape
    // as lock/rep above.
    if (!strcmp(mnem, "ds") || !strcmp(mnem, "cs") || !strcmp(mnem, "es") ||
        !strcmp(mnem, "fs") || !strcmp(mnem, "gs") || !strcmp(mnem, "ss")) {
        uint8_t seg_byte = !strcmp(mnem, "es") ? 0x26
            : !strcmp(mnem, "cs")              ? 0x2e
            : !strcmp(mnem, "ss")              ? 0x36
            : !strcmp(mnem, "ds")              ? 0x3e
            : !strcmp(mnem, "fs")              ? 0x64
                                               : 0x65;
        x86_seg_prefix(buf, seg_byte);
        char *m2 = ops_str;
        char *o2 = m2;
        while (*o2 && !isspace((unsigned char)*o2)) o2++;
        if (*o2) {
            *o2++ = 0;
            o2 = skip_ws(o2);
        }
        for (char *c = m2; *c; c++) *c = (char)tolower((unsigned char)*c);
        if (!*m2) return true;
        return encode_x86(as, m2, o2);
    }
    if (!strcmp(mnem, "lock") || !strcmp(mnem, "rep") ||
        !strcmp(mnem, "repe") || !strcmp(mnem, "repne")) {
        if (!strcmp(mnem, "lock"))
            x86_lock_prefix(buf);
        else if (!strcmp(mnem, "repne"))
            x86_repne_prefix(buf);
        else
            x86_rep_prefix(buf);
        char *m2 = ops_str;
        char *o2 = m2;
        while (*o2 && !isspace((unsigned char)*o2)) o2++;
        if (*o2) {
            *o2++ = 0;
            o2 = skip_ws(o2);
        }
        for (char *c = m2; *c; c++) *c = (char)tolower((unsigned char)*c);
        if (!*m2) return true;
        return encode_x86(as, m2, o2);
    }
    if (!strcmp(mnem, "int")) {
        if (is_imm(0))
            x86_int(buf, (uint8_t)IMM(0));
        return true;
    }

    // SSE
    if (!strcmp(mnem, "movsd")) {
        if (is_reg(0) && is_reg(1))
            x86_movsd_rr(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        else if (is_mem(0) && is_reg(1))
            x86_movsd_rm(buf, parse_x86_xmm(ops[1]), M(0));
        else if (is_reg(0) && is_mem(1))
            x86_movsd_mr(buf, M(1), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "movss")) {
        if (is_reg(0) && is_reg(1))
            x86_movss_rr(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        else if (is_mem(0) && is_reg(1))
            x86_movss_rm(buf, parse_x86_xmm(ops[1]), M(0));
        else if (is_reg(0) && is_mem(1))
            x86_movss_mr(buf, M(1), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "addsd")) {
        x86_addsd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "subsd")) {
        x86_subsd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "mulsd")) {
        x86_mulsd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "divsd")) {
        x86_divsd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "addss")) {
        x86_addss(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "subss")) {
        x86_subss(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "mulss")) {
        x86_mulss(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "divss")) {
        x86_divss(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "ucomisd")) {
        x86_ucomisd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "ucomiss")) {
        x86_ucomiss(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "comisd")) {
        x86_comisd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "cvtsi2sd")) {
        int ss = reg_size_x86(ops[0]);
        x86_cvtsi2sd(buf, ss, parse_x86_xmm(ops[1]), R(0));
        return true;
    }
    if (!strcmp(mnem, "cvtsi2ss")) {
        int ss = reg_size_x86(ops[0]);
        x86_cvtsi2ss(buf, ss, parse_x86_xmm(ops[1]), R(0));
        return true;
    }
    if (!strcmp(mnem, "cvttsd2si")) {
        int ds = reg_size_x86(ops[1]);
        x86_cvttsd2si(buf, ds, R(1), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "cvttss2si")) {
        int ds = reg_size_x86(ops[1]);
        x86_cvttss2si(buf, ds, R(1), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "cvtsd2ss")) {
        x86_cvtsd2ss(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "cvtss2sd")) {
        x86_cvtss2sd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "xorpd")) {
        x86_xorpd(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "xorps")) {
        x86_xorps(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "movaps")) {
        x86_movaps(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "pxor")) {
        x86_pxor(buf, parse_x86_xmm(ops[1]), parse_x86_xmm(ops[0]));
        return true;
    }
    if (!strcmp(mnem, "fldl")) {
        x86_fldl_m(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "fstpt")) {
        x86_fstpt_m(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "cpuid")) {
        x86_cpuid(buf);
        return true;
    }
    if (!strcmp(mnem, "rdmsr")) {
        x86_rdmsr(buf);
        return true;
    }
    if (!strcmp(mnem, "wrmsr")) {
        x86_wrmsr(buf);
        return true;
    }
    if (!strncmp(mnem, "cmpxchg16b", 10)) {
        x86_cmpxchg16b_m(buf, M(0));
        return true;
    }
    if (!strncmp(mnem, "cmpxchg8b", 9)) {
        x86_cmpxchg8b_m(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "wbinvd")) {
        x86_wbinvd(buf);
        return true;
    }
    if (!strcmp(mnem, "sti")) {
        x86_sti(buf);
        return true;
    }
    if (!strcmp(mnem, "cli")) {
        x86_cli(buf);
        return true;
    }
    if (!strcmp(mnem, "hlt")) {
        x86_hlt(buf);
        return true;
    }
    if (!strcmp(mnem, "prefetcht0")) {
        x86_prefetcht0(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "prefetchnta")) {
        x86_prefetchnta(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "prefetchw")) {
        x86_prefetchw(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "clflushopt")) {
        x86_clflushopt(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "clflush")) {
        x86_clflush(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "clwb")) {
        x86_clwb(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "pause")) {
        x86_pause(buf);
        return true;
    }
    if (!strcmp(mnem, "swapgs")) {
        x86_swapgs(buf);
        return true;
    }
    if (!strcmp(mnem, "rdpmc")) {
        x86_rdpmc(buf);
        return true;
    }
    if (!strcmp(mnem, "rdpkru")) {
        x86_rdpkru(buf);
        return true;
    }
    if (!strcmp(mnem, "wrpkru")) {
        x86_wrpkru(buf);
        return true;
    }
    if (!strcmp(mnem, "verw")) {
        x86_verw_m(buf, M(0));
        return true;
    }
    if (!strcmp(mnem, "rdpid")) {
        x86_rdpid(buf, R(0));
        return true;
    }
    if (!strcmp(mnem, "lsl")) {
        if (is_mem(0))
            x86_lsl_rm(buf, M(0), R(1));
        else
            x86_lsl_rr(buf, R(0), R(1));
        return true;
    }

    // Unknown
    fprintf(stderr, "warning: unknown x86 instruction: %s\n", mnem);
#undef R
#undef IMM
#undef M
#undef is_reg
#undef is_imm
#undef is_mem
#undef is_sym
    return true;
}
#endif // X86

// ---------------------------------------------------------------------------
// Main assembler loop
// ---------------------------------------------------------------------------
int assemble_file(const char *asm_path, const char *obj_path) {
    FILE *f = fopen(asm_path, "r");
    if (!f) {
        fprintf(stderr, "asm: cannot open %s\n", asm_path);
        return -1;
    }

    ObjFile obj;
    objfile_init(&obj);

    AsmState as = {0};
    as.obj = &obj;
    as.cur_sec = SEC_TEXT;
    as.filename = asm_path;

#ifdef ARCH_ARM64
    bool is_arm64 = true;
#else
    bool is_arm64 = false;
#endif

    char line[1024];
    // Pending "global function" declaration for next label
    bool pending_globl = false;
    bool pending_weak = false;
    bool pending_func = false;
    (void)pending_globl;
    (void)pending_weak;
    (void)pending_func;

    while (fgets(line, sizeof(line), f)) {
        as.lineno++;
        char *p = skip_ws(line);
        trim_end(p);

        // Skip empty lines and comments
        if (!*p || *p == '#' || *p == ';' || (p[0] == '/' && p[1] == '/')) continue;
        // AT&T comment
        if (*p == '/') continue;

        // Labels (including ".L..." local labels) end with ":" at end-of-token.
        // Check for label BEFORE directive: ".L.return.main:" is a label, ".text" is a directive.
        char *colon = strchr(p, ':');
        bool is_label = colon && (colon[1] == '\0' || colon[1] == '\n' || colon[1] == ' ' || colon[1] == '\t');

        if (!is_label && *p == '.') {
            // Pure directive (no colon)
            char *dir = p + 1;
            char *sp = dir;
            while (*sp && !isspace((unsigned char)*sp)) sp++;
            char *args = *sp ? sp + 1 : sp;
            *sp = 0;
            for (char *d = dir; *d; d++) *d = tolower((unsigned char)*d);
            handle_directive(&as, dir, args);
            continue;
        }

        if (is_label) {
            *colon = 0;
            char *lbl = p;
            int idx = objfile_find_sym(&obj, lbl);
            bool is_global = (idx >= 0 && obj.syms[idx].bind != SB_LOCAL);
            bool is_weak = (idx >= 0 && obj.syms[idx].bind == SB_WEAK);
            bool is_func = (idx >= 0 && obj.syms[idx].type == ST_FUNC);
            define_label(&as, lbl, is_global, is_weak, is_func);
            p = skip_ws(colon + 1);
            if (!*p) continue;
        }

        if (as.cur_sec != SEC_TEXT) {
            // Non-text sections: only directives and labels handled above
            continue;
        }

        // Instruction: split mnemonic from operands
        char insn_buf[512];
        strncpy(insn_buf, p, 511);
        insn_buf[511] = 0;
        char *mnem = insn_buf;
        char *ops_str = mnem;
        while (*ops_str && !isspace((unsigned char)*ops_str)) ops_str++;
        if (*ops_str) {
            *ops_str++ = 0;
            ops_str = skip_ws(ops_str);
        }

        // Lowercase mnemonic
        for (char *m = mnem; *m; m++) *m = tolower((unsigned char)*m);

        // Remove trailing colon from mnemonic (shouldn't happen but defensive)
        char *mc = strchr(mnem, ':');
        if (mc) *mc = 0;

        // Strip comment from ops
        // Strip comments: "//" for ARM64, "//" and " #" for x86
        char *cmt = strstr(ops_str, " //");
        if (!cmt && !is_arm64) cmt = strstr(ops_str, " #");
        if (cmt) *cmt = 0;

        if (!*mnem) continue;

        // not dynamic, only statically
#ifdef ARCH_ARM64
        encode_arm64(&as, mnem, ops_str);
#else
        encode_x86(&as, mnem, ops_str);
#endif
    }

    fclose(f);

    // Check for unresolved fixups
    if (as.nfixups > 0) {
        fprintf(stderr, "asm: warning: %d unresolved fixups\n", as.nfixups);
    }

    // Write object file
    int rc;
#ifdef _WIN32
    rc = coff_write(&obj, obj_path);
#elif __APPLE__
    rc = macho_write(&obj, obj_path);
#else
    rc = elf_write(&obj, obj_path);
#endif

    objfile_free(&obj);
    return rc;
}

// ---------------------------------------------------------------------------
// GAS macro-assembler pre-pass: .macro/.endm, .irp/.endr, .ifc/.if/.endif,
// .set, .error, .purgem. Expands all of it into a flat sequence of plain
// directive/instruction/label lines *before* the real per-line assembler
// (define_label/handle_directive/encode_x86) ever runs, so none of that
// existing logic needs to know macros exist. The Linux kernel's
// _ASM_EXTABLE_TYPE_REG (asm/asm.h) is the motivating case: it defines a
// throwaway `.macro extable_type_reg` that uses `.irp`+`.ifc` to figure
// out, at assemble time, which of the 16 GP registers a uaccess
// instruction's operand was, and `.set`+`.if` to fail loudly if none
// matched.
// ---------------------------------------------------------------------------

// strtok() keeps its position in hidden global state, which this module
// cannot use: .irp expands its body once per value via a *recursive* call
// into the very same expansion machinery, and that recursive call (e.g. an
// .ifc or a nested macro invocation) tokenizes its own, unrelated comma
// list in between two of the outer loop's strtok(NULL, ...) calls,
// clobbering the outer loop's position and sending it into an infinite
// re-scan of stale/reused stack memory. An explicit cursor sidesteps that
// entirely — every loop owns its own.
static char *next_tok(char **cursor, const char *delims) {
    if (!*cursor) return NULL;
    char *p = *cursor;
    while (*p && strchr(delims, *p)) p++;
    if (!*p) {
        *cursor = NULL;
        return NULL;
    }
    char *start = p;
    while (*p && !strchr(delims, *p)) p++;
    if (*p) {
        *p = '\0';
        p++;
    } else {
        p = NULL;
    }
    *cursor = p;
    return start;
}

typedef struct {
    char *data;
    size_t len, cap;
} DynStr;

static void dynstr_append(DynStr *d, const char *s) {
    size_t n = strlen(s);
    if (d->len + n + 2 > d->cap) {
        d->cap = d->cap ? d->cap * 2 : 256;
        while (d->cap < d->len + n + 2) d->cap *= 2;
        char *tmp = realloc(d->data, d->cap);
        if (!tmp) {
            fprintf(stderr, "rcc: out of memory\n");
            exit(1);
        }
        d->data = tmp;
    }
    memcpy(d->data + d->len, s, n);
    d->len += n;
    d->data[d->len++] = '\n';
    d->data[d->len] = '\0';
}

// A chain of \NAME -> value substitutions in scope at a given nesting
// level (macro invocation, .irp iteration). Lookups fall through to the
// enclosing scope, so a macro body's .irp can still see the macro's own
// parameters (needed by extable_type_reg's `.ifc \reg, %%\rs`).
typedef struct ParamMap {
    struct ParamMap *parent;
    char names[8][32];
    char values[8][160];
    int n;
} ParamMap;

static const char *param_lookup(const ParamMap *m, const char *name) {
    for (; m; m = m->parent)
        for (int i = 0; i < m->n; i++)
            if (!strcmp(m->names[i], name))
                return m->values[i];
    return NULL;
}

static void param_set(ParamMap *m, const char *name, const char *value) {
    if (m->n >= (int)(sizeof(m->names) / sizeof(m->names[0]))) return;
    strncpy(m->names[m->n], name, sizeof(m->names[0]) - 1);
    strncpy(m->values[m->n], value, sizeof(m->values[0]) - 1);
    m->n++;
}

// Replace `\NAME` with its current-scope value and `%%` with a literal
// `%` (GAS's escape for a literal percent inside a macro body, since `%`
// alone can carry other meaning there).
static void subst_params_into(const char *line, const ParamMap *map, char *out, size_t outsz) {
    size_t oi = 0;
    for (const char *p = line; *p && oi + 1 < outsz;) {
        if (p[0] == '%' && p[1] == '%') {
            out[oi++] = '%';
            p += 2;
            continue;
        }
        if (p[0] == '\\' && (isalpha((unsigned char)p[1]) || p[1] == '_' || p[1] == '.')) {
            const char *s = p + 1, *e = s;
            while (isalnum((unsigned char)*e) || *e == '_' || *e == '.') e++;
            char name[32];
            size_t nlen = (size_t)(e - s);
            if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
            memcpy(name, s, nlen);
            name[nlen] = '\0';
            const char *val = param_lookup(map, name);
            if (val) {
                for (const char *v = val; *v && oi + 1 < outsz; v++) out[oi++] = *v;
                p = e;
                continue;
            }
        }
        out[oi++] = *p++;
    }
    out[oi] = '\0';
}

static int64_t asmvar_get(AsmState *as, const char *name) {
    for (int i = 0; i < as->nvars; i++)
        if (!strcmp(as->vars[i].name, name)) return as->vars[i].value;
    return 0;
}

static void asmvar_set(AsmState *as, const char *name, int64_t val) {
    for (int i = 0; i < as->nvars; i++)
        if (!strcmp(as->vars[i].name, name)) {
            as->vars[i].value = val;
            return;
        }
    if (as->nvars < (int)(sizeof(as->vars) / sizeof(as->vars[0]))) {
        strncpy(as->vars[as->nvars].name, name, sizeof(as->vars[0].name) - 1);
        as->vars[as->nvars].value = val;
        as->nvars++;
    }
}

// Small recursive-descent evaluator for .set/.if expressions: integer
// literals, \param / bare-name variable references (via asmvar_get), the
// usual C-ish binary operators by precedence, unary -/!, and parens.
typedef struct {
    const char *p;
    AsmState *as;
    const ParamMap *map;
    bool err; // set when the expression references something that can't be
    // known at macro-expansion time (a bare "." = current
    // position), so try_eval_full_expr must not silently treat
    // it as 0 and must instead defer to the real directive
    // handler's symbol/relocation logic.
} ExprCtx;

static void expr_ws(ExprCtx *c) {
    while (*c->p == ' ' || *c->p == '\t') c->p++;
}
static int64_t expr_or(ExprCtx *c);

static int64_t expr_primary(ExprCtx *c) {
    expr_ws(c);
    if (*c->p == '(') {
        c->p++;
        int64_t v = expr_or(c);
        expr_ws(c);
        if (*c->p == ')') c->p++;
        return v;
    }
    if (*c->p == '-') {
        c->p++;
        return -expr_primary(c);
    }
    if (*c->p == '!') {
        c->p++;
        return !expr_primary(c);
    }
    if (isdigit((unsigned char)*c->p)) {
        char *end;
        int64_t v = strtoll(c->p, &end, 0);
        c->p = end;
        return v;
    }
    bool backslash = (*c->p == '\\');
    if (backslash) c->p++;
    if (isalpha((unsigned char)*c->p) || *c->p == '_' || *c->p == '.') {
        const char *s = c->p;
        while (isalnum((unsigned char)*c->p) || *c->p == '_' || *c->p == '.') c->p++;
        char name[64];
        size_t n = (size_t)(c->p - s);
        if (n >= sizeof(name)) n = sizeof(name) - 1;
        memcpy(name, s, n);
        name[n] = '\0';
        if (backslash) {
            const char *val = param_lookup(c->map, name);
            return val ? strtoll(val, NULL, 0) : 0;
        }
        if (!strcmp(name, ".")) {
            // Bare "." (current assembly position) — a runtime-only
            // concept, not knowable during this macro-time pre-evaluation
            // pass. Without this, "SYM - ." (a cross-section PC-relative
            // reference GAS idiom, e.g. jump_label.h's __jump_table
            // entries) got silently mis-evaluated to 0 instead of being
            // left for the real directive handler's relocation logic.
            c->err = true;
            return 0;
        }
        return asmvar_get(c->as, name);
    }
    return 0;
}
static int64_t expr_mul(ExprCtx *c) {
    int64_t v = expr_primary(c);
    for (;;) {
        expr_ws(c);
        if (*c->p == '*') {
            c->p++;
            v *= expr_primary(c);
        } else if (*c->p == '/') {
            c->p++;
            int64_t d = expr_primary(c);
            v = d ? v / d : 0;
        } else
            break;
    }
    return v;
}
static int64_t expr_add(ExprCtx *c) {
    int64_t v = expr_mul(c);
    for (;;) {
        expr_ws(c);
        if (*c->p == '+') {
            c->p++;
            v += expr_mul(c);
        } else if (*c->p == '-' && c->p[1] != '\0') {
            c->p++;
            v -= expr_mul(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_shift(ExprCtx *c) {
    int64_t v = expr_add(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '<' && c->p[1] == '<') {
            c->p += 2;
            v <<= expr_add(c);
        } else if (c->p[0] == '>' && c->p[1] == '>') {
            c->p += 2;
            v >>= expr_add(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_rel(ExprCtx *c) {
    int64_t v = expr_shift(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '<' && c->p[1] == '=') {
            c->p += 2;
            v = v <= expr_shift(c);
        } else if (c->p[0] == '>' && c->p[1] == '=') {
            c->p += 2;
            v = v >= expr_shift(c);
        } else if (c->p[0] == '<') {
            c->p++;
            v = v < expr_shift(c);
        } else if (c->p[0] == '>') {
            c->p++;
            v = v > expr_shift(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_eq(ExprCtx *c) {
    int64_t v = expr_rel(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '=' && c->p[1] == '=') {
            c->p += 2;
            v = v == expr_rel(c);
        } else if (c->p[0] == '!' && c->p[1] == '=') {
            c->p += 2;
            v = v != expr_rel(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_and(ExprCtx *c) {
    int64_t v = expr_eq(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '&' && c->p[1] != '&') {
            c->p++;
            v &= expr_eq(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_xor(ExprCtx *c) {
    int64_t v = expr_and(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '^') {
            c->p++;
            v ^= expr_and(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_bor(ExprCtx *c) {
    int64_t v = expr_xor(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '|' && c->p[1] != '|') {
            c->p++;
            v |= expr_xor(c);
        } else
            break;
    }
    return v;
}
static int64_t expr_land(ExprCtx *c) {
    int64_t v = expr_bor(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '&' && c->p[1] == '&') {
            c->p += 2;
            v = (v && expr_bor(c));
        } else
            break;
    }
    return v;
}
static int64_t expr_or(ExprCtx *c) {
    int64_t v = expr_land(c);
    for (;;) {
        expr_ws(c);
        if (c->p[0] == '|' && c->p[1] == '|') {
            c->p += 2;
            v = (v || expr_land(c));
        } else
            break;
    }
    return v;
}

static int64_t eval_asm_expr(AsmState *as, const ParamMap *map, const char *text) {
    ExprCtx c = {text, as, map, false};
    return expr_or(&c);
}

// True if `text` is a *complete* assembler-time integer expression — the
// whole string parses with nothing left over. Used to tell an .Lvar-based
// arithmetic expression (`\type + (.Lregnr << 8)`, meant to be
// pre-computed here into a plain number) apart from a symbol/PC-relative
// reference (`(1b) - .`, a bare `label+addend`, ...), which must reach the
// real directive handler's symbol/relocation logic untouched.
static bool try_eval_full_expr(AsmState *as, const ParamMap *map, const char *text, int64_t *out) {
    ExprCtx c = {text, as, map, false};
    int64_t v = expr_or(&c);
    expr_ws(&c);
    if (*c.p != '\0' || c.err) return false;
    *out = v;
    return true;
}

static bool is_data_emit_dir(const char *word) {
    static const char *dirs[] = {"byte", "2byte", "4byte", "hword", "word",
                                 "long", "quad", "octa", "8byte"};
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++)
        if (!strcmp(word, dirs[i])) return true;
    return false;
}

// Directive-line matcher: `p` starts a line whose (already lower-cased in
// the caller) first word equals `dir`, e.g. is_directive(".endm", ".endm")
// or is_directive(".endm foo", ".endm").
static bool line_starts_with_dir(const char *p, const char *dir) {
    size_t dl = strlen(dir);
    if (strncmp(p, dir, dl) != 0) return false;
    return p[dl] == '\0' || isspace((unsigned char)p[dl]);
}

// Find the line index (relative to `lines`) that closes the block opened
// at `lines[start]` (one of `open_dirs`), skipping nested same-type
// blocks, terminated by `close_dir`. Returns -1 if unterminated.
static int find_block_end(char **lines, int nlines, int start,
                          const char **open_dirs, int nopen, const char *close_dir) {
    int depth = 1;
    for (int i = start + 1; i < nlines; i++) {
        char *p = skip_ws(lines[i]);
        for (int k = 0; k < nopen; k++)
            if (line_starts_with_dir(p, open_dirs[k])) {
                depth++;
                break;
            }
        if (line_starts_with_dir(p, close_dir)) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}

static void asm_expand_range(AsmState *as, char **lines, int lo, int hi,
                             const ParamMap *map, DynStr *out);

// Expand a single already-substituted line: if its first token names a
// currently-defined macro, invoke it (building a new param scope from the
// `key=value[, ...]` or positional arguments and recursing into the
// macro's stored body); otherwise emit the line as-is.
static void asm_expand_line(AsmState *as, const char *raw, const ParamMap *map, DynStr *out) {
    char subst[512];
    subst_params_into(raw, map, subst, sizeof(subst));
    char *p = skip_ws(subst);
    trim_end(p);
    if (!*p) return;

    char first[64];
    const char *s = p;
    size_t fi = 0;
    while (*s && !isspace((unsigned char)*s) && fi + 1 < sizeof(first)) first[fi++] = *s++;
    first[fi] = '\0';
    // A label ("name:") is never a macro invocation.
    bool is_label = fi > 0 && first[fi - 1] == ':';

    int midx = -1;
    if (!is_label)
        for (int i = 0; i < as->nmacros; i++)
            if (!strcmp(as->macros[i].name, first)) {
                midx = i;
                break;
            }

    if (midx < 0) {
        dynstr_append(out, p);
        return;
    }

    struct AsmMacro *mac = &as->macros[midx];
    ParamMap args = {0};
    args.parent = (ParamMap *)map;
    const char *rest = skip_ws((char *)s);
    int pos = 0;
    char argbuf[400];
    strncpy(argbuf, rest, sizeof(argbuf) - 1);
    argbuf[sizeof(argbuf) - 1] = '\0';
    char *cursor = argbuf;
    char *tok = next_tok(&cursor, ",");
    while (tok) {
        char *t = skip_ws(tok);
        trim_end(t);
        char *eq = strchr(t, '=');
        if (eq) {
            *eq = '\0';
            char *name = t;
            trim_end(name);
            char *val = skip_ws(eq + 1);
            param_set(&args, name, val);
        } else if (*t && pos < mac->nparams) {
            param_set(&args, mac->params[pos], t);
            pos++;
        }
        tok = next_tok(&cursor, ",");
    }
    // The macro body can itself contain control-flow (.irp/.ifc/.if/.set),
    // not just plain lines — route it through the full block-aware
    // processor, not the single-line one.
    asm_expand_range(as, mac->body, 0, mac->nbody, &args, out);
}

// Process lines[lo,hi) — the control-flow constructs (.macro/.irp/.ifc/
// .if/.set/.error/.purgem) that need to see more than one line at a time —
// appending the flattened, fully-substituted result to *out.
static void asm_expand_range(AsmState *as, char **lines, int lo, int hi,
                             const ParamMap *map, DynStr *out) {
    for (int i = lo; i < hi; i++) {
        char *raw = lines[i];
        char subst[512];
        subst_params_into(raw, map, subst, sizeof(subst));
        char *p = skip_ws(subst);
        trim_end(p);
        if (*p != '.') {
            asm_expand_line(as, raw, map, out);
            continue;
        }

        if (line_starts_with_dir(p, ".macro")) {
            static const char *nested[] = {".macro"};
            int end = find_block_end(lines, hi, i, nested, 1, ".endm");
            if (end < 0) {
                asm_error(as, ".macro without .endm");
                return;
            }
            char *hdr = skip_ws(p + 6);
            char name[64];
            const char *s = hdr;
            size_t ni = 0;
            while (*s && !isspace((unsigned char)*s) && ni + 1 < sizeof(name)) name[ni++] = *s++;
            name[ni] = '\0';
            if (as->nmacros < (int)(sizeof(as->macros) / sizeof(as->macros[0]))) {
                struct AsmMacro *mac = &as->macros[as->nmacros++];
                memset(mac, 0, sizeof(*mac));
                strncpy(mac->name, name, sizeof(mac->name) - 1);
                char paramsbuf[300];
                strncpy(paramsbuf, skip_ws((char *)s), sizeof(paramsbuf) - 1);
                paramsbuf[sizeof(paramsbuf) - 1] = '\0';
                char *pcursor = paramsbuf;
                char *ptok = next_tok(&pcursor, ", \t");
                while (ptok && mac->nparams < (int)(sizeof(mac->params) / sizeof(mac->params[0]))) {
                    char *colon = strchr(ptok, ':');
                    if (colon) *colon = '\0';
                    char *eqd = strchr(ptok, '=');
                    if (eqd) *eqd = '\0';
                    if (*ptok) {
                        strncpy(mac->params[mac->nparams], ptok, sizeof(mac->params[0]) - 1);
                        mac->nparams++;
                    }
                    ptok = next_tok(&pcursor, ", \t");
                }
                mac->nbody = end - (i + 1);
                if (mac->nbody > 0) {
                    mac->body = calloc((size_t)mac->nbody, sizeof(char *));
                    for (int b = 0; b < mac->nbody; b++)
                        mac->body[b] = strdup(lines[i + 1 + b]);
                }
            }
            i = end;
            continue;
        }
        if (line_starts_with_dir(p, ".purgem")) {
            char *name = skip_ws(p + 7);
            trim_end(name);
            for (int k = 0; k < as->nmacros; k++)
                if (!strcmp(as->macros[k].name, name)) {
                    for (int b = 0; b < as->macros[k].nbody; b++) free(as->macros[k].body[b]);
                    free(as->macros[k].body);
                    as->macros[k] = as->macros[--as->nmacros];
                    break;
                }
            continue;
        }
        if (line_starts_with_dir(p, ".irp")) {
            static const char *nested[] = {".irp"};
            int end = find_block_end(lines, hi, i, nested, 1, ".endr");
            if (end < 0) {
                asm_error(as, ".irp without .endr");
                return;
            }
            char argbuf[400];
            strncpy(argbuf, skip_ws(p + 4), sizeof(argbuf) - 1);
            argbuf[sizeof(argbuf) - 1] = '\0';
            char *ircursor = argbuf;
            char *sym = next_tok(&ircursor, ",");
            if (sym) {
                sym = skip_ws(sym);
                trim_end(sym);
                char *val = next_tok(&ircursor, ",");
                while (val) {
                    val = skip_ws(val);
                    trim_end(val);
                    ParamMap scope = {0};
                    scope.parent = (ParamMap *)map;
                    param_set(&scope, sym, val);
                    asm_expand_range(as, lines, i + 1, end, &scope, out);
                    val = next_tok(&ircursor, ",");
                }
            }
            i = end;
            continue;
        }
        if (line_starts_with_dir(p, ".ifc") || line_starts_with_dir(p, ".if")) {
            bool is_ifc = line_starts_with_dir(p, ".ifc");
            static const char *nested[] = {".ifc", ".if"};
            int end = find_block_end(lines, hi, i, nested, 2, ".endif");
            if (end < 0) {
                asm_error(as, ".if/.ifc without .endif");
                return;
            }
            // A lone `.else` inside [i+1, end) splits the true/false arms.
            int else_line = -1;
            for (int k = i + 1; k < end; k++) {
                char *q = skip_ws(lines[k]);
                if (line_starts_with_dir(q, ".else")) {
                    else_line = k;
                    break;
                }
            }
            bool cond;
            if (is_ifc) {
                char argbuf[400];
                strncpy(argbuf, skip_ws(p + 4), sizeof(argbuf) - 1);
                argbuf[sizeof(argbuf) - 1] = '\0';
                char *ifccursor = argbuf;
                char *a = next_tok(&ifccursor, ",");
                char *b = next_tok(&ifccursor, ",");
                char abuf[256] = "", bbuf[256] = "";
                if (a) {
                    a = skip_ws(a);
                    trim_end(a);
                    subst_params_into(a, map, abuf, sizeof(abuf));
                }
                if (b) {
                    b = skip_ws(b);
                    trim_end(b);
                    subst_params_into(b, map, bbuf, sizeof(bbuf));
                }
                cond = !strcmp(abuf, bbuf);
            } else {
                cond = eval_asm_expr(as, map, skip_ws(p + 3)) != 0;
            }
            int take_lo = cond ? i + 1 : (else_line >= 0 ? else_line + 1 : end);
            int take_hi = cond ? (else_line >= 0 ? else_line : end) : end;
            asm_expand_range(as, lines, take_lo, take_hi, map, out);
            i = end;
            continue;
        }
        if (line_starts_with_dir(p, ".set") || line_starts_with_dir(p, ".equiv")) {
            char *rest = skip_ws(p + (p[1] == 's' ? 4 : 6));
            char *comma = strchr(rest, ',');
            if (comma) {
                *comma = '\0';
                char *name = rest;
                trim_end(name);
                int64_t v = eval_asm_expr(as, map, comma + 1);
                asmvar_set(as, name, v);
                continue;
            }
            // Not a macro-time integer .set (e.g. "alias, target" symbol
            // aliasing) — let the real directive handler deal with it.
            asm_expand_line(as, raw, map, out);
            continue;
        }
        if (line_starts_with_dir(p, ".error")) {
            asm_error(as, skip_ws(p + 6));
            continue;
        }
        if (p[0] == '.' && isalpha((unsigned char)p[1])) {
            const char *w = p + 1;
            char word[16];
            size_t wl = 0;
            while (*w && !isspace((unsigned char)*w) && wl + 1 < sizeof(word)) word[wl++] = *w++;
            word[wl] = '\0';
            if (is_data_emit_dir(word)) {
                // `\type + (.Lregnr << 8)`-shaped values (post \param
                // substitution): pre-compute each comma-separated value
                // that's a *complete* assembler-time expression into a
                // plain number here, since handle_directive's own
                // .long/.byte/... only understands a bare symbol[+addend]
                // or a numeric literal — it would otherwise silently
                // truncate at the first non-numeric character (`strtoll`
                // stopping at "20 + ..." and keeping just 20).
                char rebuilt[512];
                size_t rlen = wl + 1;
                rebuilt[0] = '.';
                memcpy(rebuilt + 1, word, wl);
                char valbuf[400];
                strncpy(valbuf, w, sizeof(valbuf) - 1);
                valbuf[sizeof(valbuf) - 1] = '\0';
                char *vcursor = valbuf;
                char *vtok = next_tok(&vcursor, ",");
                bool first_val = true;
                while (vtok) {
                    char *t = skip_ws(vtok);
                    trim_end(t);
                    char piece[80];
                    int64_t v;
                    if (*t && try_eval_full_expr(as, map, t, &v))
                        snprintf(piece, sizeof(piece), "%lld", (long long)v);
                    else {
                        strncpy(piece, t, sizeof(piece) - 1);
                        piece[sizeof(piece) - 1] = '\0';
                    }
                    size_t plen = strlen(piece);
                    if (rlen + 1 + plen + 1 < sizeof(rebuilt)) {
                        rebuilt[rlen++] = first_val ? ' ' : ',';
                        memcpy(rebuilt + rlen, piece, plen);
                        rlen += plen;
                    }
                    first_val = false;
                    vtok = next_tok(&vcursor, ",");
                }
                rebuilt[rlen] = '\0';
                dynstr_append(out, rebuilt);
                continue;
            }
        }
        asm_expand_line(as, raw, map, out);
    }
}

// GAS allows ';' as a statement separator on one physical line — rare in
// compiler-generated code, but used by hand-written kernel objtool
// annotation macros, e.g.
//   912: .pushsection .discard.annotate_data,"M",@progbits,8; .long 912b - .,1 ; .popsection
// Turn each top-level ';' into a newline in place before splitting into
// lines, so every later stage (block matching, directive dispatch) only
// ever has to deal with one statement per line. Skips ';' inside a "..."
// string literal (irrelevant for any real .s content, but cheap to guard).
static void split_semicolons(char *buf) {
    bool in_str = false;
    for (char *p = buf; *p; p++) {
        if (*p == '"' && (p == buf || p[-1] != '\\'))
            in_str = !in_str;
        else if (*p == ';' && !in_str)
            *p = '\n';
    }
}

// Split `buf` (already NUL-separated at each original newline) into an
// array of line pointers. Returns the count; `*out_lines` is malloc'd.
static int split_into_lines(char *buf, char ***out_lines) {
    int cap = 64, n = 0;
    char **lines = malloc((size_t)cap * sizeof(char *));
    char *line = buf;
    while (*line || line == buf) {
        if (n == cap) {
            cap *= 2;
            char **tmp = realloc(lines, (size_t)cap * sizeof(char *));
            if (!tmp) {
                fprintf(stderr, "rcc: out of memory\n");
                exit(1);
            }
            lines = tmp;
        }
        lines[n++] = line;
        char *nl = strchr(line, '\n');
        if (!nl) break;
        *nl = '\0';
        line = nl + 1;
    }
    *out_lines = lines;
    return n;
}

// Entry point: expand every .macro/.irp/.ifc/.if/.set construct in `text`
// into a flat, newline-joined buffer ready for the plain per-line
// assembler. Always returns a fresh malloc'd buffer (a plain copy when
// there's nothing to expand).
static char *asm_macro_pass(AsmState *as, const char *text) {
    char *scratch = strdup(text);
    split_semicolons(scratch);
    char **lines;
    int n = split_into_lines(scratch, &lines);
    DynStr out = {0};
    asm_expand_range(as, lines, 0, n, NULL, &out);
    free(lines);
    free(scratch);
    if (!out.data) {
        out.data = strdup("\n");
    }
    return out.data;
}

// Insert `n` bytes of `fill` at `patch_off` within `section`'s buffer,
// shifting everything already recorded after that point: the section's own
// bytes, every local-label offset (and its mirror in obj->syms[]) at or
// past patch_off, and every other pending fixup's patch_off at or past it
// (including not-yet-resolved FIXUP_SKIP_MAXDIFF entries, so nested
// ALTERNATIVE_2-style constructs — two of these in the same buffer — chain
// correctly when resolved left to right).
//
// This is a real, if narrow, form of the "two-pass assembler" the
// alt_rlen/alt_slen padding computation was originally deferred on: safe
// here specifically because the ALTERNATIVE() macro never touches this
// section again after the padding point within a single assemble_inline
// call (it immediately .pushsection's away to .altinstructions /
// .altinstr_replacement) — so no relocation already added against this
// section can land past patch_off, and there's nothing broader to shift.
static void skip_insert_shift(AsmState *as, int section, size_t patch_off,
                              size_t n, uint8_t fill, int locals_mark,
                              int fixup_idx) {
    if (n == 0) return;
    SecBuf *sb = objfile_section_buf(as->obj, section);
    if (!sb) return;
    secbuf_reserve(sb, n);
    memmove(sb->data + patch_off + n, sb->data + patch_off, sb->len - patch_off);
    memset(sb->data + patch_off, fill, n);
    sb->len += n;

    // Chronologically-later same-section labels/fixups, not offset-later
    // ones — see add_skip_maxdiff_fixup's comment for why offset alone is
    // ambiguous right at the insertion boundary.
    for (int i = locals_mark; i < as->nlocals; i++) {
        struct LocalSym *ls = &as->locals[i];
        if (ls->section != section) continue;
        ls->offset += n;
        if (ls->sym_idx >= 0 && ls->sym_idx < as->obj->sym_count &&
            as->obj->syms[ls->sym_idx].section == section)
            as->obj->syms[ls->sym_idx].offset += n;
    }
    for (int i = fixup_idx + 1; i < as->nfixups; i++) {
        struct Fixup *fx2 = &as->fixups[i];
        if (fx2->section == section)
            fx2->patch_off += n;
    }
}

int assemble_inline(ObjFile *obj, const char *tmpl,
                    inline_fixup_fn on_forward, void *ctx) {
#ifdef ARCH_ARM64
    bool is_arm64 = true;
#else
    bool is_arm64 = false;
#endif
    AsmState as = {0};
    as.obj = obj;
    as.cur_sec = SEC_TEXT;
    as.filename = "<inline asm>";

    // Expand .macro/.irp/.ifc/.if/.set (e.g. the kernel's
    // _ASM_EXTABLE_TYPE_REG) into a flat sequence of plain lines first, so
    // the per-line loop below never has to know these constructs exist.
    char *buf = asm_macro_pass(&as, tmpl);
    if (!buf) return -1;
    for (int mi = 0; mi < as.nmacros; mi++) {
        for (int b = 0; b < as.macros[mi].nbody; b++) free(as.macros[mi].body[b]);
        free(as.macros[mi].body);
    }
    as.nmacros = 0;
    as.nvars = 0;

    char *line = buf;
    while (*line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        as.lineno++;
        char *p = skip_ws(line);
        trim_end(p);

        if (!*p || *p == '#' || *p == ';' || (p[0] == '/' && p[1] == '/')) {
            line = nl ? nl + 1 : line + strlen(line);
            continue;
        }

        // A label's colon ends the line's very first token — no whitespace
        // before it — whether or not anything (whitespace, a comment, or
        // straight into another statement like "1:jmp foo") follows on the
        // same physical line. String-literal concatenation in kernel
        // headers routinely glues "1: " onto a macro that starts mid-
        // statement (e.g. ALTERNATIVE's OLDINSTR expands to "# ALT:
        // oldinstr\n...", so "1: " + that becomes one physical line "1: #
        // ALT: oldinstr"), so the old colon[1]-must-be-whitespace/EOL check
        // missed both the label itself and left "# ALT: oldinstr" to fall
        // through as if it were an instruction (mnem "#").
        char *colon = strchr(p, ':');
        bool is_label = false;
        if (colon && colon > p) {
            is_label = true;
            for (char *c = p; c < colon; c++) {
                if (isspace((unsigned char)*c) || *c == '%' || *c == ',' ||
                    *c == '(' || *c == ')') {
                    is_label = false;
                    break;
                }
            }
        }

        if (!is_label && *p == '.') {
            char *dir = p + 1;
            char *sp = dir;
            while (*sp && !isspace((unsigned char)*sp)) sp++;
            char *args = *sp ? sp + 1 : sp;
            *sp = 0;
            for (char *d = dir; *d; d++) *d = tolower((unsigned char)*d);
            handle_directive(&as, dir, args);
            line = nl ? nl + 1 : line + strlen(line);
            continue;
        }

        if (is_label) {
            *colon = 0;
            char *lbl = p;
            int idx = objfile_find_sym(obj, lbl);
            bool is_global = (idx >= 0 && obj->syms[idx].bind != SB_LOCAL);
            bool is_weak = (idx >= 0 && obj->syms[idx].bind == SB_WEAK);
            bool is_func = (idx >= 0 && obj->syms[idx].type == ST_FUNC);
            define_label(&as, lbl, is_global, is_weak, is_func);
            p = skip_ws(colon + 1);
            if (!*p || *p == '#' || *p == ';' || (p[0] == '/' && p[1] == '/')) {
                line = nl ? nl + 1 : line + strlen(line);
                continue;
            }
            // "label: .directive ..." on one physical line (e.g. objtool
            // annotation macros: `912: .pushsection .discard..., ...`) —
            // the remainder after the label is itself a directive, not an
            // instruction; route it the same way the top-of-loop check
            // would have if the label prefix weren't there.
            if (*p == '.') {
                char *dir = p + 1;
                char *sp = dir;
                while (*sp && !isspace((unsigned char)*sp)) sp++;
                char *args = *sp ? sp + 1 : sp;
                *sp = 0;
                for (char *d = dir; *d; d++) *d = tolower((unsigned char)*d);
                handle_directive(&as, dir, args);
                line = nl ? nl + 1 : line + strlen(line);
                continue;
            }
        }

        // Instructions are allowed in any section, not just .text: the
        // kernel's ALTERNATIVE() macro puts real replacement code in a
        // .pushsection'd .altinstr_replacement, not just data directives
        // (unlike e.g. __ex_table/__jump_table, which only ever hold
        // .long/.quad entries). encode_x86()/encode_arm64() already
        // target cur_sec_buf(as), whatever that currently is.
        char insn_buf[512];
        strncpy(insn_buf, p, 511);
        insn_buf[511] = 0;
        char *mnem = insn_buf;
        char *ops_str = mnem;
        while (*ops_str && !isspace((unsigned char)*ops_str)) ops_str++;
        if (*ops_str) {
            *ops_str++ = 0;
            ops_str = skip_ws(ops_str);
        }
        for (char *m = mnem; *m; m++) *m = tolower((unsigned char)*m);
        char *mc = strchr(mnem, ':');
        if (mc) *mc = 0;
        // Strip comments: "//" for ARM64, "//" and " #" for x86
        char *cmt = strstr(ops_str, " //");
        if (!cmt && !is_arm64) cmt = strstr(ops_str, " #");
        if (cmt) *cmt = 0;
        if (!*mnem) {
            line = nl ? nl + 1 : line + strlen(line);
            continue;
        }

#ifdef ARCH_ARM64
        encode_arm64(&as, mnem, ops_str);
#else
        encode_x86(&as, mnem, ops_str);
#endif
        line = nl ? nl + 1 : line + strlen(line);
    }

    free(buf);

    // Resolve FIXUP_SKIP_MAXDIFF entries first and in array order (== source
    // order == ascending original patch_off): each insertion shifts every
    // fixup recorded after it, including later not-yet-processed
    // FIXUP_SKIP_MAXDIFF ones, so processing left to right lets nested
    // ALTERNATIVE_2-style constructs (two of these in one buffer) chain
    // correctly. Everything else — FIXUP_LABELDIFF in particular, e.g. the
    // "773b-771b" total-length field that spans this padding — must wait
    // until after this pass so it reads final, post-shift offsets.
    for (int i = 0; i < as.nfixups; i++) {
        struct Fixup *fx = &as.fixups[i];
        if (fx->kind != FIXUP_SKIP_MAXDIFF) continue;
        int sec_a = 0, sec_b = 0, sec_c = 0, sec_d = 0;
        int64_t off_a = lookup_local(&as, fx->label, &sec_a);
        int64_t off_b = lookup_local(&as, fx->label2, &sec_b);
        int64_t off_c = lookup_local(&as, fx->label3, &sec_c);
        int64_t off_d = lookup_local(&as, fx->label4, &sec_d);
        if (off_a < 0 || off_b < 0 || off_c < 0 || off_d < 0) continue;
        int64_t rlen = off_a - off_b;
        int64_t slen = off_c - off_d;
        int64_t pad = rlen - slen;
        if (pad > 0)
            skip_insert_shift(&as, fx->section, fx->patch_off, (size_t)pad,
                              (uint8_t)fx->fill_byte, (int)fx->addend, i);
    }

    // Resolve forward fixups against existing symbols
    for (int i = 0; i < as.nfixups; i++) {
        struct Fixup *fx = &as.fixups[i];
        if (fx->kind == FIXUP_SKIP_MAXDIFF) continue;
        if (fx->kind == FIXUP_LABELDIFF) {
            int sec_a = 0, sec_b = 0;
            int64_t off_a = lookup_local(&as, fx->label, &sec_a);
            int64_t off_b = lookup_local(&as, fx->label2, &sec_b);
            if (off_a >= 0 && off_b >= 0) {
                int64_t diff = off_a - off_b; // "A - B" = offset(A) - offset(B)
                SecBuf *sb = objfile_section_buf(obj, fx->section);
                if (sb) {
                    uint64_t v = (uint64_t)diff;
                    memcpy(sb->data + fx->patch_off, &v, (size_t)fx->size);
                }
            }
            continue;
        }
        int sec;
        int64_t tgt = lookup_local(&as, fx->label, &sec);
        if (tgt < 0) {
            // Try global symbol table
            int sidx = objfile_find_sym(obj, fx->label);
            if (sidx >= 0 && obj->syms[sidx].section != SEC_UNDEF) {
                tgt = (int64_t)obj->syms[sidx].offset;
                sec = obj->syms[sidx].section;
            }
        }
        if (tgt >= 0 && fx->kind == FIXUP_REL32 && sec == fx->section) {
            SecBuf *sb = objfile_section_buf(obj, fx->section);
            if (sb) {
                int32_t rel = (int32_t)(tgt - ((int64_t)fx->patch_off + 4) + fx->addend);
                memcpy(sb->data + fx->patch_off, &rel, 4);
            }
#ifndef ARCH_ARM64
        } else if (tgt >= 0 && fx->kind == FIXUP_REL32) {
            // Same-call-site forward reference but the target ended up in a
            // *different* section than the branch/call itself (e.g. a
            // .pushsection'd region jumping back to .text, or vice versa) —
            // a same-section byte-patch can't express this; needs a real
            // relocation instead, same as the cross-section case in
            // define_label()'s own per-label fixup resolution.
            int sidx = ensure_sym(&as, fx->label);
            objfile_add_reloc(obj, fx->section, fx->patch_off, sidx, R_X86_64_PC32, fx->addend - 4);
#endif
        } else if (fx->kind == FIXUP_REL32 && on_forward) {
            // Forward reference: delegate to caller
            on_forward(fx->patch_off, fx->section, fx->label, ctx);
        }
    }

    return 0;
}
