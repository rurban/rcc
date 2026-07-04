// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from chibicc by Rui Ueyama.
// Binary codegen: asm_* wrappers emit bytes via secbuf_emit*() to ObjFile.
#include "rcc.h"

#include "asm.h"
#include "codegen_asm.h"
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

static ObjFile *cg_obj;
SecBuf *cg_sec;
static bool cg_discard_result;
bool cg_dry_run; // pass 1: track regs only (extern for codegen_asm.h)
uint64_t time_peep_us = 0;

static void cg_set_section(int sec) {
    if (!cg_obj) return;
    switch (sec) {
    case SEC_TEXT: cg_sec = &cg_obj->text; break;
    case SEC_DATA: cg_sec = &cg_obj->data; break;
    case SEC_RODATA: cg_sec = &cg_obj->rodata; break;
    case SEC_INIT_ARRAY: cg_sec = &cg_obj->init_array; break;
    case SEC_FINI_ARRAY: cg_sec = &cg_obj->fini_array; break;
    case SEC_TDATA: cg_sec = &cg_obj->data_tls; break;
    case SEC_THREAD_VARS: cg_sec = &cg_obj->thread_vars; break;
    default: cg_sec = &cg_obj->text; break;
    }
}

#ifdef _WIN32
// Emit the MinGW emulated TLS control block and template for a thread-local
// global variable.  The control block lives in .data and is what
// __emutls_get_address receives; the template lives in .rdata and holds the
// per-thread initial value.
static void cg_emit_emutls_data(LVar *var) {
    char *label = var->asm_name ? var->asm_name : var->name;
    int align = var->ty->align > 1 ? var->ty->align : 1;

    cg_set_section(SEC_DATA);
    secbuf_align(cg_sec, 32);
    size_t ctrl_off = cg_sec->len;
    char *ctrl_name = format("__emutls_v.%s", label);
    SymBind bind = var->is_static ? SB_LOCAL : SB_GLOBAL;
    objfile_add_sym(cg_obj, ctrl_name, SEC_DATA, ctrl_off, 32, bind, ST_OBJECT);
    secbuf_emit64le(cg_sec, var->ty->size);
    secbuf_emit64le(cg_sec, align);
    secbuf_emit64le(cg_sec, 0);
    size_t ptr_off = cg_sec->len;
    secbuf_emit64le(cg_sec, 0);

    char *tmpl_name = format("__emutls_t.%s", label);
    int tidx = objfile_find_sym(cg_obj, tmpl_name);
    if (tidx < 0)
        tidx = objfile_add_sym(cg_obj, tmpl_name, SEC_UNDEF, 0, 0, SB_LOCAL, ST_NOTYPE);
    objfile_add_reloc(cg_obj, SEC_DATA, ptr_off, tidx, R_X86_64_64, 0);

    cg_set_section(SEC_RODATA);
    secbuf_align(cg_sec, align);
    size_t tmpl_off = cg_sec->len;
    objfile_add_sym(cg_obj, tmpl_name, SEC_RODATA, tmpl_off, var->ty->size, SB_LOCAL, ST_OBJECT);
    if (var->init_data || var->relocs) {
        int pos = 0;
        for (Reloc *rel = var->relocs; rel; rel = rel->next) {
            for (; pos < rel->offset; pos++)
                secbuf_emit8(cg_sec, (uint8_t)var->init_data[pos]);
            size_t rel_off = cg_sec->len;
            secbuf_emit64le(cg_sec, 0);
            int sidx = objfile_find_sym(cg_obj, rel->label);
            if (sidx < 0)
                sidx = objfile_add_sym(cg_obj, rel->label, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
            objfile_add_reloc(cg_obj, SEC_RODATA, rel_off, sidx, R_X86_64_64, (int64_t)rel->addend);
            pos += 8;
        }
        for (; pos < var->init_size; pos++)
            secbuf_emit8(cg_sec, (uint8_t)var->init_data[pos]);
        if (var->ty->size > var->init_size) {
            size_t pad = var->ty->size - var->init_size;
            secbuf_reserve(cg_sec, pad);
            memset(cg_sec->data + cg_sec->len, 0, pad);
            cg_sec->len += pad;
        }
    } else if (var->has_init) {
        if (var->ty->size == 1)
            secbuf_emit8(cg_sec, (uint8_t)var->init_val);
        else if (var->ty->size == 2)
            secbuf_emit16le(cg_sec, (uint16_t)var->init_val);
        else if (var->ty->size == 4)
            secbuf_emit32le(cg_sec, (uint32_t)var->init_val);
        else
            secbuf_emit64le(cg_sec, (uint64_t)var->init_val);
    } else if (var->ty->size > 0) {
        size_t pad = var->ty->size;
        secbuf_reserve(cg_sec, pad);
        memset(cg_sec->data + cg_sec->len, 0, pad);
        cg_sec->len += pad;
    }
}
#endif

// Define (finalize) a label/symbol at the current section offset. Forward
// references via emit_adrp_add/emit_adrp_got may have already created this
// symbol as a SEC_RODATA/ST_NOTYPE placeholder at offset 0; objfile_add_sym
// only updates existing symbols when their section is SEC_UNDEF, so such a
// placeholder would otherwise keep its bogus offset forever. Force-overwrite
// it here since this call site holds the authoritative offset.
static void cg_def_sym(const char *name, int sec, SymBind bind, SymType type) {
    int idx = objfile_find_sym(cg_obj, name);
    if (idx >= 0) {
        cg_obj->syms[idx].section = sec;
        cg_obj->syms[idx].offset = cg_sec->len;
        cg_obj->syms[idx].bind = bind;
        cg_obj->syms[idx].type = type;
    } else {
        objfile_add_sym(cg_obj, name, sec, cg_sec->len, 0, bind, type);
    }
}

static void cg_def_label(const char *name) {
    if (cg_dry_run) return;
    // .L. labels referenced by relocations (e.g. &&label values) must be in the
    // symbol table so the linker can resolve ARM64_RELOC_UNSIGNED relocations.
    // We use SB_LOCAL to avoid creating subsection boundaries that the linker
    // might dead-strip — local symbols are not subsection boundaries.
    cg_def_sym(name, SEC_TEXT, SB_LOCAL, ST_FUNC);
    cg_label_ht_add(name, cg_sec->len);
    asm_fixup_resolve(cg_sec, name, cg_sec->len);
}

static void cg_def_label_sec(const char *name, int sec) {
    if (cg_dry_run) return;
    cg_def_sym(name, sec, SB_LOCAL, ST_OBJECT);
    cg_label_ht_add(name, cg_sec->len);
    asm_fixup_resolve(cg_sec, name, cg_sec->len);
}

static void cg_global_label(const char *name) {
    if (cg_dry_run) return;
    objfile_add_sym(cg_obj, name, SEC_TEXT, cg_sec->len, 0, SB_GLOBAL, ST_FUNC);
    cg_label_ht_add(name, cg_sec->len);
    asm_fixup_resolve(cg_sec, name, cg_sec->len);
}

static void cg_weak_label(const char *name) {
    if (cg_dry_run) return;
    objfile_add_sym(cg_obj, name, SEC_TEXT, cg_sec->len, 0, SB_WEAK, ST_FUNC);
}

static void cg_weak_declare(const char *name) {
    if (cg_dry_run) return;
    //fprintf(stderr, "DEBUG cg_weak_declare: %s\n", name);
    int sidx = objfile_find_sym(cg_obj, name);
    if (sidx < 0)
        objfile_add_sym(cg_obj, name, SEC_UNDEF, 0, 0, SB_WEAK, ST_NOTYPE);
    else if (cg_obj->syms[sidx].section == SEC_UNDEF)
        cg_obj->syms[sidx].bind = SB_WEAK;
}

#ifdef _WIN32
// Win64 SEH unwind recording. Mirrors the .seh_* directives the golden
// codegen.c.main emitted (.seh_proc/.seh_pushreg/.seh_stackalloc/
// .seh_endprologue/.seh_endproc); coff_write turns the captured entries into
// .pdata/.xdata. The X86Reg enum value equals the Win64 unwind register
// number (RBP=5, RBX=3, R12..R15=12..15, RSI=6), so it is used directly.
static UnwindEntry *cg_uw; // current function's unwind entry (NULL when inactive)
static uint32_t cg_uw_start; // .text offset of the current function start

static void uw_begin(void) { // .seh_proc
    if (cg_dry_run || !cg_obj) {
        cg_uw = NULL;
        return;
    }
    cg_uw = objfile_add_unwind(cg_obj);
    cg_uw_start = (uint32_t)cg_sec->len;
    cg_uw->func_start = cg_uw_start;
}
static void uw_pushreg(int reg) { // .seh_pushreg
    if (!cg_uw) return;
    UnwindCode *c = &cg_uw->codes[cg_uw->code_count++];
    c->code_offset = (uint8_t)((uint32_t)cg_sec->len - cg_uw_start);
    c->op = UWOP_PUSH_NONVOL;
    c->info = (uint8_t)reg;
}
static void uw_stackalloc(int size) { // .seh_stackalloc
    if (!cg_uw) return;
    UnwindCode *c = &cg_uw->codes[cg_uw->code_count++];
    c->code_offset = (uint8_t)((uint32_t)cg_sec->len - cg_uw_start);
    if (size <= 128) {
        c->op = UWOP_ALLOC_SMALL;
        c->info = (uint8_t)(size / 8 - 1);
    } else if (size <= 8 * 0xffff) {
        c->op = UWOP_ALLOC_LARGE;
        c->info = 0;
        c->extra[0] = (uint16_t)(size / 8);
        c->extra_count = 1;
    } else {
        c->op = UWOP_ALLOC_LARGE;
        c->info = 1;
        c->extra[0] = (uint16_t)(size & 0xffff);
        c->extra[1] = (uint16_t)((uint32_t)size >> 16);
        c->extra_count = 2;
    }
}
static void uw_endprologue(void) { // .seh_endprologue
    if (!cg_uw) return;
    cg_uw->prolog_size = (uint8_t)((uint32_t)cg_sec->len - cg_uw_start);
}
static void uw_endproc(void) { // .seh_endproc
    if (!cg_uw) return;
    cg_uw->func_end = (uint32_t)cg_sec->len;
    cg_uw = NULL;
}
#endif


#ifndef ARCH_ARM64
static size_t asm_lea_rip_reg(SecBuf *s, int r, const char *label) {
    size_t off = s->len;
    X86Mem m = {X86_RIP, X86_NOREG, 1, 0};
    x86_lea(s, 8, REG(r), m);
    if (!cg_dry_run) {
        int sidx = objfile_find_sym(cg_obj, label);
        if (sidx < 0) {
            bool is_local_label = label[0] == '.';
            sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, is_local_label ? SB_LOCAL : SB_GLOBAL, ST_NOTYPE);
        }
        objfile_add_reloc(cg_obj, SEC_TEXT, off + 3, sidx, R_X86_64_PC32, -4);
    }
    asm_record(ASM_LEA_FP, off, s->len - off, r, -1, -1, 8, 0, 0, label, 0, -1, false);
    return s->len - off;
}
// movsd .Llabel(%rip), %xmm0 — 8 bytes: f2 0f 10 05 <disp32>
static size_t asm_movsd_rip_xmm(SecBuf *s, const char *label) {
    size_t off = s->len;
    secbuf_emit32le(s, 0x05100ff2); // f2 0f 10 05
    secbuf_emit32le(s, 0); // placeholder disp32
    if (!cg_dry_run) {
        int sidx = objfile_find_sym(cg_obj, label);
        if (sidx < 0) {
            bool il = label[0] == '.';
            sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, il ? SB_LOCAL : SB_GLOBAL, ST_NOTYPE);
        }
        objfile_add_reloc(cg_obj, SEC_TEXT, off + 4, sidx, R_X86_64_PC32, -4);
    }
    return s->len - off;
}
// mov sym@GOTPCREL(%rip), reg  — load GOT entry pointer (x86 only)
// On COFF/PE there is no separate GOT cell for our weak-symbol scheme: an
// unresolved weak external resolves directly to the absolute address 0 (see
// coff_write.c), so the address must be computed with lea, not loaded with
// mov (which would dereference whatever lands at that resolved address).
static size_t asm_mov_got_rip_reg(SecBuf *s, int r, const char *label) {
    size_t off = s->len;
    X86Mem m = {X86_RIP, X86_NOREG, 1, 0};
#ifdef _WIN32
    x86_lea(s, 8, REG(r), m); // lea sym(%rip), reg
#else
    x86_mov_rm(s, 8, REG(r), m); // mov sym@GOTPCREL(%rip), reg
#endif
    if (!cg_dry_run) {
        int sidx = objfile_find_sym(cg_obj, label);
        if (sidx < 0)
            sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
        objfile_add_reloc(cg_obj, SEC_TEXT, off + 3, sidx, R_X86_64_GOTPCREL, -4);
    }
    asm_record(ASM_MOV_RRBP, off, s->len - off, r, -1, -1, 8, 0, 0, label, 0, -1, false);
    return s->len - off;
}
__attribute__((unused)) static size_t asm_mov_fs0_reg(SecBuf *s, VReg r) {
    X86Reg reg = REG(r);
    EMIT_GUARD;
    size_t off = s->len;
    secbuf_emit8(s, 0x64);
    uint8_t rex = 0x48;
    if (reg & 8) rex |= 0x04;
    secbuf_emit8(s, rex);
    secbuf_emit8(s, 0x8B);
    secbuf_emit8(s, (uint8_t)(((reg & 7) << 3) | 0x04));
    secbuf_emit8(s, 0x25);
    secbuf_emit32le(s, 0);
    return s->len - off;
}
__attribute__((unused)) static size_t asm_lea_tpoff_base_reg(SecBuf *s, VReg dst, VReg base, const char *label) {
    X86Reg rd = REG(dst);
    X86Reg rb = REG(base);
    EMIT_GUARD;
    size_t off = s->len;
    uint8_t rex = 0x48;
    if (rd & 8) rex |= 0x04;
    if (rb & 8) rex |= 0x01;
    secbuf_emit8(s, rex);
    secbuf_emit8(s, 0x8D);
    if ((rb & 7) == X86_RSP) {
        secbuf_emit8(s, (uint8_t)(0x80 | ((rd & 7) << 3) | 0x04));
        secbuf_emit8(s, 0x24);
    } else {
        secbuf_emit8(s, (uint8_t)(0x80 | ((rd & 7) << 3) | (rb & 7)));
    }
    size_t disp_off = s->len;
    secbuf_emit32le(s, 0);
    if (!cg_dry_run) {
        int sidx = objfile_find_sym(cg_obj, label);
        if (sidx < 0) {
            bool il = label[0] == '.';
            sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, il ? SB_LOCAL : SB_GLOBAL, ST_NOTYPE);
        }
        objfile_add_reloc(cg_obj, SEC_TEXT, disp_off, sidx, R_X86_64_TPOFF32, 0);
    }
    return s->len - off;
}

#endif

// Forward-fixup callback for assemble_inline: registers unresolved labels
// into the codegen fixup table so cg_def_label can patch them later.
// patch_off is the offset of the 4-byte REL32 operand in cg_obj->text.
static void cg_inline_fixup_cb(size_t patch_off, const char *label, void *ctx) {
    (void)ctx;
    char *dup = arena_alloc(strlen(label) + 1);
    strcpy(dup, label);
    // instr_off = patch_off - 1 (type=0 patches at instr_off+1); delta formula is identical.
    asm_fixup_ht_add(patch_off - 1, dup, 0);
}

// Bridge for the remaining printf("  <asm line>\n", ...)-based codegen (x86 only):
// assembles the formatted line directly into cg_obj->text via assemble_inline().
// ARM64 uses direct asm_*() wrappers exclusively.
#ifndef ARCH_ARM64
static void cg_emit(const char *fmt, ...) {
    if (cg_dry_run) return;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = '\0';
    if (n == 0) return;
    char *p = buf;
    while (*p == ' ' || *p == '\t') p++;
    size_t len = strlen(p);
    if (len > 1 && p[len - 1] == ':') {
        bool is_label = true;
        for (size_t i = 0; i + 1 < len; i++)
            if (isspace((unsigned char)p[i])) {
                is_label = false;
                break;
            }
        if (is_label) {
            p[len - 1] = '\0';
            cg_def_label(p);
            return;
        }
    }
    assemble_inline(cg_obj, p, cg_inline_fixup_cb, NULL);
}
#define printf(...) cg_emit(__VA_ARGS__)
#endif /* !ARCH_ARM64 */

static uint64_t cg_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}
static Function *current_fn_def;
static TLItem *all_items;
static StrLit *all_strs;

#define FUNC_HASH_SIZE 8192
static TLItem *func_htab[FUNC_HASH_SIZE];

static uint32_t func_hash_name(const char *s) {
    uint32_t h = 2166136261u;
    for (int i = 0; s[i]; i++) {
        h ^= (unsigned char)s[i];
        h *= 16777619;
    }
    return h;
}

static char *current_fn;
static int current_fn_stack_size = 0; // fn->stack_size of the function being generated

// Debug info (DWARF .loc directives, enabled by -g)
static int last_debug_file = 0;
static int last_debug_line = 0;

static int get_debug_file_idx(char *filename) {
    if (!filename) return 1;
    int idx = objfile_add_debug_file(cg_obj, filename);
    return idx;
}

static void emit_loc(Node *node) {
    if (cg_dry_run) return; // Pass 1 uses a throwaway buffer; offsets are not real
    if (!node->tok || !node->tok->filename)
        return;
    int line = node->tok->lineno;
    if (line <= 0) return;
    int fidx = get_debug_file_idx(node->tok->filename);
    if (fidx == last_debug_file && line == last_debug_line)
        return;
    objfile_add_debug_line(cg_obj, cg_sec->len, fidx, line);
    last_debug_file = fidx;
    last_debug_line = line;
}
static int fn_struct_ret_off = 0; // next free offset within the struct-ret-buf area
static int fn_struct_ret_total = 0; // high-water mark of struct-ret-buf space used
static int rcc_label_count = 0;
static int va_gp_start;
static int va_fp_start;
static int va_st_start;
#ifndef ARCH_ARM64
static int va_reg_save_ofs;
#endif
static int break_stack[128];
static int continue_stack[128];
static int ctrl_depth = 0;
static int float_lit_count = 0;

typedef struct FloatLit FloatLit;
struct FloatLit {
    FloatLit *next;
    int id;
    double val;
    int size; // 4=float, 8=double, TODO 12|16=long double
};
static FloatLit *float_lits;
static bool alloca_needed;
static bool fn_uses_alloca;

#ifdef ARCH_ARM64
// Arm64: 12 allocatable GP registers
#define NUM_REGS 12
// x16-x18: reserved, x19-x28: callee-saved, x29: fp, x30: lr, sp/xzr
// We use x10-x15 (6 caller-saved) and x19-x24 (6 callee-saved) = 12 allocatable
static char *reg64[] = {"x10", "x11", "x12", "x13", "x14", "x15", "x19", "x20", "x21", "x22", "x23", "x24"};
static char *reg32[] = {"w10", "w11", "w12", "w13", "w14", "w15", "w19", "w20", "w21", "w22", "w23", "w24"};
// Arm64 has no dedicated 8/16-bit registers; use w regs, mask after load/store
#define reg16 reg32
#define reg8  reg32
#define NUM_REGS 12
#define FRAME_PTR CG_ARM_FP
//#define LINK_REG  "x30"
//#define STACK_REG "sp"

#else
// x86_64: 8 allocatable GP registers
static char *reg64[] = {"%r10", "%r11", "%rbx", "%r12", "%r13", "%r14", "%r15", "%rsi"};
static char *reg32[] = {"%r10d", "%r11d", "%ebx", "%r12d", "%r13d", "%r14d", "%r15d", "%esi"};
static char *reg16[] = {"%r10w", "%r11w", "%bx", "%r12w", "%r13w", "%r14w", "%r15w", "%si"};
static char *reg8[] = {"%r10b", "%r11b", "%bl", "%r12b", "%r13b", "%r14b", "%r15b", "%sil"};
#define NUM_REGS 8
#define FRAME_PTR CG_X86_FP
//#define STACK_REG "rsp"
#endif

static int used_regs = 0;
static int ever_used_regs = 0;

#ifdef ARCH_ARM64
static void emit_mov_imm64(Arm64Reg reg, uint64_t val);

// Arm64: spill to [x29, #-N]; offsets grown from frame base
static int spill_slot[NUM_REGS];
static int next_spill_slot;

static int spill_offset(int r) {
    if (!spill_slot[r]) {
        next_spill_slot += 8;
        spill_slot[r] = next_spill_slot;
    }
    return spill_slot[r];
}
#else
// Spill slot offsets from rbp for register spilling (dynamic, grows from 8)
static int spill_slot[NUM_REGS];
static int next_spill_slot;
static int spill_logand, spill_atomic_old;
#define ALL_REGS_MASK ((1 << NUM_REGS) - 1)

static int alloc_spill_slot(void);

static void init_spill_slots(void) {
    memset(spill_slot, 0, sizeof(spill_slot));
    next_spill_slot = 8;
    spill_logand = alloc_spill_slot();
    spill_atomic_old = alloc_spill_slot();
}

static int alloc_spill_slot(void) {
    int slot = next_spill_slot;
    next_spill_slot += 8;
    return slot;
}

static int spill_offset(int r) {
    if (!spill_slot[r]) {
        spill_slot[r] = alloc_spill_slot();
    }
    return spill_slot[r];
}
#endif

static int spilled_regs = 0;
static int spill_count = 0;
static const char *reg_owner[NUM_REGS];

// Pre-interned builtin name pointers for O(1) pointer-equality matching.
// All function names come from tok->name which is str_intern'd by the lexer,
// so pointer comparison is valid after we intern these literals once.
#define _BI(s) keyword_interned(s, sizeof(s) - 1)
char *bi_bswap16, *bi_bswap32, *bi_bswap64;
char *bi_clz, *bi_clzl, *bi_clzll;
char *bi_ctz, *bi_ctzl, *bi_ctzll;
char *bi_popcount, *bi_popcountl, *bi_popcountll;
char *bi_parity, *bi_parityl, *bi_parityll;
char *bi_clrsb, *bi_clrsbl, *bi_clrsbll;
char *bi_ffs, *bi_ffsl, *bi_ffsll;
char *bi_prefetch, *bi_frame_address, *bi_return_address;
char *bi_setjmp, *bi_longjmp;
char *bi_signbit, *bi_signbitf, *bi_signbitl;
char *bi_isinf, *bi_isinff, *bi_isinfl;
char *bi_copysign, *bi_copysignf, *bi_copysignl;
char *bi_abs, *bi_labs, *bi_llabs;
char *bi_add_overflow, *bi_sub_overflow;
char *bi_mul_overflow, *bi_mul_overflow_p;
char *bi_memset, *bi_memcpy, *bi_memcmp;
char *bi_strlen, *bi_strcmp, *bi_strchr;
char *bi_s_abs, *bi_s_labs, *bi_s_llabs;
char *bi_s_strlen, *bi_s_strcmp, *bi_s_strchr;
char *bi_s_printf, *bi_s_fprintf, *bi_s_vprintf, *bi_s_vfprintf;
char *bi_s_puts, *bi_s_fputs;
char *bi_s_sprintf, *bi_s_snprintf;
char *bi_s_scanf, *bi_s_fscanf, *bi_s_sscanf;
char *bi_s_alloca;
char *bi_chk_printf, *bi_chk_vprintf;
char *bi_chk_fprintf, *bi_chk_vfprintf;
char *bi_s_memset, *bi_s_memcpy, *bi_s_memcmp;
char *bi_strlen, *bi_strcmp, *bi_strchr;
static char *kw_retbuf;

void init_builtin_names(void) {
    static bool done = false;
    if (done) return;
    done = true;
    bi_bswap16 = _BI("__builtin_bswap16");
    bi_bswap32 = _BI("__builtin_bswap32");
    bi_bswap64 = _BI("__builtin_bswap64");
    bi_clz = _BI("__builtin_clz");
    bi_clzl = _BI("__builtin_clzl");
    bi_clzll = _BI("__builtin_clzll");
    bi_ctz = _BI("__builtin_ctz");
    bi_ctzl = _BI("__builtin_ctzl");
    bi_ctzll = _BI("__builtin_ctzll");
    bi_popcount = _BI("__builtin_popcount");
    bi_popcountl = _BI("__builtin_popcountl");
    bi_popcountll = _BI("__builtin_popcountll");
    bi_parity = _BI("__builtin_parity");
    bi_parityl = _BI("__builtin_parityl");
    bi_parityll = _BI("__builtin_parityll");
    bi_clrsb = _BI("__builtin_clrsb");
    bi_clrsbl = _BI("__builtin_clrsbl");
    bi_clrsbll = _BI("__builtin_clrsbll");
    bi_ffs = _BI("__builtin_ffs");
    bi_ffsl = _BI("__builtin_ffsl");
    bi_ffsll = _BI("__builtin_ffsll");
    bi_prefetch = _BI("__builtin_prefetch");
    bi_frame_address = _BI("__builtin_frame_address");
    bi_return_address = _BI("__builtin_return_address");
    bi_setjmp = _BI("__builtin_setjmp");
    bi_longjmp = _BI("__builtin_longjmp");
    bi_signbit = _BI("__builtin_signbit");
    bi_signbitf = _BI("__builtin_signbitf");
    bi_signbitl = _BI("__builtin_signbitl");
    bi_isinf = _BI("__builtin_isinf");
    bi_isinff = _BI("__builtin_isinff");
    bi_isinfl = _BI("__builtin_isinfl");
    bi_copysign = _BI("__builtin_copysign");
    bi_copysignf = _BI("__builtin_copysignf");
    bi_copysignl = _BI("__builtin_copysignl");
    bi_abs = _BI("__builtin_abs");
    bi_labs = _BI("__builtin_labs");
    bi_llabs = _BI("__builtin_llabs");
    bi_add_overflow = _BI("__builtin_add_overflow");
    bi_sub_overflow = _BI("__builtin_sub_overflow");
    bi_mul_overflow = _BI("__builtin_mul_overflow");
    bi_mul_overflow_p = _BI("__builtin_mul_overflow_p");
    bi_memset = _BI("__builtin_memset");
    bi_memcpy = _BI("__builtin_memcpy");
    bi_memcmp = _BI("__builtin_memcmp");
    bi_strlen = _BI("__builtin_strlen");
    bi_strcmp = _BI("__builtin_strcmp");
    bi_strchr = _BI("__builtin_strchr");
    bi_s_abs = _BI("abs");
    bi_s_labs = _BI("labs");
    bi_s_llabs = _BI("llabs");
    bi_s_memset = _BI("memset");
    bi_s_memcpy = _BI("memcpy");
    bi_s_memcmp = _BI("memcmp");
    bi_s_strlen = _BI("strlen");
    bi_s_strcmp = _BI("strcmp");
    bi_s_strchr = _BI("strchr");
    bi_s_printf = _BI("printf");
    bi_s_fprintf = _BI("fprintf");
    bi_s_vprintf = _BI("vprintf");
    bi_s_vfprintf = _BI("vfprintf");
    bi_s_puts = _BI("puts");
    bi_s_fputs = _BI("fputs");
    bi_s_sprintf = _BI("sprintf");
    bi_s_snprintf = _BI("snprintf");
    bi_s_scanf = _BI("scanf");
    bi_s_fscanf = _BI("fscanf");
    bi_s_sscanf = _BI("sscanf");
    bi_s_alloca = _BI("alloca");
    bi_chk_printf = _BI("__printf_chk");
    bi_chk_vprintf = _BI("__vprintf_chk");
    bi_chk_fprintf = _BI("__fprintf_chk");
    bi_chk_vfprintf = _BI("__vfprintf_chk");
    kw_retbuf = _BI("__retbuf");
}

static char *reg(VReg r, int size);
static VReg alloc_reg(void);
static void free_reg(VReg i);
static VReg gen(Node *node);
static VReg gen_addr(Node *node);
static bool is_asm_reserved(const char *name);
static void sign_extend_to(VReg r, int from_size, int to_size);
static void zero_extend_to(VReg r, int from_size, int to_size);
static void emit_scalar_to_complex(int r, Type *from, Type *base, int addr);
static void emit_complex_convert_float(int src, int dst, Type *from, Type *to);
static int alloc_int128_slot(void);
static VReg alloc_int128_addr(void);
static VReg widen_to_int128(VReg val, bool is_unsigned);
static VReg gen_to_int128(Node *operand);
static VReg gen_int128(Node *node);

// Emit a branch with fixup registration (works on both x86 and ARM64)
static size_t emit_jmp_fixup(SecBuf *s, const char *label) {
    size_t off = asm_jmp_label(s);
    asm_fixup_add(s, off, label, 0);
    return off;
}
static size_t emit_jcc_fixup(SecBuf *s, int cond, const char *label) {
    size_t off = asm_jcc_label(s, cond);
    asm_fixup_add(s, off, label, 1);
    return off;
}

static bool var_needs_got(LVar *var) {
    if (var->is_local) return false;
    if (var->is_static) return false;
    return opt_pic;
}

#if 0
static char *func_asm_name(char *name) {
    for (TLItem *item = all_items; item; item = item->next) {
        if (item->kind == TL_FUNC && item->fn->name == name)
            return item->fn->asm_name ? item->fn->asm_name : item->fn->name;
    }
    return name;
}
#endif

// Mach-O symbols get a leading underscore
static const char *sym_name(const char *name) {
#if defined(__APPLE__)
    if (name[0] == '.' || name[0] == '/')
        return name;
    return format("_%s", name);
#else
    return name;
#endif
}

// Assembly label for a variable: respects __asm__ names (used as-is) and
// applies sym_name() to regular C identifiers.
static const char *var_sym_label(LVar *var) {
    if (var->asm_name) return var->asm_name;
    if (is_asm_reserved(var->name)) return format(".L_rcc_%s", var->name);
    return sym_name(var->name);
}

static const char *asm_sym_name(const char *name);
// Assembly label for a function: respects __asm__ names (used as-is) and
// applies sym_name() to regular C identifiers.
static const char *func_label(char *name) {
    uint32_t h = func_hash_name(name) % FUNC_HASH_SIZE;
    for (TLItem *item = func_htab[h]; item; item = item->hash_next)
        if (item->fn->name == name) {
            if (item->fn->asm_name)
                return item->fn->asm_name;
            return asm_sym_name(sym_name(item->fn->name));
        }
    return asm_sym_name(sym_name(name));
}

// Assembly-safe symbol name: quotes non-ASCII identifiers for LLVM assembler
static const char *asm_sym_name(const char *name) {
#if defined(__APPLE__)
    const char *s = name;
    while (*s) {
        if ((unsigned char)*s < 0x20 || (unsigned char)*s > 0x7E)
            return format("\"%s\"", name);
        s++;
    }
#endif
    return name;
}

static void emit_direct_call(char *name) {
    if (cg_dry_run) return;
    if (is_asm_reserved(name))
        name = format(".L_rcc_%s", name);
    const char *label = func_label(name);
    size_t off = asm_call_label(cg_sec); // bl %s
    int sidx = objfile_find_sym(cg_obj, label);
#ifdef ARCH_ARM64
    if (sidx >= 0 && cg_obj->syms[sidx].section == SEC_TEXT) {
        // Same-section function: patch displacement directly, no relocation.
        int32_t disp = (int32_t)((int64_t)cg_obj->syms[sidx].offset - (int64_t)off);
        uint32_t insn = 0x94000000 | (((uint32_t)(disp / 4)) & 0x03FFFFFF);
        secbuf_patch32le(cg_sec, off, insn);
        return;
    }
    if (sidx < 0)
        sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_FUNC);
    objfile_add_reloc(cg_obj, SEC_TEXT, off, sidx, R_AARCH64_CALL26, 0);
#else
    if (sidx >= 0 && cg_obj->syms[sidx].section == SEC_TEXT) {
        // Same-section function: patch displacement directly, no relocation.
        // This avoids blowing the COFF 16-bit per-section relocation limit
        // in files with many local calls (e.g. 101_cleanup).
        int32_t disp = (int32_t)((int64_t)cg_obj->syms[sidx].offset - (int64_t)(off + 5));
        secbuf_patch32le(cg_sec, off + 1, (uint32_t)disp);
        return;
    }
    if (sidx < 0)
        sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_FUNC);
    // call label
    objfile_add_reloc(cg_obj, SEC_TEXT, off + 1, sidx, R_X86_64_PLT32, -4);
#endif
}

static bool var_has_cleanup(LVar *var) {
    if (!var->is_local) return false;
    if (var->cleanup_func) return true;
    return var->ty->kind == TY_ARRAY && var->ty->base && var->ty->base->cleanup_func;
}

static void emit_cleanup_var(LVar *var) {
    if (var->cleanup_func) {
#ifdef ARCH_ARM64
        if (var->offset <= 4095)
            asm_add_fp_imm(cg_sec, ARM64_X0, -var->offset); // add x0, x29, #{-var->offset}
        else {
            int v = var->offset;
            emit_mov_imm64(ARM64_X16, v & 0xffff); // mov x16, #v
            v >>= 16;
            int s = 16;
            while (v) {
                asm_movk(cg_sec, ARM64_X16, 1, (uint16_t)(v & 0xffff), s); // movk x16, #v, lsl #s
                v >>= 16;
                s += 16;
            }
            asm_sub_x0_fp_x16(cg_sec); // sub x0, x29, x16
        }
#elif defined(_WIN32)
        asm_lea_rbp(cg_sec, X86_RCX, 8, var->offset); // lea -offset(%rbp), %rcx
#else
        asm_lea_rbp(cg_sec, X86_RDI, 8, var->offset); // lea -offset(%rbp), %rdi
#endif
        emit_direct_call(var->cleanup_func);
        return;
    }
    // Array whose element type carries __cleanup__: call per element, LIFO
    char *func = var->ty->base->cleanup_func;
    int elem_size = var->ty->base->size;
    int nelem = elem_size ? var->ty->size / elem_size : 0;
    for (int i = nelem - 1; i >= 0; i--) {
#ifdef ARCH_ARM64
        int off = var->offset - i * elem_size;
        if (off <= 4095)
            asm_add_fp_imm(cg_sec, ARM64_X0, -off);
        else {
            int v = off;
            emit_mov_imm64(ARM64_X16, v & 0xffff); // mov x16, #v
            v >>= 16;
            int s = 16;
            while (v) {
                asm_movk(cg_sec, 16, 1, (uint16_t)(v & 0xffff), s); // movk x16, #v, lsl #s
                v >>= 16;
                s += 16;
            }
            asm_sub_x0_fp_x16(cg_sec); // sub x0, x29, x16
        }
#elif defined(_WIN32)
        asm_lea_rbp(cg_sec, X86_RCX, 8, var->offset - i * elem_size); // lea [rbp-off], rcx
#else
        asm_lea_rbp(cg_sec, X86_RDI, 8, var->offset - i * elem_size); // lea [rbp-off], rdi
#endif
        emit_direct_call(func);
    }
}

static void emit_cleanup_range(LVar *begin, LVar *end) {
    for (LVar *var = begin; var && var != end; var = var->next) {
        if (var_has_cleanup(var))
            emit_cleanup_var(var);
    }
}

#ifdef ARCH_ARM64
static int arm64_hfa_info(Type *ty, int *elem_size) {
    if (!ty)
        return -1;
    if (is_flonum(ty) && (ty->size == 4 || ty->size == 8)) {
        if (*elem_size == 0)
            *elem_size = ty->size;
        else if (*elem_size != ty->size)
            return -1;
        return 1;
    }
    if (ty->kind != TY_STRUCT)
        return -1;
    int count = 0;
    for (Member *mem = ty->members; mem; mem = mem->next) {
        int sub = arm64_hfa_info(mem->ty, elem_size);
        if (sub <= 0)
            return -1;
        count += sub;
        if (count > 4)
            return -1;
    }
    return count > 0 ? count : -1;
}

static int arm64_hfa_count(Type *ty, int *elem_size) {
    int es = 0;
    int n = arm64_hfa_info(ty, &es);
    if (n <= 0) {
        if (elem_size)
            *elem_size = 0;
        return 0;
    }
    if (elem_size)
        *elem_size = es;
    return n;
}
#endif

#ifdef ARCH_ARM64
// ARM64: emit load from [fp, #-offset] into x16 (uses x17 for large offsets)
static void arm64_load_from_fp_minus(int offset, Arm64Reg dst) {
    if (offset <= 255)
        asm_ldur_fp_phy(cg_sec, dst, offset); // ldr dst, [x29, #-offset]
    else if (offset <= 4095) {
        asm_sub_fp_imm(cg_sec, ARM64_X17, offset); // sub x17, x29, #offset
        asm_ldr_reg(cg_sec, dst, ARM64_X17); // ldr dst, [x17]
    } else {
        unsigned v = offset;
        emit_mov_imm64(ARM64_X17, (uint64_t)v); // mov r17, #v
        asm_sub_fp_reg(cg_sec, ARM64_X17, ARM64_X17); // sub x17, x29, x17
        asm_ldr_reg(cg_sec, dst, ARM64_X17); // ldr dst, [x17]
    }
}

// ARM64: emit store src to [fp, #-offset] (uses x17 for large offsets)
static void arm64_store_to_fp_minus(int offset) {
    // src fixed to x16
    if (offset <= 255)
        asm_stur_fp_phy(cg_sec, ARM64_X16, offset); // str x16, [x29, #-offset]
    else if (offset <= 4095) {
        asm_sub_fp_imm(cg_sec, ARM64_X17, offset); // sub x17,x29,#offset
        asm_str_reg(cg_sec, ARM64_X16, ARM64_X17); // str x16, [x17]
    } else {
        unsigned v = offset;
        emit_mov_imm64(ARM64_X17, v & 0xffff); // mov x17, #v
        v >>= 16;
        int s = 16;
        while (v) {
            asm_movk(cg_sec, 17, 1, (uint16_t)(v & 0xffff), s); // movk x17, #v, lsl #s
            v >>= 16;
            s += 16;
        }
        asm_sub_fp_reg(cg_sec, ARM64_X17, ARM64_X17); // sub x17, x29, x17
        asm_str_reg(cg_sec, ARM64_X16, ARM64_X17); // str x16, [x17]
    }
}
#endif

// Restore SP past any VLAs leaving scope (enables VLA reuse on re-entry).
// Locals list is newest-first; the last VLA in range is the outermost one.
static void emit_vla_dealloc(LVar *begin, LVar *end) {
    LVar *outermost_vla = NULL;
    for (LVar *v = begin; v && v != end; v = v->next)
        if (v->ty->kind == TY_VLA || ((v->ty->kind == TY_STRUCT || v->ty->kind == TY_UNION) && v->ty->vla_len_expr))
            outermost_vla = v;
    if (outermost_vla) {
#ifdef ARCH_ARM64
        arm64_load_from_fp_minus(outermost_vla->offset, ARM64_X16);
        asm_mov_sp_reg(cg_sec, ARM64_X16); // mov sp, x16
#else
        asm_mov_rbp(cg_sec, X86_RSP, 8, outermost_vla->offset); // mov [rbp-outermost_vla->offse], X86_RSP
#endif
    }
}

/* Other platforms still have it. windows deprecated it.
   Use a unique name to avoid conflicts with CRT import stubs. */
static void emit_alloca(void) {
    cg_global_label("__rcc_alloca");
#ifdef ARCH_ARM64
    // alloca(size): x0=size → round up, sub sp, return new sp
    asm_add_self_imm(cg_sec, ARM64_X0, 15); // add x0, x0, #15
    asm_and_self_imm(cg_sec, ARM64_X0, (uint64_t)(int64_t)(-16)); // and x0, x0, #-16
    asm_sub_sp_sp_reg(cg_sec, ARM64_X0); // sub sp, sp, x0
    asm_mov_reg_sp(cg_sec, ARM64_X0); // mov x0, sp
    asm_ret(cg_sec); // ret
#else
    // alloca via call: return addr was pushed; pop it, adjust rsp, push back
    asm_pop(cg_sec, X86_RDX); // popq %rdx  (save return addr)
#ifdef _WIN32
    x86_mov_rr(cg_sec, 8, X86_RAX, X86_RCX); // movq %rcx, %rax  (Windows: arg in rcx)
#else
    x86_mov_rr(cg_sec, 8, X86_RAX, X86_RDI); // movq %rdi, %rax  (Linux: arg in rdi)
#endif
    x86_add_ri(cg_sec, 8, X86_RAX, 15); // addq $15, %rax
    x86_and_ri(cg_sec, 8, X86_RAX, -16); // andq $-16, %rax
    {
        int c1 = ++rcc_label_count;
        int c2 = ++rcc_label_count;
        cg_def_label(format(".Lalloca1.%d", c1));
        x86_cmp_ri(cg_sec, 8, X86_RAX, 4096); // cmpq $4096, %rax
        size_t jb_off = asm_jcc_label(cg_sec, X86_B); // jb .Lalloca2
        // testq %rax, -4096(%rsp) — page-probe stack guard
        secbuf_emit8(cg_sec, 0x48); // REX.W
        secbuf_emit8(cg_sec, 0x85); // TEST r/m64, r64
        secbuf_emit8(cg_sec, 0x84); // ModRM: mod=10, reg=0(RAX), r/m=4(SIB)
        secbuf_emit8(cg_sec, 0x24); // SIB: base=RSP
        secbuf_emit32le(cg_sec, (uint32_t)-4096); // disp32 = -4096
        x86_sub_ri(cg_sec, 8, X86_RSP, 4096); // subq $4096, %rsp
        x86_sub_ri(cg_sec, 8, X86_RAX, 4096); // subq $4096, %rax
        size_t jmp1 = asm_jmp_label(cg_sec); // jmp .Lalloca1
        asm_fixup_add(cg_sec, jmp1, format(".Lalloca1.%d", c1), 0);
        cg_def_label(format(".Lalloca2.%d", c2));
        asm_fixup_add(cg_sec, jb_off, format(".Lalloca2.%d", c2), 1);
    }
    x86_sub_rr(cg_sec, 8, X86_RSP, X86_RAX); // subq %rax, %rsp
    x86_mov_rr(cg_sec, 8, X86_RAX, X86_RSP); // movq %rsp, %rax
    asm_push(cg_sec, X86_RDX); // pushq %rdx
    asm_ret(cg_sec); // ret
#endif
}

bool va_arg_need_copy(Type *ty) {
    if (ty->size > 8 && ty->size <= 16) {
        if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
            for (Member *m = ty->members; m; m = m->next) {
                if (is_flonum(m->ty))
                    return true;
            }
        }
    }
    return false;
}

static VReg gen_funcall(Node *node, VReg hidden_ret_reg) {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
        nargs++;

    Node *argv_stk[64];
    VReg arg_regs_stk[64];
    int arg_sizes_stk[64];
    bool arg_is_float_stk[64];
    int arg_stage_stk[64]; // per-arg staging slot (0 = not staged)
#ifdef _WIN32
    int arg_gp_idx_stk[64];
    int arg_fp_idx_stk[64];
    int arg_stack_idx_stk[64];
#elif defined(ARCH_ARM64)
    int arg_gp_idx_stk[64];
    int arg_fp_idx_stk[64];
    int arg_stack_idx_stk[64];
    int arg_hfa_count_stk[64];
    int arg_hfa_elem_size_stk[64];
#else
    int arg_gp_idx_stk[64];
    int arg_fp_idx_stk[64];
    int arg_stack_idx_stk[64];
#endif

    Node **argv;
    VReg *arg_regs;
    int *arg_sizes;
    bool *arg_is_float;
    __attribute__((unused)) int *arg_stage;
#ifdef _WIN32
    __attribute__((unused)) int *arg_gp_idx;
    int *arg_fp_idx;
    __attribute__((unused)) int *arg_stack_idx;
#elif defined(ARCH_ARM64)
    int *arg_gp_idx;
    int *arg_fp_idx;
    int *arg_stack_idx;
    int *arg_hfa_count;
    int *arg_hfa_elem_size;
#else
    int *arg_gp_idx;
    int *arg_fp_idx;
    int *arg_stack_idx;
#endif

    if (nargs <= 64) {
        argv = argv_stk;
        arg_regs = arg_regs_stk;
        arg_sizes = arg_sizes_stk;
        arg_is_float = arg_is_float_stk;
        arg_stage = arg_stage_stk;
#ifdef _WIN32
        arg_gp_idx = arg_gp_idx_stk;
        arg_fp_idx = arg_fp_idx_stk;
        arg_stack_idx = arg_stack_idx_stk;
#elif defined(ARCH_ARM64)
        arg_gp_idx = arg_gp_idx_stk;
        arg_fp_idx = arg_fp_idx_stk;
        arg_stack_idx = arg_stack_idx_stk;
        arg_hfa_count = arg_hfa_count_stk;
        arg_hfa_elem_size = arg_hfa_elem_size_stk;
#else
        arg_gp_idx = arg_gp_idx_stk;
        arg_fp_idx = arg_fp_idx_stk;
        arg_stack_idx = arg_stack_idx_stk;
#endif
    } else {
        argv = arena_alloc(sizeof(Node *) * nargs);
        arg_regs = arena_alloc(sizeof(VReg) * nargs);
        arg_sizes = arena_alloc(sizeof(int) * nargs);
        arg_is_float = arena_alloc(sizeof(bool) * nargs);
        arg_stage = arena_alloc(sizeof(int) * nargs);
#ifdef _WIN32
        arg_gp_idx = arena_alloc(sizeof(int) * nargs);
        arg_fp_idx = arena_alloc(sizeof(int) * nargs);
        arg_stack_idx = arena_alloc(sizeof(int) * nargs);
#elif defined(ARCH_ARM64)
        arg_gp_idx = arena_alloc(sizeof(int) * nargs);
        arg_fp_idx = arena_alloc(sizeof(int) * nargs);
        arg_stack_idx = arena_alloc(sizeof(int) * nargs);
        arg_hfa_count = arena_alloc(sizeof(int) * nargs);
        arg_hfa_elem_size = arena_alloc(sizeof(int) * nargs);
#else
        arg_gp_idx = arena_alloc(sizeof(int) * nargs);
        arg_fp_idx = arena_alloc(sizeof(int) * nargs);
        arg_stack_idx = arena_alloc(sizeof(int) * nargs);
#endif
    }

    int idx = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
        argv[idx++] = arg;

    char *call_target = node->funcname;
    if (!call_target && node->lhs && node->lhs->var && node->lhs->var->is_function)
        call_target = node->lhs->var->name;
    if (call_target && is_asm_reserved(call_target))
        call_target = format(".L_rcc_%s", call_target);
    init_builtin_names();
    if (0 && opt_O1 && call_target && nargs >= 2) {
        if ((strcmp(call_target, "__printf_chk") == 0 && nargs >= 2) ||
            (strcmp(call_target, "__vprintf_chk") == 0 && nargs == 3)) {
            node->args = argv[1];
            call_target = call_target[2] == 'p' ? bi_s_printf : bi_s_vprintf;
            for (int j = 0; j < nargs - 1; j++)
                argv[j] = argv[j + 1];
            nargs--;
        } else if ((strcmp(call_target, "__fprintf_chk") == 0 && nargs >= 3) ||
                   (strcmp(call_target, "__vfprintf_chk") == 0 && nargs == 4)) {
            Node *flag = argv[1];
            argv[0]->next = flag->next;
            flag->next = NULL;
            call_target = call_target[2] == 'f' ? bi_s_fprintf : bi_s_vfprintf;
            for (int j = 1; j < nargs - 1; j++)
                argv[j] = argv[j + 1];
            nargs--;
            if (node->args == flag)
                node->args = argv[0];
        }
    }

    // Optimize // Optimize hint; // %s
    //          (void)fprintf(fp, "%s", arg) → fputs(arg, fp)
    // puts/fputs return values differ from printf/fprintf, so only when unused.
    bool discard_result = cg_discard_result;
    cg_discard_result = false;
    if (discard_result && opt_O1 && call_target &&
        !(node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION))) {
        if (nargs == 2 && call_target == bi_s_printf) {
            Node *fmt = node->args;
            if (fmt && fmt->kind == ND_STR) {
                for (StrLit *s = all_strs; s; s = s->next) {
                    if (s->id == fmt->str_id && s->prefix == 0 && strcmp(s->str, "%s\n") == 0) {
                        call_target = bi_s_puts;
                        node->args = fmt->next;
                        nargs = 1;
                        argv[0] = node->args;
                        break;
                    }
                }
            }
        } else if (nargs == 3 && call_target == bi_s_fprintf) {
            Node *fp_arg = node->args;
            Node *fmt = fp_arg ? fp_arg->next : NULL;
            if (fmt && fmt->kind == ND_STR) {
                for (StrLit *s = all_strs; s; s = s->next) {
                    if (s->id == fmt->str_id && s->prefix == 0 && strcmp(s->str, "%s") == 0) {
                        call_target = bi_s_fputs;
                        Node *str_arg = fmt->next;
                        node->args = str_arg;
                        str_arg->next = fp_arg;
                        fp_arg->next = NULL;
                        nargs = 2;
                        argv[0] = node->args;
                        argv[1] = fp_arg;
                        break;
                    }
                }
            }
        }
    }

#ifdef _WIN32
    //char *argreg32[] = {"%ecx", "%edx", "%r8d", "%r9d"};
    //char *argreg64[] = {"%rcx", "%rdx", "%r8", "%r9"};
    X86Reg cg_x86_argreg[] = {X86_RCX, X86_RDX, X86_R8, X86_R9};
    //char *argxmm[] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3"};
    int shadow_space = 32;
    int max_gp_args = 4;
#elif defined(ARCH_ARM64)
    // AAPCS64: 8 GP arg regs (x0-x7), 8 SIMD/FP arg regs (v0-v7)
    // x8 is the indirect result register for struct returns
    //char *argreg32[] = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"};
    //char *argreg64[] = {"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"};
    //char *argxmm[] = {"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"};
    int max_gp_args = 8;
    int max_fp_args = 8;
    //int shadow_space = 0;
#else
    //char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    //char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    X86Reg cg_x86_argreg[] = {X86_RDI, X86_RSI, X86_RDX, X86_RCX, X86_R8, X86_R9};
    //char *argxmm[] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"};
    int max_gp_args = 6;
    int max_fp_args = 8;
    int shadow_space = 0;
#endif

    bool has_hidden_retbuf = node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || (node->ty->kind == TY_COMPLEX
#ifdef _WIN32
                                                                                                        && node->ty->size > 8
#endif
                                                                                                        ));

    // Cross-architecture builtins (x86_64 and arm64)
    if (call_target && !has_hidden_retbuf) {
        // Quick prefix check: all names here start with "__builtin_" or are
        // short aliases (abs/labs/llabs). Skip the whole block for everything else.
        bool maybe_builtin = (call_target[0] == '_' && call_target[1] == '_') ||
            call_target == bi_s_abs || call_target == bi_s_labs || call_target == bi_s_llabs;
        bool is_bswap16 = false, is_bswap32 = false, is_bswap64 = false;
        bool is_clz = false, is_clzl = false, is_clzll = false;
        bool is_ctz = false, is_ctzl = false, is_ctzll = false;
        bool is_popcnt = false, is_popcntl = false, is_popcntll = false;
        bool is_parity = false, is_parityl = false, is_parityll = false;
        bool is_clrsb = false, is_clrsbl = false, is_clrsbll = false;
        bool is_ffs = false, is_ffsl = false, is_ffsll = false;
        bool is_prefetch = false, is_frame_addr = false, is_ret_addr = false;
        bool is_setjmp = false, is_longjmp = false;
        bool is_signbit = false, is_isinf = false, is_copysign_builtin = false, is_abs_builtin = false;
        bool is_add_overflow = false, is_sub_overflow = false;
        bool is_mul_overflow = false, is_mul_overflow_p = false;
        if (maybe_builtin) {
            is_bswap16 = call_target == bi_bswap16;
            is_bswap32 = call_target == bi_bswap32;
            is_bswap64 = call_target == bi_bswap64;
            is_clz = call_target == bi_clz;
            is_clzl = call_target == bi_clzl;
            is_clzll = call_target == bi_clzll;
            is_ctz = call_target == bi_ctz;
            is_ctzl = call_target == bi_ctzl;
            is_ctzll = call_target == bi_ctzll;
            is_popcnt = call_target == bi_popcount;
            is_popcntl = call_target == bi_popcountl;
            is_popcntll = call_target == bi_popcountll;
            is_parity = call_target == bi_parity;
            is_parityl = call_target == bi_parityl;
            is_parityll = call_target == bi_parityll;
            is_clrsb = call_target == bi_clrsb;
            is_clrsbl = call_target == bi_clrsbl;
            is_clrsbll = call_target == bi_clrsbll;
            is_ffs = call_target == bi_ffs;
            is_ffsl = call_target == bi_ffsl;
            is_ffsll = call_target == bi_ffsll;
            is_prefetch = call_target == bi_prefetch;
            is_frame_addr = call_target == bi_frame_address;
            is_ret_addr = call_target == bi_return_address;
            is_setjmp = call_target == bi_setjmp;
            is_longjmp = call_target == bi_longjmp;
            is_signbit = call_target == bi_signbit ||
                call_target == bi_signbitf ||
                call_target == bi_signbitl;
            is_isinf = call_target == bi_isinf ||
                call_target == bi_isinff ||
                call_target == bi_isinfl;
            is_copysign_builtin = call_target == bi_copysign ||
                call_target == bi_copysignf ||
                call_target == bi_copysignl;
            is_abs_builtin = call_target == bi_abs ||
                call_target == bi_labs ||
                call_target == bi_llabs ||
                call_target == bi_s_abs ||
                call_target == bi_s_labs ||
                call_target == bi_s_llabs;
            is_add_overflow = call_target == bi_add_overflow;
            is_sub_overflow = call_target == bi_sub_overflow;
            is_mul_overflow = call_target == bi_mul_overflow;
            is_mul_overflow_p = call_target == bi_mul_overflow_p;
        }

        if (is_bswap16 || is_bswap32 || is_bswap64) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
#ifdef ARCH_ARM64
                if (is_bswap16) {
                    asm_rev16(cg_sec, r, 4); // rev16 wr, wr
                    asm_and_imm(cg_sec, r, 4, 0xffff); // and wr, wr, #0xffff
                } else if (is_bswap32) {
                    asm_rev(cg_sec, r, 4); // rev wr, wr
                } else {
                    asm_rev(cg_sec, r, 8); // rev xr, xr
                }
#else
                if (is_bswap16) {
                    asm_rol_imm(cg_sec, r, 2, 8); // rol $8, %rx16
                    asm_movzx(cg_sec, r, r, 4, 2); // movzx4->r rr, rr
                } else if (is_bswap32) {
                    asm_bswap(cg_sec, r, 4); // bswap rr
                } else {
                    asm_bswap(cg_sec, r, 8); // bswap rr
                }
#endif
                return r;
            }
        }

        if (is_clz || is_clzl || is_clzll) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
                VReg r2 = alloc_reg();
                bool is64 = is_clzll || (is_clzl && sizeof(long) == 8);
#ifdef ARCH_ARM64
                if (is64)
                    asm_clz(cg_sec, r2, r, 8); // clz rr2
                else
                    asm_clz(cg_sec, r2, r, 4); // clz rr2
#else
                if (is64) {
                    asm_clz(cg_sec, r2, r, 8); // lzcnt r64, r2_64
                } else {
                    asm_clz(cg_sec, r2, r, 4); // lzcnt r32, r2_32
                }
#endif
                free_reg(r);
                return r2;
            }
        }

        if (is_ctz || is_ctzl || is_ctzll) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
                VReg r2 = alloc_reg();
                bool is64 = is_ctzll || (is_ctzl && sizeof(long) == 8);
#ifdef ARCH_ARM64
                if (is64) {
                    asm_rbit(cg_sec, r, r, 8); // rbit xr, xr
                    asm_clz(cg_sec, r2, r, 8); // clz rr2
                } else {
                    asm_rbit(cg_sec, r, r, 4); // rbit wr, wr
                    asm_clz(cg_sec, r2, r, 4); // clz rr2
                }
#else
                if (is64) {
                    asm_tzcnt(cg_sec, r2, r, 8); // tzcnt r64, r2_64
                } else {
                    asm_tzcnt(cg_sec, r2, r, 4); // tzcnt r32, r2_32
                }
#endif
                free_reg(r);
                return r2;
            }
        }

        if (is_popcnt || is_popcntl || is_popcntll || is_parity || is_parityl || is_parityll) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
                VReg r2 = alloc_reg();
                bool is64 = is_popcntll || is_parityll ||
                    ((is_popcntl || is_parityl) && sizeof(long) == 8);
#ifdef ARCH_ARM64
                // Software popcount via NEON cnt
                char *tmp = is64 ? "v30.8b" : "v30.8b";
                if (is64) {
                    asm_fmov_gp_to_d30(cg_sec, r); // fmov d30, x{r}
                } else {
                    asm_fmov_gp_to_s30(cg_sec, r); // fmov s30, w{r}
                }
                asm_neon_cnt_v30(cg_sec); // cnt v30.8b, v30.8b
                asm_neon_addv_b30(cg_sec); // addv b30, v30.8b
                asm_fmov_s30_to_gp(cg_sec, r2); // fmov w{r2}, s30
                asm_and_imm(cg_sec, r2, 4, 0xff); // and w{r2}, w{r2}, #0xff
                (void)tmp;
#else
                if (is64) {
                    asm_popcnt(cg_sec, r2, r, 8); // popcnt r64, r2_64
                } else {
                    asm_popcnt(cg_sec, r2, r, 4); // popcnt r32, r2_32
                }
#endif
                if (is_parity || is_parityl || is_parityll)
#ifdef ARCH_ARM64
                    asm_and_imm(cg_sec, r2, 4, 1); // and w{r2}, w{r2}, #1
#else
                    asm_and_imm(cg_sec, r2, 4, 1); // and $1, %r32
#endif
                free_reg(r);
                return r2;
            }
        }

        if (is_clrsb || is_clrsbl || is_clrsbll) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
                VReg r2 = alloc_reg();
                bool is64 = is_clrsbll || (is_clrsbl && sizeof(long) == 8);
#ifdef ARCH_ARM64
                if (is64) {
                    asm_cls(cg_sec, r2, r, 8); // cls x{r2}, x{r}
                } else {
                    asm_cls(cg_sec, r2, r, 4); // cls w{r2}, w{r}
                }
#else
                // clrsb(x) = (x>=0 ? clz(x) : clz(~x)) - 1
                int lbl = ++rcc_label_count;
                int r3 = alloc_reg();
                if (is64) {
                    asm_mov_reg_reg(cg_sec, r3, r, 8); // mov r -> r3 (copy)
                    asm_sar_imm(cg_sec, r3, 8, 63); // sar $63, r3_64 (r3 = sign)
                    asm_xor_reg_reg(cg_sec, r, r3, 8); // r ^= r3 (flip bits if negative)
                    asm_clz(cg_sec, r2, r, 8); // lzcnt r64, r2_64
                    asm_dec(cg_sec, r2, 8); // dec rr2
                } else {
                    asm_mov_reg_reg(cg_sec, r3, r, 4); // mov r -> r3 (copy)
                    asm_sar_imm(cg_sec, r3, 4, 31); // sar $31, r3_32 (r3 = sign)
                    asm_xor_reg_reg(cg_sec, r, r3, 4); // r ^= r3 (flip bits if negative)
                    asm_clz(cg_sec, r2, r, 4); // lzcnt r32, r2_32
                    asm_dec(cg_sec, r2, 4); // dec rr2
                }
                free_reg(r3);
                (void)lbl;
#endif
                free_reg(r);
                return r2;
            }
        }

        if (is_ffs || is_ffsl || is_ffsll) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
                VReg r2 = alloc_reg();
                int r3 = alloc_reg();
                bool is64 = is_ffsll || (is_ffsl && sizeof(long) == 8);
#ifdef ARCH_ARM64
                if (is64) {
                    asm_rbit(cg_sec, r, r, 8); // rbit xr, xr
                    asm_clz(cg_sec, r3, r, 8); // clz rr3
                    asm_add_imm(cg_sec, r3, 8, 1); // add $1, rr3
                    asm_cmp_zero(cg_sec, r, 8); // cmp $0, rr
                    asm_csel_zero_if_eq(cg_sec, r2, r3, 8); // csel r2, xzr, r3, eq
                } else {
                    asm_rbit(cg_sec, r, r, 4); // rbit wr, wr
                    asm_clz(cg_sec, r3, r, 4); // clz rr3
                    asm_add_imm(cg_sec, r3, 4, 1); // add $1, rr3
                    asm_cmp_zero(cg_sec, r, 4); // cmp $0, rr
                    asm_csel_zero_if_eq(cg_sec, r2, r3, 4); // csel r2, wzr, r3, eq
                }
#else
                if (is64) {
                    asm_movq_zero(cg_sec, r2); // xor rr2, rr2
                    asm_bsf(cg_sec, r3, r, 8); // bsf r64, r3_64
                    asm_lea_disp1(cg_sec, r3, r3, 8); // leaq 1(r3_64), r3_64
                    asm_cmov(cg_sec, r2, r3, 8, X86_NZ); // cmovnz r3_64, r2_64
                } else {
                    asm_movl_zero(cg_sec, r2); // xor r2_32, r2_32
                    asm_bsf(cg_sec, r3, r, 4); // bsf r32, r3_32
                    asm_lea_disp1(cg_sec, r3, r3, 4); // leal 1(r3_32), r3_32
                    asm_cmov(cg_sec, r2, r3, 4, X86_NZ); // cmovnz r3_32, r2_32
                }
#endif
                free_reg(r3);
                free_reg(r);
                return r2;
            }
        }

        if (is_prefetch) {
            Node *addr = node->args;
            int rw = 0, locality = 3;
            // Parse rw and locality from constant args
            if (addr && addr->next && addr->next->kind == ND_NUM)
                rw = (int)addr->next->val;
            if (addr && addr->next && addr->next->next && addr->next->next->kind == ND_NUM)
                locality = (int)addr->next->next->val;
            if (addr) {
                VReg r = gen(addr);
#ifdef ARCH_ARM64
                {
                    // prfm: pldl1keep=0, pldl1strm=1, pstl1keep=9, pstl1strm=13
                    int prfop = 0; // pldl1keep
                    if (rw == 1 && locality == 0) prfop = 13; // pstl1strm
                    else if (rw == 1)
                        prfop = 9; // pstl1keep
                    else if (locality == 0)
                        prfop = 1; // pldl1strm
                    asm_prfm(cg_sec, r, prfop); // prfm hint, [x{r}]
                }
#else
                // x86: prefetchw for write, otherwise nta/t0/t1/t2 by locality
                {
                    // hint: 0=prefetchnta, 2=prefetcht1, 1=prefetcht2, 3=prefetcht0, 4=prefetchw
                    int hint_idx = (rw == 1) ? 4 : locality == 0 ? 0
                        : locality == 1                          ? 1
                        : locality == 2                          ? 2
                                                                 : 3;
                    asm_prefetch(cg_sec, r, hint_idx); // prefetchXX (%reg)
                }
#endif
                free_reg(r);
                // Evaluate remaining args for side effects (if any expressions)
                for (Node *a = addr->next; a; a = a->next) {
                    if (a->kind != ND_NUM) {
                        int ar = gen(a);
                        if (ar >= 0) free_reg(ar);
                    }
                }
            }
            return -1; // void
        }

        if (is_signbit) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r_arg = gen(arg);
#ifdef ARCH_ARM64
                asm_shr_imm(cg_sec, r_arg, 8, 63); // shr $63, r_arg
#else
                VReg r = alloc_reg();
                asm_movq_r_xmm(cg_sec, 0, r_arg); // movq r_arg, %xmm0
                asm_movq_xmm_r(cg_sec, r, 0); // movq %xmm0, r
                asm_shr_imm(cg_sec, r, 8, 63); // shrq $63, rr
                free_reg(r_arg);
                return r;
#endif
                free_reg(r_arg);
                return r_arg;
            }
        }

        /* __builtin_isinf(x): true if exponent all-1s, mantissa 0.
         * On x86: clear sign bit, compare against the inf bit pattern. */
        if (is_isinf) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r_arg = gen(arg);
                VReg r = alloc_reg();
                VReg r_tmp = alloc_reg();
#ifdef ARCH_ARM64
                // rcc promotes all floats to doubles in GP registers;
                // use 64-bit operations regardless of the source type size
                asm_mov_reg_reg(cg_sec, r, r_arg, 8);
                emit_mov_imm64(REG(r_tmp), 0x7fffffffffffffffULL);
                asm_and_reg_reg(cg_sec, r, r_tmp, 8);
                emit_mov_imm64(REG(r_tmp), 0x7ff0000000000000ULL);
                asm_cmp_reg_reg(cg_sec, r, r_tmp, 8);
                asm_cset(cg_sec, r, ARM64_EQ);
#else
                // Same on x86_64: floats are stored as doubles in GP regs
                asm_mov_reg_reg(cg_sec, r, r_arg, 8); // movq r_arg, r
                asm_movabs_phy(cg_sec, REG(r_tmp), 0x7fffffffffffffffULL); // movabsq $0x7fff..., r_tmp
                asm_and_reg_reg(cg_sec, r, r_tmp, 8); // andq r_tmp, r (clear sign)
                asm_movabs_phy(cg_sec, REG(r_tmp), 0x7ff0000000000000ULL); // movabsq $0x7ff0..., r_tmp
                asm_cmp_reg_reg(cg_sec, r, r_tmp, 8); // cmpq r_tmp, r
                asm_setcc(cg_sec, X86_RAX, X86_E); // sete %al
                asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1); // movzbl %al, %er
#endif
                free_reg(r_tmp);
                free_reg(r_arg);
                return r;
            }
        }

        if (is_copysign_builtin) {
            Node *arg1 = node->args;
            Node *arg2 = arg1 ? arg1->next : NULL;
            if (arg1 && arg2 && !arg2->next) {
                VReg r_x = gen(arg1);
                VReg r_y = gen(arg2);
                VReg r_tmp = alloc_reg();
#ifdef ARCH_ARM64
                // sign_mask = 0x8000000000000000
                asm_mov_imm(cg_sec, r_tmp, 8, (int64_t)0x8000000000000000ULL); // mov r_tmp, #-9223372036854775808
                asm_and_reg_reg(cg_sec, r_y, r_tmp, 8); // and r_y, r_y, r_tmp
                asm_not(cg_sec, r_tmp, 8); // mvn r_tmp, r_tmp
                asm_and_reg_reg(cg_sec, r_x, r_tmp, 8); // and r_x, r_x, r_tmp
                asm_or_reg_reg(cg_sec, r_x, r_y, 8); // orr r_x, r_x, r_y
#else
                // sign_mask = 0x8000000000000000
                asm_movabs_phy(cg_sec, REG(r_tmp), 0x8000000000000000ULL); // movabsq $sign_mask, r_tmp
                asm_and_reg_reg(cg_sec, r_y, r_tmp, 8); // andq r_tmp, r_y (y_sign)
                asm_not(cg_sec, r_tmp, 8); // notq r_tmp (~sign_mask)
                asm_and_reg_reg(cg_sec, r_x, r_tmp, 8); // andq r_tmp, r_x (|x|)
                asm_or_reg_reg(cg_sec, r_x, r_y, 8); // orq r_y, r_x (|x|+sign(y))
#endif
                free_reg(r_tmp);
                free_reg(r_y);
                return r_x;
            }
        }

        if (is_abs_builtin) {
            Node *arg = node->args;
            if (arg && !arg->next) {
                VReg r = gen(arg);
#ifdef ARCH_ARM64
                int arg_size = (arg->ty && !is_flonum(arg->ty)) ? arg->ty->size : 8;
                if (arg_size <= 4 && !(arg->ty && arg->ty->is_unsigned))
                    asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
                asm_cmp_zero(cg_sec, r, 8); // cmp $0, rr
                asm_cneg_mi(cg_sec, r, 8); // cneg x{r}, x{r}, mi
                return r;
#else
                VReg r2 = alloc_reg();
                int arg_size = (arg->ty && !is_flonum(arg->ty)) ? arg->ty->size : 8;
                if (arg_size <= 4 && !(arg->ty && arg->ty->is_unsigned))
                    asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
                asm_mov_reg_reg(cg_sec, r2, r, 8); // mov rr -> rr2 (copy for sign computation)
                asm_sar_imm(cg_sec, r2, 8, 63); // sarq $63, r2_64
                asm_xor_reg_reg(cg_sec, r, r2, 8); // xorq r2, r (r ^= sign)
                asm_sub_reg_reg(cg_sec, r, r2, 8); // subq r2, r
                free_reg(r2);
                return r;
#endif
            }
        }

        if (is_frame_addr) {
            Node *arg = node->args;
            int r = alloc_reg();
            int depth = (arg && arg->kind == ND_NUM) ? (int)arg->val : 0;
#ifdef ARCH_ARM64
            arm64_orr_reg(cg_sec, 1, REG(r), ARM64_XZR, CG_ARM_FP, ARM64_LSL, 0); // mov r, x29
            for (int i = 0; i < depth; i++)
                asm_ldr_reg_off(cg_sec, r, r, 8, 0); // ldr x{r}, [x{r}]
#else
            x86_mov_rr(cg_sec, 8, REG(r), CG_X86_FP); // mov %rbp, r
            for (int i = 0; i < depth; i++)
                asm_mov_indir(cg_sec, r, 8); // mov (%rr), %rr
#endif
            return r;
        }
        if (is_ret_addr) {
            Node *arg = node->args;
            int r = alloc_reg();
            int depth = (arg && arg->kind == ND_NUM) ? (int)arg->val : 0;
            // Follow frame pointer chain to find the return address.
            // frame pointer → [fp] = saved fp, [fp+8] = return address
#ifdef ARCH_ARM64
            arm64_orr_reg(cg_sec, 1, REG(r), ARM64_XZR, CG_ARM_FP, ARM64_LSL, 0); // mov r, x29
            for (int i = 0; i < depth; i++)
                asm_ldr_reg_off(cg_sec, r, r, 8, 0); // ldr x{r}, [x{r}]
            asm_ldr_reg_off(cg_sec, r, r, 8, 8); // ldr x{r}, [x{r}, #8]  (return addr)
#else
            x86_mov_rr(cg_sec, 8, REG(r), CG_X86_FP); // mov %rbp, r
            for (int i = 0; i < depth; i++)
                asm_mov_indir(cg_sec, r, 8); // mov (%rr), %rr
            asm_mov_indir_disp(cg_sec, r, 8, 8); // mov 8(%rr), %rr
#endif
            return r;
        }
        if (is_setjmp) {
            // Inline __builtin_setjmp: save fp, resume_addr, sp to buf; return 0 or 1 (longjmp)
            int rbuf = gen(node->args);
            int c = ++rcc_label_count;
            int r = alloc_reg();
#ifdef ARCH_ARM64
            asm_str_fp_reg(cg_sec, rbuf); // str x29, [x{rbuf}]  (buf[0] = fp)
            asm_adr_x16_label(cg_sec, format(".L.setjmp.%d", c)); // adr x16, .L.setjmp.c (resume addr)
            asm_str_x16_reg_uoff(cg_sec, rbuf, 8); // str x16, [x{rbuf}, #8]   (buf[1] = resume addr)
            asm_mov_reg_sp(cg_sec, ARM64_X16); // mov x16, sp
            asm_str_x16_reg_uoff(cg_sec, rbuf, 16); // str x16, [x{rbuf}, #16]  (buf[2] = sp)
            asm_mov_imm(cg_sec, r, 4, 0); // mov r, #0  (setjmp returns 0 normally)
            {
                size_t jmp_off = asm_jmp_label(cg_sec); // b .L.setjmp_end.c (skip resume block)
                cg_def_label(format(".L.setjmp.%d", c)); // .L.setjmp.c: (longjmp jumps here)
                asm_mov_retval(cg_sec, r, 8); // mov r, x0  (longjmp return value)
                asm_fixup_add(cg_sec, jmp_off, format(".L.setjmp_end.%d", c), 0);
                cg_def_label(format(".L.setjmp_end.%d", c)); // end of setjmp inline
            }
#else
            // x86_64: buf[0]=rbp, buf[1]=resume addr, buf[2]=rsp
            x86_mov_mr(cg_sec, 8, x86_mem(REG(rbuf), 0), X86_RBP); // movq %rbp, (rbuf)
            asm_lea_rip_reg(cg_sec, r, format(".L.sjr.%d", c)); // leaq .L.sjr.c(%rip), r
            x86_mov_mr(cg_sec, 8, x86_mem(REG(rbuf), 8), REG(r)); // movq r, 8(rbuf)
            x86_mov_mr(cg_sec, 8, x86_mem(REG(rbuf), 16), X86_RSP); // movq %rsp, 16(rbuf)
            x86_xor_rr(cg_sec, 4, X86_RAX, X86_RAX); // xorl %eax, %eax
            {
                size_t jmp_off = asm_jmp_label(cg_sec); // jmp .L.sja.c
                cg_def_label(format(".L.sjr.%d", c)); // .L.sjr.c: (longjmp lands here, val in %rax)
                asm_fixup_add(cg_sec, jmp_off, format(".L.sja.%d", c), 0);
                cg_def_label(format(".L.sja.%d", c)); // .L.sja.c:
            }
            x86_mov_rr(cg_sec, 8, REG(r), X86_RAX); // movq %rax, r
#endif
            free_reg(rbuf);
            return r;
        }
        if (is_longjmp) {
            // Inline __builtin_longjmp: restore fp, sp from buf; jump to resume addr with val
            int rbuf = gen(node->args);
            int rval = gen(node->args->next);
#ifdef ARCH_ARM64
            asm_ldr_fp_reg(cg_sec, rbuf); // ldr x29, [x{rbuf}]   (buf[0] → restore fp)
            asm_ldr_x16_reg_uoff(cg_sec, rbuf, 16); // ldr x16, [x{rbuf}, #16] (buf[2] = saved sp)
            asm_mov_sp_reg(cg_sec, ARM64_X16); // mov sp, x16
            asm_ldr_x16_reg_uoff(cg_sec, rbuf, 8); // ldr x16, [x{rbuf}, #8]  (buf[1] = resume addr)
            asm_mov_x0_reg(cg_sec, REG(rval)); // mov x0, x{rval}  (setjmp return value)
            arm64_br(cg_sec, ARM64_X16); // br x16
#else
            // x86_64: restore rbp, load rax (val) and resume addr, then restore rsp, jmp
            int rtmp = alloc_reg();
            x86_mov_rm(cg_sec, 8, X86_RBP, x86_mem(REG(rbuf), 0)); // movq (rbuf), %rbp
            x86_mov_rm(cg_sec, 8, REG(rtmp), x86_mem(REG(rbuf), 8)); // movq 8(rbuf), rtmp
            x86_mov_rr(cg_sec, 8, X86_RAX, REG(rval)); // movq rval, %rax
            x86_mov_rm(cg_sec, 8, X86_RSP, x86_mem(REG(rbuf), 16)); // movq 16(rbuf), %rsp
            asm_jmp_reg(cg_sec, rtmp); // jmp *rtmp
            free_reg(rtmp);
#endif
            free_reg(rbuf);
            free_reg(rval);
            return -1;
        }
#ifdef ARCH_ARM64
        if (is_add_overflow || is_sub_overflow || is_mul_overflow || is_mul_overflow_p) {
            Node *arga = node->args;
            Node *argb = arga ? arga->next : NULL;
            Node *argres = argb ? argb->next : NULL;
            int ra = gen(arga);
            int rb = gen(argb);
            int sz = arga && arga->ty && arga->ty->size > 4 ? 8 : 4;
            // Result type determines overflow check signedness and store width
            int res_sz = sz;
            bool res_unsigned = arga && arga->ty && arga->ty->is_unsigned;
            if (argres && argres->ty && argres->ty->kind == TY_PTR && argres->ty->base) {
                res_sz = argres->ty->base->size > 4 ? 8 : 4;
                res_unsigned = argres->ty->base->is_unsigned;
            }
            int r_result = alloc_reg();
            if (is_add_overflow) {
                if (res_sz == sz) {
                    asm_adds(cg_sec, ra, rb, sz); // adds ra, ra, rb
                    asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CS : ARM64_VS); // cset r_result, cs/vs
                } else if (res_sz < sz) {
                    asm_adds(cg_sec, ra, rb, sz); // adds ra, ra, rb
                    if (res_unsigned) {
                        if (res_sz == 2) {
                            arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 16, 16); // ubfx w{r_result}, w{ra}, #16, #16
                            asm_cmp_zero(cg_sec, r_result, 4); // cmp w{r_result}, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        } else if (res_sz == 1) {
                            arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 8, 24); // ubfx w{r_result}, w{ra}, #8, #24
                            asm_cmp_zero(cg_sec, r_result, 4); // cmp w{r_result}, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        } else {
                            asm_lsr_rd_rn_imm(cg_sec, r_result, ra, 8, (uint8_t)(res_sz * 8)); // lsr x{r_result}, x{ra}, #(res_sz*8)
                            asm_cmp_zero(cg_sec, r_result, 8); // cmp x{r_result}, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        }
                    } else {
                        int rt = alloc_reg();
                        if (res_sz == 2) arm64_sxth(cg_sec, 0, REG(rt), REG(ra)); // sxth w{rt}, w{ra}
                        else
                            arm64_sxtb(cg_sec, 0, REG(rt), REG(ra)); // sxtb w{rt}, w{ra}
                        asm_cmp_reg_reg(cg_sec, rt, ra, sz); // cmp rt, ra
                        asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        free_reg(rt);
                    }
                } else {
                    // Widening: sign/zero-extend operands, do wider add
                    if (sz == 4) {
                        if (arga->ty && arga->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(ra), REG(ra)); // uxtw x{ra}, w{ra}
                        else
                            asm_sxtw(cg_sec, REG(ra), REG(ra)); // sxtw x{ra}, w{ra}
                        if (argb->ty && argb->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(rb), REG(rb)); // uxtw x{rb}, w{rb}
                        else
                            asm_sxtw(cg_sec, REG(rb), REG(rb)); // sxtw x{rb}, w{rb}
                    }
                    asm_adds(cg_sec, ra, rb, 8); // adds ra, ra, rb
                    if (res_unsigned && (!arga->ty || !arga->ty->is_unsigned || !argb->ty || !argb->ty->is_unsigned)) {
                        arm64_ands_reg(cg_sec, 1, ARM64_XZR, REG(ra), REG(ra), ARM64_LSL, 0); // tst ra, ra
                        asm_cset(cg_sec, r_result, ARM64_MI); // cset r_result, mi
                    } else {
                        asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CS : ARM64_VS); // cset r_result, cs/vs
                    }
                }
            } else if (is_sub_overflow) {
                if (res_sz == sz) {
                    asm_subs(cg_sec, ra, rb, sz); // subs ra, ra, rb
                    asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CC : ARM64_VS); // cset r_result, cc/vs
                } else if (res_sz < sz) {
                    asm_subs(cg_sec, ra, rb, sz); // subs ra, ra, rb
                    if (res_unsigned) {
                        if (res_sz == 2) {
                            arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 16, 16); // ubfx w{r_result}, w{ra}, #16, #16
                            asm_cmp_zero(cg_sec, r_result, 4);
                            asm_cset(cg_sec, r_result, ARM64_NE);
                        } else if (res_sz == 1) {
                            arm64_ubfx(cg_sec, 0, REG(r_result), REG(ra), 8, 24); // ubfx w{r_result}, w{ra}, #8, #24
                            asm_cmp_zero(cg_sec, r_result, 4);
                            asm_cset(cg_sec, r_result, ARM64_NE);
                        } else {
                            asm_lsr_rd_rn_imm(cg_sec, r_result, ra, 8, (uint8_t)(res_sz * 8)); // lsr x{r_result}, x{ra}, #(res_sz*8)
                            asm_cmp_zero(cg_sec, r_result, 8);
                            asm_cset(cg_sec, r_result, ARM64_NE);
                        }
                    } else {
                        int rt = alloc_reg();
                        if (res_sz == 2) arm64_sxth(cg_sec, 0, REG(rt), REG(ra)); // sxth w{rt}, w{ra}
                        else
                            arm64_sxtb(cg_sec, 0, REG(rt), REG(ra)); // sxtb w{rt}, w{ra}
                        asm_cmp_reg_reg(cg_sec, rt, ra, sz); // cmp rt, ra
                        asm_cset(cg_sec, r_result, ARM64_NE);
                        free_reg(rt);
                    }
                } else {
                    // Widening
                    if (sz == 4) {
                        if (arga->ty && arga->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(ra), REG(ra)); // uxtw x{ra}, w{ra}
                        else
                            asm_sxtw(cg_sec, REG(ra), REG(ra)); // sxtw x{ra}, w{ra}
                        if (argb->ty && argb->ty->is_unsigned) asm_uxtw_phy(cg_sec, REG(rb), REG(rb)); // uxtw x{rb}, w{rb}
                        else
                            asm_sxtw(cg_sec, REG(rb), REG(rb)); // sxtw x{rb}, w{rb}
                    }
                    asm_subs(cg_sec, ra, rb, 8); // subs ra, ra, rb
                    if (res_unsigned && (!arga->ty || !arga->ty->is_unsigned || !argb->ty || !argb->ty->is_unsigned)) {
                        arm64_ands_reg(cg_sec, 1, ARM64_XZR, REG(ra), REG(ra), ARM64_LSL, 0); // tst ra, ra
                        asm_cset(cg_sec, r_result, ARM64_MI); // cset r_result, mi
                    } else {
                        asm_cset(cg_sec, r_result, res_unsigned ? ARM64_CC : ARM64_VS); // cset r_result, cc/vs
                    }
                }
            } else { // mul_overflow / mul_overflow_p
                // Multiply always produces double-width result (sz*2 bits).
                if (res_sz >= sz * 2) {
                    // Result at least as wide as double-width product.
                    bool a_signed = arga->ty && !arga->ty->is_unsigned;
                    bool b_signed = argb->ty && !argb->ty->is_unsigned;
                    if (sz == 8) asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                    else
                        asm_smull(cg_sec, ra, ra, rb); // smull ra, wa, wb
                    if (res_unsigned && (a_signed || b_signed)) {
                        arm64_ands_reg(cg_sec, 1, ARM64_XZR, REG(ra), REG(ra), ARM64_LSL, 0); // tst ra, ra
                        asm_cset(cg_sec, r_result, ARM64_MI); // cset r_result, mi
                    } else {
                        asm_mov_imm(cg_sec, r_result, 4, 0); // mov w{r_result}, #0
                    }
                } else if (res_sz == sz) {
                    int r2 = alloc_reg();
                    if (sz == 8) {
                        if (res_unsigned) {
                            asm_umulh(cg_sec, r2, ra, rb); // umulh r2, ra, rb
                            asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                            asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        } else {
                            int r3 = alloc_reg();
                            asm_smulh(cg_sec, r2, ra, rb); // smulh r2, ra, rb
                            asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                            asm_asr_rd_rn_imm(cg_sec, r3, ra, 8, 63); // asr r3, ra, #63
                            asm_cmp_reg_reg(cg_sec, r2, r3, 8); // cmp r2, r3
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                            free_reg(r3);
                        }
                    } else {
                        if (res_unsigned) {
                            asm_umull(cg_sec, ra, ra, rb); // umull ra, wa, wb
                            asm_lsr_rd_rn_imm(cg_sec, r2, ra, 8, 32); // lsr r2, ra, #32
                            asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        } else {
                            // sxtw sign-extends the 32-bit result; compare to full 64-bit product
                            asm_smull(cg_sec, ra, ra, rb); // smull ra, wa, wb
                            asm_sxtw(cg_sec, REG(r2), REG(ra)); // sxtw r2, wa
                            asm_cmp_reg_reg(cg_sec, r2, ra, 8); // cmp r2, ra
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        }
                    }
                    free_reg(r2);
                } else {
                    // Narrowing: res_sz < sz.  Multiply, then range-check.
                    int r2 = alloc_reg();
                    if (sz == 8) {
                        asm_mul_reg_reg(cg_sec, ra, rb, 8); // mul ra, ra, rb
                        asm_smulh(cg_sec, r2, ra, rb); // smulh r2, ra, rb
                    } else {
                        asm_smull(cg_sec, ra, ra, rb); // smull ra, wa, wb
                    }
                    if (res_unsigned) {
                        if (sz == 8) {
                            asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                            asm_lsr_rd_rn_imm(cg_sec, r2, ra, 8, (uint8_t)(res_sz * 8)); // lsr r2, ra, #(res_sz*8)
                            asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                            arm64_csinc(cg_sec, 0, REG(r_result), REG(r_result), REG(r_result), ARM64_EQ); // cinc r_result, r_result, ne
                        } else {
                            asm_lsr_rd_rn_imm(cg_sec, r2, ra, 8, (uint8_t)(res_sz * 8)); // lsr r2, ra, #(res_sz*8)
                            asm_cmp_zero(cg_sec, r2, 8); // cmp r2, #0
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        }
                    } else {
                        int r3 = alloc_reg();
                        if (sz == 8) {
                            asm_asr_rd_rn_imm(cg_sec, r3, ra, 8, (uint8_t)(res_sz * 8 - 1)); // asr r3, ra, #(res_sz*8-1)
                            asm_cmp_reg_reg(cg_sec, r2, r3, 8); // cmp r2, r3
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                            arm64_sbfx(cg_sec, 1, REG(r3), REG(ra), 0, res_sz * 8); // sbfx r3, ra, #0, #(res_sz*8)
                            asm_cmp_reg_reg(cg_sec, r3, ra, 8); // cmp r3, ra
                            arm64_csinc(cg_sec, 0, REG(r_result), REG(r_result), REG(r_result), ARM64_EQ); // cinc r_result, r_result, ne
                        } else {
                            asm_asr_rd_rn_imm(cg_sec, r2, ra, 8, (uint8_t)(res_sz * 8 - 1)); // asr r2, ra, #(res_sz*8-1)
                            asm_lsr_rd_rn_imm(cg_sec, r3, ra, 8, (uint8_t)(res_sz * 8)); // lsr r3, ra, #(res_sz*8)
                            asm_cmp_reg_reg(cg_sec, r2, r3, 8); // cmp r2, r3
                            asm_cset(cg_sec, r_result, ARM64_NE); // cset r_result, ne
                        }
                        free_reg(r3);
                    }
                    if (sz == 8)
                        free_reg(r2);
                }
            }
            if (argres && !is_mul_overflow_p) {
                VReg rr = gen(argres);
                if (argres->ty && argres->ty->kind == TY_PTR && argres->ty->base &&
                    argres->ty->base->kind == TY_INT128) {
                    asm_str_reg_off(cg_sec, ra, rr, 8, 0);
                    asm_sar_imm(cg_sec, ra, 8, 63);
                    asm_str_reg_off(cg_sec, ra, rr, 8, 8); // str ra, [rr, #8]
                } else {
                    asm_str_reg_off(cg_sec, ra, rr, res_sz, 0); // str reg(ra, res_sz), [rr]
                }
                free_reg(rr);
            }
            free_reg(ra);
            free_reg(rb);
            return r_result;
        }
#endif
#ifndef ARCH_ARM64
        if (is_add_overflow || is_sub_overflow || is_mul_overflow || is_mul_overflow_p) {
            Node *arga = node->args;
            Node *argb = arga ? arga->next : NULL;
            Node *argres = argb ? argb->next : NULL;
            int ra = gen(arga);
            int rb = gen(argb);
            // sz: operation size from the operands (at least 4).
            // sz_store: result pointer's pointed-to type (may differ).
            // sz_op: compute in max(sz, sz_store) so we can range-check the result.
            int sz = arga && arga->ty && arga->ty->size > 4 ? 8 : 4;
            int sz_store = sz;
            bool is_unsigned_store = arga && arga->ty && arga->ty->is_unsigned;
            // is_unsigned_op: true only when BOTH operands are unsigned (affects mul instruction
            // choice and widening strategy; mixed signed/unsigned must use signed arithmetic).
            bool ra_unsigned = arga && arga->ty && arga->ty->is_unsigned;
            bool rb_unsigned = argb && argb->ty && argb->ty->is_unsigned;
            bool is_unsigned_op = ra_unsigned && rb_unsigned;
            if (argres && argres->ty && argres->ty->kind == TY_PTR && argres->ty->base) {
                if (argres->ty->base->kind == TY_INT128) {
                    // Treat as 8-byte for computation; store 128-bit separately below
                    sz_store = 8;
                    is_unsigned_store = argres->ty->base->is_unsigned;
                } else {
                    sz_store = argres->ty->base->size > 4 ? 8 : 4;
                    is_unsigned_store = argres->ty->base->is_unsigned;
                }
            }
            bool store_to_int128 = argres && argres->ty && argres->ty->kind == TY_PTR &&
                argres->ty->base && argres->ty->base->kind == TY_INT128;
            int sz_op = sz > sz_store ? sz : sz_store; // compute in larger size
            // If operands are narrower than sz_op, widen them per-operand signedness.
            if (sz_op > sz && sz == 4) {
                if (ra_unsigned)
                    asm_movzx(cg_sec, ra, ra, 8, 4); // movl ra, ra (zero-ext)
                else
                    asm_movsx(cg_sec, ra, ra, 8, 4);
                if (rb_unsigned)
                    asm_mov_reg_reg(cg_sec, rb, rb, 4);
                else
                    asm_movsx(cg_sec, rb, rb, 8, 4);
            }
            if (is_add_overflow || is_sub_overflow) {
                if (is_add_overflow)
                    asm_add_reg_reg(cg_sec, ra, rb, sz_op); // add rb, ra
                else
                    asm_sub_reg_reg(cg_sec, ra, rb, sz_op); // sub rb, ra
                // Detect overflow into sz_store bits.
                if (sz_op == sz_store && sz_op == sz) {
                    // Same-size, same-type: use hardware flag directly.
                    asm_setcc(cg_sec, X86_RAX, is_unsigned_store ? X86_C : X86_O); // setc/seto %al
                } else if (sz_op == sz_store && sz_op > sz) {
                    // Widened signed ops into target type: check if mathematical
                    // result fits.  For unsigned target a negative result overflows;
                    // for signed target the 64-bit operation cannot itself overflow
                    // (small inputs), so seto would always be 0 — use range check.
                    if (is_unsigned_store) {
                        // negative result can't fit in unsigned
                        asm_setcc(cg_sec, X86_RAX, X86_S); // sets %al
                    } else {
                        // signed: result fits in sz_op bits already; range-check to sz_store
                        x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx
                        x86_movsx(cg_sec, 8, 4, X86_RCX, X86_RCX); // movslq %ecx, %rcx
                        x86_cmp_rr(cg_sec, 8, X86_RCX, REG(ra)); // cmpq %rcx, ra
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                    }
                } else {
                    // sz_op > sz_store: result is in full precision, range-check.
                    if (is_unsigned_store) {
                        x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx (zero-ext)
                        // movl zero-extends implicitly; compare full result vs trunc
                        x86_cmp_rr(cg_sec, 8, X86_RCX, REG(ra)); // cmpq %rcx, ra
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                    } else {
                        x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx
                        x86_movsx(cg_sec, 8, 4, X86_RCX, X86_RCX); // movslq %ecx, %rcx
                        x86_cmp_rr(cg_sec, 8, X86_RCX, REG(ra)); // cmpq %rcx, ra
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                    }
                }
                if (argres) {
                    int rr = gen(argres);
                    if (store_to_int128) {
                        asm_mov_reg_mem(cg_sec, ra, rr, 8); // movq ra, (rr)
                        x86_mov_rr(cg_sec, 8, X86_RAX, REG(ra)); // movq ra, %rax
                        x86_sar_ri(cg_sec, 8, X86_RAX, 63); // sarq $63, %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(REG(rr), 8), X86_RAX); // movq %rax, 8(rr)
                    } else {
                        x86_mov_mr(cg_sec, sz_store, x86_mem(REG(rr), 0), REG(ra)); // mov ra, (rr)
                    }
                    free_reg(rr);
                }
            } else { // mul_overflow / mul_overflow_p
                int r2 = alloc_reg();
                if (sz_op == 8) {
                    x86_mov_rr(cg_sec, 8, X86_RAX, REG(ra)); // movq ra, %rax
                    if (is_unsigned_op && sz == sz_op) {
                        // Native unsigned 64-bit: mulq gives rdx:rax
                        asm_mul_1op(cg_sec, rb, 8); // mulq rb
                        x86_mov_rr(cg_sec, 8, REG(ra), X86_RAX); // movq %rax, ra
                        x86_mov_rr(cg_sec, 8, REG(r2), X86_RDX); // movq %rdx, r2
                        x86_test_rr(cg_sec, 8, X86_RDX, X86_RDX); // testq %rdx, %rdx
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                    } else {
                        // Signed 64-bit (including sign-extended small operands):
                        // imulq gives rdx:rax; overflow if rdx != sign_ext(rax)
                        x86_imul_r(cg_sec, 8, REG(rb)); // imulq rb
                        x86_mov_rr(cg_sec, 8, REG(ra), X86_RAX); // movq %rax, ra
                        x86_mov_rr(cg_sec, 8, REG(r2), X86_RDX); // movq %rdx, r2
                        x86_sar_ri(cg_sec, 8, X86_RAX, 63); // sarq $63, %rax
                        x86_cmp_rr(cg_sec, 8, X86_RDX, X86_RAX); // cmpq %rax, %rdx
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                        // For unsigned target with negative result: also signal overflow.
                        if (is_unsigned_store && !is_unsigned_op) {
                            // OR in the sign bit of the 64-bit result
                            x86_test_rr(cg_sec, 8, REG(ra), REG(ra)); // testq ra, ra
                            asm_setcc(cg_sec, X86_RCX, X86_S); // sets %cl
                            x86_or_rr(cg_sec, 1, X86_RAX, X86_RCX); // orb %cl, %al
                        }
                    }
                } else {
                    // 32-bit operands
                    if (is_unsigned_op) {
                        x86_mov_rr(cg_sec, 4, X86_RAX, REG(ra)); // movl ra, %eax
                        asm_mul_1op(cg_sec, rb, 4); // mull rb
                        x86_mov_rr(cg_sec, 4, REG(ra), X86_RAX); // movl %eax, ra
                        x86_mov_rr(cg_sec, 4, REG(r2), X86_RDX); // movl %edx, r2
                        x86_test_rr(cg_sec, 4, X86_RDX, X86_RDX); // testl %edx, %edx
                        asm_setcc(cg_sec, X86_RAX, X86_NE);
                    } else {
                        // Mixed or signed operands: extend each per its own signedness,
                        // then signed 64-bit multiply (exact for 32-bit inputs).
                        if (ra_unsigned)
                            x86_mov_rr(cg_sec, 4, X86_RAX, REG(ra)); // movl ra, %eax (zero-ext)
                        else
                            x86_movsx(cg_sec, 8, 4, X86_RAX, REG(ra)); // movslq ra, %rax
                        if (rb_unsigned)
                            x86_mov_rr(cg_sec, 4, X86_RCX, REG(rb)); // movl rb, %ecx (zero-ext)
                        else
                            x86_movsx(cg_sec, 8, 4, X86_RCX, REG(rb)); // movslq rb, %rcx
                        x86_imul_rr(cg_sec, 8, X86_RAX, X86_RCX); // imulq %rcx, %rax
                        x86_mov_rr(cg_sec, 4, REG(ra), X86_RAX); // movl %eax, ra
                        if (is_unsigned_store && sz_store == 8) {
                            // Negative result doesn't fit in unsigned 64-bit
                            x86_test_rr(cg_sec, 8, X86_RAX, X86_RAX); // testq %rax, %rax
                            asm_setcc(cg_sec, X86_RAX, X86_S); // sets %al
                            x86_movsx(cg_sec, 8, 4, REG(ra), REG(ra)); // movslq ra, ra
                        } else {
                            // Range check: sign-extend or zero-extend truncated result back
                            if (is_unsigned_store)
                                x86_mov_rr(cg_sec, 4, X86_RCX, REG(ra)); // movl ra, %ecx (zero-ext)
                            else
                                x86_movsx(cg_sec, 8, 4, X86_RCX, REG(ra)); // movslq ra, %rcx
                            x86_cmp_rr(cg_sec, 8, X86_RCX, X86_RAX); // cmpq %rax, %rcx
                            asm_setcc(cg_sec, X86_RAX, X86_NE);
                        }
                    }
                }
                free_reg(r2);
                if (argres && !is_mul_overflow_p) {
                    int rr = gen(argres);
                    if (store_to_int128) {
                        // Sign-extend 64-bit result to 128 bits and store
                        x86_mov_mr(cg_sec, 8, x86_mem(REG(rr), 0), REG(ra)); // movq ra, (rr)
                        x86_mov_rr(cg_sec, 8, X86_RAX, REG(ra)); // movq ra, %rax
                        x86_sar_ri(cg_sec, 8, X86_RAX, 63); // sarq $63, %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(REG(rr), 8), X86_RAX); // movq %rax, 8(rr)
                    } else {
                        x86_mov_mr(cg_sec, sz_store, x86_mem(REG(rr), 0), REG(ra)); // mov ra, (rr)
                    }
                    free_reg(rr);
                }
            }
            int r_result = alloc_reg();
            asm_movzx_phys(cg_sec, r_result, X86_RAX, 4, 1);
            free_reg(ra);
            free_reg(rb);
            return r_result;
        }
#endif
    }

    // Inline expansion for common libc builtins (x86_64 only for now)
#ifndef ARCH_ARM64
    if (call_target && !has_hidden_retbuf) {
        // Inline expansion for common libc builtins
        bool is_memset = call_target == bi_s_memset || call_target == bi_memset;
        bool is_memcpy = call_target == bi_s_memcpy || call_target == bi_memcpy;
        bool is_memcmp = call_target == bi_s_memcmp || call_target == bi_memcmp;
        bool is_strlen = call_target == bi_s_strlen || call_target == bi_strlen;
        bool is_strcmp = call_target == bi_s_strcmp || call_target == bi_strcmp;
        bool is_strchr = call_target == bi_s_strchr || call_target == bi_strchr;

        if (is_memset || is_memcpy) {
            Node *dst = node->args;
            Node *v2 = dst ? dst->next : NULL;
            Node *len = v2 ? v2->next : NULL;
            if (dst && v2 && len && !len->next) {
                VReg r = alloc_reg();
                VReg dst_r = gen(dst);
                VReg v2_r = gen(v2);
                VReg len_r = gen(len);
                asm_mov_reg_reg(cg_sec, r, dst_r, 8); // mov rdst_r -> rr (return dst)

                asm_cld(cg_sec); // cld
                asm_push(cg_sec, X86_RDI); // pushq %rdi
                if (is_memcpy) asm_push(cg_sec, X86_RSI); // pushq %rsi
                asm_push(cg_sec, X86_RCX); // pushq %rcx
                x86_mov_rr(cg_sec, 8, X86_RDI, REG(dst_r));
                x86_mov_rr(cg_sec, 8, X86_RCX, REG(len_r));
                if (is_memset) {
                    x86_movzx(cg_sec, 4, 1, X86_RAX, REG(v2_r));
                    x86_rep_prefix(cg_sec);
                    x86_stosb(cg_sec); // rep stosb
                } else {
                    x86_mov_rr(cg_sec, 8, X86_RSI, REG(v2_r));
                    x86_rep_prefix(cg_sec);
                    x86_movsb(cg_sec); // rep movsb
                }
                asm_pop(cg_sec, X86_RCX); // popq %rdi
                if (is_memcpy) asm_pop(cg_sec, X86_RSI); // pop rX86_RSI
                asm_pop(cg_sec, X86_RDI); // cld

                free_reg(dst_r);
                free_reg(v2_r);
                free_reg(len_r);

                return r;
            }
        }

        if (is_memcmp) {
            Node *src1 = node->args;
            Node *src2 = src1 ? src1->next : NULL;
            Node *len = src2 ? src2->next : NULL;
            if (src1 && src2 && len && !len->next) {
                VReg s1_r = gen(src1);
                VReg s2_r = gen(src2);
                VReg len_r = gen(len);

                asm_cld(cg_sec); // cld
                asm_push(cg_sec, X86_RDI); // pushq %rdi
                asm_push(cg_sec, X86_RSI); // pushq %rsi
                asm_push(cg_sec, X86_RCX); // pushq %rcx
                int cl = ++rcc_label_count;
                x86_mov_rr(cg_sec, 8, X86_RDI, REG(s1_r));
                x86_mov_rr(cg_sec, 8, X86_RSI, REG(s2_r));
                x86_mov_rr(cg_sec, 8, X86_RCX, REG(len_r));
                x86_rep_prefix(cg_sec);
                x86_cmpsb(cg_sec); // repe cmpsb
                {
                    size_t o = asm_jcc_label(cg_sec, X86_NE);
                    asm_fixup_add(cg_sec, o, format(".L.memcmp_diff.%d", cl), 1);
                }
                x86_xor_rr(cg_sec, 4, X86_RAX, X86_RAX); // xorl %eax, %eax (equal)
                {
                    size_t o = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, o, format(".L.memcmp_end.%d", cl), 0);
                }
                cg_def_label(format(".L.memcmp_diff.%d", cl));
                x86_movsx_rm(cg_sec, 4, 1, X86_RAX, x86_mem(X86_RDI, -1)); // movsbl -1(%rdi),%eax
                x86_movsx_rm(cg_sec, 4, 1, X86_RCX, x86_mem(X86_RSI, -1)); // movsbl -1(%rsi),%ecx
                x86_sub_rr(cg_sec, 4, X86_RAX, X86_RCX); // subl %ecx, %eax
                cg_def_label(format(".L.memcmp_end.%d", cl));
                asm_pop(cg_sec, X86_RCX);
                asm_pop(cg_sec, X86_RSI);
                asm_pop(cg_sec, X86_RDI);
                free_reg(s1_r);
                free_reg(s2_r);
                free_reg(len_r);
                int r = alloc_reg();
                asm_mov_retval(cg_sec, r, 4); // movl %eax, rr
                return r;
            }
        }

        if (is_strlen) {
            Node *str = node->args;
            if (str && !str->next) {
                VReg str_r = gen(str);

                asm_cld(cg_sec); // cld
                asm_push(cg_sec, X86_RDI); // pushq %rdi
                asm_push(cg_sec, X86_RCX); // pushq %rcx
                x86_mov_rr(cg_sec, 8, X86_RDI, REG(str_r));
                x86_xor_rr(cg_sec, 1, X86_RAX, X86_RAX); // xorb %%al, %%al
                x86_mov_ri(cg_sec, 8, X86_RCX, -1); // movq $-1, %rcx
                x86_repne_prefix(cg_sec);
                x86_scasb(cg_sec); // repne scasb
                x86_not_r(cg_sec, 8, X86_RCX); // notq %rcx
                x86_sub_ri(cg_sec, 8, X86_RCX, 1); // decq %rcx
                x86_mov_rr(cg_sec, 8, X86_RAX, X86_RCX);
                asm_pop(cg_sec, X86_RCX); // popq %rcx
                asm_pop(cg_sec, X86_RDI); // popq %rdi

                free_reg(str_r);

                int r = alloc_reg();
                asm_mov_retval(cg_sec, r, 8); // movq %rax, rr
                return r;
            }
        }

        if (is_strcmp) {
            Node *s1 = node->args;
            Node *s2 = s1 ? s1->next : NULL;
            if (s1 && s2 && !s2->next) {
                VReg r = gen(s1);
                VReg r2 = gen(s2);
                int cl = ++rcc_label_count;
                asm_push(cg_sec, X86_RDI); // pushq %rdi
                asm_push(cg_sec, X86_RSI); // pushq %rsi
                x86_mov_rr(cg_sec, 8, X86_RDI, REG(r));
                x86_mov_rr(cg_sec, 8, X86_RSI, REG(r2));
                cg_def_label(format(".L.strcmp_loop.%d", cl));
                x86_movzx_rm(cg_sec, 4, 1, X86_RAX, x86_mem(X86_RDI, 0)); // movb (%rdi),%%al
                x86_cmp_rm(cg_sec, 1, X86_RAX, x86_mem(X86_RSI, 0)); // cmpb (%rsi),%%al
                {
                    size_t o = asm_jcc_label(cg_sec, X86_NE);
                    asm_fixup_add(cg_sec, o, format(".L.strcmp_diff.%d", cl), 1);
                }
                x86_test_rr(cg_sec, 1, X86_RAX, X86_RAX); // testb %%al,%%al
                {
                    size_t o = asm_jcc_label(cg_sec, X86_Z);
                    asm_fixup_add(cg_sec, o, format(".L.strcmp_eq.%d", cl), 1);
                }
                x86_inc_r(cg_sec, 8, X86_RDI); // incq %rdi
                x86_inc_r(cg_sec, 8, X86_RSI); // incq %rsi
                {
                    size_t o = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, o, format(".L.strcmp_loop.%d", cl), 0);
                }
                cg_def_label(format(".L.strcmp_diff.%d", cl));
                x86_movzx(cg_sec, 4, 1, X86_RAX, X86_RAX); // movzbl %%al,%eax
                x86_movzx_rm(cg_sec, 4, 1, X86_RCX, x86_mem(X86_RSI, 0)); // movzbl (%rsi),%ecx
                x86_sub_rr(cg_sec, 4, X86_RAX, X86_RCX); // subl %ecx,%eax
                {
                    size_t o = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, o, format(".L.strcmp_end.%d", cl), 0);
                }
                cg_def_label(format(".L.strcmp_eq.%d", cl));
                x86_xor_rr(cg_sec, 4, X86_RAX, X86_RAX); // xorl %eax,%eax
                cg_def_label(format(".L.strcmp_end.%d", cl));
                asm_pop(cg_sec, X86_RSI); // popq %rsi
                asm_pop(cg_sec, X86_RDI); // popq %rdi
                free_reg(r);
                free_reg(r2);
                int ret = alloc_reg();
                asm_mov_retval(cg_sec, ret, 4); // movl %eax, rret
                return ret;
            }
        }

        if (is_strchr) {
            Node *s = node->args;
            Node *c = s ? s->next : NULL;
            if (s && c && !c->next) {
                VReg sr = gen(s);
                VReg cr = gen(c);
                int cl = ++rcc_label_count;
                asm_push(cg_sec, X86_RDI); // pushq %rdi
                asm_push(cg_sec, X86_RCX); // pushq %rcx
                x86_mov_rr(cg_sec, 8, X86_RDI, REG(sr));
                x86_movzx(cg_sec, 4, 1, X86_RAX, REG(cr)); // movzbl %s, %eax
                cg_def_label(format(".L.strchr.%d", cl)); // .L.strchr_loop.%d:
                x86_cmp_rm(cg_sec, 1, X86_RAX, x86_mem(X86_RDI, 0)); // cmpb %%al, (%rdi)
                {
                    size_t o = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, o, format(".L.strchr_end.%d", cl), 1);
                }
                x86_cmp_mi(cg_sec, 1, x86_mem(X86_RDI, 0), 0); // cmpb $0, (%rdi)
                {
                    size_t o = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, o, format(".L.strchr_ret.%d", cl), 1);
                }
                x86_inc_r(cg_sec, 8, X86_RDI); // incq %rdi
                {
                    size_t o = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, o, format(".L.strchr.%d", cl), 0);
                }
                cg_def_label(format(".L.strchr_end.%d", cl)); // .L.strchr_found.%d:
                x86_mov_rr(cg_sec, 8, X86_RAX, X86_RDI); // movq %rdi, %rax
                {
                    size_t o = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, o, format(".L.strchr_done.%d", cl), 0);
                }
                cg_def_label(format(".L.strchr_ret.%d", cl)); // .L.strchr_null.%d:
                x86_xor_rr(cg_sec, 4, X86_RAX, X86_RAX); // xorl %eax, %eax
                cg_def_label(format(".L.strchr_done.%d", cl)); // .L.strchr_end.%d:
                asm_pop(cg_sec, X86_RCX); // popq %rcx
                asm_pop(cg_sec, X86_RDI); // popq %rdi
                free_reg(sr);
                free_reg(cr);
                int ret = alloc_reg();
                asm_mov_retval(cg_sec, ret, 8); // movq %rax, rret
                return ret;
            }
        }
    }
#endif /* !ARCH_ARM64 — inline builtins */

#ifdef ARCH_ARM64
    // Inline alloca: directly adjust sp without any register save/restore
    // (save/restore around a bl __rcc_alloca would use the stack, which alloca moves)
    if (call_target && call_target == bi_s_alloca) {
        alloca_needed = true;
        fn_uses_alloca = true;
        VReg ra = gen(node->args);
        // Round up to 16 bytes and adjust sp
        asm_add_imm(cg_sec, ra, 8, 15); // add ra, ra, #15
        asm_and_imm(cg_sec, ra, 8, -16); // and ra, ra, #-16
        asm_sub_sp_sp_reg(cg_sec, REG(ra)); // sub sp, sp, ra
        asm_mov_reg_sp(cg_sec, REG(ra)); // mov ra, sp
        // Save current sp/ptr to the alloca var slot
        if (node->args && node->args->ty) {
            // caller uses return value from ra
        }
        return ra;
    }
#endif

#ifdef _WIN32
    int fixed_reg_args = nargs + (has_hidden_retbuf ? 1 : 0);
    int stack_args = fixed_reg_args > max_gp_args ? fixed_reg_args - max_gp_args : 0;
    int stack_pad = (stack_args & 1) ? 8 : 0;
    // Win64 ABI: caller must always allocate 32-byte shadow space, even with no stack args
    int stack_reserve = shadow_space + stack_args * 8 + stack_pad;
#elif defined(ARCH_ARM64)
    // AAPCS64: 8 GP + 8 FP arg registers
    // Linux: variadic floats go in FP regs only (no GP copy)
    // Apple: variadic args always on stack
    int gp_reg_args = 0;
    int fp_reg_args = 0;
    int stack_args = 0;
    Type *fn_type = NULL;
    if (node->lhs && node->lhs->ty) {
        if (node->lhs->ty->kind == TY_PTR)
            fn_type = node->lhs->ty->base;
        else if (node->lhs->ty->kind == TY_FUNC && (node->lhs->kind != ND_LVAR || node->lhs->ty->is_variadic))
            fn_type = node->lhs->ty;
    }
    if (!fn_type && call_target) {
        static Type variadic_fn;
        if (call_target == bi_s_sprintf ||
            call_target == bi_s_snprintf ||
            call_target == bi_s_fprintf ||
            call_target == bi_s_printf) {
            variadic_fn.kind = TY_FUNC;
            variadic_fn.is_variadic = true;
            fn_type = &variadic_fn;
        }
    }
    bool is_variadic = fn_type && fn_type->kind == TY_FUNC && fn_type->is_variadic;
    bool is_oldstyle = !fn_type || (fn_type->kind == TY_FUNC && fn_type->is_oldstyle);
    int named_count = 0;
    if (fn_type && fn_type->kind == TY_FUNC)
        for (Type *t = fn_type->param_types; t; t = t->param_next)
            named_count++;
    if (named_count == 0 && call_target && is_variadic) {
        if (call_target == bi_s_printf)
            named_count = 1;
        else if (call_target == bi_s_sprintf ||
                 call_target == bi_s_fprintf)
            named_count = 2;
        else if (call_target == bi_s_snprintf)
            named_count = 3;
    }
    for (int i = 0; i < nargs; i++) {
        arg_regs[i] = -1;
        arg_sizes[i] = (argv[i]->ty->kind == TY_ARRAY) ? 8 : argv[i]->ty->size;
        if (is_oldstyle && arg_sizes[i] == 4 && is_flonum(argv[i]->ty))
            arg_sizes[i] = 8; // old-style float -> double promotion
        arg_is_float[i] = is_flonum(argv[i]->ty);
        arg_gp_idx[i] = -1;
        arg_fp_idx[i] = -1;
        arg_stack_idx[i] = -1;
        arg_hfa_count[i] = 0;
        arg_hfa_elem_size[i] = 0;
        bool is_named = (i < named_count);
        if (!arg_is_float[i] && (argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION))
            arg_hfa_count[i] = arm64_hfa_count(argv[i]->ty, &arg_hfa_elem_size[i]);
        if (arg_is_float[i]) {
            // Long double (> 8 bytes): pass in q-register (SIMD), not d-register or stack
            if (arg_sizes[i] > 8) {
                if (fp_reg_args < max_fp_args)
                    arg_fp_idx[i] = fp_reg_args++;
                else
                    arg_stack_idx[i] = stack_args++;
                continue;
            }
            if (is_variadic && !is_named) {
#if defined(__APPLE__)
                // Apple ARM64: variadic args always on the stack
                arg_stack_idx[i] = stack_args++;
#else
                // Linux AAPCS64: variadic floats promoted to double, in FP regs only
                // (GP copy for unnamed FP args is optional; glibc does not require it)
                if (arg_sizes[i] == 4) arg_sizes[i] = 8; // float -> double promotion for variadic
                if (fp_reg_args < max_fp_args)
                    arg_fp_idx[i] = fp_reg_args++;
                else
                    arg_stack_idx[i] = stack_args++;
#endif
            } else if (fp_reg_args < max_fp_args) {
                arg_fp_idx[i] = fp_reg_args++;
                // Named FP args in variadic functions go in FP regs only (no GP copy)
            } else {
                // All FP regs consumed: arg goes on stack (AAPCS64 6.4.2)
                arg_stack_idx[i] = stack_args++;
            }
            continue;
        }
        if (arg_hfa_count[i] > 0 && !is_variadic) {
            if (fp_reg_args + arg_hfa_count[i] <= max_fp_args) {
                arg_fp_idx[i] = fp_reg_args;
                fp_reg_args += arg_hfa_count[i];
            } else {
                arg_stack_idx[i] = stack_args;
                stack_args += (argv[i]->ty->size + 7) / 8;
            }
            continue;
        }
#if defined(__APPLE__)
        if (is_variadic && !is_named) {
            if (argv[i]->ty->kind == TY_INT128) {
                if (stack_args & 1) stack_args++;
                arg_stack_idx[i] = stack_args;
                stack_args += 2;
            } else if (argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) {
                if (argv[i]->ty->size == 0)
                    arg_stack_idx[i] = -1; // zero-size: no stack slot, nothing to push
                else {
                    arg_stack_idx[i] = stack_args;
                    stack_args += (argv[i]->ty->size + 7) / 8;
                }
            } else {
                arg_stack_idx[i] = stack_args++;
            }
            continue;
        }
#endif
        // int128: two consecutive GP register slots or 16-byte stack
        if (argv[i]->ty && argv[i]->ty->kind == TY_INT128) {
            if (gp_reg_args + 1 < max_gp_args) {
                arg_gp_idx[i] = gp_reg_args;
                gp_reg_args += 2;
            } else {
                if (stack_args & 1) stack_args++;
                arg_stack_idx[i] = stack_args;
                stack_args += 2;
            }
            continue;
        }
        // complex: one or two GP register slots
        if (argv[i]->ty && is_complex(argv[i]->ty)) {
            if (argv[i]->ty->size <= 8) {
                if (gp_reg_args < max_gp_args)
                    arg_gp_idx[i] = gp_reg_args++;
                else
                    arg_stack_idx[i] = stack_args++;
            } else {
                if (gp_reg_args + 1 < max_gp_args) {
                    arg_gp_idx[i] = gp_reg_args;
                    gp_reg_args += 2;
                } else {
                    if (stack_args & 1) stack_args++;
                    arg_stack_idx[i] = stack_args;
                    stack_args += 2;
                }
            }
            continue;
        }
        if ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8) {
            // Large struct > 8 bytes passed by pointer via GP reg or stack
            if (gp_reg_args < max_gp_args)
                arg_gp_idx[i] = gp_reg_args++;
            else
                arg_stack_idx[i] = stack_args++;
            continue;
        }
        if (gp_reg_args < max_gp_args)
            arg_gp_idx[i] = gp_reg_args++;
        else
            arg_stack_idx[i] = stack_args++;
    }
    //int stack_pad = (stack_args & 1) ? 8 : 0;
    //int stack_reserve = stack_args > 0 ? stack_args * 8 + stack_pad : 0;
#else
    int gp_reg_args = has_hidden_retbuf ? 1 : 0;
    int fp_reg_args = 0;
    int stack_args = 0;
    Type *fn_type = (node->lhs && node->lhs->ty && node->lhs->ty->kind == TY_PTR)
        ? node->lhs->ty->base
        : NULL;
    bool is_variadic = fn_type && fn_type->kind == TY_FUNC && fn_type->is_variadic;
    bool is_oldstyle = !fn_type || (fn_type->kind == TY_FUNC && fn_type->is_oldstyle);
    int named_count = 0;
    if (fn_type && fn_type->kind == TY_FUNC)
        for (Type *t = fn_type->param_types; t; t = t->param_next)
            named_count++;
    for (int i = 0; i < nargs; i++) {
        arg_regs[i] = -1;
        arg_sizes[i] = (argv[i]->ty->kind == TY_ARRAY) ? 8 : argv[i]->ty->size;
        if (is_oldstyle && arg_sizes[i] == 4 && is_flonum(argv[i]->ty))
            arg_sizes[i] = 8; // old-style float -> double promotion
        arg_is_float[i] = is_flonum(argv[i]->ty);
        arg_gp_idx[i] = -1;
        arg_fp_idx[i] = -1;
        arg_stack_idx[i] = -1;
        bool is_named = (i < named_count);

        if (arg_is_float[i]) {
            if (argv[i]->ty->kind == TY_LDOUBLE && is_variadic && !is_named) {
                if (stack_args & 1)
                    stack_args++;
                arg_stack_idx[i] = stack_args;
                stack_args += 2;
                continue;
            }
            if (fp_reg_args < max_fp_args) {
                arg_fp_idx[i] = fp_reg_args++;
            } else {
                arg_stack_idx[i] = stack_args++;
            }
            continue;
        }

        // int128: needs two consecutive GP register slots
        if (argv[i]->ty && argv[i]->ty->kind == TY_INT128) {
            if (gp_reg_args + 1 < max_gp_args) {
                arg_gp_idx[i] = gp_reg_args;
                gp_reg_args += 2;
            } else {
                // Align to 16 bytes on stack
                if (stack_args & 1) stack_args++;
                arg_stack_idx[i] = stack_args;
                stack_args += 2;
            }
            continue;
        }
        // complex: one or two slots depending on size; float complex uses SSE regs
        if (argv[i]->ty && is_complex(argv[i]->ty)) {
            bool cfloat = argv[i]->ty->base && is_flonum(argv[i]->ty->base);
            if (cfloat) {
                if (argv[i]->ty->size <= 8) {
                    // _Complex float: one xmm reg
                    if (fp_reg_args < max_fp_args) {
                        arg_fp_idx[i] = fp_reg_args++;
                    } else {
                        arg_stack_idx[i] = stack_args++;
                    }
                } else {
                    // _Complex double: two xmm regs
                    if (fp_reg_args + 1 < max_fp_args) {
                        arg_fp_idx[i] = fp_reg_args;
                        fp_reg_args += 2;
                    } else {
                        if (stack_args & 1) stack_args++;
                        arg_stack_idx[i] = stack_args;
                        stack_args += 2;
                    }
                }
            } else {
                // Integer complex: one or two GP regs
                if (argv[i]->ty->size <= 8) {
                    if (gp_reg_args < max_gp_args)
                        arg_gp_idx[i] = gp_reg_args++;
                    else
                        arg_stack_idx[i] = stack_args++;
                } else {
                    if (gp_reg_args + 1 < max_gp_args) {
                        arg_gp_idx[i] = gp_reg_args;
                        gp_reg_args += 2;
                    } else {
                        if (stack_args & 1) stack_args++;
                        arg_stack_idx[i] = stack_args;
                        stack_args += 2;
                    }
                }
            }
            continue;
        }

        if (gp_reg_args < max_gp_args)
            arg_gp_idx[i] = gp_reg_args++;
        else
            arg_stack_idx[i] = stack_args++;
    }

    int stack_pad = (stack_args & 1) ? 8 : 0;
    int stack_reserve = shadow_space + stack_args * 8 + stack_pad;
#endif

#ifdef ARCH_ARM64
    // ARM64: evaluate args for register passing, track stack args
    for (int i = 0; i < nargs; i++) {
        // Skip regular stack args, but evaluate HFAs on stack for data copying
        if (arg_stack_idx[i] >= 0 && arg_hfa_count[i] <= 0)
            continue;
        if (arg_hfa_count[i] > 0 || ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8)) {
            VReg addr = gen_addr(argv[i]);

            if (addr < 0) {
                // Non-lvalue struct/union (e.g. cast to struct): allocate temp slot and store value
                int sz = argv[i]->ty->size;
                int alloc = (sz + 15) & ~15;
                fn_struct_ret_off += alloc;
                if (fn_struct_ret_off > fn_struct_ret_total)
                    fn_struct_ret_total = fn_struct_ret_off;
                int tmp_slot = current_fn_stack_size + fn_struct_ret_off;
                addr = alloc_reg();
                emit_mov_imm64(ARM64_X16, (uint64_t)tmp_slot);
                asm_sub_reg3(cg_sec, addr, ARM64_X29, ARM64_X16, 8); // sub x{addr}, x29, x16
                // Zero out the slot using stp xzr, xzr, [x{addr}, #zb]
                for (int zb = 0; zb < alloc; zb += 16) {
                    int32_t imm7 = zb / 8; // in units of 8 bytes
                    asm_stp_xzr_xzr(cg_sec, addr, imm7); // stp xzr, xzr, [x{addr}, #zb]
                }
                // Store the computed value into the first bytes of the temp slot
                VReg val = gen(argv[i]);
                if (val >= 0) {
                    int vsz = argv[i]->ty->size < 8 ? argv[i]->ty->size : 8;
                    if (vsz == 4)
                        asm_str_reg_off(cg_sec, val, addr, 4, 0); // str w{val}, [x{addr}]
                    else
                        asm_str_reg_off(cg_sec, val, addr, 8, 0); // str x{val}, [x{addr}]
                    free_reg(val);
                }
            }
            arg_regs[i] = addr;
        } else
            arg_regs[i] = gen(argv[i]);
    }

    // Allocate one block for caller-saved regs + stack args.
    // Layout (from low sp upward):
    //   [sp + 0 .. stack_args*8-1]        : stack args (callee reads via x29+16+idx*8)
    //   [sp + stack_args*8 .. total-1]    : saved caller-saved regs
    // This keeps stack args at the standard AAPCS64 location relative to sp_at_call.
    int arm64_saved_mask = used_regs & 63;
    int sv_count = __builtin_popcount(arm64_saved_mask);

    if (sv_count > 0 || stack_args > 0) {
        int total = (sv_count + stack_args) * 8;
        total = (total + 15) & ~15;
        if (total < 4096)
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, total, 0); // sub sp, sp, #total
        else
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, total >> 12, 1); // sub sp, sp, #total (shifted)
    }

    // Save caller-saved regs (virtual 0-5 = physical x10-x15) above stack args area
    int sv_off = stack_args * 8;
    for (int i = 0; i < 6; i++) {
        if (arm64_saved_mask & (1 << i)) {
            arm64_str_uoff(cg_sec, 3, REG(i), ARM64_SP, sv_off / 8); // str x{i}, [sp, #sv_off]
            sv_off += 8;
        }
    }

    // Push stack args at [sp + arg_stack_idx[i]*8]
    for (int i = nargs - 1; i >= 0; i--) {
        if (arg_stack_idx[i] < 0)
            continue;
        int off = arg_stack_idx[i] * 8;
        VReg r;
        if ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8) {
#if defined(__APPLE__) && defined(ARCH_ARM64)
            if (is_variadic && !(i < named_count)) {
                int vaddr = gen_addr(argv[i]);
                int vsz = argv[i]->ty->size;
                for (int voff = 0; voff < vsz; voff += 8) {
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(vaddr), (uint32_t)(voff / 8));
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, ARM64_SP, (uint32_t)(arg_stack_idx[i] + voff / 8));
                }
                free_reg(vaddr);
                continue;
            }
            // HFA overflow: struct data goes on stack as value, not pointer
            if (arg_hfa_count[i] > 0 && arg_fp_idx[i] < 0) {
                int vaddr = gen_addr(argv[i]);
                int vsz = argv[i]->ty->size;
                for (int voff = 0; voff < vsz; voff += 8) {
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(vaddr), (uint32_t)(voff / 8));
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, ARM64_SP, (uint32_t)(arg_stack_idx[i] + voff / 8));
                }
                free_reg(vaddr);
                continue;
            }
#endif
            r = gen_addr(argv[i]);
            arm64_str_uoff(cg_sec, 3, REG(r), ARM64_SP, (uint32_t)(off / 8)); // str x{r}, [sp, #off]
            free_reg(r);
        } else if (argv[i]->ty->kind == TY_INT128) {
            int addr = gen_int128(argv[i]);
            arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr), 0);
            arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr), 1);
            arm64_str_uoff(cg_sec, 3, ARM64_X16, ARM64_SP, (uint32_t)arg_stack_idx[i]);
            arm64_str_uoff(cg_sec, 3, ARM64_X17, ARM64_SP, (uint32_t)(arg_stack_idx[i] + 1));
            free_reg(addr);
            continue;
        } else {
            // For struct args <=8 bytes, use gen_addr + ldr x16 to avoid
            // putting the value in a managed register (prevents spill conflicts).
            if (argv[i]->ty && (argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION)) {
                int addr = gen_addr(argv[i]);
                int sz = argv[i]->ty->size <= 4 ? 4 : 8;
                arm64_ldr_uoff(cg_sec, sz == 8 ? 3 : 2, ARM64_X16, REG(addr), 0); // ldr x16/w16, [addr]
                arm64_str_uoff(cg_sec, 3, ARM64_X16, ARM64_SP, (uint32_t)(off / 8)); // str x16, [sp]
                free_reg(addr);
            } else {
                r = gen(argv[i]);
                if (arg_is_float[i]) {
                    asm_fmov_i2f(cg_sec, 0, r, 1);
                    asm_str_fp_sp_off(cg_sec, 0, (uint32_t)off);
                } else {
                    arm64_str_uoff(cg_sec, 3, REG(r), ARM64_SP, (uint32_t)(off / 8));
                }
                free_reg(r);
            }
        }
    }

    VReg temp_ret_reg = R_NONE;
    int temp_ret_slot = 0;
    if (has_hidden_retbuf) {
        if (hidden_ret_reg == -1) {
            temp_ret_reg = alloc_reg();
            int alloc = (node->ty->size + 15) & ~15;
            fn_struct_ret_off += alloc;
            if (fn_struct_ret_off > fn_struct_ret_total)
                fn_struct_ret_total = fn_struct_ret_off;
            temp_ret_slot = current_fn_stack_size + fn_struct_ret_off;
            if (temp_ret_slot <= 4095)
                asm_add_reg_fp_imm(cg_sec, temp_ret_reg, -temp_ret_slot); // add temp_ret_reg, x29, #-temp_ret_slot
            else {
                int v = temp_ret_slot;
                emit_mov_imm64(ARM64_X16, (uint64_t)v); // mov x16, #v
                asm_sub_reg_fp_phy(cg_sec, temp_ret_reg, ARM64_X16, 8); // sub temp_ret_reg, x29, x16
            }
            hidden_ret_reg = temp_ret_reg;
        }
        asm_mov_x8_reg(cg_sec, hidden_ret_reg); // mov x8, x{hidden_ret_reg}
    }

    // Pre-pass: long double args.
    // Pre-pass: long double args. Pass as 64-bit double in d register.
    // rcc handles its own va_arg, so no quad conversion is needed.
    for (int i = 0; i < nargs; i++) {
        if (arg_stack_idx[i] >= 0)
            continue;
        if (arg_is_float[i] && arg_sizes[i] > 8 && arg_fp_idx[i] >= 0) {
            asm_fmov_gp_to_d(cg_sec, arg_fp_idx[i], arg_regs[i]); // fmov d{fp_idx}, x{arg_regs[i]}
            free_reg(arg_regs[i]);
        }
    }

    // Move evaluated args into arg registers
    for (int i = 0; i < nargs; i++) {
        if (arg_stack_idx[i] >= 0)
            continue;
        // Skip args with no assigned destination (e.g. zero-size variadic structs)
        if (arg_gp_idx[i] < 0 && arg_fp_idx[i] < 0) {
            if (arg_regs[i] >= 0) free_reg(arg_regs[i]);
            continue;
        }
        if (arg_is_float[i] && arg_sizes[i] > 8 && arg_fp_idx[i] >= 0) {
            continue;
        }
        if (arg_hfa_count[i] > 0 && arg_fp_idx[i] >= 0) {
            for (int j = 0; j < arg_hfa_count[i]; j++) {
                int off = j * arg_hfa_elem_size[i];
                // ldr s/d{fp_idx+j}, [x{arg_regs[i]}, #off]
                asm_ldr_fp_off(cg_sec, arg_fp_idx[i] + j, arg_regs[i],
                               arg_hfa_elem_size[i], (uint32_t)off);
            }
            free_reg(arg_regs[i]);
            continue;
        }
        // Variadic small HFA struct (size<=8): gen_addr gave address, but GP register
        // must hold the VALUE (va_arg reads it directly, not via pointer).
        if (arg_hfa_count[i] > 0 && arg_gp_idx[i] >= 0 && arg_fp_idx[i] < 0 && arg_sizes[i] <= 8) {
            if (arg_sizes[i] == 4)
                asm_ldr_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 0); // ldr w{gp_idx}, [x{arg_reg}]
            else
                asm_ldr_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 1); // ldr x{gp_idx}, [x{arg_reg}]
            free_reg(arg_regs[i]);
            continue;
        }
        if (arg_is_float[i] && arg_fp_idx[i] >= 0) {
            // Float values are always stored as double in GP regs (see ND_LVAR with is_flonum).
            // Move as double to FP arg register, then convert to float if callee expects float.
            asm_fmov_i2f(cg_sec, arg_fp_idx[i], arg_regs[i], 1); // fmov d{fp_idx}, x{arg_reg}
            // Convert double->float only if callee param is known float,
            // not for variadic args (which stay promoted to double)
            bool keep_double = (is_variadic && i >= named_count) ||
                (fn_type && (fn_type->is_oldstyle || !fn_type->param_types));
            bool callee_expects_float = false;
            if (!keep_double) {
                callee_expects_float = argv[i]->ty->size == 4;
                if (fn_type && fn_type->param_types && i < named_count) {
                    Type *pt = fn_type->param_types;
                    for (int j = 0; j < i && pt; j++) pt = pt->param_next;
                    callee_expects_float = pt && pt->kind == TY_FLOAT;
                }
            }
            if (callee_expects_float)
                asm_fcvt(cg_sec, 0, 1, arg_fp_idx[i], arg_fp_idx[i]); // fcvt s{fp_idx}, d{fp_idx}  (opc=0: double->single)
            // Non-float args that landed in a GP slot (kept for non-ARM64 paths)
            if (arg_gp_idx[i] >= 0)
                asm_mov_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 1); // mov x{gp_idx}, x{arg_reg}
        } else if (arg_is_float[i] && arg_gp_idx[i] >= 0) {
            asm_mov_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 1); // mov x{gp_idx}, x{arg_reg}
        } else if (argv[i]->ty && argv[i]->ty->kind == TY_INT128) {
            // int128: load two 64-bit values into consecutive arg registers
            arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(arg_regs[i]), 0); // ldr x16, [addr]
            arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(arg_regs[i]), 1); // ldr x17, [addr, #8]
            asm_mov_phy_phy(cg_sec, arg_gp_idx[i], ARM64_X16, 1); // mov x{gp_idx}, x16
            asm_mov_phy_phy(cg_sec, arg_gp_idx[i] + 1, ARM64_X17, 1); // mov x{gp_idx+1}, x17
        } else if (arg_sizes[i] == 1 || arg_sizes[i] == 2) {
            asm_mov_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 1); // mov x{gp_idx}, x{arg_reg}
        } else if (arg_sizes[i] == 4) {
            asm_mov_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 1); // mov x{gp_idx}, x{arg_reg}

        } else {
#ifdef ARCH_ARM64
            if (argv[i]->ty && is_complex(argv[i]->ty) && argv[i]->ty->size <= 8) {
                arm64_ldr_uoff(cg_sec, 3, (Arm64Reg)arg_gp_idx[i], REG(arg_regs[i]), 0); // ldr x{gp_idx}, [arg_regs[i]]
            } else if (argv[i]->ty && is_complex(argv[i]->ty) && argv[i]->ty->size > 8) {
                int base_sz = argv[i]->ty->base ? argv[i]->ty->base->size : 8;
                arm64_ldr_uoff(cg_sec, 3, (Arm64Reg)arg_gp_idx[i], REG(arg_regs[i]), 0); // ldr x{gp_idx}, [arg_regs[i]]
                arm64_ldr_uoff(cg_sec, 3, (Arm64Reg)(arg_gp_idx[i] + 1), REG(arg_regs[i]), (uint32_t)(base_sz / 8)); // ldr x{gp_idx+1}, [arg_regs[i], #base_sz]
            } else
#endif
                asm_mov_phy_reg(cg_sec, arg_gp_idx[i], arg_regs[i], 1); // mov x{gp_idx}, x{arg_reg}
        }
        free_reg(arg_regs[i]);
    }

    // For indirect calls: save arg registers before evaluating the callee,
    // since gen(node->lhs) may call functions that clobber x0-x8 / d0-d7.
    // (x86_64 avoids this by evaluating the callee first; ARM64 evaluates args first.)
    int indirect_gp_mask = 0;
    int indirect_fp_mask = 0;
    bool indirect_save_x8 = false;
    int indirect_save_size = 0;
    if (!call_target) {
        for (int i = 0; i < nargs; i++) {
            if (arg_gp_idx[i] >= 0) indirect_gp_mask |= (1 << arg_gp_idx[i]);
            if (arg_fp_idx[i] >= 0) indirect_fp_mask |= (1 << arg_fp_idx[i]);
        }
        indirect_save_x8 = has_hidden_retbuf;
        int n_save = __builtin_popcount(indirect_gp_mask) + __builtin_popcount(indirect_fp_mask) + (indirect_save_x8 ? 1 : 0);
        if (n_save > 0) {
            indirect_save_size = (n_save * 8 + 15) & ~15;
#ifdef ARCH_ARM64
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, indirect_save_size, 0);
#else
            x86_sub_ri(cg_sec, 8, X86_RSP, indirect_save_size);
#endif // sub sp, sp, #indirect_save_size
            int slot = 0;
            if (indirect_save_x8)
                arm64_str_uoff(cg_sec, 3, ARM64_X8, ARM64_SP, (uint32_t)slot++); // str x8, [sp, #slot*8]
            for (int j = 0; j < 8; j++)
                if (indirect_gp_mask & (1 << j))
                    arm64_str_uoff(cg_sec, 3, (Arm64Reg)(ARM64_X0 + j), ARM64_SP, (uint32_t)slot++); // str x{j}, [sp, #slot*8]
            for (int j = 0; j < 8; j++)
                if (indirect_fp_mask & (1 << j))
                    arm64_str_fp(cg_sec, 3, (Arm64Reg)(ARM64_D0 + j), ARM64_SP, (uint32_t)(slot++ * 8)); // str d{j}, [sp, #slot*8]
        }
    }

    if (call_target) {
        if (call_target == bi_s_alloca) {
            alloca_needed = true;
            fn_uses_alloca = true;
            emit_direct_call("__rcc_alloca");
        } else {
            emit_direct_call(call_target);
        }
    } else {
        int callee = gen(node->lhs);
        if (indirect_save_size > 0) {
            int slot = 0;
            if (indirect_save_x8)
                arm64_ldr_uoff(cg_sec, 3, ARM64_X8, ARM64_SP, (uint32_t)slot++); // ldr x8, [sp]
            for (int j = 0; j < 8; j++)
                if (indirect_gp_mask & (1 << j))
                    arm64_ldr_uoff(cg_sec, 3, (Arm64Reg)(ARM64_X0 + j), ARM64_SP, (uint32_t)slot++); // ldr x{j}, [sp, #slot*8]
            for (int j = 0; j < 8; j++)
                if (indirect_fp_mask & (1 << j))
                    arm64_ldr_fp(cg_sec, 3, (Arm64Reg)(ARM64_D0 + j), ARM64_SP, (uint32_t)(slot++ * 8)); // ldr d{j}, [sp, #slot*8]
            arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, indirect_save_size, 0); // add sp, sp, #indirect_save_size
        }
        asm_call_reg(cg_sec, callee); // call callee
        free_reg(callee);
    }

    // Restore caller-saved registers from above-stack-args area
    if (arm64_saved_mask) {
        int sv = stack_args * 8;
        for (int i = 0; i < 6; i++) {
            if (arm64_saved_mask & (1 << i)) {
                arm64_ldr_uoff(cg_sec, 3, REG(i), ARM64_SP, sv / 8); // ldr x{i}, [sp, #sv]
                sv += 8;
            }
        }
    }

    // Mark saved arg registers as free after the call.
    // They held argument values that are dead post-call.
    // This prevents the allocator from using callee-saved x19-x24
    // when many calls happen in sequence (pr92904).
    int dead_mask = 0;
    for (int i = 0; i < nargs; i++) {
        if (arg_regs[i] >= 0 && arg_regs[i] < 6)
            dead_mask |= (1 << arg_regs[i]);
    }
    used_regs &= ~(arm64_saved_mask & dead_mask);

    // Restore sp
    if (sv_count > 0 || stack_args > 0) {
        int total = (sv_count + stack_args) * 8;
        total = (total + 15) & ~15;
        if (total < 4096)
            arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, total, 0); // add sp, sp, #total
        else
            arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, total >> 12, 1); // add sp, sp, #total (shifted)
    }

    if (has_hidden_retbuf) {
        if (temp_ret_reg != R_NONE) {
            if (temp_ret_slot <= 4095)
                asm_add_reg_fp_imm(cg_sec, temp_ret_reg, -temp_ret_slot); // add temp_ret_reg, x29, #-temp_ret_slot
            else {
                int v = temp_ret_slot;
                emit_mov_imm64(ARM64_X16, v & 0xffff); // mov x16, #v
                v >>= 16;
                int s = 16;
                while (v) {
                    asm_movk(cg_sec, 16, 1, (uint16_t)(v & 0xffff), s); // movk x16, #v, lsl #s
                    v >>= 16;
                    s += 16;
                }
                asm_sub_reg_fp_phy(cg_sec, temp_ret_reg, ARM64_X16, 8); // sub temp_ret_reg, x29, x16
            }
        }
        return temp_ret_reg != R_NONE ? temp_ret_reg : hidden_ret_reg;
    }
    // int128 return: spill x0:x1 to a 16-byte slot
    if (node->ty && node->ty->kind == TY_INT128) {
        int addr = alloc_int128_addr();
        arm64_str_uoff(cg_sec, 3, ARM64_X0, REG(addr), 0); // str x0, [x{addr}]
        arm64_str_uoff(cg_sec, 3, ARM64_X1, REG(addr), 1); // str x1, [x{addr}, #8]
        return addr;
    }

    int r = alloc_reg();
    if (node->ty && is_flonum(node->ty)) {
        if (node->ty->kind == TY_FLOAT)
            asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0 (widen float return to double, opc=1=single->double)
        asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0 (store float return as int bits)
    } else {
        int sz = node->ty ? (node->ty->size < 8 ? 4 : 8) : 8;
        asm_mov_retval(cg_sec, r, sz); // mov x{r}, x0 (return value)
        // AAPCS64: narrow return values sign/zero-extend to full register width
        // Skip void functions (size 1 with TY_VOID kind)
        if (node->ty && node->ty->kind != TY_VOID && node->ty->size < sz && node->ty->size < 4) {
            if (node->ty->is_unsigned)
                zero_extend_to(r, node->ty->size, 8);
            else
                sign_extend_to(r, node->ty->size, 8);
        }
    }
    return r;
#else
    // === x86_64 (Windows + Linux) calling convention ===
    int saved_scratch = used_regs & 3;
    // The hidden_ret_reg exclusion only applies to the has_hidden_retbuf path,
    // where temp_ret_reg (r10/r11) is reloaded via a frame-relative `lea`
    // after the call. If there's no hidden retbuf, hidden_ret_reg (if set)
    // just holds a destination address from gen_addr() that must survive the
    // call like any other live register.
    if (saved_scratch & 1) {
        asm_mov_phyreg_rbp(cg_sec, X86_R10, 8, spill_offset(0)); // mov [rbp-spill_offset(0)], X86_R10
        // Keep r10 marked as in-use so alloc_reg() doesn't reuse it for the
        // hidden ret buffer, which would overwrite a caller's live arg value.
    }
    if (saved_scratch & 2) {
        asm_mov_phyreg_rbp(cg_sec, X86_R11, 8, spill_offset(1)); // mov [rbp-spill_offset(1)], X86_R11
        // Same for r11.
    }

    VReg callee_reg = -1;
    if (!call_target) {
        callee_reg = gen(node->lhs);
    }

#ifdef _WIN32
    Type *fn_type_w = (node->lhs && node->lhs->ty && node->lhs->ty->kind == TY_PTR)
        ? node->lhs->ty->base
        : NULL;
    bool is_oldstyle = !fn_type_w || (fn_type_w->kind == TY_FUNC && fn_type_w->is_oldstyle);
    int reg_nargs = nargs < max_gp_args - (has_hidden_retbuf ? 1 : 0) ? nargs : max_gp_args - (has_hidden_retbuf ? 1 : 0);
    for (int i = 0; i < reg_nargs; i++) {
        if ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8)
            arg_regs[i] = gen_addr(argv[i]);
        else
            arg_regs[i] = gen(argv[i]);
#ifdef _WIN32
        // For struct-returning function used as arg: gen returns buffer address;
        // for structs ≤8 bytes, load the value from it.
        if (argv[i]->kind == ND_FUNCALL && (argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size <= 8 && argv[i]->ty->size > 0) {
            x86_mov_rm(cg_sec, 8, REG(arg_regs[i]), x86_mem(REG(arg_regs[i]), 0)); // movq (%reg), %reg
        }
        // Win64: an 8-byte _Complex is passed by value in a GP register.
        // gen() returns the address of the complex value; load it.
        if (is_complex(argv[i]->ty) && argv[i]->ty->size <= 8) {
            x86_mov_rm(cg_sec, 8, REG(arg_regs[i]), x86_mem(REG(arg_regs[i]), 0)); // movq (%reg), %reg
        }
#endif
        arg_sizes[i] = (argv[i]->ty->kind == TY_ARRAY) ? 8 : argv[i]->ty->size;
        if (is_oldstyle && arg_sizes[i] == 4 && is_flonum(argv[i]->ty))
            arg_sizes[i] = 8; // old-style float -> double promotion
        arg_is_float[i] = is_flonum(argv[i]->ty);
        // Win64: XMM register N matches GP register N (rcx/xmm0, rdx/xmm1, r8/xmm2, r9/xmm3)
        arg_fp_idx[i] = i + (has_hidden_retbuf ? 1 : 0);
    }

    if (stack_reserve > 0 && (!call_target || call_target != bi_s_alloca))
        x86_sub_ri(cg_sec, 8, X86_RSP, stack_reserve); // subq $stack_reserve, %rsp

    for (int i = nargs - 1; i >= reg_nargs; i--) {
        // Win64: large structs (>8 bytes) are passed by pointer on the stack
        VReg r = ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8)
            ? gen_addr(argv[i])
            : gen(argv[i]);
        int off = shadow_space + (i - reg_nargs) * 8; // skip 32-byte home space
        if (is_flonum(argv[i]->ty)) {
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, off), REG(r)); // movq reg, off(%rsp)
        } else if (argv[i]->ty && (argv[i]->ty->kind == TY_PTR || argv[i]->ty->kind == TY_ARRAY || argv[i]->ty->kind == TY_FUNC)) {
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, off), REG(r)); // movq reg, off(%rsp)
        } else if (is_complex(argv[i]->ty) && argv[i]->ty->size <= 8) {
            // Win64: an 8-byte _Complex is passed by value on the stack.
            // gen() returns the address of the complex value; load it.
            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(r), 0)); // movq (reg), %rax
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, off), X86_RAX); // movq %rax, off(%rsp)
        } else {
            if (argv[i]->ty->size == 1)
                asm_movzx(cg_sec, r, r, 4, 1); // movzx4->r rr, rr
            else if (argv[i]->ty->size == 4)
                asm_mov_reg_reg(cg_sec, r, r, 4); // mov rr -> rr
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, off), REG(r)); // movq reg, off(%rsp)
        }
        free_reg(r);
    }
#else
    // When more args need scratch registers than exist, evaluating them all up
    // front would exhaust the register set and force colliding per-register
    // spills.  Stage each evaluated value into a unique stack slot instead, then
    // reload right before placement.  Mirrors the golden codegen logic.
    for (int i = 0; i < nargs; i++)
        arg_stage[i] = 0;
    int nreg_args_count = 0;
    for (int i = 0; i < nargs; i++)
        if (arg_stack_idx[i] < 0)
            nreg_args_count++;
    bool use_staging = (nreg_args_count > NUM_REGS);

    for (int i = 0; i < nargs; i++) {
        if (arg_stack_idx[i] >= 0)
            continue;
        if (is_complex(argv[i]->ty) || ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8)) {
            int addr = gen_addr(argv[i]);
            if (addr < 0) {
                // Non-lvalue (e.g. cast to struct): allocate temp, evaluate, store
                int alloc = (argv[i]->ty->size + 7) & ~7;
                fn_struct_ret_off += alloc;
                if (fn_struct_ret_off > fn_struct_ret_total)
                    fn_struct_ret_total = fn_struct_ret_off;
                int tmp_slot = current_fn_stack_size + fn_struct_ret_off;
                addr = alloc_reg();
                asm_lea_rbp_reg(cg_sec, addr, 8, tmp_slot); // lea [rbp-8], raddr
                VReg val = gen(argv[i]);
                asm_mov_mem_via_reg(cg_sec, val, addr, argv[i]->ty->size > 4 ? 8 : argv[i]->ty->size); // mov val, (addr)
                free_reg(val);
            }
            arg_regs[i] = addr;
        } else
            arg_regs[i] = gen(argv[i]);

        if (use_staging) {
            fn_struct_ret_off += 8;
            if (fn_struct_ret_off > fn_struct_ret_total)
                fn_struct_ret_total = fn_struct_ret_off;
            arg_stage[i] = current_fn_stack_size + fn_struct_ret_off;
            asm_mov_reg_rbp(cg_sec, arg_regs[i], 8, arg_stage[i]); // movq arg_regs[i], -arg_stage[i](%rbp)
            // Release without triggering spill restore; value is safe in staging slot.
            spilled_regs &= ~(1 << arg_regs[i]);
            used_regs &= ~(1 << arg_regs[i]);
            reg_owner[arg_regs[i]] = NULL;
            arg_regs[i] = -1;
        }
    }

    // Bitmask of scratch registers still needed for register-passed args.
    // Stack arg computation below may free and reuse these via the spill
    // mechanism; re-marking them as in-use forces a proper spill/restore
    // instead of a silent overwrite that would lose the pre-computed value.
    int reg_arg_mask = 0;
    for (int i = 0; i < nargs; i++)
        if (arg_regs[i] >= 0 && arg_stack_idx[i] < 0)
            reg_arg_mask |= (1 << arg_regs[i]);

    if (stack_reserve > 0 && (!call_target || call_target != bi_s_alloca))
        x86_sub_ri(cg_sec, 8, X86_RSP, stack_reserve); // subq $stack_reserve, %rsp

    for (int i = nargs - 1; i >= 0; i--) {
        if (arg_stack_idx[i] < 0)
            continue;
        used_regs |= reg_arg_mask; // keep pre-computed reg args live
        if (argv[i]->ty->kind == TY_LDOUBLE) {
            VReg r = gen(argv[i]);
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8), REG(r)); // movq %s, %d(%rsp)
            X86Mem mld = x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8); // fldl %d(%rsp)
            x86_fldl_m(cg_sec, mld); // fldl %d(%rsp)
            x86_fstpt_m(cg_sec, mld); // fstpt %d(%rsp)
            free_reg(r);
            used_regs |= reg_arg_mask; // restore any bits cleared by free_reg
            continue;
        }
        VReg r;
        if (argv[i]->ty && argv[i]->ty->kind == TY_INT128) {
            r = gen(argv[i]); // returns address of 16-byte slot
            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(r), 0)); // movq (%s), %%rax
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8), X86_RAX); // movq %%rax, ...
            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(r), 8)); // movq 8(%s), %%rax
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + (arg_stack_idx[i] + 1) * 8), X86_RAX); // movq %%rax, ...
            free_reg(r);
            used_regs |= reg_arg_mask;
            continue;
        }
        if (is_complex(argv[i]->ty) || ((argv[i]->ty->kind == TY_STRUCT || argv[i]->ty->kind == TY_UNION) && argv[i]->ty->size > 8))
            r = gen_addr(argv[i]);
        else
            r = gen(argv[i]);
        if (is_complex(argv[i]->ty)) {
            int sz = argv[i]->ty->size;
            int base_sz = argv[i]->ty->base->size;
            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(r), 0)); // movq (%s), %%rax
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8), X86_RAX); // movq %%rax, ...
            if (sz > 8) {
                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(r), base_sz)); // movq base_sz(%s), %%rax
                x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + (arg_stack_idx[i] + 1) * 8), X86_RAX); // movq %%rax, ...
            }
        } else if (is_flonum(argv[i]->ty)) {
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8), REG(r)); // movq %s, %d(%rsp)
        } else if (argv[i]->ty->kind == TY_PTR || argv[i]->ty->kind == TY_ARRAY || argv[i]->ty->kind == TY_FUNC) {
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8), REG(r)); // movq %s, %d(%rsp)
        } else {
            // Structs/unions <=8 bytes already loaded as full 8-byte register;
            // sign/zero extending by size would corrupt bytes beyond size.
            if (argv[i]->ty->kind != TY_STRUCT && argv[i]->ty->kind != TY_UNION) {
                if (argv[i]->ty->is_unsigned)
                    zero_extend_to(r, argv[i]->ty->size, 8);
                else
                    sign_extend_to(r, argv[i]->ty->size, 8);
            }
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, shadow_space + arg_stack_idx[i] * 8), REG(r)); // movq r, off(%rsp)
        }
        free_reg(r);
        used_regs |= reg_arg_mask; // restore any bits cleared by free_reg
    }
#endif

    int xmm_args = 0;
#ifdef _WIN32
    for (int i = 0; i < reg_nargs; i++) {
        if (arg_is_float[i]) xmm_args++;
    }
#else
    xmm_args = fp_reg_args;
#endif

    int temp_ret_reg = -1;
    int temp_ret_slot = 0;
    if (has_hidden_retbuf) {
        if (hidden_ret_reg == -1) {
            temp_ret_reg = alloc_reg();
            int alloc = (node->ty->size + 15) & ~15;
            fn_struct_ret_off += alloc;
            if (fn_struct_ret_off > fn_struct_ret_total)
                fn_struct_ret_total = fn_struct_ret_off;
            temp_ret_slot = current_fn_stack_size + fn_struct_ret_off;
            asm_lea_rbp_reg(cg_sec, temp_ret_reg, 8, temp_ret_slot); // lea [rbp-8], rtemp_ret_reg
            hidden_ret_reg = temp_ret_reg;
        }
        x86_mov_rr(cg_sec, 8, cg_x86_argreg[0], REG(hidden_ret_reg)); // mov hidden_ret_reg, argreg64[0]
#ifdef _WIN32
        // Store hidden retbuf register to shadow space so variadic callees can find it
        if (!call_target || strcmp(call_target, "alloca") != 0)
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, 0), cg_x86_argreg[0]); // movq %rcx, 0(%rsp)
#endif
    }

#ifdef _WIN32
    for (int i = 0; i < reg_nargs; i++) {
        int argi = i + (has_hidden_retbuf ? 1 : 0);
        if (arg_is_float[i]) {
            asm_movq_r_xmm(cg_sec, arg_fp_idx[i], arg_regs[i]); // movq arg_regs[i], %xmm{fp_idx}
            x86_mov_rr(cg_sec, 8, cg_x86_argreg[argi], REG(arg_regs[i])); // movq %s, 0(%rsp)
        } else if (arg_sizes[i] == 1) {
            if (argv[i]->ty && !argv[i]->ty->is_unsigned)
                x86_movsx(cg_sec, 4, 1, cg_x86_argreg[argi], REG(arg_regs[i])); // movq %s, %s
            else
                x86_movzx(cg_sec, 4, 1, cg_x86_argreg[argi], REG(arg_regs[i])); // mov %s, %s
        } else if (arg_sizes[i] == 2) {
            if (argv[i]->ty && !argv[i]->ty->is_unsigned)
                x86_movsx(cg_sec, 4, 2, cg_x86_argreg[argi], REG(arg_regs[i])); // movsbl %s, %s
            else
                x86_movzx(cg_sec, 4, 2, cg_x86_argreg[argi], REG(arg_regs[i])); // movzbl %s, %s
        } else if (arg_sizes[i] == 4) {
            if (argv[i]->ty->is_unsigned)
                x86_mov_rr(cg_sec, 4, cg_x86_argreg[argi], REG(arg_regs[i])); // movswl %s, %s
            else
                x86_movsx(cg_sec, 8, 4, cg_x86_argreg[argi], REG(arg_regs[i])); // movzwl %s, %s
        } else {
            x86_mov_rr(cg_sec, 8, cg_x86_argreg[argi], REG(arg_regs[i])); // mov %s, %s
        }
        // Also store to shadow space so variadic callees can find args via va_list
        if (!call_target || call_target != bi_s_alloca)
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, argi * 8), cg_x86_argreg[argi]); // movq %reg, argi*8(%rsp)
        free_reg(arg_regs[i]);
    }
#else
    // Two-pass placement: reg64[7]=="%rsi"==argreg64[1], so any arg whose scratch
    // register is rsi must be placed before the arg that writes to rsi, otherwise
    // the write clobbers the source value.  Pass 0: rsi-sourced args.  Pass 1: rest.
    // With staging, values are in unique stack slots so no ordering conflict
    // exists; a single pass suffices, reloading each staged value first.
    for (int pass = 0; pass < (use_staging ? 1 : 2); pass++) {
        for (int i = 0; i < nargs; i++) {
            if (arg_stack_idx[i] >= 0)
                continue;
            if (!use_staging) {
                bool rsi_src = (!arg_is_float[i] && arg_regs[i] == 7);
                if (pass == 0 && !rsi_src) continue;
                if (pass == 1 && rsi_src) continue;
            }
            if (use_staging && arg_stage[i]) {
                arg_regs[i] = alloc_reg();
                asm_mov_rbp_reg(cg_sec, arg_regs[i], 8, arg_stage[i]); // movq -arg_stage[i](%rbp), arg_regs[i]
            }
            if (argv[i]->ty && argv[i]->ty->kind == TY_INT128) {
                // lo in argreg64[arg_gp_idx[i]], hi in argreg64[arg_gp_idx[i]+1]
                x86_mov_rm(cg_sec, 8, cg_x86_argreg[arg_gp_idx[i]], x86_mem(REG(arg_regs[i]), 0)); // movq (%s), %s
                x86_mov_rm(cg_sec, 8, cg_x86_argreg[arg_gp_idx[i] + 1], x86_mem(REG(arg_regs[i]), 8)); // movq 8(%s), %s
            } else if (arg_is_float[i]) {
                asm_movq_r_xmm(cg_sec, arg_fp_idx[i], arg_regs[i]); // movq arg_regs[i], %%xmm{fp_idx}
            } else if (argv[i]->ty && is_complex(argv[i]->ty)) {
                bool cfloat = argv[i]->ty->base && is_flonum(argv[i]->ty->base);
                if (cfloat) {
                    // Float complex: load real and imag into xmm regs.
                    // Each component is 8 bytes for float/double complex.
                    int base_sz = argv[i]->ty->base ? argv[i]->ty->base->size : 8;
                    if (argv[i]->ty->size <= 8) {
                        // _Complex float: 8 bytes packed -> one xmm
                        x86_movsd_rm(cg_sec, (X86XmmReg)arg_fp_idx[i], x86_mem(REG(arg_regs[i]), 0));
                    } else {
                        // _Complex double: 16 bytes -> two xmm
                        x86_movsd_rm(cg_sec, (X86XmmReg)arg_fp_idx[i], x86_mem(REG(arg_regs[i]), 0));
                        x86_movsd_rm(cg_sec, (X86XmmReg)(arg_fp_idx[i] + 1), x86_mem(REG(arg_regs[i]), base_sz));
                    }
                } else {
                    // Integer complex
                    x86_mov_rm(cg_sec, 8, cg_x86_argreg[arg_gp_idx[i]], x86_mem(REG(arg_regs[i]), 0)); // mov %s, %s
                    if (argv[i]->ty->size > 8) {
                        int base_sz = argv[i]->ty->base ? argv[i]->ty->base->size : 8;
                        x86_mov_rm(cg_sec, 8, cg_x86_argreg[arg_gp_idx[i] + 1], x86_mem(REG(arg_regs[i]), base_sz));
                    }
                }
            } else if (arg_sizes[i] == 1) {
                if (argv[i]->ty && !argv[i]->ty->is_unsigned)
                    x86_movsx(cg_sec, 4, 1, cg_x86_argreg[arg_gp_idx[i]], REG(arg_regs[i])); // movsbl %s, %s
                else
                    x86_movzx(cg_sec, 4, 1, cg_x86_argreg[arg_gp_idx[i]], REG(arg_regs[i])); // movzbl %s, %s
            } else if (arg_sizes[i] == 2) {
                if (argv[i]->ty && !argv[i]->ty->is_unsigned)
                    x86_movsx(cg_sec, 4, 2, cg_x86_argreg[arg_gp_idx[i]], REG(arg_regs[i])); // movswl %s, %s
                else
                    x86_movzx(cg_sec, 4, 2, cg_x86_argreg[arg_gp_idx[i]], REG(arg_regs[i])); // movzwl %s, %s
            } else if (arg_sizes[i] == 4) {
                x86_mov_rr(cg_sec, 4, cg_x86_argreg[arg_gp_idx[i]], REG(arg_regs[i])); // mov %s, %s
            } else {
                x86_mov_rr(cg_sec, 8, cg_x86_argreg[arg_gp_idx[i]], REG(arg_regs[i])); // movslq %s, %s
            }
            free_reg(arg_regs[i]);
        }
    } // end two-pass
#endif

    x86_mov_ri(cg_sec, 4, X86_RAX, xmm_args); // movl $xmm_args, %eax
    if (call_target) {
        if (call_target == bi_s_alloca) {
            alloca_needed = true;
            fn_uses_alloca = true;
            emit_direct_call("__rcc_alloca");
        } else {
            emit_direct_call(call_target);
        }
    } else {
        asm_call_reg(cg_sec, callee_reg); // call rcallee_reg
        free_reg(callee_reg);
    }

    if (stack_reserve > 0 && (!call_target || call_target != bi_s_alloca))
        x86_add_ri(cg_sec, 8, X86_RSP, stack_reserve); // addq $stack_reserve, %rsp

    if (saved_scratch & 2) {
        used_regs |= 2;
        asm_mov_rbp(cg_sec, X86_R11, 8, spill_offset(1)); // mov [rbp-spill_offset(1], X86_R11)
    }
    if (saved_scratch & 1) {
        used_regs |= 1;
        asm_mov_rbp(cg_sec, X86_R10, 8, spill_offset(0)); // mov [rbp-spill_offset(0], X86_R10)
    }

    if (has_hidden_retbuf) {
        if (temp_ret_reg != R_NONE) {
            // temp_ret_reg (r10/r11) may have been clobbered by the callee; reload
            // the frame-relative address — it is always valid.
            asm_lea_rbp_reg(cg_sec, temp_ret_reg, 8, temp_ret_slot); // lea [rbp-8], rtemp_ret_reg
        }
        if ((saved_scratch & 1) && (!has_hidden_retbuf || hidden_ret_reg != 0)) {
            used_regs |= 1;
            asm_mov_rbp(cg_sec, X86_R10, 8, spill_offset(0)); // mov [rbp-spill_offset(0], X86_R10
        }
        int ret = temp_ret_reg != R_NONE ? temp_ret_reg : hidden_ret_reg;
#ifdef _WIN32
        // On Windows, structs ≤8 bytes are passed by value. When the caller
        // did NOT provide a hidden ret buffer (hidden_ret_reg == -1), the
        // return register holds the buffer address; load the value from it.
        if (hidden_ret_reg == -1 && node->ty->size <= 8 && node->ty->size > 0) {
            x86_mov_rm(cg_sec, node->ty->size, REG(ret), x86_mem(X86_RAX, 0)); // movsx (size) (%rax), %ret
        }
#endif
        return ret;
    }

#ifdef _WIN32
    // Win64: an 8-byte _Complex return value comes back by value in RAX (no
    // hidden return pointer — see has_hidden_retbuf above). Store it where
    // the caller wants it (hidden_ret_reg, e.g. from `c = f()`) or, if no
    // destination was provided, materialize it on the stack and return its
    // address, matching the convention that gen() returns the address of a
    // complex value.
    if (node->ty && is_complex(node->ty) && node->ty->size <= 8) {
        int addr = hidden_ret_reg != -1 ? hidden_ret_reg : alloc_int128_addr();
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 0), X86_RAX); // movq %rax, (reg)
        return addr;
    }
#endif
    // int128 return: spill rax:rdx (x86-64) or x0:x1 (ARM64) to a 16-byte slot
    if (node->ty && node->ty->kind == TY_INT128) {
        int addr = alloc_int128_addr();
#ifdef ARCH_ARM64
        asm_str_reg_off_phy(cg_sec, ARM64_X0, addr, 8, 0); // str x0, [addr]
        asm_str_reg_off_phy(cg_sec, ARM64_X1, addr, 8, 1); // str x1, [addr, #8]
#else
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 0), X86_RAX); // movq %rax, (reg)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 8), X86_RDX); // movq %rdx, 8(reg)
#endif
        return addr;
    }
    VReg r = alloc_reg();
    if (node->ty && is_flonum(node->ty)) {
#ifndef ARCH_ARM64
        if (node->ty->kind == TY_FLOAT)
            asm_cvtss2sd(cg_sec); // cvtss2sd %xmm0, %xmm0
        asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, rr
#endif
    } else {
        asm_mov_retval(cg_sec, r, 8); // movq %rax, rr
    }
    return r;
#endif
}


static int add_float_literal(double val, int size) {
    FloatLit *fl = arena_alloc(sizeof(FloatLit));
    if (cg_dry_run) return 0;
    fl->id = float_lit_count++;
    fl->val = val;
    fl->size = size;
    fl->next = float_lits;
    float_lits = fl;
    return fl->id;
}

// Names that GAS treats as keywords in Intel syntax (segment registers and
// expression operators).  Global symbols with these names need a safe alias.
static bool is_asm_reserved(const char *name) {
    static const char *kw[] = {
        "cs", "ds", "es", "fs", "gs", "ss",
        "%al", "%ah", "%ax", "%eax", "%rax",
        "%bl", "%bh", "%bx", "%ebx", "%rbx",
        "%cl", "%ch", "%cx", "%ecx", "%rcx",
        "%dl", "%dh", "%dx", "%edx", "%rdx",
        "%sil", "%si", "%esi", "%rsi",
        "%dil", "%di", "%edi", "%rdi",
        "%bpl", "%bp", "%ebp", "%rbp",
        "%spl", "%sp", "%esp", "%rsp",
        "%r8", "%r8b", "%r8w", "%r8d",
        "%r9", "%r9b", "%r9w", "%r9d",
        "%r10", "%r10b", "%r10w", "%r10d",
        "%r11", "%r11b", "%r11w", "%r11d",
        "%r12", "%r12b", "%r12w", "%r12d",
        "%r13", "%r13b", "%r13w", "%r13d",
        "%r14", "%r14b", "%r14w", "%r14d",
        "%r15", "%r15b", "%r15w", "%r15d",
        "and", "or", "not", "xor", "shl", "shr",
        NULL};
    for (int i = 0; kw[i]; i++)
        if (strcmp(name, kw[i]) == 0) return true;
    return false;
}

#ifndef ARCH_ARM64
static char *var_label(LVar *var) {
    if (var->asm_name) return var->asm_name;
    if (is_asm_reserved(var->name)) return format(".L_rcc_%s", var->name);
    return var->name;
}
#endif

static char *reg(VReg r, int size) {
#ifdef ARCH_ARM64
    if (r < 0 || r > 11)
        error("invalid register %d, arm64 has only 12", r);
    if (size == 8) return reg64[r];
    return reg32[r]; // arm64: wN for 1/2/4 bytes
#else
    if (r < 0 || r > 7)
        error("invalid register %d, x86_64 has only 8", r);
    if (size == 1) return reg8[r];
    if (size == 2) return reg16[r];
    if (size == 4) return reg32[r];
    return reg64[r];
#endif
}

#ifdef ARCH_ARM64
// Emit mov reg, #imm64 handling any size (movz + movk)
static void emit_mov_imm64(Arm64Reg reg, uint64_t val) {
    int sf = 1; // always 64-bit for Arm64Reg
    asm_movz(cg_sec, reg, sf, (uint16_t)(val & 0xffff), 0); // movz x{reg}, #lo16
    for (int sh = 16; sh <= 48; sh += 16) {
        if ((val >> sh) != 0) // only emit if remaining bits non-zero
            asm_movk(cg_sec, reg, sf, (uint16_t)((val >> sh) & 0xffff), sh);
    }
}

// Emit mov reg, #imm for a signed 32-bit immediate, choosing 32- or 64-bit encoding
static void emit_mov_imm(const char *reg, int imm) {
    bool is_w = (reg[0] == 'w');
    int sf = is_w ? 0 : 1;
    int rd = atoi(reg + 1);
    uint64_t val = is_w ? (uint64_t)(uint32_t)imm : (uint64_t)(int64_t)(int32_t)imm;
    asm_movz(cg_sec, rd, sf, (uint16_t)(val & 0xffff), 0); // movz x{rd}, #val
    val >>= 16;
    int shift = 16;
    int max_shift = is_w ? 16 : 48;
    while (val && shift <= max_shift) {
        asm_movk(cg_sec, rd, sf, (uint16_t)(val & 0xffff), shift); // movk x{rd}, #val, lsl #shift
        val >>= 16;
        shift += 16;
    }
}

#endif // ARCH_ARM64

// Emit adrp+add pair for label address, with platform-appropriate relocations.
// Linux:         R_AARCH64_ADR_PREL_PG_HI21 + R_AARCH64_ADD_ABS_LO12_NC  (ADRP+ADD)
// Darwin local:  same (labels starting with '.' are always in this .o)
// Darwin C syms: R_AARCH64_ADR_GOT_PAGE + R_AARCH64_LD64_GOT_LO12_NC    (ADRP+LDR via GOT)
//   Labels starting with '.' are assembler-local (.LC*, .LF*, .L.label.*) — always
//   defined in the current .o, so ADRP+ADD is correct regardless of forward refs.
//   Labels starting with '_' are C-level symbols. They may be:
//     - undefined dylib symbols: ld64 rejects ARM64_RELOC_PAGEOFF12 for these
//     - locally defined C globals: ld64 synthesises a GOT entry when GOT relocs used
//   In both cases GOT access is correct on Darwin.
#ifdef ARCH_ARM64
static void emit_adrp_add(VReg r, const char *label) {
    if (cg_dry_run) return;
    Arm64Reg rd = REG(r);
    int sidx = objfile_find_sym(cg_obj, label);
    if (sidx < 0) {
        int sec = (label[0] == '.') ? SEC_RODATA : SEC_UNDEF;
        int bind = (label[0] == '.') ? SB_LOCAL : SB_GLOBAL;
        sidx = objfile_add_sym(cg_obj, label, sec, 0, 0, bind, ST_NOTYPE);
    }
#ifdef __APPLE__
    // Darwin uses GOT for data symbols.
    // Exception: .L. flow-control labels are local text labels, use direct access.
    if (label[0] != '.' || (label[1] != 'L' || label[2] != '.')) {
        size_t adrp_off = cg_sec->len;
        asm_adrp(cg_sec, rd); // adrp xrd, label@GOTPAGE
        objfile_add_reloc(cg_obj, SEC_TEXT, adrp_off, sidx, R_AARCH64_ADR_GOT_PAGE, 0);
        size_t ldr_off = cg_sec->len;
        asm_ldr_rd_rd(cg_sec, rd); // ldr xrd, [xrd, label@GOTPAGEOFF]
        objfile_add_reloc(cg_obj, SEC_TEXT, ldr_off, sidx, R_AARCH64_LD64_GOT_LO12_NC, 0);
        return;
    }
#endif
    size_t adrp_off = cg_sec->len;
    asm_adrp(cg_sec, rd); // adrp xrd, label
    objfile_add_reloc(cg_obj, SEC_TEXT, adrp_off, sidx, R_AARCH64_ADR_PREL_PG_HI21, 0);
    size_t add_off = cg_sec->len;
    asm_add_rd_rd_0(cg_sec, rd); // add x{rd}, x{rd}, #0 (reloc placeholder)
    objfile_add_reloc(cg_obj, SEC_TEXT, add_off, sidx, R_AARCH64_ADD_ABS_LO12_NC, 0);
}

// GOT-based address load: undefined weak → NULL, defined → address.
// On Darwin, emit_adrp_add already uses GOT for undefined external symbols.
static void emit_adrp_got(VReg r, const char *label) {
    if (cg_dry_run) return;
    Arm64Reg rd = REG(r);
    int sidx = objfile_find_sym(cg_obj, label);
    if (sidx < 0) {
        int sec = (label[0] == '.') ? SEC_RODATA : SEC_UNDEF;
        int bind = (label[0] == '.') ? SB_LOCAL : SB_GLOBAL;
        sidx = objfile_add_sym(cg_obj, label, sec, 0, 0, bind, ST_NOTYPE);
    }
    if (sidx >= 0 && cg_obj->syms[sidx].bind == SB_WEAK) {
        // Weak symbols: use GOT indirection so undefined weak resolves to NULL
        size_t adrp_off = cg_sec->len;
        asm_adrp(cg_sec, rd); // adrp x{rd}, :got:label
        objfile_add_reloc(cg_obj, SEC_TEXT, adrp_off, sidx, R_AARCH64_ADR_GOT_PAGE, 0);
        size_t ldr_off = cg_sec->len;
        asm_ldr_rd_rd(cg_sec, rd); // ldr x{rd}, [x{rd}, #:got_lo12:label]
        objfile_add_reloc(cg_obj, SEC_TEXT, ldr_off, sidx, R_AARCH64_LD64_GOT_LO12_NC, 0);
    } else {
        // Non-weak (global data): compute absolute address then load from it
        emit_adrp_add(r, label);
        asm_ldr_rd_rd(cg_sec, rd); // ldr x{rd}, [x{rd}] (load value from address)
    }
}

// TLS address load.
// TLS address load.
// NOTE: On Darwin/macOS, this uses the Linux local-exec model which produces
// output that the system linker cannot handle for TLS.  Proper Darwin TLS
// requires TLV descriptors in __thread_vars and TLVP relocations, which need
// Mach-O writer support not yet implemented.
// Darwin requires __thread_vars section with TLV descriptors, __thread_data
// for initial values, $tlv$init functions, and __tlv_bootstrap thunks.
static void emit_tls_addr(VReg r, LVar *var) {
    if (cg_dry_run) return;
    Arm64Reg rd = REG(r);
    const char *label = asm_sym_name(var_sym_label(var));
    int sidx = objfile_find_sym(cg_obj, label);
    if (sidx < 0)
        sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_TLS);
#if defined(__APPLE__)
    // Darwin ARM64: TLVP (General Dynamic) model via TLV descriptors
    int tlv_boot = objfile_find_sym(cg_obj, "__tlv_bootstrap");
    if (tlv_boot < 0)
        tlv_boot = objfile_add_sym(cg_obj, "__tlv_bootstrap", SEC_UNDEF, 0, 0, SB_GLOBAL, ST_FUNC);
    size_t page_off = cg_sec->len;
    arm64_adrp(cg_sec, rd, 0);
    objfile_add_reloc(cg_obj, SEC_TEXT, page_off, sidx, R_AARCH64_TLSLE_ADD_TPREL_HI12, 0);
    size_t off_off = cg_sec->len;
    arm64_ldr_uoff(cg_sec, 3, rd, rd, 0);
    objfile_add_reloc(cg_obj, SEC_TEXT, off_off, sidx, R_AARCH64_TLSLE_ADD_TPREL_LO12, 0);
    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, rd, 0);
    arm64_orr_reg(cg_sec, 1, ARM64_X0, ARM64_XZR, rd, ARM64_LSL, 0); // mov x0, rd (descriptor arg)
    arm64_blr(cg_sec, ARM64_X16);
    if (rd != ARM64_X0)
        arm64_add_reg(cg_sec, 1, rd, ARM64_X0, ARM64_XZR, ARM64_LSL, 0);
#else
    // Linux ARM64: Local Exec TLS model
    // mrs x{rd}, tpidr_el0
    arm64_mrs(cg_sec, rd, 0x5e82); // mrs x{rd}, tpidr_el0
    size_t hi_off = cg_sec->len;
    arm64_add_imm(cg_sec, 1, rd, rd, 0, 1); // add x{rd}, x{rd}, #:tprel_hi12:label, lsl #12
    objfile_add_reloc(cg_obj, SEC_TEXT, hi_off, sidx, R_AARCH64_TLSLE_ADD_TPREL_HI12, 0);
    size_t lo_off = cg_sec->len;
    arm64_add_imm(cg_sec, 1, rd, rd, 0, 0); // add x{rd}, x{rd}, #:tprel_lo12_nc:label
    objfile_add_reloc(cg_obj, SEC_TEXT, lo_off, sidx, R_AARCH64_TLSLE_ADD_TPREL_LO12, 0);
#endif
}
#endif

static int op_size(Type *ty) {
    if (is_integer(ty) && ty->size < 4)
        return 4;
    if (ty->size > 8)
        return 8;
    return ty->size;
}

static Type *promote_int(Type *ty) {
    if (ty && is_integer(ty) && ty->size < 4)
        return ty_int;
    return ty;
}

#ifndef ARCH_ARM64
static char size_suffix(int sz) {
    if (sz == 1) return 'b';
    if (sz == 2) return 'w';
    if (sz == 4) return 'l';
    return 'q';
}
#endif

static bool use_unsigned(Type *ty) {
    return ty && is_integer(ty) && ty->is_unsigned;
}

static bool use_unsigned_cmp(Node *node) {
    Type *lhs = promote_int(node->lhs->ty);
    Type *rhs = promote_int(node->rhs->ty);
    int size = lhs->size > rhs->size ? lhs->size : rhs->size;
    if (size < 4)
        size = 4;
    return get_integer_type(size, lhs->is_unsigned || rhs->is_unsigned)->is_unsigned;
}

static void sign_extend_to(VReg r, int from_size, int to_size) {
    if (to_size <= from_size)
        return;
    if (to_size == 8) {
        if (from_size == 4)
#ifdef ARCH_ARM64
            asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
#else
            asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
#endif
        else if (from_size == 2)
#ifdef ARCH_ARM64
            asm_movsx(cg_sec, r, r, 8, 2); // movsx8->r rr, rr
#else
            asm_movsx(cg_sec, r, r, 8, 2); // movsx8->r rr, rr
#endif
        else if (from_size == 1)
#ifdef ARCH_ARM64
            asm_movsx(cg_sec, r, r, 8, 1); // movsx8->r rr, rr
#else
            asm_movsx(cg_sec, r, r, 8, 1); // movsx8->r rr, rr
#endif
        else
#ifdef ARCH_ARM64
            asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
#else
            asm_movsx(cg_sec, r, r, 8, 4); // movsx8->r rr, rr
#endif
    } else if (to_size == 4) {
        if (from_size == 2)
#ifdef ARCH_ARM64
            asm_movsx(cg_sec, r, r, 4, 2); // movsx4->r rr, rr
#else
            asm_movsx(cg_sec, r, r, 4, 2); // movsx4->r rr, rr
#endif
        else if (from_size == 1)
#ifdef ARCH_ARM64
            asm_movsx(cg_sec, r, r, 4, 1); // movsx4->r rr, rr
#else
            asm_movsx(cg_sec, r, r, 4, 1); // movsx4->r rr, rr
#endif
        else
            asm_mov_reg_reg(cg_sec, r, r, 4); // mov rr -> rr
    }
}

static void zero_extend_to(VReg r, int from_size, int to_size) {
    if (to_size <= from_size)
        return;
    if (to_size == 8) {
        if (from_size == 4)
            asm_mov_reg_reg(cg_sec, r, r, 4); // mov rr -> rr
        else if (from_size == 2)
#ifdef ARCH_ARM64
            asm_movzx(cg_sec, r, r, 8, 2); // movzx8->r rr, rr
#else
            asm_movzx(cg_sec, r, r, 4, 2); // movzx4->r rr, rr
#endif
        else if (from_size == 1)
#ifdef ARCH_ARM64
            asm_movzx(cg_sec, r, r, 8, 1); // movzx8->r rr, rr
#else
            asm_movzx(cg_sec, r, r, 4, 1); // movzx4->r rr, rr
#endif
        else
            asm_mov_reg_reg(cg_sec, r, r, 4); // mov rr -> rr
    } else if (to_size == 4) {
        if (from_size == 2)
#ifdef ARCH_ARM64
            asm_movzx(cg_sec, r, r, 4, 2); // movzx4->r rr, rr
#else
            asm_movzx(cg_sec, r, r, 4, 2); // movzx4->r rr, rr
#endif
        else if (from_size == 1)
#ifdef ARCH_ARM64
            asm_movzx(cg_sec, r, r, 4, 1); // movzx4->r rr, rr
#else
            asm_movzx(cg_sec, r, r, 4, 1); // movzx4->r rr, rr
#endif
        else
            asm_mov_reg_reg(cg_sec, r, r, 4); // mov rr -> rr
    }
}

// Convert the scalar value in register r (of type `from`, an integer or
// floating-point type) to the representation of complex base type `base`,
// store it as the real part at the buffer pointed to by register `addr`,
// and zero-fill the imaginary part at offset base->size.
static void emit_scalar_to_complex(int r, Type *from, Type *base, int addr) {
    int base_sz = base->size;
    bool base_flo = is_flonum(base);
#ifdef ARCH_ARM64
    if (base_flo) {
        if (is_flonum(from)) {
            asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
        } else if (from->is_unsigned) {
            asm_ucvtf(cg_sec, ARM64_D0, r, from->size == 8 ? 1 : 0); // ucvtf d0, w/x{r}
        } else {
            asm_scvtf(cg_sec, ARM64_D0, r, from->size == 8 ? 1 : 0); // scvtf d0, w/x{r}
        }
        if (base_sz == 4) {
            asm_fcvt(cg_sec, 0, 1, ARM64_S0, ARM64_D0); // fcvt s0, d0
            asm_str_fp(cg_sec, ARM64_S0, addr, 4); // str s0, [x{addr}]
            arm64_str_uoff(cg_sec, 2, ARM64_XZR, REG(addr), 1); // str wzr, [x{addr}, #4]
        } else {
            asm_str_fp(cg_sec, ARM64_D0, addr, 8); // str d0, [x{addr}]
            arm64_str_uoff(cg_sec, 3, ARM64_XZR, REG(addr), 1); // str xzr, [x{addr}, #8]
        }
    } else {
        if (is_flonum(from)) {
            asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
            asm_fcvtzs(cg_sec, r, base_sz); // fcvtzs w/x{r}, d0
        } else if (from->size < base_sz) {
            if (from->is_unsigned)
                zero_extend_to(r, from->size, base_sz);
            else
                sign_extend_to(r, from->size, base_sz);
        }
        if (base_sz == 4) {
            asm_str_reg_off(cg_sec, r, addr, 4, 0);
            arm64_str_uoff(cg_sec, 2, ARM64_XZR, REG(addr), 1); // str wzr, [x{addr}, #4]
        } else {
            asm_str_reg_off(cg_sec, r, addr, 8, 0);
            arm64_str_uoff(cg_sec, 3, ARM64_XZR, REG(addr), 1); // str xzr, [x{addr}, #8]
        }
    }
#else
    X86Mem m_addr0 = x86_mem(REG(addr), 0);
    X86Mem m_addr_base = x86_mem(REG(addr), base_sz);
    if (base_flo) {
        if (is_flonum(from)) {
            asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq reg64[r], %xmm0
        } else if (from->is_unsigned && from->size == 8) {
            int c = ++rcc_label_count;
            asm_test_reg_reg(cg_sec, r, r, 8);
            char *label_high = format(".L.u2cx.high.%d", c);
            char *label_end = format(".L.u2cx.end.%d", c);
            size_t off_js = asm_jcc_label(cg_sec, X86_S);
            asm_fixup_add(cg_sec, off_js, label_high, 1); // js .L.u2cx.high.%d
            x86_cvtsi2sd(cg_sec, 8, X86_XMM0, REG(r)); // cvtsi2sd reg64[r], %xmm0
            size_t off_jmp = asm_jmp_label(cg_sec);
            asm_fixup_add(cg_sec, off_jmp, label_end, 0); // jmp .L.u2cx.end.%d
            cg_def_label(label_high); // .L.u2cx.high.%d:
            x86_mov_rr(cg_sec, 8, X86_RCX, REG(r)); // movq reg64[r], %rcx
            x86_shr_ri(cg_sec, 8, X86_RCX, 1); // shrq %rcx
            x86_cvtsi2sd(cg_sec, 8, X86_XMM0, X86_RCX); // cvtsi2sd %rcx, %xmm0
            x86_addsd(cg_sec, X86_XMM0, X86_XMM0); // addsd %xmm0, %xmm0
            cg_def_label(label_end); // .L.u2cx.end.%d:
        } else if (from->is_unsigned && from->size == 4) {
            x86_cvtsi2sd(cg_sec, 8, X86_XMM0, REG(r)); // cvtsi2sd reg64[r], %xmm0
        } else {
            x86_cvtsi2sd(cg_sec, from->size < 4 ? 4 : from->size, X86_XMM0, REG(r)); // cvtsi2sd reg32/64[r], %xmm0
        }
        if (base_sz == 4) {
            x86_cvtsd2ss(cg_sec, X86_XMM0, X86_XMM0); // cvtsd2ss %xmm0, %xmm0
            x86_movss_mr(cg_sec, m_addr0, X86_XMM0); // movss %xmm0, (reg64[addr])
            x86_mov_mi(cg_sec, 4, m_addr_base, 0); // movl $0, base_sz(reg64[addr])
        } else {
            x86_movsd_mr(cg_sec, m_addr0, X86_XMM0); // movsd %xmm0, (reg64[addr])
            x86_mov_mi(cg_sec, 8, m_addr_base, 0); // movq $0, base_sz(reg64[addr])
        }
    } else {
        if (is_flonum(from)) {
            asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq reg64[r], %xmm0
            x86_cvttsd2si(cg_sec, base_sz, REG(r), X86_XMM0); // cvttsd2si %xmm0, reg32/64[r]
        } else if (from->size < base_sz) {
            if (from->is_unsigned)
                zero_extend_to(r, from->size, base_sz);
            else
                sign_extend_to(r, from->size, base_sz);
        }
        if (base_sz == 4) {
            x86_mov_mr(cg_sec, 4, m_addr0, REG(r)); // movl reg32[r], (reg64[addr])
            x86_mov_mi(cg_sec, 4, m_addr_base, 0); // movl $0, base_sz(reg64[addr])
        } else {
            x86_mov_mr(cg_sec, 8, m_addr0, REG(r)); // movq reg64[r], (reg64[addr])
            x86_mov_mi(cg_sec, 8, m_addr_base, 0); // movq $0, base_sz(reg64[addr])
        }
    }
#endif
}

// Convert a TY_COMPLEX value at address `src` (real-floating base type
// `from->base`, e.g. _Complex float) to a TY_COMPLEX value at address `dst`
// (real-floating base type `to->base`, e.g. _Complex double), narrowing or
// widening each component as needed (cvtss2sd/cvtsd2ss).  Both components
// are loaded before either is stored, so src == dst (in-place conversion,
// e.g. a smaller hidden-retbuf result being widened in place) is safe even
// when to->size > from->size.
static void emit_complex_convert_float(int src, int dst, Type *from, Type *to) {
    (void)to; // used via to->base->size in ARCH_ARM64 path
    int fsz = from->base->size;
#ifdef ARCH_ARM64
    int tsz = to->base->size;
    if (fsz == 4) {
        asm_ldr_fp(cg_sec, ARM64_S0, src, 4); // ldr s0, [x{src}]
        arm64_ldr_fp(cg_sec, 2, (Arm64Reg)(ARM64_S0 + 1), REG(src), 4); // ldr s1, [x{src}, #4]
        asm_fcvt(cg_sec, 1, 0, ARM64_D0, ARM64_S0); // fcvt d0, s0
        asm_fcvt(cg_sec, 1, 0, (Arm64Reg)(ARM64_D0 + 1), (Arm64Reg)(ARM64_S0 + 1)); // fcvt d1, s1
    } else {
        asm_ldr_fp(cg_sec, ARM64_D0, src, 8); // ldr d0, [x{src}]
        arm64_ldr_fp(cg_sec, 3, (Arm64Reg)(ARM64_D0 + 1), REG(src), 8); // ldr d1, [x{src}, #8]
        asm_fcvt(cg_sec, 0, 1, ARM64_S0, ARM64_D0); // fcvt s0, d0
        asm_fcvt(cg_sec, 0, 1, (Arm64Reg)(ARM64_S0 + 1), (Arm64Reg)(ARM64_D0 + 1)); // fcvt s1, d1
    }
    if (tsz == 4) {
        asm_str_fp(cg_sec, ARM64_S0, dst, 4); // str s0, [x{dst}]
        arm64_str_fp(cg_sec, 2, (Arm64Reg)(ARM64_S0 + 1), REG(dst), 4); // str s1, [x{dst}, #4]
    } else {
        asm_str_fp(cg_sec, ARM64_D0, dst, 8); // str d0, [x{dst}]
        arm64_str_fp(cg_sec, 3, (Arm64Reg)(ARM64_D0 + 1), REG(dst), 8); // str d1, [x{dst}, #8]
    }
#else
    if (fsz == 4) {
        // _Complex float → _Complex double: load singles, widen to doubles
        asm_mov_fp_rm(cg_sec, 4, X86_XMM0, x86_mem(REG(src), 0)); // movss (src), %xmm0
        asm_mov_fp_rm(cg_sec, 4, X86_XMM1, x86_mem(REG(src), 4)); // movss 4(src), %xmm1
        x86_cvtss2sd(cg_sec, X86_XMM0, X86_XMM0); // cvtss2sd %xmm0, %xmm0
        x86_cvtss2sd(cg_sec, X86_XMM1, X86_XMM1); // cvtss2sd %xmm1, %xmm1
        asm_mov_fp_mr(cg_sec, 8, x86_mem(REG(dst), 0), X86_XMM0); // movsd %xmm0, (dst)
        asm_mov_fp_mr(cg_sec, 8, x86_mem(REG(dst), 8), X86_XMM1); // movsd %xmm1, 8(dst)
    } else {
        // _Complex double → _Complex float: load doubles, narrow to singles
        asm_mov_fp_rm(cg_sec, 8, X86_XMM0, x86_mem(REG(src), 0)); // movsd (src), %xmm0
        asm_mov_fp_rm(cg_sec, 8, X86_XMM1, x86_mem(REG(src), 8)); // movsd 8(src), %xmm1
        x86_cvtsd2ss(cg_sec, X86_XMM0, X86_XMM0); // cvtsd2ss %xmm0, %xmm0
        x86_cvtsd2ss(cg_sec, X86_XMM1, X86_XMM1); // cvtsd2ss %xmm1, %xmm1
        asm_mov_fp_mr(cg_sec, 4, x86_mem(REG(dst), 0), X86_XMM0); // movss %xmm0, (dst)
        asm_mov_fp_mr(cg_sec, 4, x86_mem(REG(dst), 4), X86_XMM1); // movss %xmm1, 4(dst)
    }
#endif
}

// Convert a TY_COMPLEX value at address `src` (integer base type
// `from->base`, e.g. _Complex char) to a TY_COMPLEX value at address `dst`
// (integer base type `to->base`, e.g. _Complex int), sign- or
// zero-extending (per from->base->is_unsigned) or truncating each
// component as needed. Both components are loaded before either is
// stored, so src == dst is safe.
static void emit_complex_convert_int(int src, int dst, Type *from, Type *to) {
    int fsz = from->base->size, tsz = to->base->size;
    bool uns = from->base->is_unsigned;
#ifdef ARCH_ARM64
    if (fsz == 8) {
        arm64_ldr_uoff(cg_sec, 3, ARM64_X0, REG(src), 0); // ldr x0, [x{src}]
        arm64_ldr_uoff(cg_sec, 3, ARM64_X1, REG(src), 1); // ldr x1, [x{src}, #8]
    } else if (fsz == 4) {
        if (uns) {
            arm64_ldr_uoff(cg_sec, 2, ARM64_X0, REG(src), 0); // ldr w0, [x{src}]
            arm64_ldr_uoff(cg_sec, 2, ARM64_X1, REG(src), 1); // ldr w1, [x{src}, #4]
        } else {
            arm64_ldrsw_uoff(cg_sec, ARM64_X0, REG(src), 0); // ldrsw x0, [x{src}]
            arm64_ldrsw_uoff(cg_sec, ARM64_X1, REG(src), 1); // ldrsw x1, [x{src}, #4]
        }
    } else if (fsz == 2) {
        arm64_ldrh_uoff(cg_sec, ARM64_X0, REG(src), 0); // ldrh x0, [x{src}]
        arm64_ldrh_uoff(cg_sec, ARM64_X1, REG(src), 1); // ldrh x1, [x{src}, #2]
    } else {
        arm64_ldrb_uoff(cg_sec, ARM64_X0, REG(src), 0); // ldrb w0, [x{src}]
        arm64_ldrb_uoff(cg_sec, ARM64_X1, REG(src), 1); // ldrb w1, [x{src}, #1]
    }
    if (tsz == 8) {
        arm64_str_uoff(cg_sec, 3, ARM64_X0, REG(dst), 0); // str x0, [x{dst}]
        arm64_str_uoff(cg_sec, 3, ARM64_X1, REG(dst), 1); // str x1, [x{dst}, #8]
    } else if (tsz == 4) {
        arm64_str_uoff(cg_sec, 2, ARM64_X0, REG(dst), 0); // str w0, [x{dst}]
        arm64_str_uoff(cg_sec, 2, ARM64_X1, REG(dst), 1); // str w1, [x{dst}, #4]
    } else if (tsz == 2) {
        arm64_strh_uoff(cg_sec, ARM64_X0, REG(dst), 0); // strh w0, [x{dst}]
        arm64_strh_uoff(cg_sec, ARM64_X1, REG(dst), 1); // strh w1, [x{dst}, #2]
    } else {
        arm64_strb_uoff(cg_sec, ARM64_X0, REG(dst), 0); // strb w0, [x{dst}]
        arm64_strb_uoff(cg_sec, ARM64_X1, REG(dst), 1); // strb w1, [x{dst}, #1]
    }
#else
    // x86_64: load both components from src into RAX/RCX, then store to dst
    // Sign/zero-extend from source size, truncate to target size
    if (fsz == 8) {
        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(src), 0)); // movq (src), %rax
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(src), 8)); // movq 8(src), %rcx
    } else if (fsz == 4) {
        if (uns) {
            x86_mov_rm(cg_sec, 4, X86_RAX, x86_mem(REG(src), 0)); // movl (src), %eax
            x86_mov_rm(cg_sec, 4, X86_RCX, x86_mem(REG(src), 4)); // movl 4(src), %ecx
        } else {
            x86_movsx_rm(cg_sec, 8, 4, X86_RAX, x86_mem(REG(src), 0)); // movslq (src), %rax
            x86_movsx_rm(cg_sec, 8, 4, X86_RCX, x86_mem(REG(src), 4)); // movslq 4(src), %rcx
        }
    } else if (fsz == 2) {
        if (uns) {
            x86_movzx_rm(cg_sec, 4, 2, X86_RAX, x86_mem(REG(src), 0)); // movzwl (src), %eax
            x86_movzx_rm(cg_sec, 4, 2, X86_RCX, x86_mem(REG(src), 2)); // movzwl 2(src), %ecx
        } else {
            x86_movsx_rm(cg_sec, 4, 2, X86_RAX, x86_mem(REG(src), 0)); // movswl (src), %eax
            x86_movsx_rm(cg_sec, 4, 2, X86_RCX, x86_mem(REG(src), 2)); // movswl 2(src), %ecx
        }
    } else {
        if (uns) {
            x86_movzx_rm(cg_sec, 4, 1, X86_RAX, x86_mem(REG(src), 0)); // movzbl (src), %eax
            x86_movzx_rm(cg_sec, 4, 1, X86_RCX, x86_mem(REG(src), 1)); // movzbl 1(src), %ecx
        } else {
            x86_movsx_rm(cg_sec, 4, 1, X86_RAX, x86_mem(REG(src), 0)); // movsbl (src), %eax
            x86_movsx_rm(cg_sec, 4, 1, X86_RCX, x86_mem(REG(src), 1)); // movsbl 1(src), %ecx
        }
    }
    // Store to dst at target size
    x86_mov_mr(cg_sec, tsz, x86_mem(REG(dst), 0), X86_RAX); // mov %al/%ax/%eax/%rax, (dst)
    x86_mov_mr(cg_sec, tsz, x86_mem(REG(dst), tsz), X86_RCX); // mov %cl/%cx/%ecx/%rcx, tsz(dst)
#endif
}

#ifndef ARCH_ARM64
// Given a 32-bit x86_64 register name (e.g. "%eax", "%r9d"), return the
// 8-bit ("%al", "%r9b") or 16-bit ("%ax", "%r9w") sub-register name, used
// to store a truncated _Complex char/short integer component.
static const char *x86_subreg(const char *reg32, int sz) {
    static const char *map8[] = {
        "%eax", "%al", "%ecx", "%cl", "%edx", "%dl", "%edi", "%dil",
        "%r8d", "%r8b", "%r9d", "%r9b", "%r10d", "%r10b", "%r11d", "%r11b", NULL};
    static const char *map16[] = {
        "%eax", "%ax", "%ecx", "%cx", "%edx", "%dx", "%edi", "%di",
        "%r8d", "%r8w", "%r9d", "%r9w", "%r10d", "%r10w", "%r11d", "%r11w", NULL};
    const char **map = sz == 1 ? map8 : map16;
    for (int i = 0; map[i]; i += 2)
        if (!strcmp(reg32, map[i]))
            return map[i + 1];
    return reg32;
}
#endif

#ifdef ARCH_ARM64
// Sentinels for base register index in emit_store / emit_load:
// >= 0: virtual register index, REG maps to physical register
// ARM64_BASE_FP (-29): frame pointer (x29)
// ARM64_BASE_SP (-31): stack pointer (x31)
#define ARM64_BASE_FP (-29)
#define ARM64_BASE_SP (-31)
#define ARM64_BASE_X16 (-16)

// Get physical ARM64 register number from base index
static int arm64_base_phys(int base) {
    if (base == ARM64_BASE_FP || base == 29) return 29;
    if (base == ARM64_BASE_SP || base == 31) return 31;
    if (base == ARM64_BASE_X16 || base == 16) return 16;
    return REG(base);
}

// Check if base register matches destination virtual register
static bool arm64_base_conflicts(int base, VReg r) {
    return base >= 0 && base == r;
}

static void emit_store(Type *ty, VReg src, VReg base, int off) {
    int sz = ty->size;
    if (off == 0) {
        asm_str_reg_off(cg_sec, src, base, sz, 0); // str {src}, [base]
    } else if (off > -256 && off < 256) {
        asm_stur_sz(cg_sec, src, REG(base), sz, off); // stur {src}, [base, #off]
    } else {
        // Offset outside STUR range: use X17 scratch
        if (off < 0 && -off <= 4095) {
            arm64_sub_imm(cg_sec, 1, ARM64_X17, REG(base), -off, 0); // sub x17, base, #-off
        } else if (off >= 0 && off <= 4095) {
            arm64_add_imm(cg_sec, 1, ARM64_X17, REG(base), off, 0); // add x17, base, #off
        } else {
            int abs_off = off < 0 ? -off : off;
            emit_mov_imm64(ARM64_X17, (uint64_t)abs_off);
            if (off < 0)
                arm64_sub_reg(cg_sec, 1, ARM64_X17, REG(base), ARM64_X17, ARM64_LSL, 0);
            else
                arm64_add_reg(cg_sec, 1, ARM64_X17, ARM64_X17, REG(base), ARM64_LSL, 0);
        }
        switch (sz) {
        case 1: arm64_strb_uoff(cg_sec, REG(src), ARM64_X17, 0); break;
        case 2: arm64_strh_uoff(cg_sec, REG(src), ARM64_X17, 0); break;
        case 4: arm64_str_uoff(cg_sec, 2, REG(src), ARM64_X17, 0); break;
        default: arm64_str_uoff(cg_sec, 3, REG(src), ARM64_X17, 0); break;
        }
    }
}

static void emit_store_offset(Type *ty, VReg r, Arm64Reg base, int offset) {
    int sz = ty->size;
    int off = offset;
    if (off > -256 && off < 256) {
        asm_stur_sz(cg_sec, r, base, sz, off); // stur r, [base, #off]
        return;
    }
    int abs_off = off < 0 ? -off : off;
    // Use X17 scratch instead of alloc_reg to avoid spill pressure
    if (abs_off <= 4095) {
        if (off < 0)
            arm64_sub_imm(cg_sec, 1, ARM64_X17, base, abs_off, 0); // sub x17, base, #abs_off
        else
            arm64_add_imm(cg_sec, 1, ARM64_X17, base, abs_off, 0); // add x17, base, #abs_off
    } else {
        emit_mov_imm64(ARM64_X17, (uint64_t)abs_off);
        if (off < 0)
            arm64_sub_reg(cg_sec, 1, ARM64_X17, base, ARM64_X17, ARM64_LSL, 0);
        else
            arm64_add_reg(cg_sec, 1, ARM64_X17, ARM64_X17, base, ARM64_LSL, 0);
    }
    switch (sz) {
    case 1: arm64_strb_uoff(cg_sec, REG(r), ARM64_X17, 0); break;
    case 2: arm64_strh_uoff(cg_sec, REG(r), ARM64_X17, 0); break;
    case 4: arm64_str_uoff(cg_sec, 2, REG(r), ARM64_X17, 0); break;
    default: arm64_str_uoff(cg_sec, 3, REG(r), ARM64_X17, 0); break;
    }
}
#endif

static void emit_load(Type *ty, VReg r, int base, int off) {
#ifdef ARCH_ARM64
    // ARM64: ldr wN,[xN] is CONSTRAINED UNPREDICTABLE — avoid base==dest
    if (arm64_base_conflicts(base, r)) {
        asm_mov_x16_reg(cg_sec, base); // mov x16, x{base}
        base = ARM64_BASE_X16;
    }
    if (off > -256 && off < 256) {
        asm_ldur_sz(cg_sec, r, arm64_base_phys(base), ty->size, off); // ldur r, [base, #off]
        // Sign-extend narrow signed types (ARM64 ldur{b,h} zero-extend)
        if (!ty->is_unsigned) {
            if (ty->size == 1)
                arm64_sxtb(cg_sec, 0, REG(r), REG(r)); // sxtb w{r}, w{r}
            else if (ty->size == 2)
                arm64_sxth(cg_sec, 0, REG(r), REG(r)); // sxth w{r}, w{r}
        }
    } else {
        // Offset outside LDUR range: use X17 as scratch for address computation
        // (never in the register pool, avoids spill pressure)
        Arm64Reg bp = arm64_base_phys(base);
        if (off < 0 && -off <= 4095) {
            arm64_sub_imm(cg_sec, 1, ARM64_X17, bp, -off, 0); // sub x17, base, #-off
        } else if (off >= 0 && off <= 4095) {
            arm64_add_imm(cg_sec, 1, ARM64_X17, bp, off, 0); // add x17, base, #off
        } else {
            int abs_off = off < 0 ? -off : off;
            emit_mov_imm64(ARM64_X17, (uint64_t)abs_off);
            if (off < 0)
                arm64_sub_reg(cg_sec, 1, ARM64_X17, bp, ARM64_X17, ARM64_LSL, 0);
            else
                arm64_add_reg(cg_sec, 1, ARM64_X17, ARM64_X17, bp, ARM64_LSL, 0);
        }
        // Load from [x17] with correct size
        switch (ty->size) {
        case 1: arm64_ldrb_uoff(cg_sec, REG(r), ARM64_X17, 0); break;
        case 2: arm64_ldrh_uoff(cg_sec, REG(r), ARM64_X17, 0); break;
        case 4: arm64_ldr_uoff(cg_sec, 2, REG(r), ARM64_X17, 0); break;
        default: arm64_ldr_uoff(cg_sec, 3, REG(r), ARM64_X17, 0); break;
        }
        if (!ty->is_unsigned) {
            if (ty->size == 1)
                arm64_sxtb(cg_sec, 0, REG(r), REG(r));
            else if (ty->size == 2)
                arm64_sxth(cg_sec, 0, REG(r), REG(r));
        }
    }
#else
#ifndef X86_BASE_RBP
#define X86_BASE_RBP (-1)
#endif
    // x86_64: base >= 0 = virtual reg, X86_BASE_RBP = rbp-relative /* ldr %s, [%s]\n */
    int sz = op_size(ty);
    // Struct/union of 5-7 bytes: no x86 instruction loads exactly 5-7 bytes;
    // round up to 8 (64-bit load) so all bytes reach the register.
    if (sz > 4 && sz < 8)
        sz = 8;
    if (base == X86_BASE_RBP) {
        if (ty->size == 1) {
            if (ty->is_unsigned)
                asm_movzx_rbp_reg(cg_sec, r, 4, 1, off); // movzbl -off(%rbp), rr
            else
                asm_movsx_rbp_reg(cg_sec, r, 4, 1, off); // movsbl -off(%rbp), rr
        } else if (ty->size == 2) {
            if (ty->is_unsigned)
                asm_movzx_rbp_reg(cg_sec, r, 4, 2, off); // movzwl -off(%rbp), rr
            else
                asm_movsx_rbp_reg(cg_sec, r, 4, 2, off); // movswl -off(%rbp), rr
        } else {
            asm_mov_rbp_reg(cg_sec, r, sz, off); // mov -off(%rbp), rr
        }
    } else {
        if (ty->size == 1) {
            if (ty->is_unsigned)
                asm_movzx_base_off_reg(cg_sec, r, base, (int64_t)off, 4, 1); // movzbl off(base), rr
            else
                asm_movsx_base_off_reg(cg_sec, r, base, (int64_t)off, 4, 1); // movsbl off(base), rr
        } else if (ty->size == 2) {
            if (ty->is_unsigned)
                asm_movzx_base_off_reg(cg_sec, r, base, (int64_t)off, 4, 2); // movzwl off(base), rr
            else
                asm_movsx_base_off_reg(cg_sec, r, base, (int64_t)off, 4, 2); // movswl off(base), rr
        } else {
            asm_mov_base_off_reg(cg_sec, r, base, (int64_t)off, sz); // movl/movq off(base), rr
        }
    }
    if (sz == 8 && ty->size < 4)
        asm_movzx(cg_sec, r, r, 4, ty->size); // movzx4->r rr, rr
#endif
}

static VReg alloc_reg(void) {
    for (int i = 0; i < NUM_REGS; i++) {
        if ((used_regs & (1 << i)) == 0) {
            used_regs |= (1 << i);
            ever_used_regs |= (1 << i);
            return (VReg)i;
        }
    }
    // All registers are in use. Spill the register with the highest index
    // (least likely to be referenced by outer callers right now).
    for (int i = NUM_REGS - 1; i >= 0; i--) {
        if (used_regs & (1 << i)) {
            if (opt_W) {
                if (reg_owner[i])
                    fprintf(stderr, "\033[1;33mwarning:\033[0m spilling %s (%s) to stack in %s\n", reg64[i], reg_owner[i], current_fn);
                else
                    fprintf(stderr, "\033[1;33mwarning:\033[0m spilling %s to stack in %s\n", reg64[i], current_fn);
            }
#ifdef ARCH_ARM64
            asm_stur_fp(cg_sec, i, spill_offset(i)); // str x(i), [x29, #-spill_offset(i)]
#else
            asm_mov_reg_rbp(cg_sec, i, 8, spill_offset(i)); // mov ri, [rbp-8]
#endif
            spilled_regs |= (1 << i);
            spill_count++;
            used_regs &= ~(1 << i);
            used_regs |= (1 << i); // reclaim for new value
            ever_used_regs |= (1 << i);
            return (VReg)i;
        }
    }
    error("Register exhaustion");
    return 0;
}

static void free_reg(VReg i) {
    if (spilled_regs & (1 << i)) {
#ifdef ARCH_ARM64
        asm_ldur_fp(cg_sec, i, spill_offset(i)); // ldr x(i), [x29, #-spill_offset(i)]
#else
        asm_mov_rbp_reg(cg_sec, i, 8, spill_offset(i)); // mov [rbp-8], ri
#endif
        spilled_regs &= ~(1 << i);
    }
    used_regs &= ~(1 << i);
    reg_owner[i] = NULL;
}

#ifndef ARCH_ARM64
#ifdef _WIN32
// MinGW emulated TLS: load the address of a thread-local variable by calling
// __emutls_get_address with a pointer to the variable's control block.
static void emit_emutls_addr(VReg dst, const char *label) {
    bool save0 = (used_regs & (1 << 0)) && dst != 0;
    bool save1 = (used_regs & (1 << 1)) && dst != 1;
    if (save0) asm_mov_phyreg_rbp(cg_sec, X86_R10, 8, spill_offset(0));
    if (save1) asm_mov_phyreg_rbp(cg_sec, X86_R11, 8, spill_offset(1));

    VReg tmp = alloc_reg();
    const char *ctrl = format("__emutls_v.%s", label);
    asm_lea_rip_reg(cg_sec, tmp, ctrl);
    x86_mov_rr(cg_sec, 8, X86_RCX, REG(tmp));
    free_reg(tmp);

    x86_sub_ri(cg_sec, 8, X86_RSP, 32);
    size_t call_off = asm_call_label(cg_sec);
    int sidx = objfile_find_sym(cg_obj, "__emutls_get_address");
    if (sidx < 0)
        sidx = objfile_add_sym(cg_obj, "__emutls_get_address", SEC_UNDEF, 0, 0, SB_GLOBAL, ST_FUNC);
    objfile_add_reloc(cg_obj, SEC_TEXT, call_off + 1, sidx, R_X86_64_PC32, -4);
    x86_add_ri(cg_sec, 8, X86_RSP, 32);

    x86_mov_rr(cg_sec, 8, REG(dst), X86_RAX);
    if (save1) asm_mov_rbp_reg(cg_sec, 1, 8, spill_offset(1));
    if (save0) asm_mov_rbp_reg(cg_sec, 0, 8, spill_offset(0));
}
#endif
#endif

static VReg gen(Node *node);

#ifdef ARCH_ARM64
// Minimal ARM64 inline-asm template validator.
// Reports rcc-style errors for invalid mnemonics and range violations
// so that test 139 gets our messages instead of gas messages.
static void arm64_validate_asm_template(const char *tmpl, Token *tok) {
    // Known ARM64 mnemonics (comprehensive but not exhaustive)
    static const char *const known[] = {
        "add", "adds", "sub", "subs", "neg", "negs", "mul", "madd", "msub", "mneg",
        "div", "udiv", "sdiv", "smull", "umull", "smulh", "umulh",
        "and", "ands", "orr", "eor", "bic", "bics", "orn", "eon",
        "lsl", "lsr", "asr", "ror", "extr",
        "mov", "movz", "movk", "movn",
        "adrp", "adr",
        "ldr", "ldrb", "ldrh", "ldrsb", "ldrsh", "ldrsw",
        "ldur", "ldurb", "ldurh", "ldursb", "ldursh", "ldursw",
        "str", "strb", "strh", "stur", "sturb", "sturh",
        "ldp", "stp", "ldnp", "stnp",
        "b", "bl", "blr", "br", "ret",
        "cbz", "cbnz", "tbz", "tbnz",
        "b.eq", "b.ne", "b.lt", "b.le", "b.gt", "b.ge",
        "b.cc", "b.cs", "b.hi", "b.ls", "b.mi", "b.pl", "b.vs", "b.vc",
        "b.lo", "b.hs", "b.al", "b.nv", "b.eq", "beq", "bne", "blt", "ble", "bgt", "bge",
        "mrs", "msr", "nop", "brk", "svc", "hlt",
        "dmb", "dsb", "isb",
        "csel", "cset", "csetm", "csinc", "csinv", "csneg",
        "clz", "cls", "rbit", "rev", "rev16", "rev32",
        "sbfm", "ubfm", "bfm", "sbfx", "ubfx", "bfi", "bfxil",
        "sxth", "sxtw", "sxtb", "uxtb", "uxth",
        "fmov", "fadd", "fsub", "fmul", "fdiv", "fneg", "fabs", "fsqrt",
        "fcmp", "fccmp", "fcmpe", "fccmpe",
        "fcvt", "fcvtzu", "fcvtzs", "scvtf", "ucvtf", "fcvtas", "fcvtau",
        "fcvtms", "fcvtmu", "fcvtns", "fcvtnu", "fcvtps", "fcvtpu",
        "fmax", "fmin", "fmaxnm", "fminnm",
        "paciasp", "autiasp", "pacibsp", "autibsp", "xpaclri",
        "prfm", "prefetch",
        "eret", "drps", "eor",
        "lda", "ldaex", "stl", "stlex", "stlr", "ldar", "ldaxr", "stlxr",
        "ldxr", "stxr", "ldaxp", "stlxp", "ldxp", "stxp",
        "clrex", "yield", "wfe", "wfi", "sev", "sevl",
        "hint", "sys", "sysl", "at", "dc", "ic", "tlbi", "msr", "mrs",
        NULL};

    const char *p = tmpl;
    while (*p) {
        // skip whitespace / separators
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
               *p == ';') p++;
        if (!*p) break;

        // skip operand substitutions (%N, %w0, %[name], etc.)
        if (*p == '%') {
            while (*p && *p != ';' && *p != '\n') p++;
            continue;
        }

        // Extract mnemonic (first word)
        char mnem[64];
        int mlen = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != ';' && *p != '\n' && *p != ':' && mlen < 63)
            mnem[mlen++] = tolower((unsigned char)*p++);
        mnem[mlen] = '\0';
        if (!mlen) continue;

        // Label definition (e.g. ".L_skip:") — not an instruction, skip it.
        if (*p == ':') {
            p++;
            continue;
        }

        // Skip to rest of instruction
        while (*p == ' ' || *p == '\t') p++;
        const char *operands = p;

        // Validate mnemonic
        bool found = false;
        for (int j = 0; known[j]; j++) {
            if (strcmp(mnem, known[j]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            error_tok_simple(tok, "ARM64 instruction '%s' not implemented", mnem);
            // skip rest of line and continue
            while (*p && *p != ';' && *p != '\n') p++;
            continue;
        }

        // --- Specific instruction validation ---

        // LSL/LSR/ASR immediate shift range
        if ((strcmp(mnem, "lsl") == 0 || strcmp(mnem, "lsr") == 0 || strcmp(mnem, "asr") == 0) &&
            operands[0] && operands[0] != '%') {
            bool is32 = (tolower((unsigned char)operands[0]) == 'w');
            const char *hash = NULL;
            for (const char *q = operands; *q && *q != ';' && *q != '\n'; q++)
                if (*q == '#') {
                    hash = q + 1;
                    break;
                }
            if (hash) {
                long val = strtol(hash, NULL, 0);
                int maxshift = is32 ? 31 : 63;
                if (val < 0 || val > maxshift)
                    error_tok_simple(tok, "shift immediate out of range");
            }
        }

        // MRS: validate system register name
        if (strcmp(mnem, "mrs") == 0) {
            static const char *const sysregs[] = {
                "fpcr", "fpsr", "nzcv", "daif", "currentel", "spsel",
                "midr_el1", "mpidr_el1", "id_aa64pfr0_el1", "id_aa64pfr1_el1",
                "id_aa64dfr0_el1", "id_aa64dfr1_el1", "id_aa64isar0_el1",
                "id_aa64isar1_el1", "id_aa64mmfr0_el1", "id_aa64mmfr1_el1",
                "sctlr_el1", "tcr_el1", "ttbr0_el1", "ttbr1_el1", "mair_el1",
                "tpidr_el0", "tpidrro_el0", "tpidr_el1",
                "sp_el0", "elr_el1", "spsr_el1",
                "cntkctl_el1", "cntpct_el0", "cntvct_el0", "cntfrq_el0",
                "pmcr_el0", "pmcntenset_el0", "pmccntr_el0", "pmintenset_el1",
                "esr_el1", "far_el1", "par_el1", "afsr0_el1", "afsr1_el1",
                "revidr_el1", "aidr_el1", "csselr_el1", "clidr_el1", "ctr_el0",
                "dczid_el0", "isr_el1", "vbar_el1", "rvbar_el1", "rmr_el1",
                "contextidr_el1", "tpidr_el0", "tpidrro_el0", "amair_el1",
                NULL};
            const char *comma = strchr(operands, ',');
            if (comma) {
                const char *r = comma + 1;
                while (*r == ' ' || *r == '\t') r++;
                char sysreg[64];
                int sl = 0;
                while (*r && *r != ' ' && *r != '\t' && *r != '\n' && *r != ';' && sl < 63)
                    sysreg[sl++] = tolower((unsigned char)*r++);
                sysreg[sl] = '\0';
                if (*sysreg && *sysreg != '%') {
                    bool ok = false;
                    for (int j = 0; sysregs[j]; j++)
                        if (strcmp(sysreg, sysregs[j]) == 0) {
                            ok = true;
                            break;
                        }
                    if (!ok)
                        error_tok_simple(tok, "unsupported system register");
                }
            }
        }

        // MSR: validate system register name (first operand)
        if (strcmp(mnem, "msr") == 0) {
            static const char *const sysregs_msr[] = {
                "fpcr", "fpsr", "nzcv", "daif", "currentel", "spsel",
                "sctlr_el1", "tcr_el1", "ttbr0_el1", "ttbr1_el1", "mair_el1",
                "tpidr_el0", "tpidr_el1", "vbar_el1", "contextidr_el1", "amair_el1",
                "cntkctl_el1", "pmcr_el0", "pmcntenset_el0", "pmintenset_el1",
                NULL};
            char sysreg[64];
            int sl = 0;
            const char *r = operands;
            while (*r == ' ' || *r == '\t') r++;
            while (*r && *r != ',' && *r != ' ' && *r != '\t' && *r != '\n' && sl < 63)
                sysreg[sl++] = tolower((unsigned char)*r++);
            sysreg[sl] = '\0';
            (void)sysregs_msr;
            (void)sysreg; // validated by gas; don't double-error
        }

        // DMB/DSB: validate barrier option
        if (strcmp(mnem, "dmb") == 0 || strcmp(mnem, "dsb") == 0) {
            static const char *const opts[] = {
                "sy", "st", "ld", "ish", "ishst", "ishld",
                "nsh", "nshst", "nshld", "osh", "oshst", "oshld",
                "full", NULL};
            char opt[32];
            int ol = 0;
            const char *r = operands;
            while (*r == ' ' || *r == '\t') r++;
            while (*r && *r != ' ' && *r != '\t' && *r != '\n' && *r != ';' && ol < 31)
                opt[ol++] = tolower((unsigned char)*r++);
            opt[ol] = '\0';
            if (*opt && *opt != '%') {
                // allow numeric
                bool ok = (*opt >= '0' && *opt <= '9');
                for (int j = 0; !ok && opts[j]; j++)
                    if (strcmp(opt, opts[j]) == 0) ok = true;
                if (!ok)
                    error_tok_simple(tok, "invalid operand '%s'", opt);
            }
        }

        // ADD/SUB: check for missing third operand (when operands are literal)
        if ((strcmp(mnem, "add") == 0 || strcmp(mnem, "sub") == 0) &&
            operands[0] && operands[0] != '%') {
            int ncommas = 0;
            for (const char *q = operands; *q && *q != ';' && *q != '\n'; q++)
                if (*q == ',') ncommas++;
            if (ncommas < 2)
                error_tok_simple(tok, "missing third operand");
        }

        // MOVZ/MOVK: check immediate value and shift
        if (strcmp(mnem, "movz") == 0 || strcmp(mnem, "movk") == 0) {
            const char *hash = NULL;
            for (const char *q = operands; *q && *q != ';' && *q != '\n'; q++)
                if (*q == '#') {
                    hash = q + 1;
                    break;
                }
            if (hash) {
                long val = strtol(hash, NULL, 0);
                if (val < 0 || val > 65535)
                    error_tok_simple(tok, "move wide immediate out of range");
                // Check for lsl shift
                const char *comma2 = NULL;
                {
                    // Find comma after the first arg (dest reg)
                    const char *q = operands;
                    int nc = 0;
                    while (*q && *q != ';' && *q != '\n') {
                        if (*q == ',') {
                            nc++;
                            if (nc == 2) {
                                comma2 = q;
                                break;
                            }
                        }
                        q++;
                    }
                }
                if (comma2) {
                    const char *h2 = strchr(comma2, '#');
                    if (h2) {
                        long shift = strtol(h2 + 1, NULL, 0);
                        if (shift != 0 && shift != 16 && shift != 32 && shift != 48)
                            error_tok_simple(tok, "move wide shift out of range");
                    }
                }
            }
        }

        // Skip rest of this instruction
        while (*p && *p != ';' && *p != '\n') p++;
    }
}
#endif // ARCH_ARM64

#ifdef ARCH_ARM64
// Try to extract an integer constant from a node (traversing casts).
// Returns true and sets *val if the node reduces to a compile-time constant.
static bool try_const_int(Node *n, int64_t *val) {
    while (n && n->kind == ND_CAST)
        n = n->lhs;
    if (n && n->kind == ND_NUM) {
        *val = n->val;
        return true;
    }
    return false;
}
#endif // ARCH_ARM64

// Generate code to compute the absolute address of an lvalue.
static VReg gen_addr(Node *node) {
    switch (node->kind) {
    case ND_LVAR: {
        //fprintf(stderr, "DEBUG ND_LVAR: %s is_local=%d is_function=%d is_weak=%d\n",
        //        node->var->name, node->var->is_local, node->var->is_function, node->var->is_weak);
        VReg r = alloc_reg();
        if (opt_W) reg_owner[r] = node->var->name;
        if (node->var->is_local) {
            if (node->var->ty->kind == TY_VLA || ((node->var->ty->kind == TY_STRUCT || node->var->ty->kind == TY_UNION) && node->var->ty->vla_len_expr)) {
#ifdef ARCH_ARM64
                arm64_load_from_fp_minus(node->var->offset - 8, REG(r));
#else
                asm_mov_rbp_reg(cg_sec, r, 8, node->var->offset - 8); // mov [rbp-8], rr
#endif
            } else {
#ifdef ARCH_ARM64
                if (node->var->offset <= 4095)
                    asm_sub_reg_fp_imm(cg_sec, r, node->var->offset); // sub r, x29, #node->var->offset
                else {
                    int v = node->var->offset;
                    asm_mov_imm(cg_sec, r, 8, v & 0xffff); // mov r, #v
                    v >>= 16;
                    int s = 16;
                    while (v) {
                        asm_movk(cg_sec, REG(r), 1, (uint16_t)(v & 0xffff), s); // movk r, #v, lsl #s
                        v >>= 16;
                        s += 16;
                    }
                    asm_sub_reg_fp_reg(cg_sec, r, r, 8); // sub r, x29, r
                }
#else
                asm_lea_rbp_reg(cg_sec, r, 8, node->var->offset); // lea [rbp-8], rr
#endif
            }
        } else {
            if (node->var->is_weak)
                cg_weak_declare(asm_sym_name(var_sym_label(node->var)));
#ifdef ARCH_ARM64
            if (node->var->is_tls)
                emit_tls_addr(r, node->var);
            else if (node->var->is_weak)
                emit_adrp_got(r, asm_sym_name(var_sym_label(node->var)));
            else
                emit_adrp_add(r, asm_sym_name(var_sym_label(node->var)));
#else
            if (node->var->is_tls) {
#ifdef _WIN32
                emit_emutls_addr(r, var_label(node->var));
#else
                VReg base = alloc_reg();
                asm_mov_fs0_reg(cg_sec, base);
                asm_lea_tpoff_base_reg(cg_sec, r, base, var_sym_label(node->var));
                free_reg(base);
#endif
            } else if (node->var->is_weak || var_needs_got(node->var))
                asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
            else
                asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // lea rip, rr
#endif
        }
        return r;
    }
    case ND_DEREF:
        return gen(node->lhs);
    case ND_REAL:
    case ND_IMAG: {
        int r = gen_addr(node->lhs);
        int offset = (node->kind == ND_IMAG) ? node->lhs->ty->base->size : 0;
        if (offset > 0) {
#ifdef ARCH_ARM64
            if (offset <= 4095)
                asm_add_imm(cg_sec, r, 8, offset); // add r, r, #offset
            else {
                VReg ti = alloc_reg();
                emit_mov_imm64(REG(ti), (uint64_t)offset); // mov ti, #offset
                asm_add_reg_reg(cg_sec, r, ti, 8); // add r, r, ti
                free_reg(ti);
            }
#else
            if (offset <= 4095)
                asm_add_imm(cg_sec, r, 8, offset); // add r, r, #offset
            else {
                VReg ti = alloc_reg();
                asm_mov_imm(cg_sec, ti, 8, offset); // mov $offset, ti
                asm_add_reg_reg(cg_sec, r, ti, 8); // add r, r, ti
                free_reg(ti);
            }
#endif
        }
        return r;
    }
    case ND_MEMBER: {
        VReg r = gen_addr(node->lhs);
        if (node->member->offset_expr) {
            VReg o = gen(node->member->offset_expr);
#ifdef ARCH_ARM64
            asm_add_reg_reg(cg_sec, r, o, 8); // add r, r, o
#else
            asm_add_reg_reg(cg_sec, r, o, 8); // add r, r, o
#endif
            free_reg(o);
            return r;
        }
#ifdef ARCH_ARM64
        if (node->member->offset > 0 && node->member->offset <= 4095)
            asm_add_imm(cg_sec, r, 8, node->member->offset); // add r, r, #node->member->offset
        else if (node->member->offset > 0) {
            VReg ti = alloc_reg();
            emit_mov_imm64(REG(ti), (uint64_t)node->member->offset); // mov ti, #node->member->offset
            asm_add_reg_reg(cg_sec, r, ti, 8); // add r, r, ti
            free_reg(ti);
        } else if (node->member->offset < 0 && -node->member->offset <= 4095)
            asm_sub_imm(cg_sec, r, 8, -node->member->offset); // sub r, r, #-node->member->offset
        else if (node->member->offset < 0) {
            VReg ti = alloc_reg();
            emit_mov_imm64(REG(ti), (uint64_t)(-node->member->offset)); // mov ti, #-node->member->offset
            asm_sub_reg_reg(cg_sec, r, ti, 8); // sub r, r, ti
            free_reg(ti);
        }
#else
        asm_add_imm(cg_sec, r, 8, node->member->offset); // add $node->member->offset, rr
#endif
        return r;
    }
    case ND_COND: {
        // Struct/union ternary lvalue: (cond ? a : b).member
        int c = ++rcc_label_count;
        VReg r = alloc_reg();
        VReg cond = gen(node->cond);
#ifdef ARCH_ARM64
        asm_cmp_zero(cg_sec, cond, node->cond->ty->size); // cmp $0, rcond
        {
            size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // jcc label
            asm_fixup_add(cg_sec, o, format(".L.else.%d", c), 1);
        }
        free_reg(cond);
        int then_r = gen_addr(node->then);
        asm_mov_reg_reg(cg_sec, r, then_r, 8); // mov rthen_r -> rr
        free_reg(then_r);
        {
            size_t o = asm_jmp_label(cg_sec); // b .L.end.%d
            asm_fixup_add(cg_sec, o, format(".L.end.%d", c), 0);
        }
        cg_def_label(format(".L.else.%d", c));
        VReg else_r = gen_addr(node->els);
        asm_mov_reg_reg(cg_sec, r, else_r, 8); // mov relse_r -> rr
        free_reg(else_r);
        cg_def_label(format(".L.end.%d", c));
#else
        asm_cmp_zero(cg_sec, cond, node->cond->ty->size); // cmp $0, rcond
        {
            size_t o = asm_jcc_label(cg_sec, X86_E); // jcc label
            asm_fixup_add(cg_sec, o, format(".L.else.%d", c), 1);
        }
        free_reg(cond);
        VReg then_r = gen_addr(node->then);
        asm_mov_reg_reg(cg_sec, r, then_r, 8); // mov rthen_r -> rr
        free_reg(then_r);
        {
            size_t o = asm_jmp_label(cg_sec); // je .L.else.%d
            asm_fixup_add(cg_sec, o, format(".L.end.%d", c), 0);
        }
        cg_def_label(format(".L.else.%d", c));
        VReg else_r = gen_addr(node->els);
        asm_mov_reg_reg(cg_sec, r, else_r, 8); // mov relse_r -> rr
        free_reg(else_r);
        cg_def_label(format(".L.end.%d", c));
#endif
        return r;
    }
    case ND_CAST:
        if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_COMPLEX || node->ty->kind == TY_ARRAY)) {
            // Complex -> complex cast with a differently-sized base type of
            // the same kind (e.g. _Complex char -> _Complex int, from the
            // usual arithmetic conversions). The source storage doesn't
            // match node->ty's size, so handing back gen_addr(node->lhs)'s
            // address as-is would make callers read/write past the smaller
            // object. Materialize a converted copy on the stack instead.
            if (node->ty->kind == TY_COMPLEX && node->lhs->ty && node->lhs->ty->kind == TY_COMPLEX &&
                is_flonum(node->lhs->ty->base) == is_flonum(node->ty->base) &&
                node->lhs->ty->base->size != node->ty->base->size) {
                VReg src = gen_addr(node->lhs);
                if (src < 0)
                    src = gen(node->lhs); // gen() for complex exprs also returns an address
                int alloc = (node->ty->size + 7) & ~7;
                fn_struct_ret_off += alloc;
                if (fn_struct_ret_off > fn_struct_ret_total) fn_struct_ret_total = fn_struct_ret_off;
                int result_off = current_fn_stack_size + fn_struct_ret_off;
                VReg dst = alloc_reg();
#ifdef ARCH_ARM64
                asm_sub_reg_fp_imm(cg_sec, dst, result_off); // sub dst, x29, #result_off
#else
                asm_lea_rbp_reg(cg_sec, dst, 8, result_off); // lea -result_off(%rbp), dst
#endif
                if (is_flonum(node->lhs->ty->base))
                    emit_complex_convert_float(src, dst, node->lhs->ty, node->ty);
                else
                    emit_complex_convert_int(src, dst, node->lhs->ty, node->ty);
                free_reg(src);
                return dst;
            }
            VReg r = gen_addr(node->lhs);
            if (r >= 0) return r;
            // Non-lvalue cast to complex/struct: materialize on stack
            if (node->ty->kind == TY_COMPLEX) {
                Type *from = node->lhs->ty;
                if (from && (is_flonum(from) || is_integer(from)))
                    // Scalar -> complex: delegate to gen(), which performs the
                    // proper int/float conversion to the complex base type and
                    // materializes the {real, imag} pair on the stack.
                    return gen(node);
                return -1;
            }
            return -1;
        }
        error_tok(node->tok, "lvalue required as left operand of assignment");
        return -1;
    case ND_COMMA: {
        // Compound literal: evaluate LHS side effects, return address of RHS
        VReg r1 = gen(node->lhs);
        if (r1 != -1) free_reg(r1);
        return gen_addr(node->rhs);
    }
    case ND_FUNCALL:
        if (node->ty && (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION || node->ty->kind == TY_COMPLEX))
            return gen_funcall(node, -1);
        error_tok(node->tok, "lvalue required as left operand of assignment");
        return -1;
    case ND_ASSIGN:
        // Assignment expression used as lvalue: return address of lhs
        return gen_addr(node->lhs);
    case ND_STMT_EXPR: {
        // Statement expression used as lvalue (e.g. d = ({ bar(); }))
        // Evaluate the block and return address of the last expression
        VReg result = -1;
        for (Node *n = node->body; n; n = n->next) {
            VReg r = gen(n);
            if (node->stmt_expr_result && n->kind == ND_EXPR_STMT && n->lhs == node->stmt_expr_result) {
                result = gen_addr(node->stmt_expr_result);
            }
            if (r != -1) free_reg(r);
        }
        if (result != -1)
            return result;
        error_tok(node->tok, "lvalue required as left operand of assignment");
        return -1;
    }
    case ND_NUM:
    case ND_FNUM:
        return -1; // not an lvalue; caller should handle via temp allocation
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_NEG:
        return -1; // expression result, not an lvalue
    default:
        error_tok(node->tok, "lvalue required as left operand of assignment");
        return -1;
    }
}

static void gen_cond_branch_inv(Node *cond, const char *label) {
    if (cond->kind == ND_LOGAND) {
        gen_cond_branch_inv(cond->lhs, label);
        gen_cond_branch_inv(cond->rhs, label);
        return;
    }
    if (cond->kind == ND_LOGOR) {
        int c = ++rcc_label_count;
        const char *skip_label = format(".L.or_skip.%d", c);
        // If lhs is true, skip to end (don't branch to label)
        VReg lhs = gen(cond->lhs);
        asm_cmp_zero(cg_sec, lhs, cond->lhs->ty->size); // cmp $0, rlhs
        free_reg(lhs);
#ifdef ARCH_ARM64
        size_t o = asm_jcc_label(cg_sec, ARM64_NE); // jcc label
        asm_fixup_add(cg_sec, o, skip_label, 1); // fcmp d0, d1
#else
        size_t o = asm_jcc_label(cg_sec, X86_NE); // jcc label
        asm_fixup_add(cg_sec, o, skip_label, 1); // fixup add for forward branch
#endif
        // If lhs was false, check rhs; if rhs is also false, branch to label
        gen_cond_branch_inv(cond->rhs, label);
        cg_def_label(skip_label); // b.eq %s
        return;
    }
    if (cond->kind == ND_EQ || cond->kind == ND_NE || cond->kind == ND_LT || cond->kind == ND_LE) {
        if (cond->lhs->ty && cond->lhs->ty->kind == TY_INT128) {
            // __int128 comparison: gen_int128 returns a boolean VReg (0 or 1)
            VReg r = gen_int128(cond);
            asm_cmp_zero(cg_sec, r, 4); // cmpl $0, rr
            free_reg(r);
#ifdef ARCH_ARM64
            size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // b.eq label
#else
            size_t o = asm_jcc_label(cg_sec, X86_E); // je label
#endif
            asm_fixup_add(cg_sec, o, label, 1);
            return;
        }
        if (cond->lhs->ty && is_complex(cond->lhs->ty)) {
            // Complex EQ/NE: compare both parts, branch if not equal
            int r = gen(cond); // use the expression handler which now handles complex
#ifdef ARCH_ARM64
            asm_cmp_zero(cg_sec, r, 4);
            {
                size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // b.eq label
                asm_fixup_add(cg_sec, o, label, 1);
            }
#else
            asm_cmp_zero(cg_sec, r, 4); // testl r, r
            {
                size_t o = asm_jcc_label(cg_sec, X86_E); // je label
                asm_fixup_add(cg_sec, o, label, 1);
            }
#endif
            free_reg(r);
            return;
        }
        if (is_flonum(cond->lhs->ty)) {
            VReg r_lhs = gen(cond->lhs);
            VReg r_rhs = gen(cond->rhs);
#ifdef ARCH_ARM64
            asm_fmov_i2f(cg_sec, 0, r_lhs, 1); // fmov d0, x{r_lhs}
            asm_fmov_i2f(cg_sec, 1, r_rhs, 1); // fmov d1, x{r_rhs}
            asm_fcmp(cg_sec, 1); // fcmp d0, d1
            if (cond->kind == ND_EQ) {
                {
                    size_t o = asm_jcc_label(cg_sec, ARM64_NE); // b.ne label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, ARM64_VS); // b.vs label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
            } else if (cond->kind == ND_NE) {
                int c = ++rcc_label_count;
                {
                    size_t _cj = asm_jcc_label(cg_sec, ARM64_VS);
                    asm_fixup_add(cg_sec, _cj, format(".L.fc.skip.%d", c), 1);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // b.eq label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
                cg_def_label(format(".L.fc.skip.%d", c));
            } else if (cond->kind == ND_LT) {
                size_t o = asm_jcc_label(cg_sec, ARM64_PL); // b.pl label
                asm_fixup_add(cg_sec, o, label, 1);
            } else if (cond->kind == ND_LE) {
                size_t o = asm_jcc_label(cg_sec, ARM64_HI); // b.hi label
                asm_fixup_add(cg_sec, o, label, 1);
            }
#else
            asm_movq_r_xmm(cg_sec, X86_XMM0, r_lhs); // movq X86_XMM0, r_lhs
            asm_movq_r_xmm(cg_sec, X86_XMM1, r_rhs); // movq X86_XMM1, r_rhs
            asm_ucomisd(cg_sec); // je %s
            if (cond->kind == ND_EQ) {
                {
                    size_t o = asm_jcc_label(cg_sec, X86_NE); // jne label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, X86_P); // jp label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
            } else if (cond->kind == ND_NE) {
                int c = ++rcc_label_count;
                {
                    size_t o = asm_jcc_label(cg_sec, X86_P); // jp .L.fc.skip.c
                    asm_fixup_add(cg_sec, o, format(".L.fc.skip.%d", c), 1);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, X86_E); // je label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
                cg_def_label(format(".L.fc.skip.%d", c));
            } else if (cond->kind == ND_LT) {
                {
                    size_t o = asm_jcc_label(cg_sec, X86_AE); // jae label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, X86_P); // jp label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
            } else if (cond->kind == ND_LE) {
                {
                    size_t o = asm_jcc_label(cg_sec, X86_A); // ja label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, X86_P); // jp label
                    asm_fixup_add(cg_sec, o, label, 1);
                }
            }
#endif
            free_reg(r_rhs);
            free_reg(r_lhs);
            return;
        }
        VReg r_lhs = gen(cond->lhs);
        int sz = op_size(cond->lhs->ty);
        if (sz < op_size(cond->rhs->ty))
            sz = op_size(cond->rhs->ty);
        if (cond->rhs->kind == ND_NUM && cond->rhs->val == (int32_t)cond->rhs->val) {
#ifdef ARCH_ARM64
            int32_t imm = (int32_t)cond->rhs->val;
            if (imm >= 0 && imm <= 4095) {
                asm_cmp_imm(cg_sec, r_lhs, sz, imm); // cmp $imm, rr_lhs
            } else if (imm < 0 && imm >= -4095) {
                asm_cmn_vreg_imm(cg_sec, r_lhs, sz, -imm); // cmn x{r_lhs}, #(-imm)
            } else {
                emit_mov_imm64(ARM64_X16, (uint64_t)(int64_t)imm);
                asm_cmp_reg_phy(cg_sec, r_lhs, ARM64_X16, sz); // cmp r16, rr_lhs
            }
#else
            asm_cmp_imm(cg_sec, r_lhs, sz, (int32_t)cond->rhs->val); // cmp $(int32_t)cond->rhs->val, rr_lhs
#endif
        } else {
            VReg r_rhs = gen(cond->rhs);
            asm_cmp_reg_reg(cg_sec, r_lhs, r_rhs, sz); // cmp rr_rhs, rr_lhs
            free_reg(r_rhs);
        }
        free_reg(r_lhs);

        (void)cond; // used in branch emission below
#ifndef ARCH_ARM64
        __attribute__((unused)) char *jmp = "";
        if (cond->kind == ND_EQ)
            jmp = "jne";
        else if (cond->kind == ND_NE)
            jmp = "je";
        else if (cond->kind == ND_LT)
            jmp = use_unsigned_cmp(cond) ? "jae" : "jge";
        else if (cond->kind == ND_LE)
            jmp = use_unsigned_cmp(cond) ? "ja" : "jg";
#endif

        // Emit conditional branch
#ifdef ARCH_ARM64
        if (cond->kind == ND_EQ) {
            size_t o = asm_jcc_label(cg_sec, ARM64_NE); // jcc label
            asm_fixup_add(cg_sec, o, label, 1); // fixup add for forward branch
        } else if (cond->kind == ND_NE) {
            size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // jcc label
            asm_fixup_add(cg_sec, o, label, 1); // fixup add for forward branch
        } else if (cond->kind == ND_LT) {
            size_t o = asm_jcc_label(cg_sec, use_unsigned_cmp(cond) ? ARM64_HS : ARM64_GE); // jcc label
            asm_fixup_add(cg_sec, o, label, 1); // fixup add for forward branch
        } else if (cond->kind == ND_LE) {
            size_t o = asm_jcc_label(cg_sec, use_unsigned_cmp(cond) ? ARM64_HI : ARM64_GT); // jcc label
            asm_fixup_add(cg_sec, o, label, 1); // fixup add for forward branch
        }
#else
        if (cond->kind == ND_EQ) {
            size_t o = asm_jcc_label(cg_sec, X86_NE); // jcc label
            asm_fixup_add(cg_sec, o, label, 1); // fixup add for forward branch
        } else if (cond->kind == ND_NE) {
            size_t o = asm_jcc_label(cg_sec, X86_E);
            asm_fixup_add(cg_sec, o, label, 1);
        } else if (cond->kind == ND_LT) {
            size_t o = asm_jcc_label(cg_sec, use_unsigned_cmp(cond) ? X86_AE : X86_GE);
            asm_fixup_add(cg_sec, o, label, 1);
        } else if (cond->kind == ND_LE) {
            size_t o = asm_jcc_label(cg_sec, use_unsigned_cmp(cond) ? X86_A : X86_G);
            asm_fixup_add(cg_sec, o, label, 1);
        }
#endif
        return;
    }

    VReg r = gen(cond);
    int sz = cond->ty->size > 8 ? 8 : (cond->ty->size > 0 ? cond->ty->size : 4);
#ifdef ARCH_ARM64
    asm_cmp_zero(cg_sec, r, sz);
    free_reg(r);
    size_t cond_off = asm_jcc_label(cg_sec, ARM64_EQ);
    asm_fixup_add(cg_sec, cond_off, label, 1);
#else
    asm_cmp_zero(cg_sec, r, sz);
    free_reg(r);
    size_t cond_off = asm_jcc_label(cg_sec, X86_E);
    asm_fixup_add(cg_sec, cond_off, label, 1);
#endif
}

// ===== __int128 support =====
// Convention: all TY_INT128 values live in a 16-byte stack slot.
// gen_int128(node) returns a GP register holding the slot address.
// lo word is at [addr+0], hi word at [addr+8].

// Allocate a fresh 16-byte-aligned int128 scratch slot in the struct-ret area.
static int alloc_int128_slot(void) {
    fn_struct_ret_off = (fn_struct_ret_off + 15) & ~15;
    fn_struct_ret_off += 16;
    if (fn_struct_ret_off > fn_struct_ret_total)
        fn_struct_ret_total = fn_struct_ret_off;
    return current_fn_stack_size + fn_struct_ret_off;
}

// Return a GP register holding the address of a fresh 16-byte scratch slot.
static VReg alloc_int128_addr(void) {
    int slot = alloc_int128_slot();
    VReg r = alloc_reg();
#ifdef ARCH_ARM64
    if (slot <= 4095) {
        asm_sub_reg_fp_imm(cg_sec, r, slot); // sub r, x29, #slot
    } else {
        int v = slot;
        asm_mov_imm(cg_sec, r, 8, v & 0xffff);
        v >>= 16;
        int s = 16;
        while (v) {
            asm_movk(cg_sec, REG(r), 1, (uint16_t)(v & 0xffff), s);
            v >>= 16;
            s += 16;
        }
        asm_sub_reg_fp_reg(cg_sec, r, r, 8); // sub ta, x29, ta
    }
#else
    asm_lea_rbp_reg(cg_sec, r, 8, slot);
#endif
    return r;
}

// Widen val (64-bit GP register) to a fresh 128-bit slot; frees val.
static VReg widen_to_int128(VReg val, bool is_unsigned) {
    VReg addr = alloc_int128_addr();
#ifdef ARCH_ARM64
    asm_str_reg_off(cg_sec, val, addr, 8, 0);
    if (is_unsigned) {
        asm_str_xzr_uoff(cg_sec, REG(addr), 1); // str xzr, [%s, #8]
    } else {
        asm_asr_x16_src_63(cg_sec, val); // asr x16, %s, #63
        asm_str_x16_uoff(cg_sec, addr, 1); // str x16, [%s, #8]
    }
#else
    asm_mov_reg_to_retval(cg_sec, val, 8); // movq val, %rax
    asm_mov_rax_mem(cg_sec, addr); // movq %rax, (addr)
    if (is_unsigned) {
        asm_movq_zero_mem8(cg_sec, addr); // movq $0, 8(addr)
    } else {
        asm_mov_reg_to_retval(cg_sec, val, 8); // movq val, %rax
        asm_shift_rax_imm(cg_sec, false, 63); // sarq $63, %rax
        asm_movq_rax_mem8(cg_sec, addr); // movq %rax, 8(addr)
    }
#endif
    free_reg(val);
    return addr;
}

// Ensure operand is available as an int128 slot address.
static VReg gen_to_int128(Node *operand) {
    if (operand->ty && operand->ty->kind == TY_INT128)
        return gen_int128(operand);
    int val = gen(operand);
    Type *ty = operand->ty ? operand->ty : ty_llong;
    // Extend to 64-bit first if narrower
    if (ty->size < 8) {
        if (ty->is_unsigned)
            zero_extend_to(val, ty->size, 8);
        else
            sign_extend_to(val, ty->size, 8);
    }
    return widen_to_int128(val, ty->is_unsigned);
}


static VReg gen_int128(Node *node) {
    if (!node) return R_NONE;
    if (opt_g) emit_loc(node);

    switch (node->kind) {

    case ND_NUM: {
        long long lo = node->val;
        long long hi = (lo < 0 && !(node->ty && node->ty->is_unsigned)) ? -1LL : 0LL;
        int addr = alloc_int128_addr();
#ifdef ARCH_ARM64
        int t = alloc_reg();
        if (lo >= -65536 && lo <= 65535) {
            asm_mov_imm(cg_sec, t, 8, lo); // mov %s, #%lld
        } else {
            uint64_t v = (uint64_t)lo;
            asm_mov_imm(cg_sec, t, 8, v & 0xffff);
            v >>= 16;
            int s = 16;
            while (v) {
                asm_movk(cg_sec, t, 1, (uint16_t)(v & 0xffff), s);
                v >>= 16;
                s += 16;
            }
        }
        asm_str_reg_off(cg_sec, t, addr, 8, 0);
        if (hi == 0) {
            asm_str_xzr_uoff(cg_sec, REG(addr), 1); // str xzr, [%s, #8]
        } else {
            asm_mov_imm(cg_sec, t, 8, -1); // mov %s, #-1
            asm_str_reg_off(cg_sec, t, addr, 8, 8); // store hi to [%s, #8]
        }
        free_reg(t);
#else
        if (lo == (int32_t)lo) {
            X86Mem m = {REG(addr), X86_NOREG, 1, 0};
            x86_mov_mi(cg_sec, 8, m, lo); // movq $%lld, (%s)
        } else {
            asm_movabs_phy(cg_sec, X86_RAX, lo); // movabsq $%lld, %%rax
            asm_mov_rax_mem(cg_sec, addr); // movq %%rax, (%s)
        }
        if (hi == 0) {
            asm_movq_zero_mem8(cg_sec, addr); // movq $0, 8(%s)
        } else {
            asm_movabs_phy(cg_sec, X86_RAX, hi); // movabsq $%lld, %%rax
            asm_movq_rax_mem8(cg_sec, addr); // movq %%rax, 8(%s)
        }
#endif
        return addr;
    }

    case ND_LVAR:
    case ND_MEMBER:
        return gen_addr(node);

    case ND_DEREF:
        // *ptr where ptr → 128-bit value: the pointer value IS the address
        return gen(node->lhs);

    case ND_ADDR:
        // & of something: gen_addr gives the address (result is pointer, not int128)
        return gen_addr(node->lhs);

    case ND_COMMA:
    case ND_CHAIN: {
        int r = gen(node->lhs);
        if (r >= 0) free_reg(r);
        return gen_int128(node->rhs);
    }

    case ND_ASSIGN: {
#ifdef ARCH_ARM64
        int rhs_a = gen_to_int128(node->rhs);
        int lhs_a = gen_addr(node->lhs);
        int t1 = alloc_reg(), t2 = alloc_reg();
        asm_ldr_reg_off(cg_sec, t1, rhs_a, 8, 0);
        asm_ldr_reg_off(cg_sec, t2, rhs_a, 8, 8); // ldr %s, [%s, #8]
        asm_str_reg_off(cg_sec, t1, lhs_a, 8, 0);
        asm_str_reg_off(cg_sec, t2, lhs_a, 8, 8); // str %s, [%s, #8]
        free_reg(t1);
        free_reg(t2);
#else
        // Compute rhs first to avoid volatile-register clobber on Windows:
        // __udivti3 etc. use r10 internally, which may hold lhs_a if evaluated first.
        int rhs_a = gen_to_int128(node->rhs);
        VReg lhs_a = gen_addr(node->lhs);
        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(rhs_a), 0)); // movq (%rs), %rax
        x86_mov_rm(cg_sec, 8, X86_RDX, x86_mem(REG(rhs_a), 8)); // movq 8(%rs), %rdx
        x86_mov_mr(cg_sec, 8, x86_mem(REG(lhs_a), 0), X86_RAX); // movq %rax, (%rl)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(lhs_a), 8), X86_RDX); // movq %rdx, 8(%rl)
#endif
        free_reg(rhs_a);
        return lhs_a;
    }

    case ND_CAST: {
        Type *from = node->lhs->ty;
        Type *to = node->ty;
        // from int128 to int128 (e.g. signed ↔ unsigned, or same)
        if (from && from->kind == TY_INT128)
            return gen_int128(node->lhs);
        // from float to int128
        if (from && is_flonum(from)) {
            int r = gen(node->lhs);
            int addr = alloc_int128_addr();
#ifdef ARCH_ARM64
            asm_fmov_d0_x(cg_sec, r); // fmov d0, x{r}
            free_reg(r);
            if (to->is_unsigned)
                asm_fcvtzu_x16(cg_sec); // fcvtzu x16, d0
            else
                asm_fcvtzs_x16(cg_sec); // fcvtzs x16, d0
            asm_str_x16_uoff(cg_sec, addr, 0); // str x16, [%s]
            if (to->is_unsigned) {
                asm_str_xzr_uoff(cg_sec, REG(addr), 1); // str xzr, [%s, #8]
            } else {
                asm_asr_x16_63(cg_sec); // asr x16, x16, #63
                asm_str_x16_uoff(cg_sec, addr, 1); // str x16, [%s, #8]
            }
#else
            (void)0 /* TODO: movq to/from xmm */;
            free_reg(r);
            if (to->is_unsigned) {
                // Unsigned float→int128: handle values >= 2^63 via the 2-step trick
                int c = ++rcc_label_count;
                asm_movabs_phy(cg_sec, X86_RAX, 0x43e0000000000000LL); // movq $0x43e0000000000000, %%rax (2^63 as double)
                x86_movq_r_xmm(cg_sec, X86_XMM1, X86_RAX); // movq %%rax, %%xmm1
                x86_comisd(cg_sec, X86_XMM0, X86_XMM1); // comisd %%xmm1, %%xmm0
                emit_jcc_fixup(cg_sec, X86_B, format(".L.f2i128u.%d", c)); // jb .L.f2i128u.%d
                x86_subsd(cg_sec, X86_XMM0, X86_XMM1); // subsd %%xmm1, %%xmm0
                x86_cvttsd2si(cg_sec, 8, X86_RAX, X86_XMM0); // cvttsd2si %%xmm0, %%rax
                asm_movabs_phy(cg_sec, X86_RDX, 0x8000000000000000LL); // movq $0x8000000000000000, %%rdx
                x86_xor_rr(cg_sec, 8, X86_RAX, X86_RDX); // xorq %%rdx, %%rax (add 2^63 back)
                emit_jmp_fixup(cg_sec, format(".L.f2i128u_lo.%d", c)); // jmp .L.f2i128u_lo.%d
                cg_def_label(format(".L.f2i128u.%d", c)); // .L.xxx.c
                x86_cvttsd2si(cg_sec, 8, X86_RAX, X86_XMM0); // cvttsd2si %%xmm0, %%rax
                cg_def_label(format(".L.f2i128u_lo.%d", c)); // .L.xxx.c
                asm_mov_rax_mem(cg_sec, addr); // movq %%rax, (%s)
                asm_movq_zero_mem8(cg_sec, addr); // movq $0, 8(%s)
            } else {
                x86_cvttsd2si(cg_sec, 8, X86_RAX, X86_XMM0); // cvttsd2si %%xmm0, %%rax
                asm_mov_rax_mem(cg_sec, addr); // movq %%rax, (%s)
                asm_shift_rax_imm(cg_sec, false, 63); // sarq $63, %%rax
                asm_movq_rax_mem8(cg_sec, addr); // movq %%rax, 8(%s)
            }
#endif
            return addr;
        }
        // from smaller integer to int128
        int val = gen(node->lhs);
        if (from && from->size < 8) {
            if (from->is_unsigned)
                zero_extend_to(val, from->size, 8);
            else
                sign_extend_to(val, from->size, 8);
        }
        return widen_to_int128(val, from ? from->is_unsigned : false);
    }

    case ND_NEG: {
        int src = gen_to_int128(node->lhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        int t = alloc_reg();
        asm_ldr_reg_off(cg_sec, t, src, 8, 0);
        asm_negs(cg_sec, t, t); // negs %s, %s
        asm_str_reg_off(cg_sec, t, dst, 8, 0);
        asm_ldr_reg_off(cg_sec, t, src, 8, 8);
        asm_ngc(cg_sec, t, t); // ngc %s, %s
        asm_str_reg_off(cg_sec, t, dst, 8, 8);
        free_reg(t);
#else
        asm_mov_mem_rax(cg_sec, src);
        asm_mov_mem8_rdx(cg_sec, src);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
        asm_negq_mem(cg_sec, dst); // negq (%s)
        asm_adcq_mem8_0(cg_sec, dst); // adcq $0, 8(%s)
        asm_negq_mem8(cg_sec, dst); // negq 8(%s)
#endif
        free_reg(src);
        return dst;
    }

    case ND_BITNOT: {
        int src = gen_to_int128(node->lhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        int t = alloc_reg();
        asm_ldr_reg_off(cg_sec, t, src, 8, 0);
        asm_not(cg_sec, t, 8);
        asm_str_reg_off(cg_sec, t, dst, 8, 0);
        asm_ldr_reg_off(cg_sec, t, src, 8, 8);
        asm_not(cg_sec, t, 8);
        asm_str_reg_off(cg_sec, t, dst, 8, 8);
        free_reg(t);
#else
        asm_mov_mem_rax(cg_sec, src);
        asm_mov_mem8_rdx(cg_sec, src);
        asm_notq_rax(cg_sec);
        asm_notq_rdx(cg_sec);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
#endif
        free_reg(src);
        return dst;
    }

    case ND_ADD: {
        int lhs = gen_to_int128(node->lhs);
        int rhs = gen_to_int128(node->rhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        int t1 = alloc_reg(), t2 = alloc_reg(), t3 = alloc_reg(), t4 = alloc_reg();
        asm_ldr_reg_off(cg_sec, t1, lhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t2, lhs, 8, 8);
        asm_ldr_reg_off(cg_sec, t3, rhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t4, rhs, 8, 8);
        asm_adds_phy(cg_sec, t1, t1, REG(t3));
        asm_adc_phy(cg_sec, t2, t2, REG(t4)); // adc %s, %s, %s
        asm_str_reg_off(cg_sec, t1, dst, 8, 0);
        asm_str_reg_off(cg_sec, t2, dst, 8, 8);
        free_reg(t1);
        free_reg(t2);
        free_reg(t3);
        free_reg(t4);
#else
        asm_mov_mem_rax(cg_sec, lhs);
        asm_mov_mem8_rdx(cg_sec, lhs);
        asm_addq_mem_rax(cg_sec, rhs);
        asm_adcq_mem8_rdx(cg_sec, rhs);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
#endif
        free_reg(lhs);
        free_reg(rhs);
        return dst;
    }

    case ND_SUB: {
        int lhs = gen_to_int128(node->lhs);
        int rhs = gen_to_int128(node->rhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        int t1 = alloc_reg(), t2 = alloc_reg(), t3 = alloc_reg(), t4 = alloc_reg();
        asm_ldr_reg_off(cg_sec, t1, lhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t2, lhs, 8, 8);
        asm_ldr_reg_off(cg_sec, t3, rhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t4, rhs, 8, 8);
        asm_subs_phy(cg_sec, t1, t1, REG(t3));
        asm_sbc_phy(cg_sec, t2, t2, REG(t4)); // sbc %s, %s, %s
        asm_str_reg_off(cg_sec, t1, dst, 8, 0);
        asm_str_reg_off(cg_sec, t2, dst, 8, 8);
        free_reg(t1);
        free_reg(t2);
        free_reg(t3);
        free_reg(t4);
#else
        asm_mov_mem_rax(cg_sec, lhs);
        asm_mov_mem8_rdx(cg_sec, lhs);
        asm_op_mem_rax(cg_sec, "subq", rhs);
        asm_sbbq_mem8_rdx(cg_sec, rhs);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
#endif
        free_reg(lhs);
        free_reg(rhs);
        return dst;
    }

    case ND_MUL: {
        int lhs = gen_to_int128(node->lhs);
        int rhs = gen_to_int128(node->rhs);
#ifdef ARCH_ARM64
        int dst = alloc_int128_addr();
        // (alo:ahi) * (blo:bhi): result_lo = alo*blo.lo; result_hi = alo*blo.hi + ahi*blo.lo + alo*bhi.lo
        int a_lo = alloc_reg(), a_hi = alloc_reg(), b_lo = alloc_reg(), b_hi = alloc_reg();
        asm_ldr_reg_off(cg_sec, a_lo, lhs, 8, 0);
        asm_ldr_reg_off(cg_sec, a_hi, lhs, 8, 8);
        asm_ldr_reg_off(cg_sec, b_lo, rhs, 8, 0);
        asm_ldr_reg_off(cg_sec, b_hi, rhs, 8, 8);
        int r_lo = alloc_reg(), r_hi = alloc_reg(), tmp = alloc_reg();
        asm_mul_rd_rn_rm(cg_sec, r_lo, a_lo, b_lo, 8); // mul r_lo, a_lo, b_lo
        asm_umulh(cg_sec, r_hi, a_lo, b_lo); // umulh r_hi, a_lo, b_lo
        asm_mul_rd_rn_rm(cg_sec, tmp, a_hi, b_lo, 8); // mul tmp, a_hi, b_lo
        asm_add_rd_rn_rm(cg_sec, r_hi, r_hi, tmp, 8); // add r_hi, r_hi, tmp
        asm_mul_rd_rn_rm(cg_sec, tmp, a_lo, b_hi, 8); // mul tmp, a_lo, b_hi
        asm_add_rd_rn_rm(cg_sec, r_hi, r_hi, tmp, 8); // add r_hi, r_hi, tmp
        free_reg(tmp);
        free_reg(a_lo);
        free_reg(a_hi);
        free_reg(b_lo);
        free_reg(b_hi);
        asm_str_reg_off(cg_sec, r_lo, dst, 8, 0);
        asm_str_reg_off(cg_sec, r_hi, dst, 8, 8);
        free_reg(r_lo);
        free_reg(r_hi);
#else
        // Load all values first using alloc_reg(), then release lhs/rhs, then allocate dst.
        // This avoids using hardcoded registers that might alias lhs/rhs address regs.
        int a_lo = alloc_reg(), a_hi = alloc_reg(), b_lo = alloc_reg(), b_hi = alloc_reg();
        asm_ldr_reg_off(cg_sec, a_lo, lhs, 8, 0);
        asm_ldr_reg_off(cg_sec, a_hi, lhs, 8, 8);
        asm_ldr_reg_off(cg_sec, b_lo, rhs, 8, 0);
        asm_ldr_reg_off(cg_sec, b_hi, rhs, 8, 8);
        free_reg(lhs);
        free_reg(rhs);
        int dst = alloc_int128_addr();
        asm_mov_reg_to_retval(cg_sec, a_lo, 8);
        asm_mulq(cg_sec, b_lo);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
        asm_imulq_reg_reg(cg_sec, a_hi, b_lo);
        asm_add_reg_mem8(cg_sec, dst, a_hi);
        asm_imulq_reg_reg(cg_sec, a_lo, b_hi);
        asm_add_reg_mem8(cg_sec, dst, a_lo);
        free_reg(a_lo);
        free_reg(a_hi);
        free_reg(b_lo);
        free_reg(b_hi);
#endif
        // lhs and rhs freed inside the #else block above (x86-64) or after ARM64 block
#ifdef ARCH_ARM64
        free_reg(lhs);
        free_reg(rhs);
#endif
        return dst;
    }

    case ND_DIV:
    case ND_MOD: {
        bool is_mod = (node->kind == ND_MOD);
        bool is_unsigned = node->ty && node->ty->is_unsigned;
        char *fn = (char *)(is_unsigned ? (is_mod ? "__umodti3" : "__udivti3")
                                        : (is_mod ? "__modti3" : "__divti3"));
        (void)fn; // used in x86 path
        int lhs = gen_to_int128(node->lhs);
        int rhs = gen_to_int128(node->rhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        // Pass args in x0-x3: x0=lhs.lo, x1=lhs.hi, x2=rhs.lo, x3=rhs.hi
        arm64_ldr_uoff(cg_sec, 3, ARM64_X0, REG(lhs), 0); // ldr x0, [lhs]
        arm64_ldr_uoff(cg_sec, 3, ARM64_X1, REG(lhs), 1); // ldr x1, [lhs, #8]
        arm64_ldr_uoff(cg_sec, 3, ARM64_X2, REG(rhs), 0); // ldr x2, [rhs]
        arm64_ldr_uoff(cg_sec, 3, ARM64_X3, REG(rhs), 1); // ldr x3, [rhs, #8]
        arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, 32, 0); // sub sp, sp, #32
        arm64_str_uoff(cg_sec, 3, REG(dst), ARM64_SP, 2); // str dst, [sp, #16]
        emit_direct_call(fn); // bl fn
        arm64_ldr_uoff(cg_sec, 3, ARM64_X9, ARM64_SP, 2); // ldr x9, [sp, #16]
        arm64_str_uoff(cg_sec, 3, ARM64_X0, ARM64_X9, 0); // str x0, [x9]
        arm64_str_uoff(cg_sec, 3, ARM64_X1, ARM64_X9, 1); // str x1, [x9, #8]
        arm64_ldr_uoff(cg_sec, 3, REG(dst), ARM64_SP, 2); // ldr dst, [sp, #16]
        arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, 32, 0); // add sp, sp, #32
#else
#ifdef _WIN32
        // Windows x64: __udivti3 etc. take &a in rcx, &b in rdx, return in xmm0
        x86_mov_rr(cg_sec, 8, X86_RCX, REG(lhs)); // movq lhs, %rcx
        x86_mov_rr(cg_sec, 8, X86_RDX, REG(rhs)); // movq rhs, %rdx
        x86_sub_ri(cg_sec, 8, X86_RSP, 32); // subq $32, %rsp
        emit_direct_call(fn);
        x86_add_ri(cg_sec, 8, X86_RSP, 32); // addq $32, %rsp
        x86_movdqu_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0); // movdqu %xmm0, (dst)
#else
        x86_mov_rm(cg_sec, 8, X86_RDI, x86_mem(REG(lhs), 0)); // movq (lhs), %rdi
        x86_mov_rm(cg_sec, 8, X86_RSI, x86_mem(REG(lhs), 8)); // movq 8(lhs), %rsi
        x86_mov_rm(cg_sec, 8, X86_RDX, x86_mem(REG(rhs), 0)); // movq (rhs), %rdx
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(rhs), 8)); // movq 8(rhs), %rcx
        emit_direct_call(fn);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
#endif
#endif
        free_reg(lhs);
        free_reg(rhs);
        return dst;
    }

    case ND_SHL:
    case ND_SHR: {
        bool is_shl = (node->kind == ND_SHL);
        bool is_unsigned = node->ty && node->ty->is_unsigned;
        int lhs = gen_to_int128(node->lhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        bool is_rhs_const = (node->rhs->kind == ND_NUM);
        if (is_rhs_const) {
            int shift = (int)(node->rhs->val & 127);
            int t1 = alloc_reg(), t2 = alloc_reg();
            asm_ldr_reg_off(cg_sec, t1, lhs, 8, 0);
            asm_ldr_reg_off(cg_sec, t2, lhs, 8, 8);
            free_reg(lhs);
            if (shift == 0) {
                // no-op
            } else if (is_shl) {
                if (shift < 64) {
                    // hi = (hi<<shift) | (lo>>(64-shift)), lo = lo<<shift
                    arm64_lsl_imm(cg_sec, 1, ARM64_X16, REG(t2), shift);
                    arm64_lsr_imm(cg_sec, 1, ARM64_X17, REG(t1), 64 - shift);
                    arm64_orr_reg(cg_sec, 1, REG(t2), ARM64_X16, ARM64_X17, ARM64_LSL, 0); // orr t2, x16, x17
                    asm_shl_imm(cg_sec, t1, 8, (uint8_t)shift); // shl t1, #shift
                } else if (shift == 64) {
                    asm_mov_reg_reg(cg_sec, t2, t1, 8);
                    asm_mov_xzr_phy(cg_sec, REG(t1)); // mov %s, xzr
                } else {
                    arm64_lsl_imm(cg_sec, 1, REG(t2), REG(t1), shift - 64); // lsl t2, t1, #(shift-64)
                    asm_mov_xzr_phy(cg_sec, REG(t1)); // mov %s, xzr
                }
            } else {
                if (shift < 64) {
                    // lo = (lo>>shift) | (hi<<(64-shift)), hi >>= shift
                    arm64_lsr_imm(cg_sec, 1, ARM64_X16, REG(t1), shift);
                    arm64_lsl_imm(cg_sec, 1, ARM64_X17, REG(t2), 64 - shift);
                    arm64_orr_reg(cg_sec, 1, REG(t1), ARM64_X16, ARM64_X17, ARM64_LSL, 0); // orr t1, x16, x17
                    asm_shift_imm(cg_sec, t2, 8, is_unsigned, shift); // lsr/asr t2, t2, #shift
                } else if (shift == 64) {
                    asm_mov_reg_reg(cg_sec, t1, t2, 8);
                    if (is_unsigned)
                        asm_mov_xzr_phy(cg_sec, REG(t2)); // mov t2, xzr
                    else
                        asm_sar_imm(cg_sec, t2, 8, 63);
                } else {
                    if (is_unsigned)
                        arm64_lsr_imm(cg_sec, 1, REG(t1), REG(t2), shift - 64); // lsr t1, t2, #(shift-64)
                    else
                        arm64_asr_imm(cg_sec, 1, REG(t1), REG(t2), shift - 64); // asr t1, t2, #(shift-64)
                    if (is_unsigned)
                        asm_mov_xzr_phy(cg_sec, REG(t2)); // mov t2, xzr
                    else
                        asm_sar_imm(cg_sec, t2, 8, 63);
                }
            }
            asm_str_reg_off(cg_sec, t1, dst, 8, 0);
            asm_str_reg_off(cg_sec, t2, dst, 8, 8);
            free_reg(t1);
            free_reg(t2);
        } else {
            // Variable shift: call libgcc
            char *fn = is_shl ? "__ashlti3" : (is_unsigned ? "__lshrti3" : "__ashrti3");
            int r_shift = gen(node->rhs);
            arm64_ldr_uoff(cg_sec, 3, ARM64_X0, REG(lhs), 0); // ldr x0, [lhs]
            arm64_ldr_uoff(cg_sec, 3, ARM64_X1, REG(lhs), 1); // ldr x1, [lhs, #8]
            arm64_orr_reg(cg_sec, 1, ARM64_X2, ARM64_XZR, REG(r_shift), ARM64_LSL, 0); // mov x2, r_shift
            free_reg(r_shift);
            free_reg(lhs);
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, 32, 0); // sub sp, sp, #32
            arm64_str_uoff(cg_sec, 3, REG(dst), ARM64_SP, 2); // str dst, [sp, #16]
            emit_direct_call(fn); // bl fn
            arm64_ldr_uoff(cg_sec, 3, ARM64_X9, ARM64_SP, 2); // ldr x9, [sp, #16]
            arm64_str_uoff(cg_sec, 3, ARM64_X0, ARM64_X9, 0); // str x0, [x9]
            arm64_str_uoff(cg_sec, 3, ARM64_X1, ARM64_X9, 1); // str x1, [x9, #8]
            arm64_ldr_uoff(cg_sec, 3, REG(dst), ARM64_SP, 2); // ldr dst, [sp, #16]
            arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, 32, 0); // add sp, sp, #32
        }
#else
        // x86-64: rax=lo, rdx=hi; use shldq/shrdq
        asm_mov_mem_rax(cg_sec, lhs); // movq (lhs), %%rax
        asm_mov_mem8_rdx(cg_sec, lhs); // movq 8(lhs), %%rdx
        free_reg(lhs);
        if (node->rhs->kind == ND_NUM) {
            int shift = (int)node->rhs->val;
            if (shift == 0) {
                // no-op
            } else if (is_shl) {
                if (shift < 64) {
                    asm_shldq_imm(cg_sec, shift); // shldq $%d, %%rax, %%rdx
                    asm_shl_rax_imm(cg_sec, shift); // shlq $%d, %%rax
                } else if (shift == 64) {
                    asm_mov_rax_rdx(cg_sec); // movq %%rax, %%rdx
                    asm_xor_rax_rax(cg_sec); // xorq %%rax, %%rax
                } else if (shift < 128) {
                    asm_shl_rax_imm(cg_sec, shift - 64); // shlq $%d, %%rax
                    asm_mov_rax_rdx(cg_sec); // movq %%rax, %%rdx
                    asm_xor_rax_rax(cg_sec); // xorq %%rax, %%rax
                } else {
                    asm_xor_rax_rax(cg_sec); // xorq %%rax, %%rax
                    asm_xor_rdx_rdx(cg_sec); // xorq %%rdx, %%rdx
                }
            } else {
                if (shift < 64) {
                    asm_shrdq_imm(cg_sec, shift); // shrdq $%d, %%rdx, %%rax
                    asm_shift_rdx_imm(cg_sec, is_unsigned, shift); // %s $%d, %%rdx
                } else if (shift == 64) {
                    asm_mov_rdx_rax(cg_sec); // movq %%rdx, %%rax
                    if (is_unsigned)
                        asm_xor_rdx_rdx(cg_sec); // xorq %%rdx, %%rdx
                    else
                        asm_shift_rdx_imm(cg_sec, false, 63); // sarq $63, %%rdx
                } else if (shift < 128) {
                    asm_mov_rdx_rax(cg_sec); // movq %%rdx, %%rax
                    asm_shift_rax_imm(cg_sec, is_unsigned, shift - 64); // %s $%d, %%rax
                    if (is_unsigned)
                        asm_xor_rdx_rdx(cg_sec); // xorq %%rdx, %%rdx
                    else
                        asm_shift_rax_imm(cg_sec, false, 63); // sarq $63, %%rax
                } else {
                    if (is_unsigned) {
                        asm_xor_rax_rax(cg_sec); // xorq %%rax, %%rax
                        asm_xor_rdx_rdx(cg_sec); // xorq %%rdx, %%rdx
                    } else {
                        asm_shift_rax_imm(cg_sec, false, 63); // sarq $63, %%rax
                        asm_mov_rax_rdx(cg_sec); // movq %%rax, %%rdx
                    }
                }
            }
        } else {
            int r_shift = gen(node->rhs);
            asm_mov_reg_ecx(cg_sec, r_shift); // movl %r_shift, %ecx
            free_reg(r_shift);
            if (is_shl) {
                asm_shldq_cl(cg_sec); // shldq %%cl, %%rax, %%rdx
                asm_shl_rax_cl(cg_sec); // shlq %%cl, %%rax
                // shld/shl only use count mod 64. For count >= 64:
                // rdx gets the shifted lo, rax becomes 0.
                x86_test_ri(cg_sec, 4, X86_RCX, 0x40); // testl $0x40, %ecx
                size_t o = asm_jcc_label(cg_sec, X86_Z);
                int c = ++rcc_label_count;
                asm_fixup_add(cg_sec, o, format(".L.shl128_%d", c), 1);
                asm_mov_rax_rdx(cg_sec); // movq %%rax, %%rdx
                asm_xor_rax_rax(cg_sec); // xorq %%rax, %%rax
                cg_def_label(format(".L.shl128_%d", c));
            } else {
                asm_shrdq_cl(cg_sec); // shrdq %%cl, %%rdx, %%rax
                asm_shift_rdx_cl(cg_sec, is_unsigned); // %s %%cl, %%rdx
                int c = ++rcc_label_count;
                x86_test_ri(cg_sec, 4, X86_RCX, 0x40); // testl $0x40, %ecx
                size_t o = asm_jcc_label(cg_sec, X86_Z);
                asm_fixup_add(cg_sec, o, format(".L.shr128_%d", c), 1);
                asm_mov_rdx_rax(cg_sec); // movq %%rdx, %%rax
                if (is_unsigned)
                    asm_xor_rdx_rdx(cg_sec); // xorq %%rdx, %%rdx
                else
                    asm_shift_rdx_imm(cg_sec, false, 63); // sarq $63, %%rdx
                cg_def_label(format(".L.shr128_%d", c));
            }
        }
        asm_mov_rax_mem(cg_sec, dst); // movq %%rax, (dst)
        asm_mov_rdx_mem8(cg_sec, dst); // movq %%rdx, 8(dst)
#endif
        return dst;
    }

    case ND_BITAND:
    case ND_BITOR:
    case ND_BITXOR: {
        int lhs = gen_to_int128(node->lhs);
        int rhs = gen_to_int128(node->rhs);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        int t1 = alloc_reg(), t2 = alloc_reg(), t3 = alloc_reg(), t4 = alloc_reg();
        asm_ldr_reg_off(cg_sec, t1, lhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t2, lhs, 8, 8);
        asm_ldr_reg_off(cg_sec, t3, rhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t4, rhs, 8, 8);
        if (node->kind == ND_BITAND) {
            arm64_and_reg(cg_sec, 1, REG(t1), REG(t1), REG(t3), ARM64_LSL, 0); // and t1, t1, t3
            arm64_and_reg(cg_sec, 1, REG(t2), REG(t2), REG(t4), ARM64_LSL, 0); // and t2, t2, t4
        } else if (node->kind == ND_BITOR) {
            arm64_orr_reg(cg_sec, 1, REG(t1), REG(t1), REG(t3), ARM64_LSL, 0); // orr t1, t1, t3
            arm64_orr_reg(cg_sec, 1, REG(t2), REG(t2), REG(t4), ARM64_LSL, 0); // orr t2, t2, t4
        } else {
            arm64_eor_reg(cg_sec, 1, REG(t1), REG(t1), REG(t3), ARM64_LSL, 0); // eor t1, t1, t3
            arm64_eor_reg(cg_sec, 1, REG(t2), REG(t2), REG(t4), ARM64_LSL, 0); // eor t2, t2, t4
        }
        asm_str_reg_off(cg_sec, t1, dst, 8, 0);
        asm_str_reg_off(cg_sec, t2, dst, 8, 8);
        free_reg(t1);
        free_reg(t2);
        free_reg(t3);
        free_reg(t4);
#else
        const char *op = node->kind == ND_BITAND ? "andq"
            : node->kind == ND_BITOR             ? "orq"
                                                 : "xorq";
        asm_mov_mem_rax(cg_sec, lhs);
        asm_mov_mem8_rdx(cg_sec, lhs);
        asm_op_mem_rax(cg_sec, op, rhs);
        asm_op_mem8_rdx(cg_sec, op, rhs);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
#endif
        free_reg(lhs);
        free_reg(rhs);
        return dst;
    }

    // Comparisons: result is int (0 or 1)
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        int lhs = gen_to_int128(node->lhs);
        int rhs = gen_to_int128(node->rhs);
        bool is_unsigned_cmp = node->lhs->ty && node->lhs->ty->is_unsigned;
        int result = alloc_reg();
        int c = ++rcc_label_count;
#ifdef ARCH_ARM64
        int t1 = alloc_reg(), t2 = alloc_reg(), t3 = alloc_reg(), t4 = alloc_reg();
        asm_ldr_reg_off(cg_sec, t1, lhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t2, lhs, 8, 8);
        asm_ldr_reg_off(cg_sec, t3, rhs, 8, 0);
        asm_ldr_reg_off(cg_sec, t4, rhs, 8, 8);
        free_reg(lhs);
        free_reg(rhs);
        if (node->kind == ND_EQ || node->kind == ND_NE) {
            asm_cmp_reg_reg(cg_sec, t1, t3, 8);
            asm_ccmp_eq(cg_sec, t2, t4); // ccmp %s, %s, #0, eq
            asm_cset(cg_sec, result, node->kind == ND_EQ ? ARM64_EQ : ARM64_NE);
        } else {
            // LT or LE: compare hi first (signed or unsigned), then lo (unsigned)
            int hicond_lt = is_unsigned_cmp ? ARM64_LO : ARM64_LT;
            int hicond_gt = is_unsigned_cmp ? ARM64_HI : ARM64_GT;
            int locond = node->kind == ND_LT ? ARM64_LO : ARM64_LS;
            asm_movq_zero(cg_sec, result);
            asm_cmp_reg_reg(cg_sec, t2, t4, 8);
            emit_jcc_fixup(cg_sec, hicond_lt, format(".L.cmp128y.%d", c));
            emit_jcc_fixup(cg_sec, hicond_gt, format(".L.cmp128n.%d", c));
            asm_cmp_phy_phy(cg_sec, t1, t3); // cmp t1, t3 (lo compare, always unsigned)
            emit_jcc_fixup(cg_sec, locond, format(".L.cmp128y.%d", c));
            emit_jmp_fixup(cg_sec, format(".L.cmp128n.%d", c));
            cg_def_label(format(".L.cmp128y.%d", c));
            asm_mov_imm(cg_sec, result, 4, 1);
            cg_def_label(format(".L.cmp128n.%d", c));
        }
        free_reg(t1);
        free_reg(t2);
        free_reg(t3);
        free_reg(t4);
#else
        if (node->kind == ND_EQ || node->kind == ND_NE) {
            int t1 = alloc_reg(), t2 = alloc_reg(), t3 = alloc_reg(), t4 = alloc_reg();
            asm_ldr_reg_off(cg_sec, t1, lhs, 8, 0);
            asm_ldr_reg_off(cg_sec, t2, lhs, 8, 8);
            asm_ldr_reg_off(cg_sec, t3, rhs, 8, 0);
            asm_ldr_reg_off(cg_sec, t4, rhs, 8, 8);
            asm_xor_reg_reg(cg_sec, t1, t3, 8);
            asm_xor_reg_reg(cg_sec, t2, t4, 8);
            asm_or_reg_reg(cg_sec, t1, t2, 8);
            asm_setcc(cg_sec, X86_RAX, node->kind == ND_EQ ? X86_E : X86_NE);
            asm_movzx_phys(cg_sec, result, X86_RAX, 4, 1);
            free_reg(t1);
            free_reg(t2);
            free_reg(t3);
            free_reg(t4);
        } else {
            asm_mov_mem8_rdx(cg_sec, lhs);
            asm_op_mem8_rdx(cg_sec, "cmpq", rhs);
            if (is_unsigned_cmp) {
                emit_jcc_fixup(cg_sec, X86_B, format(".L.cmp128y.%d", c));
                emit_jcc_fixup(cg_sec, X86_A, format(".L.cmp128n.%d", c));
            } else {
                emit_jcc_fixup(cg_sec, X86_L, format(".L.cmp128y.%d", c));
                emit_jcc_fixup(cg_sec, X86_G, format(".L.cmp128n.%d", c));
            }
            asm_mov_mem_rax(cg_sec, lhs);
            asm_op_mem_rax(cg_sec, "cmpq", rhs);
            if (node->kind == ND_LT)
                emit_jcc_fixup(cg_sec, X86_B, format(".L.cmp128y.%d", c));
            else
                emit_jcc_fixup(cg_sec, X86_BE, format(".L.cmp128y.%d", c));
            cg_def_label(format(".L.cmp128n.%d", c));
            asm_xorl_reg_reg(cg_sec, result, result);
            emit_jmp_fixup(cg_sec, format(".L.cmp128e.%d", c));
            cg_def_label(format(".L.cmp128y.%d", c));
            asm_movl_imm(cg_sec, result, 1);
            cg_def_label(format(".L.cmp128e.%d", c));
        }
        free_reg(lhs);
        free_reg(rhs);
#endif
        return result;
    }

    case ND_COND: {
        // ternary with int128 result
        int c = ++rcc_label_count;
        int cond_r = gen(node->cond);
        int dst = alloc_int128_addr();
#ifdef ARCH_ARM64
        asm_cmp_zero(cg_sec, cond_r, node->cond->ty ? node->cond->ty->size : 4);
        emit_jcc_fixup(cg_sec, ARM64_EQ, format(".L.else.%d", c));
#else
        asm_cmp_zero(cg_sec, cond_r, node->cond->ty ? node->cond->ty->size : 4);
        emit_jcc_fixup(cg_sec, X86_E, format(".L.else.%d", c));
#endif
        free_reg(cond_r);
        int then_a = gen_to_int128(node->then);
#ifdef ARCH_ARM64
        {
            int t1 = alloc_reg(), t2 = alloc_reg();
            asm_ldr_reg_off(cg_sec, t1, then_a, 8, 0);
            asm_ldr_reg_off(cg_sec, t2, then_a, 8, 8);
            asm_str_reg_off(cg_sec, t1, dst, 8, 0);
            asm_str_reg_off(cg_sec, t2, dst, 8, 8);
            free_reg(t1);
            free_reg(t2);
        }
        emit_jmp_fixup(cg_sec, format(".L.end.%d", c));
#else
        asm_mov_mem_rax(cg_sec, then_a);
        asm_mov_mem8_rdx(cg_sec, then_a);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
        emit_jmp_fixup(cg_sec, format(".L.end.%d", c));
#endif
        free_reg(then_a);
        cg_def_label(format(".L.else.%d", c));
        int else_a = gen_to_int128(node->els);
#ifdef ARCH_ARM64
        {
            int t1 = alloc_reg(), t2 = alloc_reg();
            asm_ldr_reg_off(cg_sec, t1, else_a, 8, 0);
            asm_ldr_reg_off(cg_sec, t2, else_a, 8, 8);
            asm_str_reg_off(cg_sec, t1, dst, 8, 0);
            asm_str_reg_off(cg_sec, t2, dst, 8, 8);
            free_reg(t1);
            free_reg(t2);
        }
#else
        asm_mov_mem_rax(cg_sec, else_a);
        asm_mov_mem8_rdx(cg_sec, else_a);
        asm_mov_rax_mem(cg_sec, dst);
        asm_mov_rdx_mem8(cg_sec, dst);
#endif
        free_reg(else_a);
        cg_def_label(format(".L.end.%d", c));
        return dst;
    }

    case ND_FUNCALL:
        return gen_funcall(node, -1);

#ifdef ARCH_ARM64
    case ND_VA_ARG: {
        // va_arg(ap, __int128) on ARM64: 2 consecutive GP register slots
        int r = gen(node->lhs); // va_list pointer
        int addr = alloc_int128_addr();
        int c = ++rcc_label_count;
        // Check for 2 register slots available (gr_offs <= -16 = at least 2 slots)
        arm64_ldr_uoff(cg_sec, 2, ARM64_X16, REG(r), 6); // ldr w16, [x{r}, #24] (__gr_offs)
        asm_cmp_w16_imm(cg_sec, -16); // cmp w16, #-16
        emit_jcc_fixup(cg_sec, ARM64_GT, format(".L.va128_stk.%d", c)); // b.gt .L.va128_stk.%d
        // Register save area: load both 64-bit halves
        arm64_add_imm(cg_sec, 0, ARM64_X17, ARM64_X16, 16, 0); // add w17, w16, #16
        arm64_str_uoff(cg_sec, 2, ARM64_X17, REG(r), 6); // str w17, [x{r}, #24]
        arm64_ldr_uoff(cg_sec, 3, ARM64_X9, REG(r), 1); // ldr x9, [x{r}, #8] (__gr_top)
        asm_sxtw_x17_w16(cg_sec); // sxtw x17, w16
        arm64_add_reg(cg_sec, 1, ARM64_X9, ARM64_X9, ARM64_X17, ARM64_LSL, 0); // add x9, x9, x17
        arm64_ldp(cg_sec, 1, ARM64_X16, ARM64_X17, ARM64_X9, 0, false, false); // ldp x16, x17, [x9]
        arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(addr), 0); // str x16, [x{addr}]
        arm64_str_uoff(cg_sec, 3, ARM64_X17, REG(addr), 1); // str x17, [x{addr}, #8]
        emit_jmp_fixup(cg_sec, format(".L.va128_d.%d", c)); // b .L.va128_d.%d
        // Overflow stack: 16-byte aligned
        cg_def_label(format(".L.va128_stk.%d", c)); // .L.xxx.c
        arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(r), 0); // ldr x16, [x{r}] (__stack)
        arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X16, 15, 0); // add x17, x16, #15
        arm64_and_imm(cg_sec, 1, ARM64_X17, ARM64_X17, (uint64_t)-16); // and x17, x17, #-16 (align to 16)
        arm64_str_uoff(cg_sec, 3, ARM64_X17, REG(r), 0); // str x17, [x{r}]
        arm64_ldp(cg_sec, 1, ARM64_X16, ARM64_X17, ARM64_X17, 0, false, false); // ldp x16, x17, [x17]
        arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(addr), 0); // str x16, [x{addr}]
        arm64_str_uoff(cg_sec, 3, ARM64_X17, REG(addr), 1); // str x17, [x{addr}, #8]
        cg_def_label(format(".L.va128_d.%d", c)); // .L.xxx.c
        free_reg(r);
        return addr;
    }
#elif defined(_WIN32)
    case ND_VA_ARG: {
        // Windows x64: va_list is char*. __int128 is passed by pointer (8-byte slot).
        // The shadow space slot holds a pointer to the 16-byte int128 value.
        int r = gen_addr(node->lhs); // address of the char* ap variable
        int addr = alloc_int128_addr();
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(r), 0)); // movq (r), %rcx  (rcx = ap)
        x86_add_mi(cg_sec, 8, x86_mem(REG(r), 0), 8); // addq $8, (r)  (ap += 8)
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RCX, 0)); // movq (%rcx), %rcx  (rcx = *slot)
        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RCX, 0)); // movq (%rcx), %rax  (rax = lo)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 0), X86_RAX); // movq %rax, (addr)
        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RCX, 8)); // movq 8(%rcx), %rax  (rax = hi)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 8), X86_RAX); // movq %rax, 8(addr)
        free_reg(r);
        return addr;
    }
#else
    case ND_VA_ARG: {
        // va_arg(ap, __int128) on x86-64: 2 consecutive GP register slots
        int r = gen(node->lhs); // va_list pointer
        int addr = alloc_int128_addr();
        int c = ++rcc_label_count;
        // gp_offset + 16 <= 48 means <= 32 (2+ register slots available)
        x86_cmp_mi(cg_sec, 4, (X86Mem){REG(r), X86_NOREG, 1, 0}, 32); // cmpl $32, (r) (gp_offset)
        emit_jcc_fixup(cg_sec, X86_A, format(".L.va128_stk.%d", c));
        // Register save area: load both 64-bit halves
        x86_mov_rm(cg_sec, 4, X86_RCX, (X86Mem){REG(r), X86_NOREG, 1, 0}); // movl (r), %ecx (gp_offset)
        x86_add_rm(cg_sec, 8, X86_RCX, (X86Mem){REG(r), X86_NOREG, 1, 16}); // addq 16(r), %rcx (reg_save_area)
        x86_add_mi(cg_sec, 4, (X86Mem){REG(r), X86_NOREG, 1, 0}, 16); // addl $16, (r)
        x86_mov_rm(cg_sec, 8, X86_RAX, (X86Mem){X86_RCX, X86_NOREG, 1, 0}); // movq (%rcx), %rax (lo)
        asm_mov_rax_mem(cg_sec, addr); // movq %rax, (addr)
        x86_mov_rm(cg_sec, 8, X86_RAX, (X86Mem){X86_RCX, X86_NOREG, 1, 8}); // movq 8(%rcx), %rax (hi)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 8), X86_RAX); // movq %rax, 8(addr)
        emit_jmp_fixup(cg_sec, format(".L.va128_d.%d", c));
        // Overflow stack: 16-byte aligned
        cg_def_label(format(".L.va128_stk.%d", c));
        x86_mov_rm(cg_sec, 8, X86_RAX, (X86Mem){REG(r), X86_NOREG, 1, 8}); // movq 8(r), %rax (overflow_arg_area)
        VReg t = alloc_reg();
        asm_mov_reg_reg(cg_sec, t, r, 8); // t = r (copy va_list ptr)
        x86_lea(cg_sec, 8, REG(t), (X86Mem){X86_RAX, X86_NOREG, 1, 15}); // leaq 15(%rax), %rt
        x86_and_ri(cg_sec, 8, REG(t), -16); // andq $-16, %rt (align)
        x86_lea(cg_sec, 8, REG(t), (X86Mem){REG(t), X86_NOREG, 1, 16}); // leaq 16(%rt), %rt (skip aligned block)
        x86_mov_mr(cg_sec, 8, (X86Mem){REG(r), X86_NOREG, 1, 8}, REG(t)); // movq %rt, 8(r) (update overflow)
        x86_mov_rm(cg_sec, 8, X86_RAX, (X86Mem){REG(t), X86_NOREG, 1, -16}); // movq -16(%rt), %rax (lo)
        asm_mov_rax_mem(cg_sec, addr); // movq %rax, (addr)
        x86_mov_rm(cg_sec, 8, X86_RAX, (X86Mem){REG(t), X86_NOREG, 1, -8}); // movq -8(%rt), %rax (hi)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 8), X86_RAX); // movq %rax, 8(addr)
        free_reg(t);
        cg_def_label(format(".L.va128_d.%d", c));
        free_reg(r);
        return addr;
    }
#endif
    case ND_POST_DEC:
    case ND_PRE_INC:
    case ND_PRE_DEC: {
        bool is_pre = (node->kind == ND_PRE_INC || node->kind == ND_PRE_DEC);
        bool is_inc = (node->kind == ND_POST_INC || node->kind == ND_PRE_INC);
        VReg lhs_a = gen_addr(node->lhs);
        VReg old_addr = is_pre ? R_NONE : alloc_int128_addr();
#ifdef ARCH_ARM64
        VReg t1 = alloc_reg(), t2 = alloc_reg();
        asm_ldr_reg_off(cg_sec, t1, lhs_a, 8, 0); // ldr x{t1}, [x{lhs_a}]
        asm_ldr_reg_off(cg_sec, t2, lhs_a, 8, 8); // ldr x{t2}, [x{lhs_a}, #8]
        if (!is_pre) {
            asm_str_reg_off(cg_sec, t1, old_addr, 8, 0); // str x{t1}, [x{old_addr}]
            asm_str_reg_off(cg_sec, t2, old_addr, 8, 8); // str x{t2}, [x{old_addr}, #8]
        }
        if (is_inc) {
            arm64_adds_imm(cg_sec, 1, REG(t1), REG(t1), 1, 0); // adds x{t1}, x{t1}, #1
            asm_adc_phy(cg_sec, t2, t2, ARM64_XZR); // adc x{t2}, x{t2}, xzr
        } else {
            arm64_subs_imm(cg_sec, 1, REG(t1), REG(t1), 1, 0); // subs x{t1}, x{t1}, #1
            asm_sbc_phy(cg_sec, t2, t2, ARM64_XZR); // sbc x{t2}, x{t2}, xzr
        }
        asm_str_reg_off(cg_sec, t1, lhs_a, 8, 0); // str x{t1}, [x{lhs_a}]
        asm_str_reg_off(cg_sec, t2, lhs_a, 8, 8); // str x{t2}, [x{lhs_a}, #8]
        free_reg(t1);
        free_reg(t2);
#else
        asm_mov_mem_rax(cg_sec, lhs_a); // movq (lhs_a), %rax
        asm_mov_mem8_rdx(cg_sec, lhs_a); // movq 8(lhs_a), %rdx
        if (!is_pre) {
            asm_mov_rax_mem(cg_sec, old_addr); // movq %rax, (old_addr)
            asm_mov_rdx_mem8(cg_sec, old_addr); // movq %rdx, 8(old_addr)
        }
        if (is_inc) {
            x86_add_ri(cg_sec, 8, X86_RAX, 1); // addq $1, %rax
            secbuf_emit32le(cg_sec, 0x00D28348); // adcq $0, %rdx (REX.W 83 /2 ib)
        } else {
            x86_sub_ri(cg_sec, 8, X86_RAX, 1); // subq $1, %rax
            secbuf_emit32le(cg_sec, 0x00DA8348); // sbbq $0, %rdx (REX.W 83 /3 ib)
        }
        asm_mov_rax_mem(cg_sec, lhs_a); // movq %rax, (lhs_a)
        asm_mov_rdx_mem8(cg_sec, lhs_a); // movq %rdx, 8(lhs_a)
#endif
        if (is_pre)
            return lhs_a;
        free_reg(lhs_a);
        return old_addr;
    }
    default:
        error("int128: unsupported node kind %d", node->kind);
        return R_NONE;
    }
}

// Generate code for a given node.
static VReg gen(Node *node) {
    if (!node) return R_NONE;

    if (opt_g)
        emit_loc(node);

    // Dispatch int128 nodes to gen_int128()
    if (node->ty && node->ty->kind == TY_INT128)
        return gen_int128(node);
    // Cast from int128 to a smaller type: extract value from 16-byte slot
    if (node->kind == ND_CAST && node->lhs && node->lhs->ty &&
        node->lhs->ty->kind == TY_INT128 && node->ty && node->ty->kind != TY_INT128) {
        VReg addr = gen_int128(node->lhs);
        VReg r = alloc_reg();
#ifdef ARCH_ARM64
        if (is_flonum(node->ty)) {
            // int128 → float: load lo word, convert
            asm_ldr_reg_off(cg_sec, r, addr, 8, 0);
            free_reg(addr);
            asm_fmov_d0_x(cg_sec, r); // fmov d0, x{r}
            arm64_scvtf(cg_sec, 1, 1, ARM64_D0, ARM64_D0); // scvtf d0, d0
            asm_fmov_x_d0(cg_sec, r); // fmov x{r}, d0
        } else {
            asm_ldr_reg_off(cg_sec, r, addr, 8, 0);
            free_reg(addr);
            if (node->ty->size < 8) {
                if (!node->ty->is_unsigned) sign_extend_to(r, node->ty->size, 8);
                else
                    zero_extend_to(r, node->ty->size, 8);
            }
        }
#else
        asm_ldr_reg_off(cg_sec, r, addr, 8, 0); // movq (%s), r
        free_reg(addr);
        if (node->ty->size < 8) {
            if (!node->ty->is_unsigned) sign_extend_to(r, node->ty->size, 8);
            else
                zero_extend_to(r, node->ty->size, 8);
        }
#endif
        return r;
    }
    // Comparisons where the operand is int128 (result is int)
    if (node->lhs && node->lhs->ty && node->lhs->ty->kind == TY_INT128) {
        switch (node->kind) {
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            return gen_int128(node);
        default: break;
        }
    }


    switch (node->kind) {
    case ND_NUM: {
        VReg r = alloc_reg();
#ifdef ARCH_ARM64
        uint64_t v = (uint64_t)(long long)node->val;
        // Use x register for 64-bit immediates, w for 32-bit (stops at lsl #16)
        if (op_size(node->ty) == 4) {
            asm_mov_imm(cg_sec, r, 4, v & 0xffff); // mov $v & 0xffff, rr
            v >>= 16;
            if (v) {
                asm_movk(cg_sec, REG(r), 0, (uint16_t)(v), 16); // movk w{r}, #v, lsl #16
            }
        } else {
            asm_mov_imm(cg_sec, r, 8, v & 0xffff); // mov $v & 0xffff, rr
            v >>= 16;
            int shift = 16;
            while (v) {
                asm_movk(cg_sec, REG(r), 1, (uint16_t)(v & 0xffff), shift); // movk x{r}, #v, lsl #shift
                v >>= 16;
                shift += 16;
            }
        }
#else
        if (node->val == (int32_t)node->val) {
            asm_mov_imm(cg_sec, r, op_size(node->ty), (long long)node->val); // mov $(long long)node->val, rr
        } else {
            asm_mov_imm(cg_sec, r, 8, (long long)node->val); // mov $(long long)node->val, rr
        }
#endif
        return r;
    }
    case ND_FNUM: {
        VReg r = alloc_reg();
#ifdef ARCH_ARM64
        int fsize = (node->ty && node->ty->kind == TY_FLOAT) ? 4 : 8;
#else
        int fsize = 8; // Always store as double; movsd loads 8 bytes
#endif
        int id = add_float_literal(node->fval, fsize);
#ifdef ARCH_ARM64
        emit_adrp_add(r, format(".LF%d", id));
        if (fsize == 4) {
            asm_ldr_fp(cg_sec, 0, r, 4); // ldr s0, [x{r}]
            asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0 (promote to double for GP storage)
        } else {
            asm_ldr_fp(cg_sec, 0, r, 8); // ldr d0, [x{r}]
        }
        asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0 (always 64-bit)
#else
        asm_movsd_rip_xmm(cg_sec, format(".LF%d", id)); // movsd .LF%d(%rip), %%xmm0
        asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %%xmm0, %s
#endif
        return r;
    }
    case ND_LVAR: {
        VReg r = alloc_reg();
        if (opt_W) reg_owner[r] = node->var->name;
#ifndef ARCH_ARM64
        __attribute__((unused)) char *label = var_label(node->var);
#endif
        if (node->var->ty->kind == TY_VLA || ((node->var->ty->kind == TY_STRUCT || node->var->ty->kind == TY_UNION) && node->var->ty->vla_len_expr)) {
            if (node->var->is_local) {
#ifdef ARCH_ARM64
                arm64_load_from_fp_minus(node->var->offset - 8, REG(r));
#else
                asm_mov_rbp_reg(cg_sec, r, 8, node->var->offset - 8); // mov [rbp-8], rr
#endif
            } else {
#ifdef ARCH_ARM64
                if (node->var->is_tls)
                    emit_tls_addr(r, node->var);
                else if (var_needs_got(node->var))
                    emit_adrp_got(r, asm_sym_name(var_sym_label(node->var)));
                else
                    emit_adrp_add(r, asm_sym_name(var_sym_label(node->var)));
                asm_ldr_reg_off(cg_sec, r, r, 8, 0); // ldr x{r}, [x{r}]
#else
                if (var_needs_got(node->var)) {
                    asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
                    asm_mov_mem_reg(cg_sec, r, r, 8); // movq (%r), rr
                } else {
                    asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // lea rip, rr
                    asm_mov_mem_reg(cg_sec, r, r, 8); // movq (%r), rr
                }
#endif
            }
        } else if (node->var->ty->kind == TY_ARRAY || node->var->ty->kind == TY_COMPLEX) {
            if (node->var->is_local)
#ifdef ARCH_ARM64
                if (node->var->offset <= 4095)
                    asm_sub_reg_fp_imm(cg_sec, r, node->var->offset); // sub r, x29, #node->var->offset
                else {
                    int v = node->var->offset;
                    asm_mov_imm(cg_sec, r, 8, v & 0xffff); // mov r, #v
                    v >>= 16;
                    int s = 16;
                    while (v) {
                        asm_movk(cg_sec, REG(r), 1, (uint16_t)(v & 0xffff), s); // movk r, #v, lsl #s
                        v >>= 16;
                        s += 16;
                    }
                    asm_sub_reg_fp_reg(cg_sec, r, r, 8); // sub r, x29, r
                }
#else
                asm_lea_rbp_reg(cg_sec, r, 8, node->var->offset); // lea [rbp-8], rr
#endif
            else
#ifdef ARCH_ARM64
                if (var_needs_got(node->var))
                emit_adrp_got(r, asm_sym_name(var_sym_label(node->var)));
            else
                emit_adrp_add(r, asm_sym_name(var_sym_label(node->var)));
#else
                if (var_needs_got(node->var))
                asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
            else
                asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // lea rip, rr
#endif
        } else if (!node->var->is_local && node->var->is_function) {
            //fprintf(stderr, "DEBUG ND_LVAR func: %s is_weak=%d\n", node->var->name, node->var->is_weak);
            if (node->var->is_weak)
                cg_weak_declare(asm_sym_name(var_sym_label(node->var)));
#ifdef ARCH_ARM64
            if (node->var->is_weak)
                emit_adrp_got(r, asm_sym_name(var_sym_label(node->var)));
            else
                emit_adrp_add(r, asm_sym_name(var_sym_label(node->var)));
#else
            if (node->var->is_weak || var_needs_got(node->var))
                asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
            else
                asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // lea rip, rr
#endif
        } else if (is_flonum(node->var->ty)) {
            {
                if (node->var->is_local) {
                    if (node->var->ty->size == 4) {
#ifdef ARCH_ARM64
                        if (node->var->offset <= 4095) {
                            asm_sub_fp_imm(cg_sec, ARM64_X17, node->var->offset);
                            arm64_ldr_fp(cg_sec, 2, ARM64_S0, ARM64_X17, 0); // ldr s0, [x17]
                        } else {
                            emit_mov_imm64(ARM64_X17, (uint64_t)node->var->offset);
                            asm_sub_fp_reg(cg_sec, ARM64_X17, ARM64_X17);
                            arm64_ldr_fp(cg_sec, 2, ARM64_S0, ARM64_X17, 0); // ldr s0, [x17]
                        }
                        asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0
#else
                        asm_movss_rm_rbp(cg_sec, 0, node->var->offset); // movss -off(%rbp), %xmm0
                        asm_cvtss2sd(cg_sec); // cvtss2sd %xmm0, %xmm0
#endif
                    } else {
#ifdef ARCH_ARM64
                        if (node->var->offset <= 4095) {
                            asm_sub_fp_imm(cg_sec, ARM64_X17, node->var->offset);
                            arm64_ldr_fp(cg_sec, 3, ARM64_D0, ARM64_X17, 0); // ldr d0, [x17]
                        } else {
                            emit_mov_imm64(ARM64_X17, (uint64_t)node->var->offset);
                            asm_sub_fp_reg(cg_sec, ARM64_X17, ARM64_X17);
                            arm64_ldr_fp(cg_sec, 3, ARM64_D0, ARM64_X17, 0); // ldr d0, [x17]
                        }
#else
                        asm_movsd_rm_rbp(cg_sec, 0, node->var->offset); // movsd -off(%rbp), %xmm0
#endif
                    }
                } else {
                    if (node->var->ty->size == 4) {
#ifdef ARCH_ARM64
                        if (node->var->is_tls)
                            emit_tls_addr(r, node->var);
                        else if (var_needs_got(node->var))
                            emit_adrp_got(r, asm_sym_name(var_sym_label(node->var)));
                        else
                            emit_adrp_add(r, asm_sym_name(var_sym_label(node->var)));
                        asm_ldr_fp(cg_sec, 0, r, 4); // ldr s0, [x{r}]
                        asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0
#else
                        if (var_needs_got(node->var)) {
                            asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
                            asm_ldr_fp(cg_sec, 0, r, 4); // movss (r), %xmm0
                        } else {
                            asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // leaq label(%rip), r
                            asm_ldr_fp(cg_sec, 0, r, 4); // movss (r), %xmm0
                        }
                        asm_cvtss2sd(cg_sec); // cvtss2sd %xmm0, %xmm0
#endif
                    } else {
#ifdef ARCH_ARM64
                        if (node->var->is_tls)
                            emit_tls_addr(r, node->var);
                        else if (var_needs_got(node->var))
                            emit_adrp_got(r, asm_sym_name(var_sym_label(node->var)));
                        else
                            emit_adrp_add(r, asm_sym_name(var_sym_label(node->var)));
                        asm_ldr_fp(cg_sec, 0, r, 8); // ldr d0, [x{r}]
#else
                        if (var_needs_got(node->var)) {
                            asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
                            asm_ldr_fp(cg_sec, 0, r, 8); // movsd (r), %xmm0
                        } else {
                            asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // leaq label(%rip), r
                            asm_ldr_fp(cg_sec, 0, r, 8); // movsd (r), %xmm0
                        }
#endif
                    }
                }
#ifdef ARCH_ARM64
                asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0
#else
                asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq r, xmm0
#endif
            }
        } else {
            if (node->var->is_local)
#ifdef ARCH_ARM64
                emit_load(node->ty, r, ARM64_BASE_FP, -node->var->offset);
#else
                emit_load(node->ty, r, X86_BASE_RBP, node->var->offset);
#endif
            else {
#ifdef ARCH_ARM64
                // Global variable: load address via ADRP+ADD, then deref
                int ta = alloc_reg();
                if (node->var->is_tls)
                    emit_tls_addr(ta, node->var);
                else if (var_needs_got(node->var))
                    emit_adrp_got(ta, asm_sym_name(var_sym_label(node->var)));
                else
                    emit_adrp_add(ta, asm_sym_name(var_sym_label(node->var)));
                emit_load(node->ty, r, ta, 0);
                free_reg(ta);
#else
                if (node->var->is_tls) {
#ifdef _WIN32
                    emit_emutls_addr(r, var_label(node->var));
#else
                    VReg base = alloc_reg();
                    asm_mov_fs0_reg(cg_sec, base);
                    asm_lea_tpoff_base_reg(cg_sec, r, base, var_sym_label(node->var));
                    free_reg(base);
#endif
                    emit_load(node->ty, r, r, 0);
                } else if (var_needs_got(node->var)) {
                    asm_mov_got_rip_reg(cg_sec, r, var_sym_label(node->var)); // mov sym@GOTPCREL(%rip), r
                    emit_load(node->ty, r, r, 0);
                } else {
                    asm_lea_rip_reg(cg_sec, r, var_sym_label(node->var)); // lea rip, rr
                    emit_load(node->ty, r, r, 0);
                }
#endif
            }
#ifdef ARCH_ARM64
            // emit_load already handles sign/zero-extension for narrow types
#endif
        }
        return r;
    }
    case ND_ASSIGN: {
        if (node->lhs->ty->kind == TY_ARRAY || node->lhs->ty->kind == TY_STRUCT || node->lhs->ty->kind == TY_UNION || node->lhs->ty->kind == TY_COMPLEX) {
            int c = ++rcc_label_count;
            bool lhs_vla_struct = (node->lhs->ty->kind == TY_STRUCT || node->lhs->ty->kind == TY_UNION) && node->lhs->ty->vla_len_expr;
            VReg dst = lhs_vla_struct ? gen(node->lhs) : gen_addr(node->lhs);
            VReg src;
            if (node->rhs->kind == ND_FUNCALL && node->rhs->ty &&
                (node->rhs->ty->kind == TY_STRUCT || node->rhs->ty->kind == TY_UNION || node->rhs->ty->kind == TY_COMPLEX)) {
#ifndef ARCH_ARM64
                int sav = -1;
                if (dst == 0 || dst == 1) {
                    sav = dst;
                    asm_mov_reg_rbp(cg_sec, dst, 8, spill_offset(dst)); // movq dst, -spill_offset(dst)(%rbp)
                }
#endif
                gen_funcall(node->rhs, dst);
#ifndef ARCH_ARM64
                if (sav >= 0)
                    asm_mov_rbp_reg(cg_sec, sav, 8, spill_offset(sav)); // movq -spill_offset(sav)(%rbp), sav
#endif
                // If the call's complex return type and the destination's
                // complex type have differently-sized real-floating bases
                // (e.g. _Complex float -> _Complex double), convert the
                // components in place before anything else uses dst.
                if (node->ty->kind == TY_COMPLEX && node->rhs->ty->kind == TY_COMPLEX &&
                    is_flonum(node->ty->base) && is_flonum(node->rhs->ty->base) &&
                    node->ty->base->kind != TY_LDOUBLE && node->rhs->ty->base->kind != TY_LDOUBLE &&
                    node->ty->base->size != node->rhs->ty->base->size) {
                    emit_complex_convert_float(dst, dst, node->rhs->ty, node->ty);
                }
                // For complex return values used as a condition (e.g. if (c = f())),
                // test whether the value is non-zero.
                if (node->ty->kind == TY_COMPLEX) {
                    int base_sz = node->ty->base->size;
                    int rt = alloc_reg();
#ifdef ARCH_ARM64
                    asm_ldr_reg_off(cg_sec, rt, dst, 8, 0);
                    if (node->ty->size > base_sz) {
                        int rt2 = alloc_reg();
                        asm_ldr_reg_off(cg_sec, rt2, dst, 8, base_sz); // ldr rt2, [dst, #base_sz]
                        arm64_orr_reg(cg_sec, 1, REG(rt), REG(rt), REG(rt2), ARM64_LSL, 0); // orr rt, rt, rt2
                        free_reg(rt2);
                    }
                    asm_cmp_zero(cg_sec, rt, 8);
                    asm_cset(cg_sec, rt, ARM64_NE);
#else
                    // The whole complex value fits in 8 bytes (_Complex float)
                    // or two 8-byte halves (_Complex double): load (and OR
                    // together) the raw bits and test against zero.
                    x86_mov_rm(cg_sec, 8, REG(rt), x86_mem(REG(dst), 0)); // movq (dst), rt
                    if (node->ty->size > 8)
                        x86_or_rm(cg_sec, 8, REG(rt), x86_mem(REG(dst), base_sz)); // orq base_sz(dst), rt
                    asm_cmp_zero(cg_sec, rt, 8); // cmp $0, rt
                    asm_setcc(cg_sec, X86_RAX, X86_NE); // setne %al
                    asm_movzx_phys(cg_sec, rt, X86_RAX, 4, 1); // movzbl %al, rt
#endif
                    free_reg(dst);
                    return rt;
                }
                return dst;
            }
            // String literal → char array: limit copy to actual string size, zero-fill rest
            if (node->rhs->kind == ND_STR && node->lhs->ty->kind == TY_ARRAY) {
                src = gen(node->rhs); // loads address of string
                int str_len = 0;
                for (StrLit *s = all_strs; s; s = s->next) {
                    if (s->id != node->rhs->str_id) continue;
                    if (s->prefix != 0) {
                        // Wide string: count Unicode chars and multiply by elem_size
                        int count = 0;
                        char *p = s->str;
                        while (*p) {
                            char *next;
                            decode_utf8(&next, p);
                            if (next == p) {
                                p++;
                                continue;
                            }
                            p = next;
                            count++;
                        }
                        str_len = (count + 1) * s->elem_size;
                    } else {
                        str_len = s->len + s->elem_size;
                    }
                    break;
                }
                int lhs_size = node->lhs->ty->size;
                int copy_len = str_len < lhs_size ? str_len : lhs_size;
                if (copy_len > 0)
#ifdef ARCH_ARM64
                {
                    emit_mov_imm64(ARM64_X9, copy_len); // mov x9, #copy_len
                    cg_def_label(format(".L.strcpy.%d", c)); // .L.copy.%d:
                    arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X9, 0, 0); // cmp x9, #0
                    {
                        size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // b.eq .L.strcpy_end.c
                        asm_fixup_add(cg_sec, o, format(".L.strcpy_end.%d", c), 1);
                    }
                    arm64_sub_imm(cg_sec, 1, ARM64_X9, ARM64_X9, 1, 0); // sub x9, x9, #1
                    // ldrb w16, [x{src}, x9]
                    asm_ldrb_w16_x9(cg_sec, src); // ldrb w16, [x{src}, x9]
                    // strb w16, [x{dst}, x9]
                    asm_strb_w16_x9(cg_sec, dst); // strb w16, [x{dst}, x9]
                    {
                        size_t o = asm_jmp_label(cg_sec); // b .L.strcpy.c
                        asm_fixup_add(cg_sec, o, format(".L.strcpy.%d", c), 0);
                    }
                    cg_def_label(format(".L.strcpy_end.%d", c));
                }
                if (copy_len < lhs_size) {
                    // Zero dst[copy_len .. lhs_size-1]; count x12 from lhs_size down to copy_len
                    int c2 = ++rcc_label_count;
                    emit_mov_imm64(ARM64_X9, lhs_size); // mov x9, #lhs_size
                    cg_def_label(format(".L.strzero.%d", c2)); // .L.copy_end.%d:
                    if (copy_len >= 0 && copy_len <= 4095)
                        arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X9, copy_len, 0); // cmp x9, #copy_len
                    else {
                        emit_mov_imm64(ARM64_X16, (uint64_t)copy_len); // mov x16, #copy_len
                        /* TODO
                        if (copy_len >> 16)
                          arm64_movk(cg_sec, ARM64_X16, 1, (copy_len >> 16) & 0xffff, 16); // movk x16, #(copy_len>>16), lsl #16
                        */
                        arm64_subs_reg(cg_sec, 1, ARM64_XZR, ARM64_X9, ARM64_X16, ARM64_LSL, 0); // cmp x9, x16
                    }
                    {
                        size_t o = asm_jcc_label(cg_sec, ARM64_EQ); // b.eq .L.strzero_end.c2
                        asm_fixup_add(cg_sec, o, format(".L.strzero_end.%d", c2), 1);
                    }
                    arm64_sub_imm(cg_sec, 1, ARM64_X9, ARM64_X9, 1, 0); // sub x9, x9, #1
                    asm_strb_wzr_x9(cg_sec, dst); // strb wzr, [x{dst}, x9]
                    {
                        size_t o = asm_jmp_label(cg_sec); // b .L.strzero.c2
                        asm_fixup_add(cg_sec, o, format(".L.strzero.%d", c2), 0);
                    }
                    cg_def_label(format(".L.strzero_end.%d", c2));
                }
#else
                {
                    asm_mov_imm_phy(cg_sec, X86_RCX, 8, copy_len); // movq $copy_len, %rcx
                    cg_def_label(format(".L.strcpy.%d", c));
                    x86_cmp_ri(cg_sec, 8, X86_RCX, 0); // cmpq $0, %rcx
                    {
                        size_t o = asm_jcc_label(cg_sec, X86_E); // je .L.strcpy_end.c
                        asm_fixup_add(cg_sec, o, format(".L.strcpy_end.%d", c), 1);
                    }
                    x86_mov_rm(cg_sec, 1, X86_RAX, x86_mem_idx(REG(src), X86_RCX, 1, -1)); // movb -1(src, %rcx), %al
                    x86_mov_mr(cg_sec, 1, x86_mem_idx(REG(dst), X86_RCX, 1, -1), X86_RAX); // movb %al, -1(dst, %rcx)
                    x86_sub_ri(cg_sec, 8, X86_RCX, 1); // subq $1, %rcx
                    {
                        size_t o = asm_jmp_label(cg_sec); // jmp .L.strcpy.c
                        asm_fixup_add(cg_sec, o, format(".L.strcpy.%d", c), 0);
                    }
                    cg_def_label(format(".L.strcpy_end.%d", c));
                }
                if (copy_len < lhs_size) {
                    VReg cnt2 = alloc_reg();
                    asm_mov_imm(cg_sec, cnt2, 8, lhs_size - copy_len); // movq $count, cnt2
                    int c2 = ++rcc_label_count;
                    cg_def_label(format(".L.strzero.%d", c2));
                    x86_cmp_ri(cg_sec, 8, REG(cnt2), 0); // cmpq $0, cnt2
                    {
                        size_t o = asm_jcc_label(cg_sec, X86_E); // je .L.strzero_end.c2
                        asm_fixup_add(cg_sec, o, format(".L.strzero_end.%d", c2), 1);
                    }
                    // movb $0, copy_len-1(dst, cnt2)
                    x86_mov_mi(cg_sec, 1, x86_mem_idx(REG(dst), REG(cnt2), 1, copy_len - 1), 0);
                    x86_sub_ri(cg_sec, 8, REG(cnt2), 1); // subq $1, cnt2
                    {
                        size_t o = asm_jmp_label(cg_sec); // jmp .L.strzero.c2
                        asm_fixup_add(cg_sec, o, format(".L.strzero.%d", c2), 0);
                    }
                    cg_def_label(format(".L.strzero_end.%d", c2));
                    free_reg(cnt2);
                }
#endif
                free_reg(src);
                return dst;
            }

            if (node->rhs->ty && (node->rhs->ty->kind == TY_STRUCT || node->rhs->ty->kind == TY_UNION || node->rhs->ty->kind == TY_ARRAY))
                // For chain assignments (d = e = a[0] = c), use gen() to trigger
                // inner assignment evaluation, not gen_addr() which skips it.
                src = (node->rhs->kind == ND_ASSIGN) ? gen(node->rhs) : gen_addr(node->rhs);
            else if (node->rhs->ty && is_complex(node->rhs->ty) && node->rhs->kind == ND_LVAR && !node->rhs->var->is_local)
                src = gen_addr(node->rhs);
            else
                src = gen(node->rhs);

            // Complex-to-complex assignment with differing real-floating base
            // types (e.g. _Complex float = _Complex double or vice versa):
            // convert each component instead of doing a raw byte copy.
            if (node->lhs->ty->kind == TY_COMPLEX && node->rhs->ty && node->rhs->ty->kind == TY_COMPLEX &&
                is_flonum(node->lhs->ty->base) && is_flonum(node->rhs->ty->base) &&
                node->lhs->ty->base->kind != TY_LDOUBLE && node->rhs->ty->base->kind != TY_LDOUBLE &&
                node->lhs->ty->base->size != node->rhs->ty->base->size) {
                emit_complex_convert_float(src, dst, node->rhs->ty, node->lhs->ty);
                free_reg(src);
                return dst;
            }

            // Complex-to-complex assignment with differing integer base
            // types (e.g. _Complex int = _Complex char): convert components.
            if (node->lhs->ty->kind == TY_COMPLEX && node->rhs->ty && node->rhs->ty->kind == TY_COMPLEX &&
                !is_flonum(node->lhs->ty->base) && !is_flonum(node->rhs->ty->base) &&
                node->lhs->ty->base->size != node->rhs->ty->base->size) {
                emit_complex_convert_int(src, dst, node->rhs->ty, node->lhs->ty);
                free_reg(src);
                return dst;
            }

            // If RHS is a scalar (e.g. pointer/function) and LHS is a small struct/union,
            // store the value directly instead of copying bytes from the address in the register.
            if (node->rhs->ty && !(node->rhs->ty->kind == TY_STRUCT || node->rhs->ty->kind == TY_UNION || node->rhs->ty->kind == TY_ARRAY || node->rhs->ty->kind == TY_COMPLEX) && node->lhs->ty->size <= 8) {
#ifdef ARCH_ARM64
                emit_store(node->lhs->ty, src, dst, 0);
#else
                {
                    int st_sz = node->lhs->ty->size;
                    if (st_sz < 4) st_sz = st_sz;
                    asm_mov_reg_mem(cg_sec, src, dst, st_sz); // movl/movq src, (%dst)
                }
#endif
                free_reg(src);
                return dst;
            }

            // For VLA-containing structs, use the runtime size instead of ty->size
            bool copy_is_vla_struct = (node->lhs->ty->kind == TY_STRUCT || node->lhs->ty->kind == TY_UNION) && node->lhs->ty->vla_len_expr;
            VReg r_vla_sz = -1;
            if (copy_is_vla_struct)
                r_vla_sz = gen(node->lhs->ty->vla_len_expr);
#ifdef ARCH_ARM64
            if (copy_is_vla_struct && r_vla_sz >= 0)
                asm_mov_x9_vreg(cg_sec, r_vla_sz); // mov x9, x{r_vla_sz}
            else
                emit_mov_imm64(ARM64_X9, (uint64_t)node->lhs->ty->size); // mov x9, #size
            cg_def_label(format(".L.copy.%d", c));
            arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X9, 0, 0); // cmp x9, #0
            {
                size_t _cj = asm_jcc_label(cg_sec, ARM64_EQ);
                asm_fixup_add(cg_sec, _cj, format(".L.copy_end.%d", c), 1);
            }
            arm64_sub_imm(cg_sec, 1, ARM64_X9, ARM64_X9, 1, 0); // sub x9, x9, #1
            asm_ldrb_w16_x9(cg_sec, src); // ldrb w16, [x{src}, x9]
            asm_strb_w16_x9(cg_sec, dst); // strb w16, [x{dst}, x9]
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.copy.%d", c), 0);
            }
            cg_def_label(format(".L.copy_end.%d", c));
#else
            {
                VReg cnt = alloc_reg(); // dedicated counter, doesn't conflict with src/dst
                if (copy_is_vla_struct && r_vla_sz >= 0)
                    asm_mov_reg_reg(cg_sec, cnt, r_vla_sz, 8); // movq r_vla_sz, cnt
                else
                    asm_mov_imm(cg_sec, cnt, 8, node->lhs->ty->size); // movq $size, cnt
                cg_def_label(format(".L.copy.%d", c));
                x86_cmp_ri(cg_sec, 8, REG(cnt), 0); // cmpq $0, cnt
                size_t cj1 = asm_jcc_label(cg_sec, X86_E);
                asm_fixup_add(cg_sec, cj1, format(".L.copy_end.%d", c), 1);
                {
                    X86Mem msrc = x86_mem_idx(REG(src), REG(cnt), 1, -1); // -1(src, cnt)
                    X86Mem mdst = x86_mem_idx(REG(dst), REG(cnt), 1, -1); // -1(dst, cnt)
                    x86_mov_rm(cg_sec, 1, X86_RAX, msrc); // movb -1(src,cnt), %al
                    x86_mov_mr(cg_sec, 1, mdst, X86_RAX); // movb %al, -1(dst,cnt)
                }
                x86_sub_ri(cg_sec, 8, REG(cnt), 1); // subq $1, cnt
                size_t cj2 = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, cj2, format(".L.copy.%d", c), 0);
                cg_def_label(format(".L.copy_end.%d", c));
                free_reg(cnt);
            }
#endif
            if (r_vla_sz >= 0) free_reg(r_vla_sz);
            free_reg(src);
            return dst;
        }
        if (is_flonum(node->lhs->ty)) {
#ifdef ARCH_ARM64
            VReg r2 = gen(node->rhs);
            VReg r1 = gen_addr(node->lhs);
            asm_fmov_i2f(cg_sec, 0, r2, 1); // fmov d0, x{r2}
            if (node->lhs->ty->size == 4) {
                asm_fcvt(cg_sec, 0, 1, 0, 0); // fcvt s0, d0 (opc=0=double->single)
                asm_str_fp(cg_sec, 0, r1, 4); // str s0, [x{r1}]
            } else {
                asm_str_fp(cg_sec, 0, r1, 8); // str d0, [x{r1}]
            }
            free_reg(r1);
            return r2;
#else
#ifndef _WIN32
            if (node->lhs->ty->kind == TY_LDOUBLE) {
                // Store long double as 64-bit double (truncated)
                VReg r2 = gen(node->rhs);
                VReg r1 = gen_addr(node->lhs);
                asm_movq_r_xmm(cg_sec, 0, r2); // movq r2, %xmm0
                asm_str_fp(cg_sec, 0, r1, 8); // movsd %xmm0, (r1)
                free_reg(r2);
                free_reg(r1);
                VReg dummy = alloc_reg();
                asm_movq_zero(cg_sec, dummy); // xor rdummy, rdummy
                return dummy;
            }
#endif
            VReg r2 = gen(node->rhs);
            VReg r1 = gen_addr(node->lhs);
            asm_movq_r_xmm(cg_sec, 0, r2); // movq r2, %xmm0
            if (node->lhs->ty->size == 4) {
                asm_cvtsd2ss(cg_sec); // cvtsd2ss %xmm0, %xmm0
                asm_str_fp(cg_sec, 0, r1, 4); // movss %xmm0, (r1)
            } else {
                asm_str_fp(cg_sec, 0, r1, 8); // movsd %xmm0, (r1)
            }
            free_reg(r1);
            return r2;
#endif
        }
        if (node->lhs->kind == ND_LVAR && node->lhs->var->is_local && node->lhs->var->ty->kind != TY_ARRAY) {
            VReg r2 = gen(node->rhs);
#ifdef ARCH_ARM64
            emit_store_offset(node->lhs->ty, r2, FRAME_PTR, -node->lhs->var->offset);
#else
            asm_mov_reg_rbp(cg_sec, r2, node->lhs->ty->size, node->lhs->var->offset); // mov rr2, [rbp-node->lhs->ty->size]
#endif
            // Truncate result to match the variable's type width for unsigned narrow types
            if (node->lhs->ty->is_unsigned && node->lhs->ty->size < 4) {
                int mask = (1 << (node->lhs->ty->size * 8)) - 1;
                asm_and_imm(cg_sec, r2, 4, mask); // and $mask, reg[r2]
            }
            return r2;
        }
        // Bitfield assignment: read-modify-write
        if (node->lhs->kind == ND_MEMBER && node->lhs->member &&
            node->lhs->member->bit_width > 0) {
            int bw = node->lhs->member->bit_width;
            int bo = node->lhs->member->bit_offset;
            int unit_sz = node->lhs->member->bf_load_size
                ? node->lhs->member->bf_load_size
                : node->lhs->member->ty->size;
            unsigned long long mask = ((1ULL << bw) - 1) << bo;

            // Check if RHS reads the same bitfield (compound assignment like s.x += 1)
            bool rhs_reads_same = false;
            if (node->rhs->kind == ND_MEMBER && node->rhs->member == node->lhs->member) {
                rhs_reads_same = true;
            } else if (node->rhs->kind == ND_ADD || node->rhs->kind == ND_SUB ||
                       node->rhs->kind == ND_MUL || node->rhs->kind == ND_DIV ||
                       node->rhs->kind == ND_BITAND || node->rhs->kind == ND_BITOR ||
                       node->rhs->kind == ND_BITXOR) {
                if (node->rhs->lhs && node->rhs->lhs->kind == ND_MEMBER &&
                    node->rhs->lhs->member == node->lhs->member)
                    rhs_reads_same = true;
                if (node->rhs->rhs && node->rhs->rhs->kind == ND_MEMBER &&
                    node->rhs->rhs->member == node->lhs->member)
                    rhs_reads_same = true;
            }

            // Helper: emit unsigned load of unit_sz bytes
            // Helper: emit unsigned load of unit_sz bytes
#ifdef ARCH_ARM64
#define BF_LOAD(sz, ra, rt) do { asm_ldr_reg_off(cg_sec, rt, ra, sz, 0); } while (0)
#define BF_STORE(sz, ra, rt) do { asm_str_reg_off(cg_sec, rt, ra, sz, 0); } while (0)
#else
#define BF_LOAD(sz, ra, rt) do { \
    if ((sz) == 1) asm_movzx_mem_reg(cg_sec, rt, ra, 4, 1); \
    else if ((sz) == 2) asm_movzx_mem_reg(cg_sec, rt, ra, 4, 2); \
    else if ((sz) == 4) asm_mov_mem_reg(cg_sec, rt, ra, 4); \
    else asm_mov_mem_reg(cg_sec, rt, ra, 8); \
} while (0)
#define BF_STORE(sz, ra, rt) do { \
    if ((sz) == 1) asm_mov_reg_mem(cg_sec, rt, ra, 1); \
    else if ((sz) == 2) asm_mov_reg_mem(cg_sec, rt, ra, 2); \
    else if ((sz) == 4) asm_mov_reg_mem(cg_sec, rt, ra, 4); \
    else asm_mov_reg_mem(cg_sec, rt, ra, 8); \
} while (0)
#endif

            // Generate RHS (the new value to assign)
            VReg r2 = gen(node->rhs);

            if (rhs_reads_same) {
                VReg ra = gen_addr(node->lhs);
                VReg rt = alloc_reg();
                int eff_sz_rhs = unit_sz > 8 ? 8 : unit_sz;
                BF_LOAD(eff_sz_rhs, ra, rt);
#ifdef ARCH_ARM64
                emit_mov_imm64(ARM64_X16, ~mask);
                asm_and_reg_phy(cg_sec, rt, ARM64_X16, 8); // and rrt, r16
                emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
                VReg rv = alloc_reg();
                asm_mov_reg_reg(cg_sec, rv, r2, 8); // mov rr2 -> rrv
                asm_and_reg_phy(cg_sec, rv, ARM64_X16, 8); // and rrv, r16
                if (bo > 0) asm_shl_imm(cg_sec, rv, 8, (uint8_t)(bo)); // lsl x{rv}, x{rv}, #bo
                asm_or_reg_reg(cg_sec, rt, rv, 8); // orr x{rt}, x{rt}, x{rv} (merge new value into old)
                BF_STORE(eff_sz_rhs, ra, rt);
                free_reg(rv);
                // Reload stored bitfield value for assignment expression result
                {
                    int new_eff_sz_rhs = eff_sz_rhs;
                    if (new_eff_sz_rhs == 1)
                        asm_ldrb_uoff(cg_sec, rt, ra, 0); // ldrb w{rt}, [x{ra}]
                    else if (new_eff_sz_rhs == 2)
                        asm_ldrh_uoff(cg_sec, rt, ra, 0); // ldrh w{rt}, [x{ra}]
                    else if (new_eff_sz_rhs == 4)
                        asm_ldr_reg_off(cg_sec, rt, ra, 4, 0); // ldr w{rt}, [x{ra}]
                    else
                        asm_ldr_reg_off(cg_sec, rt, ra, 8, 0); // ldr x{rt}, [x{ra}]
                    if (bo > 0)
                        asm_shr_imm(cg_sec, rt, 8, (uint8_t)(bo)); // lsr x{rt}, x{rt}, #bo
                    if (bw < new_eff_sz_rhs * 8) {
                        if (node->lhs->member->ty->is_unsigned || node->lhs->member->ty->is_enum) {
                            emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
                            asm_and_reg_phy(cg_sec, rt, ARM64_X16, 8); // and x{rt}, x{rt}, x16
                        } else {
                            int shift = 64 - bw;
                            asm_shl_imm(cg_sec, rt, 8, (uint8_t)(shift)); // lsl x{rt}, x{rt}, #shift
                            asm_sar_imm(cg_sec, rt, 8, (uint8_t)(shift)); // asr x{rt}, x{rt}, #shift
                        }
                    }
                    free_reg(r2);
                    VReg ret_reg = rt;
                    free_reg(ra);
                    return ret_reg;
                }
            }

            // Simple assignment: read-modify-write
            free_reg(gen_addr(node->lhs));
            VReg ra = gen_addr(node->lhs);
            VReg rt = alloc_reg();
            int eff_sz = unit_sz > 8 ? 8 : unit_sz;
            BF_LOAD(eff_sz, ra, rt);
            emit_mov_imm64(ARM64_X16, ~mask);
            asm_and_reg_phy(cg_sec, rt, ARM64_X16, 8); // and rrt, r16
            emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
            VReg rv = alloc_reg();
            asm_mov_reg_reg(cg_sec, rv, r2, 8); // mov rr2 -> rrv
            asm_and_reg_phy(cg_sec, rv, ARM64_X16, 8); // and rrv, r16
            if (bo > 0) asm_shl_imm(cg_sec, rv, 8, (uint8_t)(bo)); // lsl x{rv}, x{rv}, #bo
            asm_or_reg_reg(cg_sec, rt, rv, 8); // orr x{rt}, x{rt}, x{rv} (merge new value into old)
            BF_STORE(eff_sz, ra, rt);
            if (unit_sz > 8 && bo + bw > 64) {
                int overflow = bo + bw - 64;
                unsigned int ovf_mask = (1u << overflow) - 1;
                asm_add_imm(cg_sec, ra, 8, 8); // add ra, ra, #8
                asm_ldrb_uoff(cg_sec, rt, ra, 0); // ldrb w{rt}, [x{ra}]
                asm_and_imm(cg_sec, rt, 4, (int32_t)(~ovf_mask & 0xFF)); // and w{rt}, w{rt}, #~ovf_mask
                asm_mov_reg_reg(cg_sec, rv, r2, 8); // mov x{rv}, x{r2}
                asm_shr_imm(cg_sec, rv, 8, (uint8_t)(64 - bo)); // lsr x{rv}, x{rv}, #(64-bo)
                asm_and_imm(cg_sec, rv, 4, (int32_t)ovf_mask); // and w{rv}, w{rv}, #ovf_mask
                asm_or_reg_reg(cg_sec, rt, rv, 4); // orr w{rt}, w{rt}, w{rv}
                asm_strb_uoff(cg_sec, rt, ra, 0); // strb w{rt}, [x{ra}]
            }
#else
                if (bo > 0) asm_shr_imm(cg_sec, rt, 8, (uint8_t)(bo)); // shr $(uint8_t)(bo), rrt
                if (bw < unit_sz * 8) {
                    unsigned long long m = (1ULL << bw) - 1;
                    asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(m)); // movabs $(uint64_t)(m), rX86_RAX
                    asm_and_rax(cg_sec, rt, 8); // andq %rax, rrt
                }
                // Re-load for modify
                BF_LOAD(unit_sz, ra, rt);
                asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(~mask)); // movabs $(uint64_t)(~mask), rX86_RAX
                asm_and_rax(cg_sec, rt, 8); // andq %rax, rrt
                VReg rv = alloc_reg();
                asm_mov_reg_reg(cg_sec, rv, r2, 8); // mov rr2 -> rrv
                asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)((1ULL << bw) - 1)); // movabs $(uint64_t)((1ULL << bw) - 1), rX86_RAX
                asm_and_rax(cg_sec, rv, 8); // andq %rax, rrv
                if (bo > 0) asm_shl_imm(cg_sec, rv, 8, (uint8_t)(bo)); // shl $(uint8_t)(bo), rrv
                asm_or_reg_reg(cg_sec, rt, rv, 8); // or rrv, rrt
                BF_STORE(unit_sz, ra, rt);
                free_reg(rv);
                // Reload stored bitfield value for assignment expression result
                {
                    int new_eff_sz = unit_sz > 8 ? 8 : unit_sz;
                    BF_LOAD(new_eff_sz, ra, rt); // reload bitfield unit from memory
                    if (bo > 0)
                        asm_shr_imm(cg_sec, rt, 8, (uint8_t)(bo)); // shr $(uint8_t)(bo), rrt
                    if (bw < new_eff_sz * 8) {
                        if (node->lhs->member->ty->is_unsigned || node->lhs->member->ty->is_enum) {
                            unsigned long long m = (1ULL << bw) - 1;
                            asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(m)); // movabs $(uint64_t)(m), rX86_RAX
                            asm_and_rax(cg_sec, rt, 8); // andq %rax, rrt
                        } else {
                            int shift = 64 - bw;
                            asm_shl_imm(cg_sec, rt, 8, (uint8_t)(shift)); // shl $(uint8_t)(shift), rrt
                            asm_sar_imm(cg_sec, rt, 8, (uint8_t)(shift)); // sar $(uint8_t)(shift), rrt
                        }
                    }
                    free_reg(r2);
                    VReg ret_reg = rt;
                    free_reg(ra);
                    return ret_reg;
                }
            }

            // Simple assignment: read-modify-write
            free_reg(gen_addr(node->lhs)); // discard; re-gen below
            VReg ra = gen_addr(node->lhs);
            VReg rt = alloc_reg();
            int eff_sz = unit_sz > 8 ? 8 : unit_sz;
            BF_LOAD(eff_sz, ra, rt);
            asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(~mask)); // movabs $(uint64_t)(~mask), rX86_RAX
            asm_and_rax(cg_sec, rt, 8); // andq %rax, rrt
            VReg rv = alloc_reg();
            asm_mov_reg_reg(cg_sec, rv, r2, 8); // mov rr2 -> rrv
            asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)((1ULL << bw) - 1)); // movabs $(uint64_t)((1ULL << bw) - 1), rX86_RAX
            asm_and_rax(cg_sec, rv, 8); // andq %rax, rrv
            if (bo > 0) asm_shl_imm(cg_sec, rv, 8, (uint8_t)(bo)); // shl $(uint8_t)(bo), rrv
            asm_or_reg_reg(cg_sec, rt, rv, 8); // or rrv, rrt
            BF_STORE(eff_sz, ra, rt);
            // Handle overflow bits beyond the first 8 bytes
            if (unit_sz > 8 && bo + bw > 64) {
                int overflow = bo + bw - 64;
                unsigned int ovf_mask = (1u << overflow) - 1;
                asm_add_imm(cg_sec, ra, 8, 8); // addq $8, ra
                x86_movzx_rm(cg_sec, 4, 1, REG(rt), x86_mem(REG(ra), 0)); // movzbl (%ra), reg32[rt]
                asm_and_imm(cg_sec, rt, 4, (unsigned)(~ovf_mask & 0xFF)); // and $~ovf_mask, reg32[rt]
                asm_mov_reg_reg(cg_sec, rv, r2, 8); // mov rr2 -> rrv
                asm_shr_imm(cg_sec, rv, 8, (uint8_t)(64 - bo)); // shr $(uint8_t)(64 - bo), rrv
                asm_and_imm(cg_sec, rv, 4, ovf_mask); // and $ovf_mask, reg32[rv]
                asm_or_reg_reg(cg_sec, rt, rv, 4); // or rrv, rrt
                asm_mov_reg_mem(cg_sec, rt, ra, 1); // movb reg8[rt], (%ra)
            }
#endif
#undef BF_LOAD
#undef BF_STORE
            free_reg(rv);
#ifdef ARCH_ARM64
            // ARM64 reload for assignment expression result
            {
                int new_eff_sz = eff_sz;
                if (new_eff_sz == 1)
                    asm_ldrb_uoff(cg_sec, rt, ra, 0); // ldrb w{rt}, [x{ra}]
                else if (new_eff_sz == 2)
                    asm_ldrh_uoff(cg_sec, rt, ra, 0); // ldrh w{rt}, [x{ra}]
                else if (new_eff_sz == 4)
                    asm_ldr_reg_off(cg_sec, rt, ra, 4, 0); // ldr w{rt}, [x{ra}]
                else
                    asm_ldr_reg_off(cg_sec, rt, ra, 8, 0); // ldr x{rt}, [x{ra}]
                if (bo > 0)
                    asm_shr_imm(cg_sec, rt, 8, (uint8_t)(bo)); // lsr x{rt}, x{rt}, #bo
                if (bw < new_eff_sz * 8) {
                    if (node->lhs->member && (node->lhs->member->ty->is_unsigned || node->lhs->member->ty->is_enum)) {
                        emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
                        asm_and_reg_phy(cg_sec, rt, ARM64_X16, 8); // and rrt, r16
                    } else {
                        int shift = 64 - bw;
                        asm_shl_imm(cg_sec, rt, 8, (uint8_t)(shift)); // lsl x{rt}, x{rt}, #shift
                        asm_sar_imm(cg_sec, rt, 8, (uint8_t)(shift)); // asr x{rt}, x{rt}, #shift
                    }
                }
                free_reg(r2);
                VReg ret_reg = rt;
                free_reg(ra);
                return ret_reg;
            }
#else
            // x86_64 reload for assignment expression result
            {
                int new_eff_sz = eff_sz;
                // Reload from [ra] into rt with appropriate size
                if (new_eff_sz == 1) asm_movzx_mem_reg(cg_sec, rt, ra, 4, 1); // movzbl (%ra), rt
                else if (new_eff_sz == 2)
                    asm_movzx_mem_reg(cg_sec, rt, ra, 4, 2); // movzwl (%ra), rt
                else if (new_eff_sz == 4)
                    asm_mov_mem_reg(cg_sec, rt, ra, 4); // movl (%ra), rt
                else
                    asm_mov_mem_reg(cg_sec, rt, ra, 8); // movq (%ra), rt
                if (bo > 0)
                    asm_shr_imm(cg_sec, rt, 8, (uint8_t)(bo)); // shr $(uint8_t)(bo), rrt
                if (bw < new_eff_sz * 8) {
                    if (node->lhs->member && (node->lhs->member->ty->is_unsigned || node->lhs->member->ty->is_enum)) {
                        unsigned long long m = (1ULL << bw) - 1;
                        asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(m)); // movabs $(uint64_t)(m), rX86_RAX
                        asm_and_rax(cg_sec, rt, 8); // andq %rax, rrt
                    } else {
                        int shift = 64 - bw;
                        asm_shl_imm(cg_sec, rt, 8, (uint8_t)(shift)); // shl $(uint8_t)(shift), rrt
                        asm_sar_imm(cg_sec, rt, 8, (uint8_t)(shift)); // sar $(uint8_t)(shift), rrt
                    }
                }
                free_reg(r2);
                VReg ret_reg = rt;
                free_reg(ra);
                return ret_reg;
            }
#endif
        }
        VReg r1 = gen_addr(node->lhs);
        VReg r2 = gen(node->rhs);
#ifdef ARCH_ARM64
        emit_store(node->lhs->ty, r2, r1, 0);
#else
        {
            int st_sz = node->lhs->ty->size;
            if (st_sz == 8) st_sz = 8;
            else if (st_sz == 4)
                st_sz = 4;
            else if (st_sz == 2)
                st_sz = 2;
            else
                st_sz = 1;
            asm_mov_reg_mem(cg_sec, r2, r1, st_sz); // movl/movq r2, (%r1)
        }
#endif
        free_reg(r1);
        return r2;
    }
    case ND_NEG: {
        VReg r = gen(node->lhs);
        if (is_flonum(node->ty)) {
#ifdef ARCH_ARM64
            asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
            asm_fneg_d0(cg_sec); // fneg d0, d0
            asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0
#else
            // Negate float: xor with sign bit
            asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq r, %xmm0
            // Use pxor with sign bit mask
            asm_mov_imm(cg_sec, r, 8, (long long)0x8000000000000000LL); // movabs $0x8000...0, r
            asm_movq_r_xmm(cg_sec, X86_XMM1, r); // movq r, %xmm1
            x86_xorpd(cg_sec, X86_XMM0, X86_XMM1); // xorpd %xmm1, %xmm0
            asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
#endif
        } else {
#ifdef ARCH_ARM64
            asm_neg(cg_sec, r, op_size(node->ty)); // neg x{r}
#else
            asm_neg(cg_sec, r, op_size(node->ty)); // neg rr
#endif
        }
        return r;
    }
    case ND_NOT: {
        VReg r = gen(node->lhs);
        if (is_flonum(node->lhs->ty)) {
#ifdef ARCH_ARM64
            asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
            asm_fmov_d1_xzr(cg_sec); // fmov d1, xzr
            asm_fcmp(cg_sec, 1); // fcmp d0, d1
            asm_cset(cg_sec, r, ARM64_EQ); // cset r, eq
            // csel r, r, wzr, vs  (clear if unordered/NaN)
            asm_csel_vs_zero(cg_sec, r, 4); // csel wr, wr, wzr, vs
#else
            asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq %s, %%xmm0
            x86_pxor(cg_sec, X86_XMM1, X86_XMM1); // xorpd %%xmm1, %%xmm1
            asm_ucomisd(cg_sec); // ucomisd %%xmm1, %%xmm0
            asm_setcc(cg_sec, X86_RCX, X86_NP); // setnp %%cl
            asm_setcc(cg_sec, X86_RAX, X86_E); // sete %%al
            x86_and_rr(cg_sec, 1, X86_RAX, X86_RCX); // andb %%cl, %%al
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1); // movzbl %%al, %s
#endif
        } else {
#ifdef ARCH_ARM64
            asm_cmp_zero(cg_sec, r, node->lhs->ty->size); // sete %%al
            asm_cset(cg_sec, r, ARM64_EQ); // andb %%cl, %%al
#else
            asm_cmp_zero(cg_sec, r, node->lhs->ty->size); // cmp $0, %s
            asm_setcc(cg_sec, X86_RAX, X86_E); // sete %%al
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1); // movzbl %%al, %s
#endif
        }
        return r;
    }
    case ND_ZERO_INIT: {
        // Zero-fill a local variable's stack memory
        LVar *var = node->lhs->var;
        if (!var || !var->is_local || var->ty->size <= 0) return -1;
        int c = ++rcc_label_count;
#ifdef ARCH_ARM64
        // Use x16/x17 scratch registers so we don't clobber allocatable x11/x9
        // which may hold live addresses (e.g. ND_ASSIGN copy-loop destination).
        if (var->offset <= 4095) {
            arm64_sub_imm(cg_sec, 1, ARM64_X16, ARM64_X29, var->offset, 0); // sub x16, x29, #var->offset
        } else {
            emit_mov_imm64(ARM64_X17, (uint64_t)var->offset);
            arm64_sub_reg(cg_sec, 1, ARM64_X16, ARM64_X29, ARM64_X17, ARM64_LSL, 0); // sub x16, x29, x17
        }
        if (var->ty->size <= 4095) {
            emit_mov_imm64(ARM64_X17, (uint64_t)var->ty->size); // mov x17, #size
        } else {
            emit_mov_imm64(ARM64_X17, (uint64_t)var->ty->size);
        }
        cg_def_label(format(".L.zero.%d", c));
        arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X17, 0, 0); // cmp x17, #0
        size_t zj1 = asm_jcc_label(cg_sec, ARM64_EQ); // b.eq .L.zero_end.c
        asm_fixup_add(cg_sec, zj1, format(".L.zero_end.%d", c), 1);
        arm64_sub_imm(cg_sec, 1, ARM64_X17, ARM64_X17, 1, 0); // sub x17, x17, #1
        arm64_str_reg(cg_sec, 0, ARM64_XZR, ARM64_X16, ARM64_X17, true, 0); // strb wzr, [x16, x17]
        size_t zj2 = asm_jmp_label(cg_sec); // b .L.zero.c
        asm_fixup_add(cg_sec, zj2, format(".L.zero.%d", c), 0); // strb wzr, [x16, x17]
        cg_def_label(format(".L.zero_end.%d", c)); // b .L.zero.%d
#else
        x86_mov_ri(cg_sec, 8, X86_RCX, var->ty->size); // movq $size, %rcx
        cg_def_label(format(".L.zero.%d", c));
        x86_cmp_ri(cg_sec, 8, X86_RCX, 0); // cmpq $0, %rcx
        size_t zj1 = asm_jcc_label(cg_sec, X86_E);
        asm_fixup_add(cg_sec, zj1, format(".L.zero_end.%d", c), 1);
        x86_mov_mi(cg_sec, 1, x86_mem_idx(X86_RBP, X86_RCX, 1, -var->offset - 1), 0); // movb $0, -off-1(%rbp,%rcx)
        x86_sub_ri(cg_sec, 8, X86_RCX, 1); // subq $1, %rcx
        size_t zj2 = asm_jmp_label(cg_sec);
        asm_fixup_add(cg_sec, zj2, format(".L.zero.%d", c), 0);
        cg_def_label(format(".L.zero_end.%d", c));
#endif
        return -1;
    }
    case ND_POST_INC:
    case ND_POST_DEC:
    case ND_PRE_INC:
    case ND_PRE_DEC: {
        bool is_pre = (node->kind == ND_PRE_INC || node->kind == ND_PRE_DEC);
        bool is_inc = (node->kind == ND_POST_INC || node->kind == ND_PRE_INC);
        VReg r = gen_addr(node->lhs);
        VReg r2 = alloc_reg();
        int sz = node->lhs->ty->size;
        // Handle bitfield post-increment/decrement with proper read-modify-write
        if (node->lhs->kind == ND_MEMBER && node->lhs->member &&
            node->lhs->member->bit_width > 0) {
            Member *mem = node->lhs->member;
            int bw = mem->bit_width;
            int bo = mem->bit_offset;
            int unit_sz = mem->bf_load_size ? mem->bf_load_size : mem->ty->size;
            unsigned long long mask = ((1ULL << bw) - 1) << bo;
            int eff_sz = unit_sz > 8 ? 8 : unit_sz;
#ifdef ARCH_ARM64
            // Load the full container word (unsigned)
            if (eff_sz == 1)
                asm_ldrb_uoff(cg_sec, r2, r, 0); // ldrb w{r2}, [x{r}]
            else if (eff_sz == 2)
                asm_ldrh_uoff(cg_sec, r2, r, 0); // ldrh w{r2}, [x{r}]
            else if (eff_sz == 4)
                asm_ldr_reg_off(cg_sec, r2, r, 4, 0); // ldr w{r2}, [x{r}]
            else
                asm_ldr_reg_off(cg_sec, r2, r, 8, 0); // ldr x{r2}, [x{r}]
            // Extract original bitfield value into r3 (for return)
            VReg r3 = alloc_reg();
            asm_mov_reg_reg(cg_sec, r3, r2, 8); // mov rr2 -> rr3
            if (bo > 0)
                asm_shr_imm(cg_sec, r3, 8, (uint8_t)(bo)); // lsr x{r3}, x{r3}, #bo
            int load_bits = eff_sz * 8;
            if (bw < load_bits) {
                if (mem->ty->is_unsigned || mem->ty->is_enum) {
                    emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
                    asm_and_reg_phy(cg_sec, r3, ARM64_X16, 8); // and x{r3}, x{r3}, x16
                } else {
                    int shift = 64 - bw;
                    asm_shl_imm(cg_sec, r3, 8, (uint8_t)(shift)); // lsl x{r3}, x{r3}, #shift
                    asm_sar_imm(cg_sec, r3, 8, (uint8_t)(shift)); // asr x{r3}, x{r3}, #shift
                }
            }
            // Clear the field bits in container word (r2)
            emit_mov_imm64(ARM64_X16, ~mask);
            asm_and_reg_phy(cg_sec, r2, ARM64_X16, 8); // and rr2, r16
            // Compute new field value in rn
            VReg rn = alloc_reg();
            asm_mov_reg_reg(cg_sec, rn, r3, 8); // mov rr3 -> rrn
            emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
            asm_and_reg_phy(cg_sec, rn, ARM64_X16, 8); // and rrn, r16
            if (is_inc)
                asm_add_imm(cg_sec, rn, 8, 1); // add $1, rrn
            else
                asm_sub_imm(cg_sec, rn, 8, 1); // sub $1, rrn
            emit_mov_imm64(ARM64_X16, (1ULL << bw) - 1);
            asm_and_reg_phy(cg_sec, rn, ARM64_X16, 8); // and rrn, r16 (wrap)
            // For prefix forms, sign/zero-extend the new field value
            VReg r4 = R_NONE;
            if (is_pre) {
                r4 = alloc_reg();
                asm_mov_reg_reg(cg_sec, r4, rn, 8); // mov rrn -> rr4
                if (bw < load_bits) {
                    if (!(mem->ty->is_unsigned || mem->ty->is_enum)) {
                        int shift = 64 - bw;
                        asm_shl_imm(cg_sec, r4, 8, (uint8_t)(shift)); // lsl x{r4}, x{r4}, #shift
                        asm_sar_imm(cg_sec, r4, 8, (uint8_t)(shift)); // asr x{r4}, x{r4}, #shift
                    }
                }
            }
            if (bo > 0)
                asm_shl_imm(cg_sec, rn, 8, (uint8_t)(bo)); // lsl x{rn}, x{rn}, #bo
            asm_or_reg_reg(cg_sec, r2, rn, 8); // orr x{r2}, x{r2}, x{rn}
            // Store
            if (eff_sz == 1)
                asm_strb_uoff(cg_sec, r2, r, 0); // strb w{r2}, [x{r}]
            else if (eff_sz == 2)
                asm_strh_uoff(cg_sec, r2, r, 0); // strh w{r2}, [x{r}]
            else if (eff_sz == 4)
                asm_str_reg_off(cg_sec, r2, r, 4, 0); // str w{r2}, [x{r}]
            else
                asm_str_reg_off(cg_sec, r2, r, 8, 0); // str x{r2}, [x{r}]
#else
            // Load the full container word (unsigned)
            if (eff_sz == 1) asm_movzx_mem_reg(cg_sec, r2, r, 4, 1); // movzbl (r), r2
            else if (eff_sz == 2)
                asm_movzx_mem_reg(cg_sec, r2, r, 4, 2); // movzwl (r), r2
            else if (eff_sz == 4)
                asm_mov_mem_reg(cg_sec, r2, r, 4); // movl (r), r2
            else
                asm_mov_mem_reg(cg_sec, r2, r, 8); // movq (r), r2
            // Extract original bitfield value into r3 (for return)
            VReg r3 = alloc_reg();
            asm_mov_reg_reg(cg_sec, r3, r2, 8); // mov rr2 -> rr3
            if (bo > 0)
                asm_shr_imm(cg_sec, r3, 8, (uint8_t)(bo)); // shr $(uint8_t)(bo), rr3
            int load_bits = eff_sz * 8;
            if (bw < load_bits) {
                if (mem->ty->is_unsigned || mem->ty->is_enum) {
                    unsigned long long m = (1ULL << bw) - 1;
                    asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(m)); // movabs $(uint64_t)(m), rX86_RAX
                    asm_and_rax(cg_sec, r3, 8); // andq %rax, rr3
                } else {
                    int shift = 64 - bw;
                    asm_shl_imm(cg_sec, r3, 8, (uint8_t)(shift)); // shl $(uint8_t)(shift), rr3
                    asm_sar_imm(cg_sec, r3, 8, (uint8_t)(shift)); // sar $(uint8_t)(shift), rr3
                }
            }
            // Clear the field bits in container word (r2)
            asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(~mask)); // movabs $(uint64_t)(~mask), rX86_RAX
            asm_and_rax(cg_sec, r2, 8); // andq %rax, rr2
            // Compute new field value in rn
            VReg rn = alloc_reg();
            asm_mov_reg_reg(cg_sec, rn, r3, 8); // mov rr3 -> rrn
            asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)((1ULL << bw) - 1)); // movabs mask, %rax
            asm_and_rax(cg_sec, rn, 8); // andq %rax, rrn (mask to bw bits)
            if (is_inc)
                asm_add_imm(cg_sec, rn, 8, 1); // addq $1, rn
            else
                asm_sub_imm(cg_sec, rn, 8, 1); // subq $1, rn
            asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)((1ULL << bw) - 1)); // movabs mask, %rax
            asm_and_rax(cg_sec, rn, 8); // andq %rax, rrn (wrap)
            // For prefix forms, sign/zero-extend the new field value
            VReg r4 = R_NONE;
            if (is_pre) {
                r4 = alloc_reg();
                asm_mov_reg_reg(cg_sec, r4, rn, 8); // mov rrn -> rr4
                if (bw < load_bits) {
                    if (!(mem->ty->is_unsigned || mem->ty->is_enum)) {
                        int shift = 64 - bw;
                        asm_shl_imm(cg_sec, r4, 8, (uint8_t)(shift)); // shl $(uint8_t)(shift), rr4
                        asm_sar_imm(cg_sec, r4, 8, (uint8_t)(shift)); // sar $(uint8_t)(shift), rr4
                    }
                }
            }
            if (bo > 0)
                asm_shl_imm(cg_sec, rn, 8, (uint8_t)(bo)); // shl $(uint8_t)(bo), rrn
            asm_or_reg_reg(cg_sec, rn, r2, 8); // or rrn, rr2
            // Store
            if (eff_sz == 1)
                asm_mov_reg_mem(cg_sec, rn, r, 1); // movb rn, (%r)
            else if (eff_sz == 2)
                asm_mov_reg_mem(cg_sec, rn, r, 2); // movw rn, (%r)
            else if (eff_sz == 4)
                asm_mov_reg_mem(cg_sec, rn, r, 4); // movl rn, (%r)
            else
                asm_mov_reg_mem(cg_sec, rn, r, 8); // movq rn, (%r)
#endif
            free_reg(rn);
            free_reg(r2);
            free_reg(r);
            if (is_pre) {
                free_reg(r3);
                return r4; // Return new value (prefix semantics)
            }
            if (r4 != R_NONE)
                free_reg(r4);
            return r3; // Return original value (post-increment semantics)
        }
#ifdef ARCH_ARM64
        // Load current value (correct load width for type)
        emit_load(node->lhs->ty, r2, r, 0);
        // Update in-place: load into temp, add/sub, store back
        VReg r3 = alloc_reg();
        emit_load(node->lhs->ty, r3, r, 0);
        if (is_flonum(node->lhs->ty)) {
            // Float inc/dec: load r3 bits into d0/s0, add/sub 1.0, store back.
            int id = add_float_literal(1.0, sz);
            int tmp = alloc_reg();
            emit_adrp_add(tmp, format(".LF%d", id));
            if (sz == 4) {
                asm_fmov_i2f(cg_sec, 0, r3, 0); // fmov s0, w{r3}
                asm_ldr_fp(cg_sec, 1, tmp, 4); // ldr s1, [x{tmp}]
                (is_inc ? asm_fadd(cg_sec, 0) : asm_fsub(cg_sec, 0)); // fadd/fsub s0, s0, s1
                asm_fmov_f2i(cg_sec, r3, 0, 0); // fmov w{r3}, s0
            } else {
                asm_fmov_i2f(cg_sec, 0, r3, 1); // fmov d0, x{r3}
                asm_ldr_fp(cg_sec, 1, tmp, 8); // ldr d1, [x{tmp}]
                (is_inc ? asm_fadd(cg_sec, 1) : asm_fsub(cg_sec, 1)); // fadd/fsub d0, d0, d1
                asm_fmov_f2i(cg_sec, r3, 0, 1); // fmov x{r3}, d0
            }
            free_reg(tmp);
        } else {
            int delta = 1;
            if (node->lhs->ty->kind == TY_PTR || node->lhs->ty->kind == TY_ARRAY)
                delta = node->lhs->ty->base->size;
            if (is_inc)
                asm_add_imm(cg_sec, r3, sz, delta); // add r3, r3, #delta
            else
                asm_sub_imm(cg_sec, r3, sz, delta); // sub r3, r3, #delta
            // Narrow unsigned/signed types: extend so that comparisons against
            // the returned register (e.g. --z > 0, unsigned char z) see the
            // correctly truncated value, not the raw 32-bit add/sub result.
            if (node->lhs->ty->size < 4) {
                if (node->lhs->ty->is_unsigned)
                    zero_extend_to(r3, node->lhs->ty->size, 4);
                else
                    sign_extend_to(r3, node->lhs->ty->size, 4);
            }
        }
        emit_store(node->lhs->ty, r3, r, 0);
        // r3 already holds the new value; r2 holds the original. Return
        // whichever the prefix/postfix semantics require and free the other.
        if (is_pre) {
            free_reg(r2);
            r2 = r3;
        } else {
            free_reg(r3);
        }
#else
        // Sign/zero-extend narrow loads into the full register — rcc keeps
        // sub-int values extended in their register, and a plain narrow `mov`
        // would leave garbage in the upper bits, breaking direct uses of the
        // result (e.g. `x-- < 0` or `--x < 0`).
        emit_load(node->lhs->ty, r2, r, 0); // mov/movsx/movzx (%r), r2
        bool is_float = is_flonum(node->lhs->ty);
        if (is_float) {
            int id = add_float_literal(1.0, sz);
            asm_movq_r_xmm(cg_sec, X86_XMM0, r2); // movq r2, %xmm0
            VReg tmp = alloc_reg();
            asm_lea_rip_reg(cg_sec, tmp, format(".LF%d", id)); // lea .LFid(%rip), tmp
            if (sz == 4) {
                x86_movss_rm(cg_sec, X86_XMM1, x86_mem(REG(tmp), 0)); // movss (tmp), %xmm1
                if (is_inc) x86_addss(cg_sec, X86_XMM0, X86_XMM1); // addss %xmm1, %xmm0
                else
                    x86_subss(cg_sec, X86_XMM0, X86_XMM1); // subss %xmm1, %xmm0
            } else {
                x86_movsd_rm(cg_sec, X86_XMM1, x86_mem(REG(tmp), 0)); // movsd (tmp), %xmm1
                if (is_inc) x86_addsd(cg_sec, X86_XMM0, X86_XMM1); // addsd %xmm1, %xmm0
                else
                    x86_subsd(cg_sec, X86_XMM0, X86_XMM1); // subsd %xmm1, %xmm0
            }
            free_reg(tmp);
            if (sz == 4)
                x86_movss_mr(cg_sec, x86_mem(REG(r), 0), X86_XMM0); // movss %xmm0, (%r)
            else
                x86_movsd_mr(cg_sec, x86_mem(REG(r), 0), X86_XMM0); // movsd %xmm0, (%r)
        } else {
            int delta = 1;
            if (node->lhs->ty->kind == TY_PTR || node->lhs->ty->kind == TY_ARRAY)
                delta = node->lhs->ty->base->size;
            if (is_inc)
                x86_add_mi(cg_sec, sz, x86_mem(REG(r), 0), delta); // add%c $delta, (%r)
            else
                x86_sub_mi(cg_sec, sz, x86_mem(REG(r), 0), delta); // sub%c $delta, (%r)
        }
        // The new value lives only in memory now (computed in-place); for
        // prefix semantics, reload it (sign/zero-extended) into r2.
        if (is_pre)
            emit_load(node->lhs->ty, r2, r, 0);
#endif
        free_reg(r);
        return r2;
    }
    case ND_MEMBER: {
        VReg r = gen_addr(node);
        if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA) {
            return r; // array/VLA decays to pointer
        }
        Type *load_ty = (node->member && node->member->bit_width > 0) ? node->member->ty : node->ty;
        if (is_flonum(load_ty)) {
#ifdef ARCH_ARM64
            if (load_ty->size == 4) {
                asm_ldr_fp(cg_sec, 0, r, 4); // ldr s0, [x{r}]
                asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0
            } else {
                asm_ldr_fp(cg_sec, 0, r, 8); // ldr d0, [x{r}]
            }
            asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0
#else
            if (load_ty->size == 4) {
                x86_movss_rm(cg_sec, X86_XMM0, x86_mem(REG(r), 0)); // movss (%s), %%xmm0
                asm_cvtss2sd(cg_sec); // cvtss2sd %%xmm0, %%xmm0
            } else {
                x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(REG(r), 0)); // movq %%xmm0, %s
            }
            asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq x{r}, xmm0
#endif
        } else if (node->member && node->member->bit_width > 0 && node->member->bf_load_size) {
            int ls = node->member->bf_load_size;
            int bw = node->member->bit_width;
            int bo = node->member->bit_offset;
            int eff_ls = ls > 8 ? 8 : ls;
            VReg r_addr = -1;
            if (ls > 8 && bo + bw > 64) {
                r_addr = alloc_reg();
#ifdef ARCH_ARM64
                asm_mov_reg_reg(cg_sec, r_addr, r, 8); // mov rr -> rr_addr
#else
                asm_mov_reg_reg(cg_sec, r_addr, r, 8); // mov rr -> rr_addr
#endif
            }
#ifdef ARCH_ARM64
            if (eff_ls == 1)
                asm_ldrb_uoff(cg_sec, r, r, 0); // ldrb w{r}, [x{r}]
            else if (eff_ls == 2)
                asm_ldrh_uoff(cg_sec, r, r, 0); // ldrh w{r}, [x{r}]
            else if (eff_ls == 4)
                asm_ldr_reg_off(cg_sec, r, r, 4, 0); // ldr w{r}, [x{r}]
            else
                asm_ldr_reg_off(cg_sec, r, r, 8, 0); // ldr x{r}, [x{r}]
            if (bo > 0)
                asm_shr_imm(cg_sec, r, 8, (uint8_t)(bo)); // lsr x{r}, x{r}, #bo
            if (r_addr >= 0) {
                int overflow = bo + bw - 64;
                int tmp = alloc_reg();
                asm_ldrb_uoff(cg_sec, tmp, r_addr, 8); // ldrb w{tmp}, [x{r_addr}, #8]
                int ovf_mask = (1 << overflow) - 1;
                asm_and_imm(cg_sec, tmp, 4, ovf_mask); // and w{tmp}, w{tmp}, #ovf_mask
                asm_shl_imm(cg_sec, tmp, 8, (uint8_t)(64 - bo)); // lsl x{tmp}, x{tmp}, #(64-bo)
                asm_or_reg_reg(cg_sec, r, tmp, 8); // orr x{r}, x{r}, x{tmp}
                free_reg(tmp);
                free_reg(r_addr);
            }
            int load_bits = ls * 8;
            if (bw < load_bits) {
                if (node->member->ty->is_unsigned || node->member->ty->is_enum) {
                    unsigned long long mask = (1ULL << bw) - 1;
                    emit_mov_imm64(ARM64_X16, mask);
                    asm_and_reg_phy(cg_sec, r, ARM64_X16, 8); // and rr, r16
                } else {
                    int shift = 64 - bw;
                    asm_shl_imm(cg_sec, r, 8, (uint8_t)(shift)); // lsl x{r}, x{r}, #shift
                    asm_sar_imm(cg_sec, r, 8, (uint8_t)(shift)); // asr x{r}, x{r}, #shift
                }
            }
            return r;
#else
            {
                asm_mov_mem_reg(cg_sec, r, r, eff_ls); // movl/movq (%r), rr
            }
            if (bo > 0)
                asm_shr_imm(cg_sec, r, 8, (uint8_t)(bo)); // shr $(uint8_t)(bo), rr
            if (r_addr >= 0) {
                int overflow = bo + bw - 64;
                int tmp = alloc_reg();
                asm_movzx_base_off_reg(cg_sec, tmp, r_addr, 8, 8, 1); // movzbl 8(r_addr), tmp
                unsigned long long mask = (1ULL << overflow) - 1;
                asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(mask)); // movabs $(uint64_t)(mask), rX86_RAX
                asm_and_rax(cg_sec, tmp, 8); // andq %rax, rtmp
                asm_shl_imm(cg_sec, tmp, 8, (uint8_t)(64 - bo)); // shl $(uint8_t)(64 - bo), rtmp
                asm_or_reg_reg(cg_sec, tmp, r, 8); // or rtmp, rr
                free_reg(tmp);
                free_reg(r_addr);
            }
            int load_bits = ls * 8;
            if (bw < load_bits) {
                if (node->member->ty->is_unsigned || node->member->ty->is_enum) {
                    unsigned long long mask = (1ULL << bw) - 1;
                    asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(mask)); // movabs $(uint64_t)(mask), rX86_RAX
                    asm_and_rax(cg_sec, r, 8); // andq %rax, rr
                } else {
                    int shift = 64 - bw;
                    asm_shl_imm(cg_sec, r, 8, (uint8_t)(shift)); // shl $(uint8_t)(shift), rr
                    asm_sar_imm(cg_sec, r, 8, (uint8_t)(shift)); // sar $(uint8_t)(shift), rr
                }
            }
            return r;
#endif
        } else {
#ifdef ARCH_ARM64
            emit_load(load_ty, r, r, 0);
#else
            emit_load(load_ty, r, r, 0);
#endif
        }
        if (node->member && node->member->bit_width > 0) {
            int bw = node->member->bit_width;
            int bo = node->member->bit_offset;
            int load_bits = node->member->bf_load_size
                ? node->member->bf_load_size * 8
                : node->member->ty->size * 8;
#ifdef ARCH_ARM64
            if (bo > 0)
                asm_shr_imm(cg_sec, r, 8, (uint8_t)(bo)); // lsr x{r}, x{r}, #bo
            if (bw < load_bits) {
                if (node->member->ty->is_unsigned || node->member->ty->is_enum) {
                    unsigned long long mask = (1ULL << bw) - 1;
                    emit_mov_imm64(ARM64_X16, mask);
                    asm_and_reg_phy(cg_sec, r, ARM64_X16, 8); // and x{r}, x{r}, x16
                } else {
                    int shift = 64 - bw;
                    asm_shl_imm(cg_sec, r, 8, (uint8_t)(shift)); // lsl x{r}, x{r}, #shift
                    asm_sar_imm(cg_sec, r, 8, (uint8_t)(shift)); // asr x{r}, x{r}, #shift
                }
            }
#else
            if (bo > 0)
                asm_shr_imm(cg_sec, r, 8, (uint8_t)(bo)); // shr $(uint8_t)(bo), rr
            if (bw < load_bits) {
                if (node->member->ty->is_unsigned || node->member->ty->is_enum) {
                    unsigned long long mask = (1ULL << bw) - 1;
                    asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(mask)); // movabs $(uint64_t)(mask), rX86_RAX
                    asm_and_rax(cg_sec, r, 8); // andq %rax, rr
                } else {
                    int shift = 64 - bw;
                    asm_shl_imm(cg_sec, r, 8, (uint8_t)(shift)); // shl $(uint8_t)(shift), rr
                    asm_sar_imm(cg_sec, r, 8, (uint8_t)(shift)); // sar $(uint8_t)(shift), rr
                }
            }
#endif
        }
        return r;
    }
    case ND_ADDR:
        return gen_addr(node->lhs);
    case ND_CAST: {
        VReg r = gen(node->lhs);
        Type *from = node->lhs->ty;
        Type *to = node->ty;
        if (is_flonum(from) && is_integer(to)) {
#ifdef ARCH_ARM64
            asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
            {
                if (to->is_unsigned)
                    asm_fcvtzu(cg_sec, r, to->size); // fcvtzu w/x{r}, d0
                else
                    asm_cvttsd2si(cg_sec, r, to->size); // fcvtzs w/x{r}, d0
            }
#else
            asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq r, %xmm0
            if (to->size == 8 && to->is_unsigned) {
                int c = ++rcc_label_count;
                asm_movabs_phy(cg_sec, X86_RAX, 0x43e0000000000000ULL); // movabs $0x43e0000000000000, %rax (2^63 as double)
                x86_movq_r_xmm(cg_sec, X86_XMM1, X86_RAX); // movq %rax, %xmm1
                asm_ucomisd(cg_sec); // comisd %xmm1, %xmm0
                {
                    size_t o = asm_jcc_label(cg_sec, X86_B); // jb .L.ucast.c
                    asm_fixup_add(cg_sec, o, format(".L.ucast.%d", c), 1);
                }
                asm_subsd(cg_sec); // subsd %xmm1, %xmm0
                asm_cvttsd2si(cg_sec, r, 8); // cvttsd2si %xmm0, rr
                x86_movabs(cg_sec, X86_RCX, 1ULL << 63); // movabs $0x8000000000000000, %rcx
                x86_or_rr(cg_sec, 8, REG(r), X86_RCX); // orq %rcx, rr
                {
                    size_t o = asm_jmp_label(cg_sec); // jmp .L.ucast_end.c
                    asm_fixup_add(cg_sec, o, format(".L.ucast_end.%d", c), 0);
                }
                cg_def_label(format(".L.ucast.%d", c)); // .L.ucast.c:
                asm_cvttsd2si(cg_sec, r, 8); // cvttsd2si %xmm0, rr
                cg_def_label(format(".L.ucast_end.%d", c)); // .L.ucast_end.c:
            } else if (to->size <= 4 && to->is_unsigned) {
                // float-to-unsigned-int: cvttsd2si is signed, so handle [2^31, 2^32) range.
                int c = ++rcc_label_count;
                asm_movabs_phy(cg_sec, X86_RAX, 0x41e0000000000000ULL); // movabs $0x41e0000000000000, %rax (2^31 as double)
                x86_movq_r_xmm(cg_sec, X86_XMM1, X86_RAX); // movq %rax, %xmm1
                asm_ucomisd(cg_sec); // comisd %xmm1, %xmm0
                {
                    size_t o = asm_jcc_label(cg_sec, X86_B); // jb .L.ucast32.c
                    asm_fixup_add(cg_sec, o, format(".L.ucast32.%d", c), 1);
                }
                asm_subsd(cg_sec); // subsd %xmm1, %xmm0
                asm_cvttsd2si(cg_sec, r, 4); // cvttsd2si %xmm0, rr
                asm_add_imm(cg_sec, r, 4, (int)(1U << 31)); // addl $0x80000000, rr
                {
                    size_t o = asm_jmp_label(cg_sec); // jmp .L.ucast32_end.c
                    asm_fixup_add(cg_sec, o, format(".L.ucast32_end.%d", c), 0);
                }
                cg_def_label(format(".L.ucast32.%d", c)); // .L.ucast32.c:
                asm_cvttsd2si(cg_sec, r, 4); // cvttsd2si %xmm0, rr
                cg_def_label(format(".L.ucast32_end.%d", c)); // .L.ucast32_end.c:
            } else if (to->size <= 4 && !to->is_unsigned) {
                int c = ++rcc_label_count;
                asm_cvttsd2si(cg_sec, r, 4); // cvttsd2si xmm0, rr
                asm_cmp_imm(cg_sec, r, 4, (int32_t)0x80000000); // cmpl $0x80000000, rr
                {
                    size_t o = asm_jcc_label(cg_sec, X86_NE); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.u2f.end.%d", c), 1);
                }
                x86_xorpd(cg_sec, X86_XMM1, X86_XMM1); // xorpd %xmm1, %xmm1
                asm_ucomisd(cg_sec); // ucomisd %xmm1, %xmm0
                {
                    size_t o = asm_jcc_label(cg_sec, X86_B); // jb .L.sat_end.c
                    asm_fixup_add(cg_sec, o, format(".L.u2f.end.%d", c), 1);
                }
                asm_mov_imm(cg_sec, r, 4, 0x7fffffff); // movl $0x7fffffff, rr
                cg_def_label(format(".L.saturate.%d", c)); // (saturate value)
                cg_def_label(format(".L.u2f.end.%d", c));
            } else {
                asm_cvttsd2si(cg_sec, r, to->size); // cvttsd2si %xmm0, rr
            }
#endif
        } else if (is_integer(from) && is_flonum(to)) {
#ifdef ARCH_ARM64
            int sf = (from->size == 8) ? 1 : 0;
            if (from->is_unsigned)
                asm_ucvtf(cg_sec, 0, r, sf); // ucvtf d0, w/x{r}
            else
                asm_scvtf(cg_sec, 0, r, sf); // scvtf d0, w/x{r}
            if (to->kind == TY_FLOAT) {
                asm_fcvt(cg_sec, 0, 1, 0, 0); // fcvt s0, d0 (double→single, round to float)
                asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0 (single→double, back to GP-friendly)
            }
            asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0 (store as int bits)
#else
            if (from->is_unsigned && from->size == 8) {
                int c = ++rcc_label_count;
                asm_test(cg_sec, REG(r), REG(r), 8); // test r, r
                {
                    size_t o = asm_jcc_label(cg_sec, X86_S); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.u2f.high.%d", c), 1);
                }
                asm_cvtsi2sd(cg_sec, r, 8); // cvtsi2sd r, xmm0
                {
                    size_t o = asm_jmp_label(cg_sec); // cvttsd2si %%xmm0, %s
                    asm_fixup_add(cg_sec, o, format(".L.u2f.end.%d", c), 0);
                }
                cg_def_label(format(".L.u2f.high.%d", c)); // ucvtf d0, %s
                x86_mov_rr(cg_sec, 8, X86_RCX, REG(r)); // movq r, %rcx
                x86_shr_ri(cg_sec, 8, X86_RCX, 1); // shrq $1, %rcx
                x86_cvtsi2sd(cg_sec, 8, X86_XMM0, X86_RCX); // cvtsi2sd %rcx, %xmm0
                x86_addsd(cg_sec, X86_XMM0, X86_XMM0); // addsd %xmm0, %xmm0 (double it)
                cg_def_label(format(".L.u2f.end.%d", c)); // .L.u2f.end.%d:
            } else if (from->is_unsigned && from->size == 4) {
                asm_cvtsi2sd(cg_sec, r, 8); // cvtsi2sd rr, %xmm0
            } else {
                asm_cvtsi2sd(cg_sec, r, from->size); // cvtsi2sd rr, %xmm0
            }
            if (to->kind == TY_FLOAT) {
                asm_cvtsd2ss(cg_sec); // jmp .L.u2f.end.%d
                asm_cvtss2sd(cg_sec); // .L.u2f.high.%d:
            }
            asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
#endif
        } else if (is_flonum(from) && is_flonum(to)) {
#ifdef ARCH_ARM64
            if (to->kind == TY_FLOAT && from->kind != TY_FLOAT) {
                asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
                asm_fcvt(cg_sec, 0, 1, 0, 0); // fcvt s0, d0 (opc=0=single dest, ftype=1=double src)
                asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0 (opc=1=d, ftype=0=s)
                asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0
            }
#else
            if (to->kind == TY_FLOAT && from->kind != TY_FLOAT) {
                asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq r, %xmm0
                asm_cvtsd2ss(cg_sec); // cvtsd2ss %xmm0, %xmm0
                asm_cvtss2sd(cg_sec); // cvtss2sd %xmm0, %xmm0
                asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
            }
#endif
        } else if ((is_flonum(from) || is_integer(from)) && is_complex(to)) {
            int alloc = (to->size + 7) & ~7;
            fn_struct_ret_off += alloc;
            if (fn_struct_ret_off > fn_struct_ret_total) fn_struct_ret_total = fn_struct_ret_off;
            int result_off = current_fn_stack_size + fn_struct_ret_off;
            int result = alloc_reg();
#ifdef ARCH_ARM64
            asm_sub_reg_fp_imm(cg_sec, result, result_off); // sub result, x29, #result_off
#else
            asm_lea_rbp_reg(cg_sec, result, 8, result_off);
#endif
            emit_scalar_to_complex(r, from, to->base, result);
            return result;
        } else if (is_complex(from) && (is_flonum(to) || is_integer(to))) {
            // complex → float/int: load real part from address
            if (is_flonum(to)) {
                if (is_integer(from->base)) {
                    // int complex → float: convert via cvtsi2ss/cvtsi2sd
#ifndef ARCH_ARM64
                    if (from->base->size <= 4) {
                        x86_mov_rm(cg_sec, 4, REG(r), x86_mem(REG(r), 0)); // movl (r), r
                        asm_cvtsi2ss(cg_sec, REG(r), 4); // cvtsi2ss r32, %xmm0
                    } else {
                        x86_mov_rm(cg_sec, 4, REG(r), x86_mem(REG(r), 0)); // movl (r), r
                        asm_cvtsi2sd(cg_sec, r, 4); // cvtsi2sd r32, %xmm0
                    }
                    if (to->size == 4)
                        asm_cvtsd2ss(cg_sec);
                    asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
#else
                    // ARM64: load real part, convert to float
                    if (from->base->size <= 4)
                        arm64_ldr_uoff(cg_sec, 2, ARM64_X0, REG(r), 0); // ldr w0, [r]
                    else
                        arm64_ldr_uoff(cg_sec, 3, ARM64_X0, REG(r), 0); // ldr x0, [r]
                    if (from->is_unsigned)
                        arm64_ucvtf(cg_sec, (from->base->size <= 4 ? 0 : 1), 1, ARM64_D0, ARM64_X0); // ucvtf d0, w0/x0
                    else
                        arm64_scvtf(cg_sec, (from->base->size <= 4 ? 0 : 1), 1, ARM64_D0, ARM64_X0); // scvtf d0, w0/x0
                    if (to->size == 4)
                        arm64_fcvt(cg_sec, 1, 2, ARM64_S0, ARM64_D0); // fcvt s0, d0
                    arm64_fmov_f2i(cg_sec, (to->size == 4 ? 0 : 1), REG(r), (to->size == 4 ? ARM64_S0 : ARM64_D0)); // fmov r, s0/d0
#endif
                } else {
#ifndef ARCH_ARM64
                    if (from->base->size == 4 && to->size == 8) {
                        // float complex → double: promote via cvtss2sd
                        asm_mov_fp_rm(cg_sec, 4, X86_XMM0, x86_mem(REG(r), 0)); // movss (r), %xmm0
                        asm_cvtss2sd(cg_sec);
                        asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
                    } else if (to->size == 4) {
                        asm_mov_fp_rm(cg_sec, 4, X86_XMM0, x86_mem(REG(r), 0)); // movss (r), %xmm0
                        asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
                    } else {
                        asm_mov_fp_rm(cg_sec, 8, X86_XMM0, x86_mem(REG(r), 0)); // movsd (r), %xmm0
                        asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
                    }
#else
                    if (from->base->size == 4 && to->size == 8) {
                        arm64_ldr_fp(cg_sec, 2, 0, REG(r), 0); // ldr s0, [r]
                        arm64_fcvt(cg_sec, 1, 2, ARM64_D0, ARM64_S0); // fcvt d0, s0
                        arm64_fmov_f2i(cg_sec, 1, REG(r), ARM64_D0); // fmov r, d0
                    } else if (to->size == 4) {
                        arm64_ldr_fp(cg_sec, 2, 0, REG(r), 0); // ldr s0, [r]
                        arm64_fmov_f2i(cg_sec, 0, REG(r), ARM64_S0); // fmov r, s0
                    } else {
                        arm64_ldr_fp(cg_sec, 3, 0, REG(r), 0); // ldr d0, [r]
                        arm64_fmov_f2i(cg_sec, 1, REG(r), ARM64_D0); // fmov r, d0
                    }
#endif
                }
            } else {
                // integer: convert from float or load directly
                if (is_flonum(from->base)) {
#ifndef ARCH_ARM64
                    if (from->base->size == 4) {
                        asm_mov_fp_rm(cg_sec, 4, X86_XMM0, x86_mem(REG(r), 0)); // movss (r), %xmm0
                        asm_cvttsd2si(cg_sec, r, 4); // cvttss2si %xmm0, r
                    } else {
                        asm_mov_fp_rm(cg_sec, 8, X86_XMM0, x86_mem(REG(r), 0)); // movsd (r), %xmm0
                        asm_cvttsd2si(cg_sec, r, 8); // cvttsd2si %xmm0, r
                    }
#else
                    if (from->base->size == 4) {
                        arm64_ldr_fp(cg_sec, 2, 0, REG(r), 0); // ldr s0, [r]
                        if (to->is_unsigned)
                            arm64_fcvtzu(cg_sec, 0, 2, REG(r), ARM64_S0); // fcvtzu w{r}, s0
                        else
                            arm64_fcvtzs(cg_sec, 0, 2, REG(r), ARM64_S0); // fcvtzs w{r}, s0
                    } else {
                        arm64_ldr_fp(cg_sec, 3, 0, REG(r), 0); // ldr d0, [r]
                        if (to->is_unsigned)
                            arm64_fcvtzu(cg_sec, 1, 3, REG(r), ARM64_D0); // fcvtzu x{r}, d0
                        else
                            arm64_fcvtzs(cg_sec, 1, 3, REG(r), ARM64_D0); // fcvtzs x{r}, d0
                    }
#endif
                } else {
#ifdef ARCH_ARM64
                    emit_load(to, r, r, 0);
#else
                    emit_load(to, r, r, 0);
#endif
                }
            }

        } else if (to->size == 1) {
#ifdef ARCH_ARM64
            if (to->is_unsigned)
                asm_and_imm(cg_sec, r, 4, 0xff); // and w{r}, w{r}, #0xff
            else
                asm_movsx(cg_sec, r, r, 4, 1); // sxtb w{r}, w{r}
#else
            if (to->is_unsigned)
                asm_movzx(cg_sec, r, r, 4, 1); // movzbl r, r
            else
                asm_movsx(cg_sec, r, r, 4, 1); // movsbl r, r
#endif
        } else if (to->size == 2) {
#ifdef ARCH_ARM64
            if (to->is_unsigned)
                asm_and_imm(cg_sec, r, 4, 0xffff); // and w{r}, w{r}, #0xffff
            else
                asm_movsx(cg_sec, r, r, 4, 2); // sxth w{r}, w{r}
#else
            if (to->is_unsigned)
                asm_movzx(cg_sec, r, r, 4, 2); // movzx4->r rr, rr
            else
                asm_movsx(cg_sec, r, r, 4, 2); // cvtsd2ss %%xmm0, %%xmm0
#endif
        } else if (to->size == 4 && from->size == 8) {
            asm_mov_reg_reg(cg_sec, r, r, 4); // cvtss2sd %%xmm0, %%xmm0
        } else if (to->size == 8 && from->size < 8) {
            if (from->kind == TY_ARRAY || from->kind == TY_VLA) {
                // Array/VLA decayed to pointer; already 8-byte address in reg
            } else if (from->is_unsigned)
                zero_extend_to(r, from->size, 8);
            else
                sign_extend_to(r, from->size, 8);
        }
        return r;
    }
    case ND_BITNOT: {
#ifndef ARCH_ARM64
        if (is_complex(node->ty)) {
            int complex_sz = node->ty->size, base_sz = node->ty->base->size;
            VReg addr = gen_addr(node->lhs), need_free = 0;
            if (addr < 0) {
                need_free = 1;
                addr = alloc_reg();
                asm_sub_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // subq $N, %rsp
                x86_mov_rr(cg_sec, 8, REG(addr), X86_RSP); // movq %rsp, addr
                VReg v = gen(node->lhs);
                if (is_complex(node->lhs->ty)) {
                    asm_mov_reg_reg(cg_sec, 0, v, 8);
                    asm_mov_reg_reg(cg_sec, addr, 0, 8);
                    if (complex_sz > 8) {
                        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(v), base_sz)); // movq base_sz(v), %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), base_sz), X86_RAX); // movq %rax, base_sz(addr)
                    }
                } else {
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(addr), 0), REG(v)); // movq v, (addr)
                    if (complex_sz > 8) asm_movq_zero(cg_sec, addr);
                }
                free_reg(v);
            }
            int alloc = (complex_sz + 7) & ~7;
            fn_struct_ret_off += alloc;
            if (fn_struct_ret_off > fn_struct_ret_total) fn_struct_ret_total = fn_struct_ret_off;
            int result_off = current_fn_stack_size + fn_struct_ret_off;
            VReg result = alloc_reg();
            asm_lea_rbp_reg(cg_sec, result, 8, result_off);
            if (is_flonum(node->ty->base)) {
                asm_mov_fp_rm(cg_sec, base_sz, X86_XMM0, x86_mem(REG(addr), 0)); // movss/sd (addr), %xmm0
                asm_mov_fp_rm(cg_sec, base_sz, X86_XMM1, x86_mem(REG(addr), base_sz)); // movss/sd base_sz(addr), %xmm1
                asm_movabs_phy(cg_sec, X86_RAX, 0x8000000000000000ULL); // movabs $0x8000000000000000, %rax
                x86_movq_r_xmm(cg_sec, X86_XMM2, X86_RAX); // movq %rax, %xmm2
                if (base_sz == 4) x86_xorps(cg_sec, X86_XMM1, X86_XMM2); // xorps %xmm2, %xmm1
                else
                    x86_xorpd(cg_sec, X86_XMM1, X86_XMM2); // xorpd %xmm2, %xmm1
                asm_mov_fp_mr(cg_sec, base_sz, x86_mem(REG(result), 0), X86_XMM0); // movss/sd %xmm0, (result)
                asm_mov_fp_mr(cg_sec, base_sz, x86_mem(REG(result), base_sz), X86_XMM1); // movss/sd %xmm1, base_sz(result)
            } else {
                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(addr), 0)); // mov (addr), %rax
                x86_mov_rm(cg_sec, 8, X86_RDX, x86_mem(REG(addr), base_sz)); // mov base_sz(addr), %rdx
                x86_neg_r(cg_sec, 8, X86_RDX); // neg %rdx
                x86_mov_mr(cg_sec, 8, x86_mem(REG(result), 0), X86_RAX); // mov %rax, (result)
                x86_mov_mr(cg_sec, 8, x86_mem(REG(result), base_sz), X86_RDX); // mov %rdx, base_sz(result)
            }
            free_reg(addr);
            if (need_free) asm_add_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // addq $N, %rsp
            return result;
        }
#endif
        VReg r = gen(node->lhs);
#ifdef ARCH_ARM64
        asm_not(cg_sec, r, 8); // not rr
#else
        asm_not(cg_sec, r, node->ty->size); // not rr
#endif
        return r;
    }
    case ND_STR: {
        VReg r = alloc_reg();
#ifdef ARCH_ARM64
        emit_adrp_add(r, format(".LC%d", node->str_id));
#else
        asm_lea_rip_reg(cg_sec, r, format(".LC%d", node->str_id)); // lea rip, rr
#endif
        return r;
    }
    case ND_DEREF: {
        if (node->ty->kind == TY_FUNC || node->ty->kind == TY_ARRAY || node->ty->kind == TY_VLA)
            return gen(node->lhs);
        if (node->lhs->kind == ND_ADD && node->lhs->lhs->ty &&
            node->lhs->lhs->ty->base && !is_flonum(node->ty) &&
            node->lhs->lhs->kind == ND_LVAR) {
            VReg idx = gen(node->lhs->rhs);
            VReg base = gen(node->lhs->lhs);
#ifdef ARCH_ARM64
            asm_add_reg_reg(cg_sec, base, idx, 8); // add base, base, idx
            free_reg(idx);
            emit_load(node->ty, base, base, 0); // load from [base]
            return base;
#else
            asm_add_reg_reg(cg_sec, idx, base, 8); // add idx, base
            free_reg(base);
            emit_load(node->ty, idx, idx, 0); // load from (idx)
            return idx;
#endif
        }
        VReg r = gen(node->lhs);
        if (is_flonum(node->ty)) {
#ifdef ARCH_ARM64
            if (node->ty->size == 4) {
                asm_ldr_fp(cg_sec, 0, r, 4); // asm_ldr_fp(0, r, 4)
                asm_fcvt(cg_sec, 1, 0, 0, 0); // fcvt d0, s0
            } else {
                asm_ldr_fp(cg_sec, 0, r, 8); // asm_ldr_fp(0, r, 8)
            }
            asm_fmov_f2i(cg_sec, r, 0, 1); // asm_fmov_f2i(r, 0, 1)
#else
            if (node->ty->size == 4) {
                asm_ldr_fp(cg_sec, 0, r, 4); // movss (%r), %xmm0
                asm_cvtss2sd(cg_sec); // cvtss2sd %xmm0, %xmm0
            } else {
                asm_ldr_fp(cg_sec, 0, r, 8); // movsd (%r), %xmm0
            }
            asm_movq_xmm_r(cg_sec, r, X86_XMM0); // asm_movq_xmm_r(r, X86_XMM0)
#endif
        } else {
#ifdef ARCH_ARM64
            emit_load(node->ty, r, r, 0);
#else
            emit_load(node->ty, r, r, 0);
#endif
        }
        return r;
    }
    case ND_RETURN: {
        if (node->lhs) {
            // Returning a _Complex value from a function whose declared
            // return type is a plain scalar (GNU extension: discards the
            // imaginary part). Wrap in an ND_CAST to the return type so the
            // scalar-return path below loads just the real component,
            // instead of mistaking this for a complex-returning function
            // and copying raw bytes into __retbuf.
            Type *ret_ty = current_fn_def->ty->return_ty;
            if (node->lhs->ty && is_complex(node->lhs->ty) && ret_ty &&
                !is_complex(ret_ty) && ret_ty->kind != TY_STRUCT && ret_ty->kind != TY_UNION) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->lhs;
                cast->ty = ret_ty;
                cast->tok = node->lhs->tok;
                node->lhs = cast;
            }
            // Implicit conversion from the expression type to the function's
            // declared return type. The parser does not insert this cast, so
            // we must perform it here to get correct sign/zero extension.
            if (node->lhs->ty && ret_ty &&
                ((is_integer(node->lhs->ty) && is_integer(ret_ty)) ||
                 (is_flonum(node->lhs->ty) && is_flonum(ret_ty))) &&
                (node->lhs->ty->size != ret_ty->size || node->lhs->ty->is_unsigned != ret_ty->is_unsigned)) {
                Node *cast = arena_alloc(sizeof(Node));
                cast->kind = ND_CAST;
                cast->lhs = node->lhs;
                cast->ty = ret_ty;
                cast->tok = node->lhs->tok;
                node->lhs = cast;
            }
            if (node->lhs->ty && node->lhs->ty->kind == TY_INT128) {
                // Return int128 in rax:rdx (lo:hi) on x86-64, x0:x1 on ARM64
                int src = gen_int128(node->lhs);
#ifdef ARCH_ARM64
                arm64_ldr_uoff(cg_sec, 3, ARM64_X0, REG(src), 0); // ldr x0, [src]
                arm64_ldr_uoff(cg_sec, 3, ARM64_X1, REG(src), 1); // ldr x1, [src, #8]
                free_reg(src);
#else
                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(src), 0)); // movq (src), %rax
                x86_mov_rm(cg_sec, 8, X86_RDX, x86_mem(REG(src), 8)); // movq 8(src), %rdx
                free_reg(src);
#endif
#ifdef _WIN32
            } else if (node->lhs->ty && is_complex(node->lhs->ty) && node->lhs->ty->size <= 8 && current_fn_def && current_fn_def->ty &&
                       is_complex(current_fn_def->ty->return_ty) && current_fn_def->ty->return_ty->size <= 8) {
                // Win64: an 8-byte _Complex return value is returned by value
                // in RAX — there is no hidden return pointer for it (see the
                // param_index computation in the prologue).
                int src = gen_addr(node->lhs);
                if (src < 0)
                    src = gen(node->lhs);
                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(src), 0)); // movq (src), %rax
                free_reg(src);
#endif
            } else if (node->lhs->ty && (node->lhs->ty->kind == TY_STRUCT || node->lhs->ty->kind == TY_UNION || is_complex(node->lhs->ty)) && current_fn_def && current_fn_def->ty && (current_fn_def->ty->return_ty->kind == TY_STRUCT || current_fn_def->ty->return_ty->kind == TY_UNION || is_complex(current_fn_def->ty->return_ty))) {
                int src = gen_addr(node->lhs);
                if (src < 0)
                    src = gen(node->lhs);
                int c = ++rcc_label_count;
                int retbuf_offset = 0;
                for (LVar *var = current_fn_def->locals; var; var = var->next) {
                    if (var->name && var->name == kw_retbuf) {
                        retbuf_offset = var->offset;
                        break;
                    }
                }
#ifdef ARCH_ARM64
                // Save src to x12 before loading retbuf into x11
                asm_mov_phy_reg(cg_sec, ARM64_X12, src, 1); // mov x12, src
                if (retbuf_offset <= 4095)
                    asm_ldur_phy(cg_sec, ARM64_X11, ARM64_X29, 3, -retbuf_offset); // ldur x11, [x29, #-retbuf_offset]
                else {
                    emit_mov_imm64(ARM64_X16, (uint64_t)retbuf_offset); // mov x16, #retbuf_offset
                    asm_sub_x11_fp_x16(cg_sec); // sub x11, x29, x16
                }
                emit_mov_imm64(ARM64_X9, (uint64_t)node->lhs->ty->size); // mov x9, #size
                cg_def_label(format(".L.retcopy.%d", c));
                arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X9, 0, 0); // cmp x9, #0
                {
                    size_t _cj = asm_jcc_label(cg_sec, ARM64_EQ);
                    asm_fixup_add(cg_sec, _cj, format(".L.retcopy_end.%d", c), 1);
                }
                arm64_sub_imm(cg_sec, 1, ARM64_X9, ARM64_X9, 1, 0); // sub x9, x9, #1
                asm_ldrb_w16_x9_phy(cg_sec, ARM64_X12); // ldrb w16, [x12, x9]
                asm_strb_w16_x9_phy(cg_sec, ARM64_X11); // strb w16, [x11, x9]
                {
                    size_t _jmp = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, _jmp, format(".L.retcopy.%d", c), 0);
                }
                cg_def_label(format(".L.retcopy_end.%d", c));
                asm_mov_x0_reg(cg_sec, ARM64_X11); // mov x0, x11
#else
                asm_mov_rbp(cg_sec, X86_R9, 8, retbuf_offset); // mov [rbp-retbuf_offset], X86_R9
                x86_mov_ri(cg_sec, 8, X86_RCX, node->lhs->ty->size); // movq $size, %%rcx
                cg_def_label(format(".L.retcopy.%d", c));
                x86_cmp_ri(cg_sec, 8, X86_RCX, 0); // cmp $0, rcx
                {
                    size_t o = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, o, format(".L.retcopy_end.%d", c), 1);
                }
                x86_mov_rm(cg_sec, 1, X86_RAX, x86_mem_idx(REG(src), X86_RCX, 1, -1)); // movb -1(src, rcx), %%al
                x86_mov_mr(cg_sec, 1, x86_mem_idx(X86_R9, X86_RCX, 1, -1), X86_RAX); // movb %%al, -1(r9, rcx)
                x86_sub_ri(cg_sec, 8, X86_RCX, 1); // dec rcx
                {
                    size_t o = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, o, format(".L.retcopy.%d", c), 0);
                }
                cg_def_label(format(".L.retcopy_end.%d", c));
                x86_mov_rr(cg_sec, 8, X86_RAX, X86_R9); // movq %%r9, %%rax
                free_reg(src);
#endif
            } else if (node->lhs->ty && is_complex(node->lhs->ty)) {
                // Complex expression returned as scalar: extract real part
                int addr = gen_addr(node->lhs);
                if (addr < 0) addr = gen(node->lhs);
                Type *ret_ty = current_fn_def->ty->return_ty;
#ifdef ARCH_ARM64
                arm64_ldr_uoff(cg_sec, 2, ARM64_X0, REG(addr), 0); // ldr w0, [addr]
#else
                x86_mov_rm(cg_sec, 4, X86_RAX, x86_mem(REG(addr), 0)); // movl (addr), %eax
#endif
                if (ret_ty && ret_ty->size == 1) {
#ifdef ARCH_ARM64
                    arm64_uxtb(cg_sec, ARM64_X0, ARM64_X0); // uxtb w0, w0
#else
                    asm_movzx(cg_sec, 0, 0, 4, 1); // movzbl %al, %eax
#endif
                }
                free_reg(addr);
            } else {
                VReg r = gen(node->lhs);
                Type *ret_ty = current_fn_def->ty->return_ty;
                if (ret_ty && is_flonum(ret_ty)) {
                    if (node->lhs->ty && is_flonum(node->lhs->ty)) {
#ifdef ARCH_ARM64
                        asm_fmov_i2f(cg_sec, 0, r, 1); // fmov d0, x{r}
                        if (ret_ty->kind == TY_FLOAT)
                            asm_fcvt(cg_sec, 0, 1, 0, 0); // fcvt s0, d0 (opc=0=single dest)
#else
                        asm_movq_r_xmm(cg_sec, X86_XMM0, r); // movq r, %xmm0
                        if (ret_ty->kind == TY_FLOAT)
                            asm_cvtsd2ss(cg_sec); // cvtsd2ss %xmm0, %xmm0
#endif
                    } else if (ret_ty->size == 4) {
#ifdef ARCH_ARM64
                        {
                            int src_sf = (node->lhs->ty && node->lhs->ty->size >= 8) ? 1 : 0;
                            if (node->lhs->ty && node->lhs->ty->is_unsigned)
                                arm64_ucvtf(cg_sec, src_sf, 0, 0, REG(r)); // ucvtf s0, reg(r, size)
                            else
                                arm64_scvtf(cg_sec, src_sf, 0, 0, REG(r)); // scvtf s0, reg(r, size)
                        }
#else
                        {
                            int src_sz = node->lhs->ty ? node->lhs->ty->size : 4;
                            bool src_u = node->lhs->ty && node->lhs->ty->is_unsigned;
                            if (src_u && src_sz == 8) {
                                // unsigned long long → float: handle high bit
                                int c = ++rcc_label_count;
                                asm_test(cg_sec, REG(r), REG(r), 8); // test r, r
                                {
                                    size_t o = asm_jcc_label(cg_sec, X86_S); // jcc label
                                    asm_fixup_add(cg_sec, o, format(".L.u2f.high.%d", c), 1);
                                }
                                asm_cvtsi2ss(cg_sec, REG(r), 8); // cvtsi2ss rr, %xmm0
                                {
                                    size_t o = asm_jmp_label(cg_sec); // jmp .L.u2f.end.%d
                                    asm_fixup_add(cg_sec, o, format(".L.u2f.end.%d", c), 0);
                                }
                                cg_def_label(format(".L.u2f.high.%d", c)); // .L.u2f.high.%d:
                                x86_mov_rr(cg_sec, 8, X86_RCX, REG(r)); // movq r, %rcx
                                x86_shr_ri(cg_sec, 8, X86_RCX, 1); // shrq $1, %rcx
                                asm_cvtsi2ss(cg_sec, X86_RCX, 8); // cvtsi2ss %rcx, %xmm0
                                x86_addss(cg_sec, X86_XMM0, X86_XMM0); // addss %xmm0, %xmm0 (double it)
                                cg_def_label(format(".L.u2f.end.%d", c)); // .L.u2f.end.%d:
                            } else if (src_u && src_sz <= 4) {
                                // unsigned int/short/char → float: zero-extend to 64-bit,
                                // then cvtsi2ss with 64-bit reg (value is non-negative 64-bit int)
                                asm_cvtsi2ss(cg_sec, REG(r), 8); // cvtsi2ss rr, %xmm0
                            } else {
                                int cssz = (src_sz >= 8) ? 8 : (src_sz < 4 ? 4 : src_sz);
                                asm_cvtsi2ss(cg_sec, REG(r), cssz); // cvtsi2ss rr, %xmm0
                            }
                        }
#endif
                    } else {
#ifdef ARCH_ARM64
                        {
                            int src_sf = (node->lhs->ty && node->lhs->ty->size >= 8) ? 1 : 0;
                            if (node->lhs->ty && node->lhs->ty->is_unsigned)
                                asm_ucvtf(cg_sec, 0, r, src_sf); // ucvtf d0, w/x{r}
                            else
                                asm_scvtf(cg_sec, 0, r, src_sf); // scvtf d0, w/x{r}
                        }
#else
                        {
                            int src_sz = node->lhs->ty ? node->lhs->ty->size : 8;
                            bool src_u = node->lhs->ty && node->lhs->ty->is_unsigned;
                            if (src_u && src_sz == 8) {
                                // unsigned long long → double: handle high bit
                                int c = ++rcc_label_count;
                                asm_test(cg_sec, REG(r), REG(r), 8); // test r, r
                                {
                                    size_t o = asm_jcc_label(cg_sec, X86_S); // jcc label
                                    asm_fixup_add(cg_sec, o, format(".L.u2f.high.%d", c), 1);
                                }
                                asm_cvtsi2sd(cg_sec, r, 8); // cvtsi2sd rr, xmm0
                                {
                                    size_t o = asm_jmp_label(cg_sec); // jmp .L.u2f.end.%d
                                    asm_fixup_add(cg_sec, o, format(".L.u2f.end.%d", c), 0);
                                }
                                cg_def_label(format(".L.u2f.high.%d", c)); // .L.u2f.high.%d:
                                x86_mov_rr(cg_sec, 8, X86_RCX, REG(r)); // movq r, %rcx
                                x86_shr_ri(cg_sec, 8, X86_RCX, 1); // shrq $1, %rcx
                                x86_cvtsi2sd(cg_sec, 8, X86_XMM0, X86_RCX); // cvtsi2sd %rcx, %xmm0
                                x86_addsd(cg_sec, X86_XMM0, X86_XMM0); // addsd %xmm0, %xmm0 (double it)
                                cg_def_label(format(".L.u2f.end.%d", c)); // .L.u2f.end.%d:
                            } else if (src_u && src_sz <= 4) {
                                // unsigned int/short/char → double: zero-extend to 64-bit
                                asm_cvtsi2sd(cg_sec, r, 8); // cvtsi2sd rr, %xmm0
                            } else {
                                int cssz = (src_sz >= 8) ? 8 : (src_sz < 4 ? 4 : src_sz);
                                asm_cvtsi2sd(cg_sec, r, cssz); // cvtsi2sd rr, %xmm0
                            }
                        }
#endif
                    }
                } else if (node->lhs->ty && is_flonum(node->lhs->ty)) {
                    // Float expression returned as integer: convert from xmm0/d0
                    int sz = ret_ty ? ret_ty->size : 8;
#ifdef ARCH_ARM64
                    {
                        bool ret_unsigned = ret_ty && ret_ty->is_unsigned;
                        if (ret_unsigned)
                            asm_fcvtzu(cg_sec, r, sz); // fcvtzu w/x{r}, d0
                        else
                            asm_fcvtzs(cg_sec, r, sz); // fcvtzs w/x{r}, d0
                    }
                    if (sz >= 8)
                        asm_mov_x0_reg(cg_sec, REG(r)); // mov x0, reg64[r]
                    else if (ret_ty && ret_ty->is_unsigned)
                        asm_mov_w0_reg32(cg_sec, REG(r)); // mov w0, reg32[r]
                    else
                        asm_sxtw(cg_sec, ARM64_X0, REG(r)); // sxtw x0, reg32[r]
#else
                    asm_cvttsd2si(cg_sec, r, sz); // cvttsd2si xmm0, rr
                    asm_mov_reg_to_retval(cg_sec, r, 8); // movq rr, %rax
#endif
                } else {
#ifdef ARCH_ARM64
                    // Truncate return value to match function return type width
                    if (ret_ty && ret_ty->size < 4) {
                        if (ret_ty->is_unsigned)
                            asm_and_imm(cg_sec, r, 4, (1 << (ret_ty->size * 8)) - 1);
                        else if (ret_ty->size == 1)
                            asm_movsx(cg_sec, r, r, 4, 1); // movsx4->r rr, rr
                        else
                            asm_movsx(cg_sec, r, r, 4, 2); // movsx4->r rr, rr
                    }
                    asm_mov_x0_reg(cg_sec, REG(r)); // mov x0, x{r}
#else
                    // Truncate BEFORE moving to %rax so return register has correct value
                    if (ret_ty && ret_ty->size < 4 && ret_ty->is_unsigned)
                        asm_and_imm(cg_sec, r, 4, (1 << (ret_ty->size * 8)) - 1); // and mask, rr
                    asm_mov_reg_to_retval(cg_sec, r, 8); // movq rr, %rax
#endif
                }
                free_reg(r);
            }
        }
        emit_cleanup_range(node->cleanup_begin, node->cleanup_end);
        size_t ret_off = asm_jmp_label(cg_sec); /* jmp/b .L.return.%s */ /* andl $%d, %eax\n */
        char *ret_lbl = format(".L.return.%s", current_fn_def->name);
        asm_fixup_add(cg_sec, ret_off, ret_lbl, 0); // fixup add for forward branch
        return -1;
    }
    case ND_NULL:
        return -1;
    case ND_COMMA:
    case ND_CHAIN: {
        VReg r1 = gen(node->lhs);
        if (r1 != -1) free_reg(r1);
        return gen(node->rhs);
    }
    case ND_ALLOCA:
    case ND_ALLOCA_ZINIT: {
#ifdef ARCH_ARM64
        if (node->kind == ND_ALLOCA_ZINIT && node->lhs && node->lhs->kind == ND_NUM && node->lhs->val == 0) {
            arm64_load_from_fp_minus(node->var->offset, ARM64_X16);
            asm_mov_sp_reg(cg_sec, ARM64_X16); // mov sp, x16
            return -1;
        }
        VReg r = gen(node->lhs);
        // Save current SP into VLA save slot
        asm_mov_reg_sp(cg_sec, ARM64_X16); // mov x16, sp
        arm64_store_to_fp_minus(node->var->offset);
        // Round size up to 16-byte alignment, keep in x16 (scratch, not in pool)
        asm_add_imm(cg_sec, r, 8, 15); // add r, r, #15
        asm_and64_imm(cg_sec, r, ~(uint64_t)15); // and r, r, #-16
        asm_mov_x16_vreg(cg_sec, r); // mov x16, x{r}
        free_reg(r);
        asm_sub_sp_sp_x16_v2(cg_sec); // sub sp, sp, x16
        if (node->kind == ND_ALLOCA_ZINIT) {
            cg_def_label(format(".L.alloca.zero.%d", rcc_label_count));
            // subs x16, x16, #8  (sets flags)
            asm_subs_x16_imm(cg_sec, 8); // subs x16, x16, #8
            {
                size_t _cj = asm_jcc_label(cg_sec, ARM64_LT);
                asm_fixup_add(cg_sec, _cj, format(".L.alloca.done.%d", rcc_label_count), 1);
            }
            // str xzr, [sp, x16]
            asm_str_xzr_sp_x16(cg_sec); // str xzr, [sp, x16]
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.alloca.zero.%d", rcc_label_count), 0);
            }
        } else {
            cg_def_label(format(".L.alloca.probe.%d", rcc_label_count));
            // subs x16, x16, #4096
            asm_subs_x16_imm(cg_sec, 4096); // subs x16, x16, #4096
            {
                size_t _cj = asm_jcc_label(cg_sec, ARM64_LT);
                asm_fixup_add(cg_sec, _cj, format(".L.alloca.done.%d", rcc_label_count), 1);
            }
            // ldrb w17, [sp, x16]
            asm_ldrb_w17_sp_x16(cg_sec); // ldrb w17, [sp, x16]
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.alloca.probe.%d", rcc_label_count), 0);
            }
        }
        cg_def_label(format(".L.alloca.done.%d", rcc_label_count));
        rcc_label_count++;
        // Save VLA data pointer
        asm_mov_reg_sp(cg_sec, ARM64_X16); // mov x16, sp
        arm64_store_to_fp_minus(node->var->offset - 8);
#else
        if (node->kind == ND_ALLOCA_ZINIT && node->lhs && node->lhs->kind == ND_NUM && node->lhs->val == 0) {
            asm_mov_rbp(cg_sec, X86_RSP, 8, node->var->offset); // mov -offset(%rbp), %rsp
            return -1;
        }
        VReg r = gen(node->lhs);
        asm_mov_phyreg_rbp(cg_sec, X86_RSP, 8, node->var->offset); // movq %rsp, -offset(%rbp)
        x86_mov_rr(cg_sec, 8, X86_RAX, REG(r)); // movq %s, %rax
        free_reg(r);
        x86_mov_rr(cg_sec, 8, X86_RCX, X86_RSP); // movq %rsp, %rcx
        x86_sub_rr(cg_sec, 8, X86_RSP, X86_RAX); // subq %rax, %rsp
        x86_and_ri(cg_sec, 8, X86_RSP, -16); // andq $-16, %rsp
        x86_sub_rr(cg_sec, 8, X86_RCX, X86_RSP); // subq %rsp, %rcx
        if (node->kind == ND_ALLOCA_ZINIT) {
            x86_pxor(cg_sec, X86_XMM0, X86_XMM0); // pxor %%xmm0, %%xmm0
            cg_def_label(format(".L.alloca.zero.%d", rcc_label_count));
            x86_sub_ri(cg_sec, 8, X86_RCX, 16); // subq $16, %rcx
            {
                size_t o = asm_jcc_label(cg_sec, X86_S); // js .L.alloca.done.%d
                asm_fixup_add(cg_sec, o, format(".L.alloca.done.%d", rcc_label_count), 1);
            }
            {
                X86Mem m = {X86_RSP, X86_RCX, 1, 0};
                x86_movaps_mr(cg_sec, m, X86_XMM0); // movaps %%xmm0, (%rsp,%rcx)
            }
            {
                size_t o = asm_jmp_label(cg_sec); // jmp .L.alloca.zero.%d
                asm_fixup_add(cg_sec, o, format(".L.alloca.zero.%d", rcc_label_count), 0);
            }
        } else {
            cg_def_label(format(".L.alloca.probe.%d", rcc_label_count));
            x86_sub_ri(cg_sec, 8, X86_RCX, 4096); // subq $4096, %rcx
            {
                size_t o = asm_jcc_label(cg_sec, X86_S); // js .L.alloca.done.%d
                asm_fixup_add(cg_sec, o, format(".L.alloca.done.%d", rcc_label_count), 1);
            }
            {
                X86Mem m = {X86_RSP, X86_RCX, 1, 0};
                x86_or_mi(cg_sec, 1, m, 0); // orb $0, (%rsp,%rcx)
            }
            {
                size_t o = asm_jmp_label(cg_sec); // jmp .L.alloca.probe.%d
                asm_fixup_add(cg_sec, o, format(".L.alloca.probe.%d", rcc_label_count), 0);
            }
        }
        cg_def_label(format(".L.alloca.done.%d", rcc_label_count));
        rcc_label_count++;
        if (node->var)
            asm_mov_phyreg_rbp(cg_sec, X86_RSP, 8, node->var->offset - 8); // movq %rsp, -offset(%rbp)
        else
            x86_mov_rr(cg_sec, 8, X86_RAX, X86_RSP); // movq %rsp, %rax
#endif
        return -1;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            VReg r = gen(n);
            if (r != -1) free_reg(r);
        }
        return -1;
    case ND_STMT_EXPR: {
        VReg result = -1;
        for (Node *n = node->body; n; n = n->next) {
            if (node->stmt_expr_result && n->kind == ND_EXPR_STMT && n->lhs == node->stmt_expr_result) {
                result = gen(node->stmt_expr_result);
            } else {
                VReg r = gen(n);
                if (r != -1)
                    free_reg(r);
            }
        }
        if (result == -1) {
            result = alloc_reg();
#ifdef ARCH_ARM64
            asm_movq_zero(cg_sec, result); // xor rresult, rresult
#else
            asm_movl_zero(cg_sec, result); // xor rresult, rresult
#endif
        }
        return result;
    }
    case ND_EXPR_STMT: {
        VReg r = gen(node->lhs);
        if (r != -1) free_reg(r);
        return -1;
    }
    case ND_FUNCALL: {
        return gen_funcall(node, -1);
    }
    case ND_LOGAND: {
        int c = ++rcc_label_count;
#ifdef ARCH_ARM64
        VReg r = alloc_reg();
        VReg lhs = gen(node->lhs);
        asm_cmp_zero(cg_sec, lhs, node->lhs->ty->size);
        asm_movq_zero(cg_sec, r);
        {
            size_t o = asm_jcc_label(cg_sec, ARM64_EQ);
            asm_fixup_add(cg_sec, o, format(".L.end.%d", c), 1);
        }
        free_reg(lhs);
        VReg rhs = gen(node->rhs);
        asm_cmp_zero(cg_sec, rhs, node->rhs->ty->size);
        asm_cset(cg_sec, r, ARM64_NE);
        free_reg(rhs);
        cg_def_label(format(".L.end.%d", c));
#else
        VReg lhs = gen(node->lhs);
        asm_cmp_zero(cg_sec, lhs, node->lhs->ty->size);
        asm_mov_rbp_imm(cg_sec, 1, spill_logand, 0); // movb $0, -spill_logand(%rbp)
        {
            size_t o = asm_jcc_label(cg_sec, X86_E);
            asm_fixup_add(cg_sec, o, format(".L.end.%d", c), 1);
        }
        free_reg(lhs);
        VReg rhs = gen(node->rhs);
        asm_cmp_zero(cg_sec, rhs, node->rhs->ty->size);
        asm_setcc(cg_sec, X86_RAX, X86_NE); // setne %al
        asm_mov_phyreg_rbp(cg_sec, X86_RAX, 1, spill_logand); // movb %al, -spill_logand(%rbp)
        free_reg(rhs);
        cg_def_label(format(".L.end.%d", c));
        VReg r = alloc_reg();
        asm_movzx_rbp_reg(cg_sec, r, 4, 1, spill_logand); // movzbl -spill_logand(%rbp), rr
#endif
        return r;
    }
    case ND_LOGOR: {
        int c = ++rcc_label_count;
#ifdef ARCH_ARM64
        VReg r = alloc_reg();
        VReg lhs = gen(node->lhs);
        asm_cmp_zero(cg_sec, lhs, node->lhs->ty->size); // cmp $0, rlhs
        asm_mov_imm(cg_sec, r, 4, 1); // mov r, #1
        size_t o = asm_jcc_label(cg_sec, ARM64_NE);
        asm_fixup_add(cg_sec, o, format(".L.end.%d", c), 1);
        free_reg(lhs);
        VReg rhs = gen(node->rhs);
        asm_cmp_zero(cg_sec, rhs, node->rhs->ty->size); // cmp $0, rrhs
        asm_cset(cg_sec, r, ARM64_NE); // cset rr
        free_reg(rhs);
        cg_def_label(format(".L.end.%d", c));
#else
        VReg lhs = gen(node->lhs);
        asm_cmp_zero(cg_sec, lhs, node->lhs->ty->size); // cmp $0, rlhs
        asm_mov_rbp_imm(cg_sec, 1, spill_logand, 1); // movb $1, -spill_logand(%rbp)
        size_t o = asm_jcc_label(cg_sec, X86_NE); // jne .L.end.%d
        asm_fixup_add(cg_sec, o, format(".L.end.%d", c), 1);
        free_reg(lhs);
        VReg rhs = gen(node->rhs);
        asm_cmp_zero(cg_sec, rhs, node->rhs->ty->size); // cmp $0, rrhs
        asm_setcc(cg_sec, X86_RAX, X86_NE); // setne %al
        asm_mov_phyreg_rbp(cg_sec, X86_RAX, 1, spill_logand); // movb %al, -spill_logand(%rbp)
        free_reg(rhs);
        cg_def_label(format(".L.end.%d", c));
        VReg r = alloc_reg();
        asm_movzx_rbp_reg(cg_sec, r, 4, 1, spill_logand); // movzbl -spill_logand(%rbp), rr
#endif
        return r;
    }
    /*
    case ND_LOGOR: {
        int c = ++rcc_label_count;
        const char *true_label = format(".L.logor_true.%d", c);
        const char *end_label = format(".L.logor_end.%d", c);
#ifdef ARCH_ARM64
        VReg r = alloc_reg();
        VReg lhs = gen(node->lhs);
        asm_cmp_zero(cg_sec, lhs, node->lhs->ty->size); // cmp $0, rlhs
        asm_mov_imm(cg_sec, r, 4, 1); // mov $1, rr
        size_t jo1 = asm_jcc_label(cg_sec, ARM64_NE); // jcc label
        asm_fixup_add(cg_sec, jo1, true_label, 1); // fixup add for forward branch
        free_reg(lhs);
        VReg rhs = gen(node->rhs);
        asm_cmp_zero(cg_sec, rhs, node->rhs->ty->size); // cmp $0, rrhs
        asm_cset(cg_sec, r, ARM64_NE); // cset rr
        size_t jo2 = asm_jmp_label(cg_sec); // jne .L.end.%d
        asm_fixup_add(cg_sec, jo2, end_label, 0); // fixup add for forward branch
        cg_def_label(true_label); // setne %%al
#else
        VReg r = alloc_reg();
        VReg lhs = gen(node->lhs);
        asm_cmp_zero(cg_sec, lhs, node->lhs->ty->size); // cmp $0, rlhs
        asm_mov_imm(cg_sec, r, 4, 1); // mov $1, rr
        size_t jo1 = asm_jcc_label(cg_sec, X86_NE); // jcc label
        asm_fixup_add(cg_sec, jo1, true_label, 1); // fixup add for forward branch
        free_reg(lhs);
        VReg rhs = gen(node->rhs);
        asm_cmp_zero(cg_sec, rhs, node->rhs->ty->size); // cmp $0, rrhs
        asm_setcc(cg_sec, REG(r), X86_NE); // setcc rr
        asm_movzx(cg_sec, r, r, 4, 1); // movzx4->r rr, rr
        size_t jo2 = asm_jmp_label(cg_sec); // .L.else.%d:
        asm_fixup_add(cg_sec, jo2, end_label, 0); // fixup add for forward branch
        cg_def_label(true_label); // cmp $0, %s
#endif
        free_reg(rhs);
        cg_def_label(end_label); // je .L.else.%d
        return r;
    }
*/
    case ND_COND: {
        int c = ++rcc_label_count;
        const char *else_label = format(".L.cond_else.%d", c);
        const char *end_label = format(".L.cond_end.%d", c);
        VReg r = alloc_reg();
        int cond = gen(node->cond);
#ifdef ARCH_ARM64
        asm_cmp_zero(cg_sec, cond, node->cond->ty->size); // cmp $0, rcond
        size_t cj1 = asm_jcc_label(cg_sec, ARM64_EQ); // jcc label
        asm_fixup_add(cg_sec, cj1, else_label, 1); // fixup add for forward branch
        free_reg(cond);
        VReg then_r = gen(node->then);
        asm_mov_reg_reg(cg_sec, r, then_r, 8); // mov rthen_r -> rr
        free_reg(then_r);
        size_t cj2 = asm_jmp_label(cg_sec); // .L.end.%d:
        asm_fixup_add(cg_sec, cj2, end_label, 0); // fixup add for forward branch
        cg_def_label(else_label); // mov %s, #0
        VReg else_r = gen(node->els);
        asm_mov_reg_reg(cg_sec, r, else_r, 8); // mov relse_r -> rr
        free_reg(else_r);
#else
        asm_cmp_zero(cg_sec, cond, node->cond->ty->size); // cmp $0, rcond
        size_t cj1 = asm_jcc_label(cg_sec, X86_E); // jcc label
        asm_fixup_add(cg_sec, cj1, else_label, 1); // fixup add for forward branch
        free_reg(cond);
        VReg then_r = gen(node->then);
        asm_mov_reg_reg(cg_sec, r, then_r, 8); // mov rthen_r -> rr
        free_reg(then_r);
        size_t cj2 = asm_jmp_label(cg_sec); // je .L.end.%d
        asm_fixup_add(cg_sec, cj2, end_label, 0); // fixup add for forward branch
        cg_def_label(else_label); // setne %%al
        VReg else_r = gen(node->els);
        asm_mov_reg_reg(cg_sec, r, else_r, 8); // mov relse_r -> rr
        free_reg(else_r);
#endif
        cg_def_label(end_label); // .L.end.%d:
        return r;
    }
    case ND_IF: {
        // Fold constant integer conditions to avoid dead code emission,
        // but keep blocks that contain labels (may be targets of goto).
        if (node->cond->kind == ND_NUM) {
            // Recursively check if a node tree contains any label or goto
            bool has_label = false;
            {
                Node *stack[512];
                int sp = 0;
                for (Node *n = node->then; n && sp < 512; n = n->next)
                    stack[sp++] = n;
                if (node->els)
                    for (Node *n = node->els; n && sp < 512; n = n->next)
                        stack[sp++] = n;
                while (sp > 0 && !has_label) {
                    Node *n = stack[--sp];
                    if (n->kind == ND_LABEL || n->kind == ND_GOTO || n->kind == ND_GOTO_IND ||
                        n->kind == ND_CASE || n->kind == ND_LABEL_VAL) {
                        has_label = true;
                    } else if (n->kind == ND_FOR || n->kind == ND_DO || n->kind == ND_IF) {
                        if (n->then && sp < 512) stack[sp++] = n->then;
                        if (n->cond && sp < 512) stack[sp++] = n->cond;
                        if (n->kind == ND_FOR) {
                            if (n->body && sp < 512) stack[sp++] = n->body;
                            if (n->init && sp < 512) stack[sp++] = n->init;
                            if (n->inc && sp < 512) stack[sp++] = n->inc;
                        }
                        if (n->els && sp < 512) stack[sp++] = n->els;
                    } else if (n->kind == ND_BLOCK) {
                        for (Node *c = n->body; c && sp < 512; c = c->next)
                            stack[sp++] = c;
                    }
                }
            }
            if (!has_label) {
                if (node->cond->val) {
                    VReg r = gen(node->then);
                    if (r != -1) free_reg(r);
                } else if (node->els) {
                    VReg r = gen(node->els);
                    if (r != -1) free_reg(r);
                }
                return -1;
            }
            // if(0) with labels: skip dead non-label nodes before the
            // first label (Duff device pattern where labels are reachable
            // via switch/goto).  Plain if(1) with labels falls through.
            if (!node->cond->val) {
                // Find outermost block body
                Node *list = node->then;
                while (list && list->kind == ND_BLOCK && list->body &&
                       list->body->kind == ND_BLOCK && !list->body->next)
                    list = list->body;
                if (list && list->kind == ND_BLOCK)
                    list = list->body;
                // Skip to first label/case
                Node *n = list;
                while (n && n->kind != ND_LABEL && n->kind != ND_CASE && n->kind != ND_LABEL_VAL)
                    n = n->next;
                // If the first label is a case (Duff), skip dead code before it.
                // Otherwise generate full body to preserve goto labels.
                if (n && n->kind == ND_CASE) {
                    for (; n; n = n->next) {
                        VReg r = gen(n);
                        if (r != -1) free_reg(r);
                    }
                    return -1;
                }
            }
        }
        int c = ++rcc_label_count;
        const char *end_label = format(".L.end.%d", c);
        const char *else_label = format(".L.else.%d", c);

        if (node->els) {
            gen_cond_branch_inv(node->cond, else_label);
            VReg r1 = gen(node->then);
            if (r1 != -1) free_reg(r1);
#ifdef ARCH_ARM64
            size_t if_jmp_off = asm_jmp_label(cg_sec); // b %s
            asm_fixup_add(cg_sec, if_jmp_off, end_label, 0); // fixup add for forward branch
#else
            size_t if_jmp_off = asm_jmp_label(cg_sec); // %s:
            asm_fixup_add(cg_sec, if_jmp_off, end_label, 0); // fixup add for forward branch
#endif
            cg_def_label(else_label); // %s:
            VReg r2 = gen(node->els);
            if (r2 != -1) free_reg(r2);
            cg_def_label(end_label); // b %s
        } else {
            gen_cond_branch_inv(node->cond, end_label);
            VReg r1 = gen(node->then);
            if (r1 != -1) free_reg(r1);
            cg_def_label(end_label); // jmp %s
        }
        return -1;
    }
    case ND_FOR: {
        int c = ++rcc_label_count;
        const char *begin_label = format(".L.begin.%d", c);
        const char *end_label = format(".L.end.%d", c);
        const char *cont_label = format(".L.continue.%d", c);

        if (node->init) {
            VReg r = gen(node->init);
            if (r != -1) free_reg(r);
        }
        break_stack[ctrl_depth] = c;
        continue_stack[ctrl_depth] = c;
        ctrl_depth++;
        cg_def_label(begin_label); // %s:
        if (node->cond) {
            gen_cond_branch_inv(node->cond, end_label);
        }
        VReg r_then = gen(node->then);
        if (r_then != -1) free_reg(r_then);
        cg_def_label(cont_label); // %s:
        if (node->inc) {
            VReg r_inc = gen(node->inc);
            if (r_inc != -1) free_reg(r_inc);
        }
#ifdef ARCH_ARM64
        size_t for_jmp_off = asm_jmp_label(cg_sec); // b %s
        asm_fixup_add(cg_sec, for_jmp_off, begin_label, 0); // fixup add for forward branch
#else
        size_t for_jmp_off = asm_jmp_label(cg_sec); // %s:
        asm_fixup_add(cg_sec, for_jmp_off, begin_label, 0); // fixup add for forward branch
#endif
        cg_def_label(end_label); // .L.continue.%d:
        ctrl_depth--;
        return -1;
    }
    case ND_DO: {
        int c = ++rcc_label_count;
        const char *begin_label = format(".L.begin.%d", c);
        const char *end_label = format(".L.end.%d", c);
        const char *cont_label = format(".L.continue.%d", c);
        cg_def_label(begin_label); // cmp %s, #0
        break_stack[ctrl_depth] = c;
        continue_stack[ctrl_depth] = c;
        ctrl_depth++;
        VReg r_then = gen(node->then);
        if (r_then != -1) free_reg(r_then);
        cg_def_label(cont_label); // b.ne .L.begin.%d
        VReg r = gen(node->cond);
#ifdef ARCH_ARM64
        asm_cmp_zero(cg_sec, r, node->cond->ty->size); // cmp $0, rr
        free_reg(r);
        size_t do_j = asm_jcc_label(cg_sec, ARM64_NE); // jcc label
        asm_fixup_add(cg_sec, do_j, begin_label, 1); // fixup add for forward branch
#else
        asm_cmp_zero(cg_sec, r, node->cond->ty->size); // cmp $0, rr
        free_reg(r);
        size_t do_j = asm_jcc_label(cg_sec, X86_NE); // jcc label
        asm_fixup_add(cg_sec, do_j, begin_label, 1); // fixup add for forward branch
#endif
        cg_def_label(end_label); // jmp %s
        ctrl_depth--;
        return -1;
    }
    case ND_SWITCH: {
        int c = ++rcc_label_count;
        VReg cond = gen(node->cond);
        int sz = op_size(node->cond->ty);
        bool is_uns = node->cond->ty && node->cond->ty->is_unsigned;
        for (Node *cs = node->case_next; cs; cs = cs->case_next) {
            if (!cs->label_id)
                cs->label_id = ++rcc_label_count;
            if (cs->is_case_range) {
                int skip_lbl = ++rcc_label_count;
#ifdef ARCH_ARM64
                if ((cs->case_val >= 0 && cs->case_val <= 4095) ||
                    (cs->case_val > 0 && cs->case_val <= 0xffffff && (cs->case_val % 4096) == 0))
                    asm_cmp_imm(cg_sec, cond, sz, (long long)cs->case_val); // cmp $(long long)cs->case_val, rcond
                else {
                    VReg tmp = alloc_reg();
                    asm_mov_imm(cg_sec, tmp, 8, (int64_t)cs->case_val); // mov tmp, #case_val
                    asm_cmp_reg_reg(cg_sec, cond, tmp, sz); // cmp rtmp, rcond
                    free_reg(tmp);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, is_uns ? ARM64_LO : ARM64_LT); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.skip.%d", skip_lbl), 1);
                } /* cmp %s, #%lld\n */
                if ((cs->case_end >= 0 && cs->case_end <= 4095) ||
                    (cs->case_end > 0 && cs->case_end <= 0xffffff && (cs->case_end % 4096) == 0))
                    asm_cmp_imm(cg_sec, cond, sz, (long long)cs->case_end); // cmp $(long long)cs->case_end, rcond
                else {
                    VReg tmp = alloc_reg();
                    asm_mov_imm(cg_sec, tmp, 8, (int64_t)cs->case_end); // mov tmp, #case_end
                    asm_cmp_reg_reg(cg_sec, cond, tmp, 8); // cmp rtmp, rcond
                    free_reg(tmp);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, is_uns ? ARM64_LS : ARM64_LE); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.case.%d", (int)cs->label_id), 1);
                } /* movabs $%lld, %s\n */
#else
                if (cs->case_val == (int32_t)cs->case_val)
                    asm_cmp_imm(cg_sec, cond, sz, (long long)cs->case_val); // cmp $(long long)cs->case_val, rcond
                else {
                    VReg tmp = alloc_reg();
                    asm_mov_imm(cg_sec, tmp, 8, (long long)cs->case_val); // mov $(long long)cs->case_val, rtmp
                    asm_cmp_reg_reg(cg_sec, cond, tmp, 8); // cmp rtmp, rcond
                    free_reg(tmp);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, is_uns ? X86_B : X86_L); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.skip.%d", skip_lbl), 1);
                } /* cmp %s, %s\n */
                if (cs->case_end == (int32_t)cs->case_end)
                    asm_cmp_imm(cg_sec, cond, sz, (long long)cs->case_end); // cmp $(long long)cs->case_end, rcond
                else {
                    VReg tmp = alloc_reg();
                    asm_mov_imm(cg_sec, tmp, 8, (long long)cs->case_end); // mov $(long long)cs->case_end, rtmp
                    asm_cmp_reg_reg(cg_sec, cond, tmp, 8); // cmp rtmp, rcond
                    free_reg(tmp);
                }
                {
                    size_t o = asm_jcc_label(cg_sec, is_uns ? X86_BE : X86_LE); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.case.%d", (int)cs->label_id), 1);
                } /* b.eq .L.case.%d\n */
#endif
                cg_def_label(format(".L.skip.%d", skip_lbl)); // cmp $%lld, %s
            } else {
#ifdef ARCH_ARM64
                if ((cs->case_val >= 0 && cs->case_val <= 4095) ||
                    (cs->case_val > 0 && cs->case_val <= 0xffffff && (cs->case_val % 4096) == 0)) {
                    asm_cmp_imm(cg_sec, cond, sz, (long long)cs->case_val); // cmp $(long long)cs->case_val, rcond
                } else {
                    VReg tmp = alloc_reg();
                    asm_mov_imm(cg_sec, tmp, 8, (int64_t)cs->case_val); // mov tmp, #case_val
                    asm_cmp_reg_reg(cg_sec, cond, tmp, sz); // cmp rtmp, rcond
                    free_reg(tmp);
                }
                {
                    size_t case_jmp = asm_jcc_label(cg_sec, ARM64_EQ); // jcc label
                    asm_fixup_add(cg_sec, case_jmp, format(".L.case.%d", (int)cs->label_id), 1);
                }
#else
                if (cs->case_val == (int32_t)cs->case_val) {
                    asm_cmp_imm(cg_sec, cond, sz, (long long)cs->case_val); // cmp $(long long)cs->case_val, rcond
                } else {
                    VReg tmp = alloc_reg();
                    asm_mov_imm(cg_sec, tmp, 8, (long long)cs->case_val); // mov $(long long)cs->case_val, rtmp
                    asm_cmp_reg_reg(cg_sec, tmp, cond, 8); // cmp rcond, rtmp
                    free_reg(tmp);
                }
                size_t case_jmp = asm_jcc_label(cg_sec, X86_E); // jcc label
                asm_fixup_add(cg_sec, case_jmp, format(".L.case.%d", cs->label_id), 1); // fixup label
#endif
            }
        }
        if (node->default_case) {
            if (!node->default_case->label_id)
                node->default_case->label_id = ++rcc_label_count;
            size_t sw_jmp = asm_jmp_label(cg_sec); // cmp %s, %s
            asm_fixup_add(cg_sec, sw_jmp, format(".L.case.%d", node->default_case->label_id), 0); // fixup label
        } else {
            size_t sw_jmp = asm_jmp_label(cg_sec); // cmp %s, #%lld
            asm_fixup_add(cg_sec, sw_jmp, format(".L.end.%d", c), 0); // fixup label
        }
        free_reg(cond);
        break_stack[ctrl_depth] = c;
        continue_stack[ctrl_depth] = ctrl_depth > 0 ? continue_stack[ctrl_depth - 1] : c;
        ctrl_depth++;
        VReg r_body = gen(node->then);
        if (r_body != -1) free_reg(r_body);
        ctrl_depth--;
        cg_def_label(format(".L.end.%d", c)); // .L.end.%d:
        return -1;
    }
    case ND_CASE: {
        if (!node->label_id)
            node->label_id = ++rcc_label_count;
        cg_def_label(format(".L.case.%d", node->label_id)); // .L.case.%d:
        return gen(node->lhs);
    }
    case ND_BREAK:
        if (ctrl_depth == 0)
            error("stray break");
        emit_cleanup_range(node->cleanup_begin, node->cleanup_end);
        emit_vla_dealloc(node->cleanup_begin, node->cleanup_end);
        {
            size_t brk_off = asm_jmp_label(cg_sec); // b .L.end.%d
            asm_fixup_add(cg_sec, brk_off, format(".L.end.%d", break_stack[ctrl_depth - 1]), 0); // fixup label
        }
        return -1;
    case ND_CONTINUE:
        if (ctrl_depth == 0)
            error("stray continue");
        emit_cleanup_range(node->cleanup_begin, node->cleanup_end);
        emit_vla_dealloc(node->cleanup_begin, node->cleanup_end);
        {
            size_t cont_off = asm_jmp_label(cg_sec); // b .L.continue.%d
            asm_fixup_add(cg_sec, cont_off, format(".L.continue.%d", continue_stack[ctrl_depth - 1]), 0); // fixup add for forward branch
        }
        return -1;
    case ND_GOTO:
        emit_cleanup_range(node->cleanup_begin, node->cleanup_end);
        emit_vla_dealloc(node->cleanup_begin, node->cleanup_end);
        {
            size_t goto_off = asm_jmp_label(cg_sec); // b .L.label.%s.%s
            asm_fixup_add(cg_sec, goto_off, format(".L.label.%s.%s", current_fn, node->label_name), 0); // fixup label
        }
        return -1;
    case ND_GOTO_IND: {
        VReg r = gen(node->lhs);
#ifdef ARCH_ARM64
        asm_jmp_reg(cg_sec, r); // jmp rr
#else
        asm_jmp_reg(cg_sec, r); // jmp rr
#endif
        free_reg(r);
        return -1;
    }
    case ND_LABEL: {
        cg_def_label(format(".L.label.%s.%s", current_fn, node->label_name)); // .L.label.%s.%s:
        return gen(node->lhs);
    }
    case ND_LABEL_VAL: {
        VReg r = alloc_reg();
#ifdef ARCH_ARM64
        emit_adrp_add(r, format(".L.label.%s.%s", current_fn, node->label_name));
#else
        asm_lea_rip_reg(cg_sec, r, format(".L.label.%s.%s", current_fn, node->label_name)); // lea label(%rip), r
#endif
        return r;
    }
    case ND_ASM: {
#ifdef ARCH_ARM64
        // ARM64 extended inline assembly handler.
        //
        // For each operand:
        //   - Memory (m, Q, Ump): gen_addr → asm_str = "[xN]"
        //   - Output-only register (=r, =w, =x, =y, =&r):
        //       gen_addr for store-back, alloc fresh register for asm use
        //   - Read-write register (+r): gen_addr, alloc fresh reg, load value, store back
        //   - Input register (r, w, x): gen value into register
        //   - Matching constraint (digit): same register as referenced output
        //   - Immediate (I,K,L,M,N): gen value, emit as register (immediate in template)
        //   - Zero (Z): use xzr
        //   - Symbol (S): gen address

        VReg op_regs[MAX_ASM_OPERANDS]; // GPR for asm template operand (-1 if unused/FP)
        VReg op_addr[MAX_ASM_OPERANDS]; // Address register for store-back (-1 if none)
        int op_is_fp[MAX_ASM_OPERANDS]; // FP constraint (use d0 as output)
        for (int i = 0; i < node->asm_noperands; i++) {
            op_regs[i] = -1;
            op_addr[i] = -1;
            op_is_fp[i] = 0;
        }

        for (int i = 0; i < node->asm_noperands; i++) {
            AsmOperand *op = &node->asm_ops[i];
            const char *c = op->constraint;
            // skip leading modifiers =, +, &
            while (*c == '=' || *c == '+' || *c == '&') c++;

            // Matching constraint: digit refers to another operand
            if (*c >= '0' && *c <= '9') {
                // defer: will resolve after all others are evaluated
                op_regs[i] = -2; // sentinel: matching
                continue;
            }

            // Determine constraint class
            bool is_mem = op->is_memory || *c == 'Q' ||
                (*c == 'U' && c[1] == 'm' && c[2] == 'p');
            bool is_fp = (*c == 'w' || *c == 'x' || *c == 'y') && !is_mem;
            bool is_imm = (*c == 'I' || *c == 'K' || *c == 'L' ||
                           *c == 'M' || *c == 'N');
            bool is_zero = (*c == 'Z');

            if (is_mem) {
                VReg r = gen_addr(op->expr);
                op_regs[i] = r;
                op->reg = r;
                snprintf(op->asm_str, sizeof(op->asm_str), "[%s]", reg64[r]);
            } else if (is_fp) {
                if (op->is_output) {
                    // FP output: get address for store-back, use d0 in template
                    VReg r = gen_addr(op->expr);
                    op_addr[i] = r;
                    op_is_fp[i] = 1;
                    op->reg = -1;
                    snprintf(op->asm_str, sizeof(op->asm_str), "d0");
                } else {
                    // FP input treated as integer register for now
                    VReg r = gen(op->expr);
                    op_regs[i] = r;
                    op->reg = r;
                    snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg64[r]);
                }
            } else if (is_zero) {
                // Zero constraint: always xzr, no register allocation needed
                snprintf(op->asm_str, sizeof(op->asm_str), "xzr");
                // op_regs[i] stays -1
            } else if (is_imm) {
                // Immediate constraint (I,K,L,M,N): emit as #value in the template.
                // Try to extract compile-time constant; fall back to register.
                int64_t cval = 0;
                if (try_const_int(op->expr, &cval)) {
                    snprintf(op->asm_str, sizeof(op->asm_str), "#0x%llx",
                             (unsigned long long)(uint64_t)cval);
                    // op_regs[i] stays -1, no register needed
                } else {
                    VReg r = gen(op->expr);
                    op_regs[i] = r;
                    op->reg = r;
                    snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg64[r]);
                }
            } else if (op->is_output && !op->is_rw) {
                // Output-only register (=r, =&r, etc.):
                // Allocate a fresh x register for the asm, store back after.
                // Always use x (64-bit) register in the template; %wN modifier handles 32-bit.
                VReg r_addr = gen_addr(op->expr);
                op_addr[i] = r_addr;
                VReg r = alloc_reg();
                op_regs[i] = r;
                op->reg = r;
                snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg64[r]);
            } else if (op->is_rw) {
                // Read-write register (+r):
                // Allocate fresh x register, load current value, store back after.
                VReg r_addr = gen_addr(op->expr);
                op_addr[i] = r_addr;
                VReg r = alloc_reg();
                op_regs[i] = r;
                op->reg = r;
                emit_load(op->expr->ty, r, r_addr, 0);
                snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg64[r]);
            } else {
                // Input register (r) or symbol address (S):
                VReg r = gen(op->expr);
                op_regs[i] = r;
                op->reg = r;
                snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg64[r]);
            }
        }

        // Resolve matching constraints (digit): share register with referenced output.
        // Also load the input value into the shared register before the asm.
        for (int i = 0; i < node->asm_noperands; i++) {
            if (op_regs[i] != -2) continue;
            AsmOperand *op = &node->asm_ops[i];
            const char *c = op->constraint;
            while (*c == '=' || *c == '+' || *c == '&') c++;
            int ref = *c - '0';
            if (ref >= 0 && ref < node->asm_noperands) {
                VReg r = op_regs[ref];
                // For output operands that compute an address (gen_addr), the
                // matching input needs a SEPARATE value register, not the address reg.
                bool ref_is_addr = node->asm_ops[ref].is_output && !node->asm_ops[ref].is_rw;
                if (ref_is_addr) {
                    // Allocate a fresh register for the input value,
                    // then move it into the output register so the asm
                    // finds it in the shared register.
                    VReg r_in = gen(op->expr);
                    asm_mov_reg_reg(cg_sec, r, r_in, 8); // mov rr_in -> rr
                    free_reg(r_in);
                    op_regs[i] = r;
                    op->reg = r;
                    snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg64[r]);
                } else {
                    op_regs[i] = r;
                    op->reg = r;
                    strncpy(op->asm_str, node->asm_ops[ref].asm_str, sizeof(op->asm_str) - 1);
                    op->asm_str[sizeof(op->asm_str) - 1] = '\0';
                    if (r >= 0) {
                        VReg r_in = gen(op->expr);
                        if (r_in != r)
                            asm_mov_reg_reg(cg_sec, r, r_in, 8); // mov rr_in -> rr
                        free_reg(r_in);
                    }
                }
            }
        }

        // Validate the asm template (catches invalid mnemonics, ranges, etc.)
        arm64_validate_asm_template(node->asm_template, node->tok);

        // Translate TCC-specific {$}N to ARM64 immediate #N
        char adj[4096];
        int alen = 0;
        const char *tp = node->asm_template;
        while (*tp && alen < (int)sizeof(adj) - 1) {
            if (*tp == '{' && *(tp + 1) == '$' && *(tp + 2) == '}') {
                adj[alen++] = '#';
                tp += 3;
            } else {
                adj[alen++] = *tp++;
            }
        }
        adj[alen] = '\0';

        // Substitute %N, %wN, %xN, %dN, %l[label], %[name] in template
        char out[4096];
        int olen = 0;
        const char *p = adj;
        while (*p && olen < (int)sizeof(out) - 1) {
            if (*p != '%') {
                out[olen++] = *p++;
                continue;
            }
            p++;
            if (*p == '%') {
                out[olen++] = '%';
                p++;
                continue;
            }
            // Check for modifier letter(s): w, x, d, s, l, h
            char mod = 0;
            if (*p == 'w' || *p == 'x' || *p == 'd' || *p == 's' ||
                *p == 'l' || *p == 'h') {
                // peek: if next char is '[' or digit, it's a modifier
                if (*(p + 1) == '[' || (*(p + 1) >= '0' && *(p + 1) <= '9'))
                    mod = *p++;
            }

            if (mod == 'l' && *p == '[') {
                // %l[name] → goto label
                p++;
                const char *end = strchr(p, ']');
                if (!end) {
                    out[olen++] = '%';
                    out[olen++] = 'l';
                    out[olen++] = '[';
                    continue;
                }
                const char *prefix = ".L.label.";
                for (const char *s = prefix; *s && olen < (int)sizeof(out) - 1;) out[olen++] = *s++;
                for (const char *s = current_fn; *s && olen < (int)sizeof(out) - 1;) out[olen++] = *s++;
                if (olen < (int)sizeof(out) - 1) out[olen++] = '.';
                for (const char *s = p; s < end && olen < (int)sizeof(out) - 1;) out[olen++] = *s++;
                p = end + 1;
            } else if (*p == '[') {
                // %[name] → named operand (not a goto label)
                p++;
                const char *end = strchr(p, ']');
                if (!end) {
                    out[olen++] = '%';
                    out[olen++] = '[';
                    continue;
                }
                // find operand with matching name
                char namebuf[32];
                int nlen = (int)(end - p) < 31 ? (int)(end - p) : 31;
                memcpy(namebuf, p, nlen);
                namebuf[nlen] = '\0';
                p = end + 1;
                const char *sub = NULL;
                for (int j = 0; j < node->asm_noperands; j++) {
                    if (strcmp(node->asm_ops[j].name, namebuf) == 0) {
                        sub = node->asm_ops[j].asm_str;
                        break;
                    }
                }
                if (!sub) sub = "";
                // Apply modifier to named operand
                if (sub[0] && (mod == 'w' || mod == 'x') && sub[0] == 'x') {
                    if (mod == 'w') {
                        if (olen < (int)sizeof(out) - 1) out[olen++] = 'w';
                        sub++; // skip leading 'x'
                    }
                }
                while (*sub && olen < (int)sizeof(out) - 1) out[olen++] = *sub++;
            } else if (*p >= '0' && *p <= '9') {
                int n = *p - '0';
                p++;
                if (n < node->asm_noperands) {
                    const char *s = node->asm_ops[n].asm_str;
                    // Apply modifier
                    if (mod == 'w' && s[0] == 'x') {
                        if (olen < (int)sizeof(out) - 1) out[olen++] = 'w';
                        s++; // skip 'x'
                    } else if (mod == 'd') {
                        // FP double form: dN where N is the reg number
                        if (op_is_fp[n]) {
                            // already stored as "d0" etc
                        } else {
                            if (olen < (int)sizeof(out) - 1) out[olen++] = 'd';
                            // s points to the register name like "x5" → "d5"
                            if (*s == 'x' || *s == 'w') s++;
                            while (*s && olen < (int)sizeof(out) - 1) out[olen++] = *s++;
                            goto next_char;
                        }
                    }
                    while (*s && olen < (int)sizeof(out) - 1) out[olen++] = *s++;
                }
            } else {
                out[olen++] = '%';
                if (mod) out[olen++] = mod;
                // pass through the character that wasn't a modifier/operand
                if (*p && *p != '%') out[olen++] = *p++;
            }
        next_char:;
        }
        out[olen] = '\0';
        if (olen > 0 && !cg_dry_run) {
            // Split on ';' for multi-instruction inline asm (e.g. "mov %0, #1; mov %1, #2")
            char *inst = out;
            while (*inst) {
                char *semi = strchr(inst, ';');
                if (semi) *semi = '\0';
                assemble_inline(cg_obj, inst, cg_inline_fixup_cb, NULL);
                if (!semi) break;
                inst = semi + 1;
                inst += strspn(inst, " \t");
            }
        }

        for (int i = 0; i < node->asm_noperands; i++) {
            AsmOperand *op = &node->asm_ops[i];
            if (op_addr[i] < 0) continue;
            if (op_is_fp[i]) {
                // Store FP result (d0) back to variable
                int sz = op->expr->ty ? op->expr->ty->size : 8;
                if (sz <= 4)
                    asm_str_fp(cg_sec, 0, op_addr[i], 4); // asm_str_fp(0, op_addr[i], 4)
                else
                    asm_str_fp(cg_sec, 0, op_addr[i], 8); // asm_str_fp(0, op_addr[i], 8)
            } else {
                emit_store(op->expr->ty, op_regs[i], op_addr[i], 0);
            }
        }

        // Free registers: value regs first, then address regs
        for (int i = 0; i < node->asm_noperands; i++)
            if (op_regs[i] >= 0) free_reg(op_regs[i]);
        for (int i = 0; i < node->asm_noperands; i++)
            if (op_addr[i] >= 0) free_reg(op_addr[i]);
        return -1;
#else
        // x86-64 inline asm operand evaluation.
        // op_regs[i]: register holding value (or address for memory operands)
        // op_addr[i]: address register for store-back after asm (-1 if none)
        VReg op_regs[MAX_ASM_OPERANDS];
        VReg op_addr[MAX_ASM_OPERANDS];
        for (int i = 0; i < node->asm_noperands; i++) {
            op_regs[i] = -1;
            op_addr[i] = -1;
        }

        // First pass: allocate registers for outputs and memory operands.
        // Defer matching input constraints (digit) to second pass.
        for (int i = 0; i < node->asm_noperands; i++) {
            AsmOperand *op = &node->asm_ops[i];
            const char *c = op->constraint;
            while (*c == '=' || *c == '+' || *c == '&') c++;

            if (op->is_memory) {
                // "m" constraint: compute address, use as memory ref in template
                VReg r = gen_addr(op->expr);
                op_regs[i] = r;
                op->reg = r;
                snprintf(op->asm_str, sizeof(op->asm_str), "(%s)", reg64[r]);
            } else if (op->is_output && !op->is_rw) {
                // Output-only: "=r" → allocate fresh register, store back after asm.
                // "=m" is handled above (is_memory).
                VReg r_addr = gen_addr(op->expr);
                op_addr[i] = r_addr;
                VReg r = alloc_reg();
                op_regs[i] = r;
                op->reg = r;
                int sz = op->expr->ty ? op->expr->ty->size : 4;
                snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg(r, sz));
            } else if (op->is_rw) {
                // Read-write "+r": load current value, store back after asm.
                VReg r_addr = gen_addr(op->expr);
                op_addr[i] = r_addr;
                VReg r = alloc_reg();
                op_regs[i] = r;
                op->reg = r;
                int sz = op->expr->ty ? op->expr->ty->size : 4;
                emit_load(op->expr->ty, r, r_addr, 0);
                snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg(r, sz));
            } else if (*c >= '0' && *c <= '9') {
                // Matching constraint: defer to second pass
                op_regs[i] = -2; // sentinel
            } else {
                // Regular input "r": load value into a register
                VReg r = gen(op->expr);
                op_regs[i] = r;
                op->reg = r;
                int sz = op->expr->ty ? op->expr->ty->size : 4;
                snprintf(op->asm_str, sizeof(op->asm_str), "%s", reg(r, sz));
            }
        }

        // Second pass: resolve matching constraints ("0".."9").
        // Load the input value into the same register as the referenced output.
        for (int i = 0; i < node->asm_noperands; i++) {
            if (op_regs[i] != -2) continue;
            AsmOperand *op = &node->asm_ops[i];
            const char *c = op->constraint;
            while (*c == '=' || *c == '+' || *c == '&') c++;
            int ref = *c - '0';
            if (ref >= 0 && ref < node->asm_noperands && op_regs[ref] >= 0) {
                VReg r = op_regs[ref];
                // Load the input value into the shared register
                VReg r_in = gen(op->expr);
                if (r_in != r) {
                    int sz = op->expr->ty ? op->expr->ty->size : 4;
                    if (sz <= 4)
                        asm_mov_reg_reg(cg_sec, r, r_in, 4); // mov r_in -> r (load input into output reg)
                    else
                        asm_mov_reg_reg(cg_sec, r, r_in, 8); // mov r_in -> r
                }
                free_reg(r_in);
                op_regs[i] = r;
                op->reg = r;
                strncpy(op->asm_str, node->asm_ops[ref].asm_str, sizeof(op->asm_str) - 1);
                op->asm_str[sizeof(op->asm_str) - 1] = '\0';
            }
        }

        // Emit template with operand substitution, wrapped in AT&T syntax
        /* inline asm already in AT&T */

        // Translate TCC-specific {$}N to GAS-immediate $N in AT&T syntax
        char adj[4096];
        int alen = 0;
        const char *tp = node->asm_template;
        while (*tp && alen < (int)sizeof(adj) - 1) {
            if (*tp == '{' && *(tp + 1) == '$' && *(tp + 2) == '}') {
                adj[alen++] = '$';
                tp += 3;
            } else {
                adj[alen++] = *tp++;
            }
        }
        adj[alen] = '\0';

        // Substitute %N and %l[name] in template
        char out[4096];
        int olen = 0;
        const char *p = adj;
        while (*p && olen < (int)sizeof(out) - 1) {
            if (*p != '%') {
                out[olen++] = *p++;
                continue;
            }
            p++;
            if (*p == '%') {
                out[olen++] = '%';
                p++;
                continue;
            }
            // check for modifier 'l'
            char mod = 0;
            if (*p == 'l') { mod = *p++; }
            if (mod == 'l' && *p == '[') {
                // %l[name] -> goto label
                p++;
                const char *end = strchr(p, ']');
                if (!end) {
                    out[olen++] = '%';
                    out[olen++] = 'l';
                    out[olen++] = '[';
                    continue;
                }
                // emit .L.label.<fn>.<name>
                const char *prefix = ".L.label.";
                for (const char *s = prefix; *s && olen < (int)sizeof(out) - 1;) out[olen++] = *s++;
                for (const char *s = current_fn; *s && olen < (int)sizeof(out) - 1;) out[olen++] = *s++;
                if (olen < (int)sizeof(out) - 1) out[olen++] = '.';
                for (const char *s = p; s < end && olen < (int)sizeof(out) - 1;) out[olen++] = *s++;
                p = end + 1;
            } else if (*p >= '0' && *p <= '9') {
                int n = *p - '0';
                p++;
                if (n < node->asm_noperands) {
                    const char *s = node->asm_ops[n].asm_str;
                    while (*s && olen < (int)sizeof(out) - 1) out[olen++] = *s++;
                }
            } else {
                out[olen++] = '%';
                if (mod) out[olen++] = mod;
                // leave other %x as-is
            }
        }
        out[olen] = '\0';
        if (olen > 0 && !cg_dry_run) {
            assemble_inline(cg_obj, out, cg_inline_fixup_cb, NULL);
        }

        // Store back register outputs ("=r", "+r") to their C variables
        for (int i = 0; i < node->asm_noperands; i++) {
            if (op_addr[i] < 0) continue;
            AsmOperand *op = &node->asm_ops[i];
            int sz = op->expr->ty ? op->expr->ty->size : 4;
            if (sz == 1)
                asm_mov_reg_mem(cg_sec, op_regs[i], op_addr[i], 1); // movb op_regs[i], (%op_addr[i])
            else if (sz == 2)
                asm_mov_reg_mem(cg_sec, op_regs[i], op_addr[i], 2); // movw op_regs[i], (%op_addr[i])
            else if (sz <= 4)
                asm_mov_reg_mem(cg_sec, op_regs[i], op_addr[i], 4); // movl op_regs[i], (%op_addr[i])
            else
                asm_mov_reg_mem(cg_sec, op_regs[i], op_addr[i], 8); // movq op_regs[i], (%op_addr[i])
        }

        // Free registers
        for (int i = 0; i < node->asm_noperands; i++)
            if (op_regs[i] >= 0) free_reg(op_regs[i]);
        for (int i = 0; i < node->asm_noperands; i++)
            if (op_addr[i] >= 0) free_reg(op_addr[i]);
        return -1;
#endif
    }
    case ND_VA_START: {
#if defined(ARCH_ARM64) && defined(__APPLE__)
        // Apple ARM64: va_list is char*. Point ap to first variadic arg on stack.
        VReg r = gen_addr(node->lhs); // address of the char* variable ap
        asm_add_x16_fp_imm(cg_sec, va_st_start); // add x16, fp, #va_st_start
        asm_str_x16_uoff(cg_sec, r, 0); // str x16, [x{r}]
#elif defined(ARCH_ARM64)
        // AAPCS64: initialize 5-field va_list struct
        VReg r = gen(node->lhs); // va_list is array-of-struct, decays to pointer
        // __stack: pointer to first stack overflow argument
        asm_add_x16_fp_imm(cg_sec, va_st_start); // add x16, x29, #va_st_start
        asm_str_x16_uoff(cg_sec, r, 0); // str x16, [x{r}]
        // __gr_top: end of GP reg save area = saved_sp + 64
        asm_ldur_fp_phy(cg_sec, ARM64_X16, 8); // ldur x16, [x29, #-8]
        asm_add_x16_imm(cg_sec, 64); // add x16, x16, #64
        asm_str_x16_uoff(cg_sec, r, 1); // str x16, [x{r}, #8]
        // __vr_top: end of FP reg save area = saved_sp + 192
        asm_ldur_fp_phy(cg_sec, ARM64_X16, 8); // ldur x16, [x29, #-8]
        asm_add_x16_imm(cg_sec, 192); // add x16, x16, #192
        asm_str_x16_uoff(cg_sec, r, 2); // str x16, [x{r}, #16]
        // __gr_offs: -(8 - gp_param) * 8
        emit_mov_imm64(ARM64_X16, (uint64_t)va_gp_start); // mov w16, #va_gp_start
        asm_str_w16_uoff(cg_sec, r, 6); // str w16, [x{r}, #24]
        // __vr_offs: -(8 - fp_param) * 16
        emit_mov_imm64(ARM64_X16, (uint64_t)va_fp_start); // mov w16, #va_fp_start
        asm_str_w16_uoff(cg_sec, r, 7); // str w16, [x{r}, #28]
#elif defined(_WIN32)
        // Windows x64: va_list is char *. Point to first variadic arg.
        // If named params use < 4 reg slots, the first variadic arg is in the
        // shadow space (rbp+16 for slot 0, +24 for slot 1, etc.).
        // If all 4 reg slots are consumed by named params, the first variadic
        // arg is on the stack at rbp+48 + n_stack*8.
        VReg r = gen_addr(node->lhs); // va_list is char *, need its address to write
        {
            // Use va_gp_start (already includes hidden struct ret ptr) — matches codegen.c.main
            int va_first = (va_gp_start < 32) ? (16 + va_gp_start) : va_st_start;
            // asm_lea_rbp negates offset, so pass -va_first to get +va_first
            asm_lea_rbp(cg_sec, X86_RDX, 8, -va_first); // leaq va_first(%rbp), %rdx
        }
        x86_mov_mr(cg_sec, 8, x86_mem(REG(r), 0), X86_RDX); // movq %rdx, (%r)
#else
        VReg r = gen(node->lhs); // va_list is array-of-struct, decays to pointer
        x86_mov_mi(cg_sec, 4, x86_mem(REG(r), 0), va_gp_start); // movl $va_gp_start, (r)
        x86_mov_mi(cg_sec, 4, x86_mem(REG(r), 4), va_fp_start); // movl $va_fp_start, 4(r)
        asm_lea_rbp(cg_sec, X86_RDX, 8, -va_st_start); // leaq va_st_start(%rbp), %rdx
        x86_mov_mr(cg_sec, 8, x86_mem(REG(r), 8), X86_RDX); // movq %rdx, 8(r)
        asm_lea_rbp(cg_sec, X86_RDX, 8, va_reg_save_ofs); // leaq -va_reg_save_ofs(%rbp), %rdx
        x86_mov_mr(cg_sec, 8, x86_mem(REG(r), 16), X86_RDX); // movq %rdx, 16(r)
#endif
        free_reg(r);
        return -1;
    }
    case ND_VA_COPY: {
#if defined(ARCH_ARM64) && defined(__APPLE__)
        // Apple ARM64: va_list is char*. Just copy the pointer.
        VReg rd = gen_addr(node->lhs); // address of dst char* variable
        VReg rs = gen(node->rhs); // value of src char* (the pointer itself)
        asm_str_reg_off(cg_sec, rs, rd, 8, 0); // str rs, [rd]
        free_reg(rd);
        free_reg(rs);
#elif defined(ARCH_ARM64)
        VReg rd = gen(node->lhs);
        VReg rs = gen(node->rhs);
        // AAPCS64 va_list is 32 bytes: copy all 4 x 8-byte words (use x16 as temp)
        for (int _vi = 0; _vi < 4; _vi++) {
            asm_ldr_x16_uoff(cg_sec, rs, _vi); // ldr x16, [x{rs}, #_vi*8]
            asm_str_x16_uoff(cg_sec, rd, _vi); // str x16, [x{rd}, #_vi*8]
        }
        free_reg(rd);
        free_reg(rs);
#elif defined(_WIN32)
        // Windows x64: va_list is char* (8 bytes). Just copy the pointer.
        VReg rd = gen_addr(node->lhs); // address of dst char* variable
        VReg rs = gen(node->rhs); // value of src char* (the pointer itself)
        x86_mov_mr(cg_sec, 8, x86_mem(REG(rd), 0), REG(rs)); // movq rs, (%rd)
        free_reg(rd);
        free_reg(rs);
#else
        VReg rd = gen(node->lhs);
        asm_push(cg_sec, REG(rd)); // pushq rd
        free_reg(rd);
        VReg rs = gen(node->rhs);
        VReg rpop = alloc_reg();
        asm_pop(cg_sec, REG(rpop)); // popq rpop
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(rs), 0)); // movq (rs), %rcx
        asm_mov_phy_base_off(cg_sec, X86_RCX, rpop, 0, 8); // movq %rcx, 0(%rpop)
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(rs), 8)); // movq 8(rs), %rcx
        asm_mov_phy_base_off(cg_sec, X86_RCX, rpop, 8, 8); // movq %rcx, 8(%rpop)
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(rs), 16)); // movq 16(rs), %rcx
        asm_mov_phy_base_off(cg_sec, X86_RCX, rpop, 16, 8); // movq %rcx, 16(%rpop)
        free_reg(rs);
        free_reg(rpop);
#endif
        return -1;
    }
    case ND_VA_ARG_PACK: {
        // Only valid inside a function inlined by the call-site
        // __builtin_va_arg_pack expander in the parser; if one reaches
        // codegen (e.g. unused inline-pack function), emit a harmless 0.
        int r = alloc_reg();
#ifdef ARCH_ARM64
        asm_movq_zero(cg_sec, r);
#else
        asm_movl_zero(cg_sec, r);
#endif
        return r;
    }
    case ND_VA_ARG: {
        Type *ty = node->ty->base;
#if defined(ARCH_ARM64) && defined(__APPLE__)
        // Apple ARM64: va_list is char*; all variadic args are 8-byte slots on the stack.
        // slot_sz = ceil(size / 8) * 8; zero-size types advance by 0.
        VReg apple_r = gen_addr(node->lhs); // address of the char* variable ap
        int slot_sz = (ty->size + 7) & ~7;
        // Use x17 (reserved scratch) to hold &ap before loading x12.
        // Without this, if gen_addr returns x12, "ldr x12,[x12]" clobbers the
        // address and "str x16,[x12]" writes new_ap into the data area instead
        // of back into the ap variable.
        asm_mov_phy_reg(cg_sec, ARM64_X17, apple_r, 1); // mov x17, x{apple_r}
        arm64_ldr_uoff(cg_sec, 3, ARM64_X12, ARM64_X17, 0); // ldr x12, [x17]
        // __int128 requires 16-byte alignment; the caller pads stack_args to even
        // before placing it, so va_arg must align ap up to 16 to match.
        if (ty->kind == TY_INT128) {
            arm64_add_imm(cg_sec, 1, ARM64_X16, ARM64_X12, 15, 0); // add x16, x12, #15
            arm64_and_imm(cg_sec, 1, ARM64_X12, ARM64_X16, ~15ULL); // and x12, x16, #-16
        }
        arm64_add_imm(cg_sec, 1, ARM64_X16, ARM64_X12, slot_sz, 0); // add x16, x12, #slot_sz
        asm_str_reg(cg_sec, ARM64_X16, ARM64_X17); // str x16, [x17]
        free_reg(apple_r);
        VReg ret = alloc_reg();
        asm_mov_vreg_x12(cg_sec, ret); // mov x{ret}, x12
        return ret;
#else
#ifndef _WIN32
        bool is_fp = is_flonum(ty);
#endif
        VReg r;
#ifdef ARCH_ARM64
        r = gen(node->lhs);
        bool is_ptr_struct = (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->size > 16;
        // rcc passes ALL structs >8 bytes by pointer (not by value).
        // va_arg must read 8-byte pointer from reg save area, then dereference.
        bool is_ptr_val_struct = (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->size > 8;

        // AArch64 ABI va_list: [__stack(8), __gr_top(8), __vr_top(8), __gr_offs(4), __vr_offs(4)]
        // gr_offs < 0 means reg arg available; addr = __gr_top + __gr_offs
        // Only use FP path for bare float types; structs (incl. HFA) always go in GP regs.
        // AArch64 ABI va_list: [__stack(8), __gr_top(8), __vr_top(8), __gr_offs(4), __vr_offs(4)]
        // gr_offs < 0 means reg arg available; addr = __gr_top + __gr_offs
        // Only use FP path for bare float types; structs (incl. HFA) always go in GP regs.
        if (is_fp) {
            int fp_size = 16;
            // ldr w16, [x{r}, #28]  — __vr_offs
            asm_ldr_w16_uoff(cg_sec, r, 7); // ldr w16, [x{r}, #28]
            arm64_subs_imm(cg_sec, 0, ARM64_XZR, ARM64_X16, 0, 0); // cmp w16, #0
            {
                size_t _cj = asm_jcc_label(cg_sec, ARM64_GE);
                asm_fixup_add(cg_sec, _cj, format(".L.va_overflow.%d", rcc_label_count), 1);
            }
            // Store new vr_offs before loading vr_top (may clobber reg holding r)
            asm_add_w17_w16_imm(cg_sec, fp_size); // add w17, w16, #fp_size
            // str w17, [x{r}, #28]
            asm_str_w17_uoff(cg_sec, r, 7); // str w17, [x{r}, #28]
            // ldr x12, [x{r}, #16]  — __vr_top (safe to clobber r now)
            asm_ldr_x12_uoff(cg_sec, r, 2); // ldr x12, [x{r}, #16]
            asm_sxtw(cg_sec, ARM64_X17, ARM64_X16); // sxtw x17, w16
            asm_add_x12_x12_x17(cg_sec); // add x12, x12, x17
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
            }
        } else {
            int gp_size = 8;
#if defined(__APPLE__) && defined(ARCH_ARM64)
            // Apple ARM64 variadic: ALL struct args go on overflow stack, not GP regs.
            // HFA structs ≤8 bytes (e.g. {float}, {double}) are not is_ptr_val_struct
            // but still skip the GP save area because they went directly to stack.
            bool is_struct = (ty->kind == TY_STRUCT || ty->kind == TY_UNION);
            if (is_struct && !is_ptr_val_struct) {
                // Force branch to overflow — gp_size irrelevant, struct on stack
                // cmp w16, #0; b.ge .L.va_overflow — but we want ALWAYS overflow
                size_t _force_jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _force_jmp, format(".L.va_overflow.%d", rcc_label_count), 0);
            } else
#endif
            {
                // ldr w16, [x{r}, #24]  — __gr_offs
                asm_ldr_w16_uoff(cg_sec, r, 6); // ldr w16, [x{r}, #24]
                arm64_subs_imm(cg_sec, 0, ARM64_XZR, ARM64_X16, 0, 0); // cmp w16, #0
                {
                    size_t _cj = asm_jcc_label(cg_sec, ARM64_GE);
                    asm_fixup_add(cg_sec, _cj, format(".L.va_overflow.%d", rcc_label_count), 1);
                }
                // Store new gr_offs before loading gr_top (may clobber reg holding r)
                asm_add_w17_w16_imm(cg_sec, gp_size); // add w17, w16, #gp_size
                // str w17, [x{r}, #24]
                asm_str_w17_uoff(cg_sec, r, 6); // str w17, [x{r}, #24]
                // ldr x12, [x{r}, #8]  — __gr_top (safe to clobber r now)
                asm_ldr_x12_uoff(cg_sec, r, 1); // ldr x12, [x{r}, #8]
                asm_sxtw(cg_sec, ARM64_X17, ARM64_X16); // sxtw x17, w16
                asm_add_x12_x12_x17(cg_sec); // add x12, x12, x17
                if (is_ptr_val_struct) {
                    asm_ldr_x12_0(cg_sec); // ldr x12, [x12]
                }
                {
                    size_t _jmp = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
                }
            }
        }

        cg_def_label(format(".L.va_overflow.%d", rcc_label_count));
        // ldr x16, [x{r}]  — __stack (use x16 so va_list ptr r is not clobbered)
        asm_ldr_x16_uoff(cg_sec, r, 0); // ldr x16, [x{r}]
#if defined(__APPLE__) && defined(ARCH_ARM64)
        if (is_ptr_val_struct) {
            // Apple ARM64: variadic struct by value on overflow stack — data is inline, no ptr deref.
            // Just advance __stack past the struct data and set x12 to original __stack.
            int ovf_sz = (ty->size + 7) & ~7;
            if (ovf_sz <= 4095)
                arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X16, ovf_sz, 0); // add x17, x16, #ovf_sz
            else {
                emit_mov_imm64(ARM64_X17, (uint64_t)ovf_sz);
                arm64_add_reg(cg_sec, 1, ARM64_X17, ARM64_X16, ARM64_X17, ARM64_LSL, 0); // add x17, x16, x17
            }
            asm_str_x17_uoff(cg_sec, r, 0); // str x17, [x{r}]  — store updated __stack
            asm_mov_x12_x16(cg_sec); // mov x12, x16  — result = original __stack
        } else if (is_ptr_struct)
#else
        if (is_ptr_struct || is_ptr_val_struct)
#endif
        {
            // Pointer-passed struct: load pointer from overflow stack, advance __stack by 8
            asm_ldr_x12_x16_0(cg_sec); // ldr x12, [x16]  — deref __stack
            arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X16, 8, 0); // add x17, x16, #8
            asm_str_x17_uoff(cg_sec, r, 0); // str x17, [x{r}]  — store new __stack
            // x12 now holds the dereferenced pointer = result
        } else {
            // Non-struct arg on overflow stack: advance __stack by alignment
            int align = ty->align;
            int ovf_size = ty->size <= 8 ? 8 : (ty->size + 7) & ~7;
            if (align > 8) {
                arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X16, align - 1, 0); // add x17, x16, #align-1
                arm64_and_imm(cg_sec, 1, ARM64_X17, ARM64_X17, (uint64_t)(int64_t)(-align)); // and x17, x17, #-align
                arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X17, ovf_size, 0); // add x17, x17, #ovf_size
            } else {
                arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X16, ovf_size, 0); // add x17, x16, #ovf_size
            }
            asm_str_x17_uoff(cg_sec, r, 0); // str x17, [x{r}]  — store new __stack
            asm_mov_x12_x16(cg_sec); // mov x12, x16  — result = original __stack
        }

        cg_def_label(format(".L.va_done.%d", rcc_label_count));
        cg_def_label(format(".L.va_done_x.%d", rcc_label_count)); // extra for jmp fixup compatibility
        asm_mov_vreg_x12(cg_sec, r); // mov x{r}, x12
#elif defined(_WIN32)
        // Windows x64: va_list is char *. Read arg from current ap, advance by 8.
        r = gen_addr(node->lhs); // va_list is char *, need its address to advance
        bool is_ptr_struct = (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->size > 8;
        // __int128 is also passed by pointer on Windows x64 (like structs > 8 bytes)
        bool is_by_ptr = is_ptr_struct || ty->kind == TY_INT128;
        if (is_by_ptr) {
            // Struct/int128 >8 bytes passed by pointer: slot holds data pointer
            x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(r), 0)); // movq (%r), %rcx  [rcx = ap]
            x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RCX, 0)); // movq (%rcx), %rcx [rcx = data ptr]
            x86_add_mi(cg_sec, 8, x86_mem(REG(r), 0), 8); // addq $8, (%r) [ap += 8]
            x86_mov_rr(cg_sec, 8, REG(r), X86_RCX); // movq %rcx, r
        } else {
            // Return old ap (address of arg slot), then advance
            x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(REG(r), 0)); // movq (%r), %rcx  [rcx = ap]
            x86_add_mi(cg_sec, 8, x86_mem(REG(r), 0), 8); // addq $8, (%r) [ap += 8]
            x86_mov_rr(cg_sec, 8, REG(r), X86_RCX); // movq %rcx, r  [result = old ap]
        }
#else
        r = gen(node->lhs);
        bool is_ptr_struct = (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && ty->size > 8;
        // va_arg x86: r points to va_list struct {gr_offs, fp_offs, overflow_arg_area, reg_save_area}
        X86Reg xr = REG(r);
        int vaf = is_fp ? 4 : 0; // offset of fp_offs or gr_offs
        int limit = is_fp ? 160 : 40; // max offset before overflow
        int step = is_fp ? 16 : 8; // increment
        if (ty->kind == TY_LDOUBLE) {
            // long double (80-bit extended in a 16-byte slot) is classified
            // as MEMORY by the x86-64 SysV ABI and is never passed in SSE
            // registers, even for variadic calls — read it straight from the
            // overflow area, 16-byte aligned, and advance by 16. The slot
            // holds an x87 80-bit extended value; rcc's long double is
            // truncated to a 64-bit double everywhere else, so convert it
            // in place (the slot is dead after this read) and hand back a
            // pointer to the resulting double.
            x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(xr, 8)); // movq 8(r), %rcx
            x86_add_ri(cg_sec, 8, X86_RCX, 15); // addq $15, %rcx
            x86_and_ri(cg_sec, 8, X86_RCX, -16); // andq $-16, %rcx
            x86_lea(cg_sec, 8, X86_RDX, x86_mem(X86_RCX, 16)); // leaq 16(%rcx), %rdx
            x86_mov_mr(cg_sec, 8, x86_mem(xr, 8), X86_RDX); // movq %rdx, 8(r)
            x86_fldt_m(cg_sec, x86_mem(X86_RCX, 0)); // fldt (%rcx)
            x86_fstpl_m(cg_sec, x86_mem(X86_RCX, 0)); // fstpl (%rcx)
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
            }
        } else if (is_fp) {
            x86_cmp_mi(cg_sec, 4, x86_mem(xr, 4), limit); // cmpl $160, 4(r)
            {
                size_t _cj = asm_jcc_label(cg_sec, X86_A);
                asm_fixup_add(cg_sec, _cj, format(".L.va_overflow.%d", rcc_label_count), 1);
            }
            x86_mov_rm(cg_sec, 4, X86_RCX, x86_mem(xr, 4)); // movl 4(r), %ecx
            x86_add_rm(cg_sec, 8, X86_RCX, x86_mem(xr, 16)); // addq 16(r), %rcx
            x86_add_mi(cg_sec, 4, x86_mem(xr, 4), step); // addl $16, 4(r)
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
            }
        } else if (is_ptr_struct) {
            x86_cmp_mi(cg_sec, 4, x86_mem(xr, 0), limit); // cmpl $40, (r)
            {
                size_t _cj = asm_jcc_label(cg_sec, X86_A);
                asm_fixup_add(cg_sec, _cj, format(".L.va_overflow.%d", rcc_label_count), 1);
            }
            x86_mov_rm(cg_sec, 4, X86_RCX, x86_mem(xr, 0)); // movl (r), %ecx
            x86_add_rm(cg_sec, 8, X86_RCX, x86_mem(xr, 16)); // addq 16(r), %rcx
            x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RCX, 0)); // movq (%rcx), %rcx
            x86_add_mi(cg_sec, 4, x86_mem(xr, 0), step); // addl $8, (r)
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
            }
        } else if (ty->kind == TY_INT128) {
            // int128 needs 2 consecutive GP reg slots (16 bytes)
            x86_cmp_mi(cg_sec, 4, x86_mem(xr, 0), 32); // cmpl $32, (r)
            {
                size_t _cj = asm_jcc_label(cg_sec, X86_A);
                asm_fixup_add(cg_sec, _cj, format(".L.va_overflow.%d", rcc_label_count), 1);
            }
            x86_mov_rm(cg_sec, 4, X86_RCX, x86_mem(xr, 0)); // movl (r), %ecx
            x86_add_rm(cg_sec, 8, X86_RCX, x86_mem(xr, 16)); // addq 16(r), %rcx
            x86_add_mi(cg_sec, 4, x86_mem(xr, 0), 16); // addl $16, (r)
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
            }
        } else {
            x86_cmp_mi(cg_sec, 4, x86_mem(xr, 0), limit); // cmpl $40, (r)
            {
                size_t _cj = asm_jcc_label(cg_sec, X86_A);
                asm_fixup_add(cg_sec, _cj, format(".L.va_overflow.%d", rcc_label_count), 1);
            }
            x86_mov_rm(cg_sec, 4, X86_RCX, x86_mem(xr, 0)); // movl (r), %ecx
            x86_add_rm(cg_sec, 8, X86_RCX, x86_mem(xr, 16)); // addq 16(r), %rcx
            x86_add_mi(cg_sec, 4, x86_mem(xr, 0), step); // addl $8, (r)
            {
                size_t _jmp = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, _jmp, format(".L.va_done.%d", rcc_label_count), 0);
            }
        }
        cg_def_label(format(".L.va_overflow.%d", rcc_label_count));
        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(xr, 8)); // movq 8(r), %rcx
        if (ty->kind == TY_INT128) {
            // int128 overflow: 16-byte aligned, advance by 16
            x86_add_ri(cg_sec, 8, X86_RCX, 15); // addq $15, %rcx
            x86_and_ri(cg_sec, 8, X86_RCX, -16); // andq $-16, %rcx
            x86_lea(cg_sec, 8, X86_RDX, x86_mem(X86_RCX, 16)); // leaq 16(%rcx), %rdx
            x86_mov_mr(cg_sec, 8, x86_mem(xr, 8), X86_RDX); // movq %rdx, 8(r)
        } else {
            { // leaq 8(%rcx), %rdx
                X86Mem ml = {X86_RCX, X86_NOREG, 1, 8};
                x86_lea(cg_sec, 8, X86_RDX, ml);
            }
            x86_mov_mr(cg_sec, 8, x86_mem(xr, 8), X86_RDX); // movq %rdx, 8(r)
            if (is_ptr_struct)
                x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RCX, 0)); // movq (%rcx), %rcx
        }
        cg_def_label(format(".L.va_done.%d", rcc_label_count));
        x86_mov_rr(cg_sec, 8, REG(r), X86_RCX); // movq %rcx, rr (va_arg result)
        (void)vaf;
#endif
        rcc_label_count++;
        return r;
#endif
    }

    case ND_ATOMIC_LOAD: {
        VReg r_addr = gen(node->lhs);
        int sz = node->ty->size;
        VReg r = alloc_reg();
        int ord = node->atomic_ord;
#ifdef ARCH_ARM64
        bool use_acquire = (ord == MEMORDER_ACQUIRE || ord == MEMORDER_ACQ_REL || ord == MEMORDER_SEQ_CST || ord == MEMORDER_CONSUME);
        if (use_acquire) {
            if (sz == 1) asm_ldarb(cg_sec, r, r_addr); // ldarb r, [r_addr]
            else if (sz == 2)
                asm_ldarh(cg_sec, r, r_addr); // ldarh r, [r_addr]
            else
                asm_ldar(cg_sec, r, r_addr, sz); // ldar r, [r_addr]
        } else
            emit_load(node->ty, r, r_addr, 0);
#else
        if (sz < 4) {
            if (sz == 1) {
                if (use_unsigned(node->ty))
                    asm_movzx_mem_reg(cg_sec, r, r_addr, 4, 1); // movzbl (%r_addr), rr
                else
                    asm_movsx_mem_reg(cg_sec, r, r_addr, 4, 1); // movsbl (%r_addr), rr
            } else {
                if (use_unsigned(node->ty))
                    asm_movzx_mem_reg(cg_sec, r, r_addr, 4, 2); // movzwl (%r_addr), rr
                else
                    asm_movsx_mem_reg(cg_sec, r, r_addr, 4, 2); // movswl (%r_addr), rr
            }
        } else if (sz == 4) {
            asm_mov_mem_reg(cg_sec, r, r_addr, 4); // movl (%r_addr), rr
        } else {
            asm_mov_mem_reg(cg_sec, r, r_addr, 8); // movq (%r_addr), rr
        }
        if (ord == MEMORDER_SEQ_CST)
            asm_mfence(cg_sec); // mfence
#endif
        free_reg(r_addr);
#ifdef ARCH_ARM64
        if (use_acquire && sz < 8 && !use_unsigned(node->ty))
            sign_extend_to(r, sz, 8);
#endif
        return r;
    }
    case ND_ATOMIC_STORE: {
        VReg r_addr = gen(node->lhs);
        VReg r_val = gen(node->rhs);
        int sz = node->lhs->ty && node->lhs->ty->base ? node->lhs->ty->base->size : 4;
        if (sz < 1 || sz > 8) sz = 4;
        int ord = node->atomic_ord;
#ifdef ARCH_ARM64
        bool use_release = (ord == MEMORDER_RELEASE || ord == MEMORDER_ACQ_REL || ord == MEMORDER_SEQ_CST);
        if (use_release) {
            if (sz == 1) asm_stlrb(cg_sec, r_val, r_addr); // stlrb r_val, [r_addr]
            else if (sz == 2)
                asm_stlrh(cg_sec, r_val, r_addr); // stlrh r_val, [r_addr]
            else
                asm_stlr(cg_sec, r_val, r_addr, sz); // stlr r_val, [r_addr]
        } else
            emit_store(node->lhs->ty->base ? node->lhs->ty->base : ty_int, r_val, r_addr, 0);
        if (ord == MEMORDER_SEQ_CST)
            asm_dmb(cg_sec); // mfence
#else
        asm_mov_reg_mem(cg_sec, r_val, r_addr, sz); // mov r_val, (r_addr)
        if (ord == MEMORDER_SEQ_CST)
            asm_mfence(cg_sec); // mfence
#endif
        free_reg(r_val);
        free_reg(r_addr);
        return -1;
    }
    case ND_ATOMIC_EXCHANGE: {
        VReg r_addr = gen(node->lhs);
        VReg r_val = gen(node->rhs);
        int sz = node->ty->size;
        VReg r_result = alloc_reg();
#ifdef ARCH_ARM64
        int lbl = rcc_label_count++;
        cg_def_label(format(".L.atom_xchg.%d", lbl));
        asm_ldxr(cg_sec, r_result, r_addr, sz); // ldxr[b/h] r_result, [r_addr]
        asm_stxr(cg_sec, r_val, r_addr, sz); // stxr[b/h] w9, r_val, [r_addr]
        {
            size_t _cj = cg_sec->len;
            arm64_cbnz(cg_sec, 0, ARM64_X9, 0); // cbnz w9, .L.atom_xchg.lbl
            asm_record(ASM_JCC, _cj, 1, ARM64_X9, -1, -1, 4, 0, 0, NULL, ARM64_NE, -1, false);
            asm_fixup_add(cg_sec, _cj, format(".L.atom_xchg.%d", lbl), 1);
        }
        if (node->atomic_ord == MEMORDER_SEQ_CST || node->atomic_ord == MEMORDER_ACQ_REL)
            asm_dmb(cg_sec); // dmb ish
#else
        asm_xchg_mem(cg_sec, r_addr, r_val, sz); // xchg (r_addr), r_val
        if (sz < 4) {
            asm_mov_reg_reg(cg_sec, r_result, r_val, 8); // mov rr_val -> rr_result
            if (use_unsigned(node->ty))
                zero_extend_to(r_result, sz, 4);
            else
                sign_extend_to(r_result, sz, 4);
        } else {
            asm_mov_reg_reg(cg_sec, r_result, r_val, 8); // mov rr_val -> rr_result
        }
#endif
        free_reg(r_val);
        free_reg(r_addr);
#ifdef ARCH_ARM64
        if (sz < 8 && !use_unsigned(node->ty))
            sign_extend_to(r_result, sz, 8);
#endif
        return r_result;
    }
    case ND_ATOMIC_CAS: {
        VReg r_addr = gen(node->lhs);
        VReg r_expectedaddr = gen(node->body);
        VReg r_desired = gen(node->rhs);
        int sz = node->lhs->ty && node->lhs->ty->base ? node->lhs->ty->base->size : 4;
        VReg r_result = alloc_reg();
#ifdef ARCH_ARM64
        VReg r_expected = alloc_reg();
        Type *elem_ty = node->lhs->ty && node->lhs->ty->base ? node->lhs->ty->base : ty_int;
        emit_load(elem_ty, r_expected, r_expectedaddr, 0);
        VReg r_old = alloc_reg();
        int lbl = rcc_label_count++;
        cg_def_label(format(".L.atom_cas.%d", lbl));
        asm_ldxr(cg_sec, r_old, r_addr, sz); // ldxr[b/h] r_old, [r_addr]
        asm_cmp_reg_reg(cg_sec, r_old, r_expected, sz > 4 ? 8 : 4); // cmp r_old, r_expected
        {
            size_t _cj = asm_jcc_label(cg_sec, ARM64_NE);
            asm_fixup_add(cg_sec, _cj, format(".L.atom_cas_fail.%d", lbl), 1);
        }
        asm_stxr(cg_sec, r_desired, r_addr, sz); // stxr[b/h] w9, r_desired, [r_addr]
        if (node->atomic_weak) {
            size_t _cj = cg_sec->len;
            arm64_cbnz(cg_sec, 0, ARM64_X9, 0); // cbnz w9, .L.atom_cas_fail.lbl
            asm_record(ASM_JCC, _cj, 1, ARM64_X9, -1, -1, 4, 0, 0, NULL, ARM64_NE, -1, false);
            asm_fixup_add(cg_sec, _cj, format(".L.atom_cas_fail.%d", lbl), 1);
        } else {
            size_t _cj = cg_sec->len;
            arm64_cbnz(cg_sec, 0, ARM64_X9, 0); // cbnz w9, .L.atom_cas.lbl
            asm_record(ASM_JCC, _cj, 1, ARM64_X9, -1, -1, 4, 0, 0, NULL, ARM64_NE, -1, false);
            asm_fixup_add(cg_sec, _cj, format(".L.atom_cas.%d", lbl), 1);
        }
        asm_mov_imm(cg_sec, r_result, 8, 1); // mov $1, rr_result
        {
            size_t _jmp = asm_jmp_label(cg_sec);
            asm_fixup_add(cg_sec, _jmp, format(".L.atom_cas_done.%d", lbl), 0);
        }
        cg_def_label(format(".L.atom_cas_fail.%d", lbl));
        asm_movq_zero(cg_sec, r_result); // xor rr_result, rr_result
        emit_store(elem_ty, r_old, r_expectedaddr, 0);
        cg_def_label(format(".L.atom_cas_done.%d", lbl));
        free_reg(r_old);
        free_reg(r_expected);
#else
        VReg r_expected = alloc_reg();
        // Load expected value from r_expectedaddr into RAX
        {
            X86Mem mex = {REG(r_expectedaddr), X86_NOREG, 1, 0};
            if (sz == 1) x86_movzx_rm(cg_sec, 4, 1, X86_RAX, mex); // movzbl (r_expectedaddr), %eax
            else if (sz == 2)
                x86_movzx_rm(cg_sec, 4, 2, X86_RAX, mex); // movzwl (r_expectedaddr), %eax
            else if (sz == 4)
                x86_mov_rm(cg_sec, 4, X86_RAX, mex); // movl (r_expectedaddr), %eax
            else
                x86_mov_rm(cg_sec, 8, X86_RAX, mex); // movq (r_expectedaddr), %rax
        }
        asm_lock_cmpxchg_mem(cg_sec, r_addr, r_desired, sz); // lock cmpxchg (r_addr), r_desired
        asm_sete(cg_sec, r_result); // sete r_result8
        asm_movzx(cg_sec, r_result, r_result, 4, 1); // movzx rr_result -> rr_result
        x86_mov_mr(cg_sec, sz, x86_mem(REG(r_expectedaddr), 0), X86_RAX);
        free_reg(r_expected);
#endif
        free_reg(r_desired);
        free_reg(r_expectedaddr);
        free_reg(r_addr);
        return r_result;
    }
    case ND_ATOMIC_FENCE: {
        int ord = node->atomic_ord;
        if (!node->atomic_signal_fence) {
            if (ord == MEMORDER_SEQ_CST || ord == MEMORDER_ACQ_REL) {
#ifdef ARCH_ARM64
                asm_dmb(cg_sec); // dmb ish
#else
                x86_mfence(cg_sec); // mfence
#endif
            } else if (ord == MEMORDER_ACQUIRE || ord == MEMORDER_CONSUME) {
#ifdef ARCH_ARM64
                asm_dmb(cg_sec); // mfence
#endif
            } else if (ord == MEMORDER_RELEASE) {
#ifdef ARCH_ARM64
                asm_dmb(cg_sec); // dmb ishld
#endif
            }
        }
        return -1;
    }
    case ND_ATOMIC_FETCH_OP: {
        VReg r_addr = gen(node->lhs);
        VReg r_val = gen(node->rhs);
        int sz = node->ty->size;
        int op = node->atomic_fetch_op;
        bool is_store = node->atomic_is_store;
#ifdef ARCH_ARM64
        int old_dummy = alloc_reg();
        int old_slot = spill_offset(old_dummy);
        free_reg(old_dummy);
        VReg r_tmp = alloc_reg();
        int sf = (sz == 8) ? 1 : 0;
        // Move r_val to physical w9/x9 for the operation
        asm_mov_x9_from_vreg(cg_sec, r_val, sf == 1 ? 8 : 4); // mov w9/x9, r_val
        free_reg(r_val);
        int lbl = rcc_label_count++;
        cg_def_label(format(".L.atom_fop.%d", lbl));
        asm_ldxr(cg_sec, r_tmp, r_addr, sz); // ldxr[b/h] r_tmp, [r_addr]
        asm_stur_fp(cg_sec, r_tmp, old_slot); // str x(r_tmp), [x29, #-old_slot]
        switch (op) {
        case 0: { // add r_tmp, r_tmp, x9
            size_t _off = cg_sec->len;
            arm64_add_reg(cg_sec, sf, REG(r_tmp), REG(r_tmp), ARM64_X9, ARM64_LSL, 0);
            asm_record(ASM_ADD_RR, _off, 1, REG(r_tmp), ARM64_X9, -1, sz, 0, 0, NULL, 0, -1, false);
            break;
        }
        case 1: { // sub r_tmp, r_tmp, x9
            size_t _off = cg_sec->len;
            arm64_sub_reg(cg_sec, sf, REG(r_tmp), REG(r_tmp), ARM64_X9, ARM64_LSL, 0);
            asm_record(ASM_SUB_RR, _off, 1, REG(r_tmp), ARM64_X9, -1, sz, 0, 0, NULL, 0, -1, false);
            break;
        }
        case 2: { // orr r_tmp, r_tmp, x9
            size_t _off = cg_sec->len;
            arm64_orr_reg(cg_sec, sf, REG(r_tmp), REG(r_tmp), ARM64_X9, ARM64_LSL, 0);
            asm_record(ASM_OR_RR, _off, 1, REG(r_tmp), ARM64_X9, -1, sz, 0, 0, NULL, 0, -1, false);
            break;
        }
        case 3: { // eor r_tmp, r_tmp, x9
            size_t _off = cg_sec->len;
            arm64_eor_reg(cg_sec, sf, REG(r_tmp), REG(r_tmp), ARM64_X9, ARM64_LSL, 0);
            asm_record(ASM_XOR_RR, _off, 1, REG(r_tmp), ARM64_X9, -1, sz, 0, 0, NULL, 0, -1, false);
            break;
        }
        case 4: { // and r_tmp, r_tmp, x9
            size_t _off = cg_sec->len;
            arm64_and_reg(cg_sec, sf, REG(r_tmp), REG(r_tmp), ARM64_X9, ARM64_LSL, 0);
            asm_record(ASM_AND_RR, _off, 1, REG(r_tmp), ARM64_X9, -1, sz, 0, 0, NULL, 0, -1, false);
            break;
        }
        case 5: { // and r_tmp, r_tmp, x9; mvn r_tmp, r_tmp
            size_t _off = cg_sec->len;
            arm64_and_reg(cg_sec, sf, REG(r_tmp), REG(r_tmp), ARM64_X9, ARM64_LSL, 0);
            asm_record(ASM_AND_RR, _off, 1, REG(r_tmp), ARM64_X9, -1, sz, 0, 0, NULL, 0, -1, false);
            asm_not(cg_sec, r_tmp, sz);
            break;
        }
        }
        // stxr w8, r_tmp, [r_addr]
        asm_stxr_8(cg_sec, r_tmp, r_addr, sz); // stxr[b/h] w8, r_tmp, [r_addr]
        // cbnz w8, .L.atom_fop.lbl
        {
            size_t _cj = cg_sec->len;
            arm64_cbnz(cg_sec, 0, ARM64_X8, 0); // cbnz w8, .L.atom_fop.lbl
            asm_record(ASM_JCC, _cj, 1, ARM64_X8, -1, -1, 4, 0, 0, NULL, ARM64_NE, -1, false);
            asm_fixup_add(cg_sec, _cj, format(".L.atom_fop.%d", lbl), 1);
        }
        if (node->atomic_ord == MEMORDER_SEQ_CST)
            asm_dmb(cg_sec); // stxrh w8, %s, [%s]
        if (!is_store)
            asm_ldur_fp(cg_sec, r_tmp, old_slot); // ldr x(r_tmp), [x29, #-old_slot]
        free_reg(r_addr);
        if (sz < 8 && !use_unsigned(node->ty))
            sign_extend_to(r_tmp, sz, 8);
        return r_tmp;
#else
        VReg r_old = alloc_reg();
        if (op == 0 || op == 1) {
            if (op == 1)
                asm_neg(cg_sec, r_val, sz); // neg rr_val
            // Save val to stack before xadd clobbers it
            asm_mov_rbp_spill(cg_sec, r_val, sz, spill_logand); // mov r_val, -spill_logand(%rbp)
            asm_mov_reg_reg(cg_sec, r_old, r_val, sz > 4 ? 8 : 4); // mov rr_val -> rr_old
            free_reg(r_val);
            if (r_old == r_addr && (spilled_regs & (1 << r_addr))) {
                asm_mov_rbp_reg(cg_sec, r_addr, 8, spill_offset(r_addr)); // mov [rbp-8], rr_addr
            }
            asm_lock_xadd_mem(cg_sec, r_addr, r_old, sz); // lock xadd (r_addr), r_old
            free_reg(r_addr);
            if (node->atomic_ord == MEMORDER_SEQ_CST)
                asm_mfence(cg_sec); // mfence
            if (is_store) {
                // add_fetch/sub_fetch: return new value = old + val
                asm_add_spill_reg(cg_sec, r_old, sz, spill_logand); // add -spill_logand(%rbp), r_old
            }
            if (sz < 4) {
                if (!use_unsigned(node->ty))
                    sign_extend_to(r_old, sz, 4);
                else
                    zero_extend_to(r_old, sz, 4);
            }
            return r_old;
        } else {
            // Save r_val to stack before it might be spilled
            asm_mov_rbp_spill(cg_sec, r_val, sz, spill_logand); // mov r_val, -spill_logand(%rbp)
            free_reg(r_val);
            VReg r_new = alloc_reg();
            int lbl2 = rcc_label_count++;
            cg_def_label(format(".L.atom_fop.%d", lbl2));
            if (r_new == r_addr && (spilled_regs & (1 << r_addr))) {
                asm_mov_rbp_reg(cg_sec, r_addr, 8, spill_offset(r_addr)); // mov [rbp-8], rr_addr
            } else if (r_old == r_addr && (spilled_regs & (1 << r_addr))) {
                asm_mov_rbp_reg(cg_sec, r_addr, 8, spill_offset(r_addr)); // mov [rbp-8], rr_addr
            }
            asm_mov_mem_reg(cg_sec, r_old, r_addr, sz); // mov (r_addr), r_old
            // Save old value before computing new (r_old may == r_new)
            asm_mov_rbp_spill(cg_sec, r_old, sz, spill_atomic_old); // mov r_old, -spill_atomic_old(%rbp)
            asm_mov_reg_reg(cg_sec, r_new, r_old, sz > 4 ? 8 : 4); // mov r_old, r_new
            char sc = size_suffix(sz);
            (void)sc;
            switch (op) {
            case 2: asm_or_rbp_reg(cg_sec, r_new, sz, spill_logand); break;
            case 3: asm_xor_rbp_reg(cg_sec, r_new, sz, spill_logand); break;
            case 4: asm_and_rbp_reg(cg_sec, r_new, sz, spill_logand); break;
            case 5:
                asm_and_rbp_reg(cg_sec, r_new, sz, spill_logand); // and -spill_logand(%rbp), r_new
                asm_not(cg_sec, r_new, sz); // not rr_new
                break;
            }
            // lock cmpxchg (r_addr), r_new  (rax=r_old loaded from spill_atomic_old)
            asm_mov_rbp(cg_sec, X86_RAX, sz, spill_atomic_old); // mov -spill_atomic_old(%rbp), %rax
            {
                size_t _cj = asm_lock_cmpxchg_rax(cg_sec, r_addr, r_new, sz);
                asm_jcc_label(cg_sec, X86_NE); // emit JNE first so fixup can patch it
                asm_fixup_add(cg_sec, _cj, format(".L.atom_fop.%d", lbl2), 1);
            }
            if (node->atomic_ord == MEMORDER_SEQ_CST)
                asm_mfence(cg_sec); // mfence
            free_reg(r_addr);
            if (is_store) {
                free_reg(r_old);
                if (sz < 4 && !use_unsigned(node->ty))
                    sign_extend_to(r_new, sz, 4);
                else if (sz < 4)
                    zero_extend_to(r_new, sz, 4);
                return r_new;
            } else {
                free_reg(r_new);
                // r_old has the old value (loaded before cmpxchg)
                asm_mov_spill_rbp(cg_sec, r_old, sz, spill_atomic_old); // reload old from spill
                if (sz < 4 && !use_unsigned(node->ty))
                    sign_extend_to(r_old, sz, 4);
                else if (sz < 4)
                    zero_extend_to(r_old, sz, 4);
                return r_old;
            }
        }
#endif
        return -1;
    }

    default:
        break;
    }
    // Complex binary arithmetic: add/sub with complex operands
    if ((node->kind == ND_ADD || node->kind == ND_SUB || node->kind == ND_MUL || node->kind == ND_DIV) && is_complex(node->ty)) {
        int complex_sz = node->ty->size;
        int base_sz = node->ty->base->size;
        bool lhs_cx = is_complex(node->lhs->ty);
        bool rhs_cx = is_complex(node->rhs->ty);
        // Non-complex operands must be converted to the complex base type and
        // materialized as a {real, imag} pair, so never reuse their plain
        // scalar address (it is too small and holds the wrong representation).
        int addr_lhs = lhs_cx ? gen_addr(node->lhs) : -1;
        int addr_rhs = rhs_cx ? gen_addr(node->rhs) : -1;
        int need_free_lhs = 0, need_free_rhs = 0;
        if (addr_lhs < 0) {
            need_free_lhs = 1;
            addr_lhs = alloc_reg();
#ifdef ARCH_ARM64
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // sub sp, sp, #N
            asm_mov_reg_sp(cg_sec, REG(addr_lhs)); // mov x{addr_lhs}, sp
#else
            asm_sub_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // subq $N, %rsp
            x86_mov_rr(cg_sec, 8, REG(addr_lhs), X86_RSP); // movq %rsp, addr_lhs
#endif
            int v = gen(node->lhs);
            if (lhs_cx) {
                // gen() returned address — copy complex data
#ifdef ARCH_ARM64
                asm_ldr_x16_reg_uoff(cg_sec, v, 0); // ldr x16, [v]
                asm_str_x16_reg_uoff(cg_sec, addr_lhs, 0); // str x16, [addr_lhs]
                if (complex_sz > 8) {
                    asm_ldr_x16_reg_uoff(cg_sec, v, base_sz); // ldr x16, [v, #base_sz]
                    asm_str_x16_reg_uoff(cg_sec, addr_lhs, base_sz); // str x16, [addr_lhs, #base_sz]
                }
#else
                asm_mov_reg_reg(cg_sec, 0, v, 8);
                asm_mov_reg_reg(cg_sec, addr_lhs, 0, 8);
                if (complex_sz > 8) {
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(v), base_sz)); // movq base_sz(v), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(addr_lhs), base_sz), X86_RAX); // movq %rax, base_sz(addr_lhs)
                }
#endif
            } else {
                emit_scalar_to_complex(v, node->lhs->ty, node->ty->base, addr_lhs);
            }
            free_reg(v);
        }
        if (addr_rhs < 0) {
            need_free_rhs = 1;
            addr_rhs = alloc_reg();
#ifdef ARCH_ARM64
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // sub sp, sp, #N
            asm_mov_reg_sp(cg_sec, REG(addr_rhs)); // mov x{addr_rhs}, sp
#else
            asm_sub_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // subq $N, %rsp
            x86_mov_rr(cg_sec, 8, REG(addr_rhs), X86_RSP); // movq %rsp, addr_rhs
#endif
            int v = gen(node->rhs);
            if (rhs_cx) {
#ifdef ARCH_ARM64
                asm_ldr_x16_reg_uoff(cg_sec, v, 0); // ldr x16, [v]
                asm_str_x16_reg_uoff(cg_sec, addr_rhs, 0); // str x16, [addr_rhs]
                if (complex_sz > 8) {
                    asm_ldr_x16_reg_uoff(cg_sec, v, base_sz); // ldr x16, [v, #base_sz]
                    asm_str_x16_reg_uoff(cg_sec, addr_rhs, base_sz); // str x16, [addr_rhs, #base_sz]
                }
#else
                asm_mov_reg_reg(cg_sec, 0, v, 8);
                asm_mov_reg_reg(cg_sec, addr_rhs, 0, 8);
                if (complex_sz > 8) {
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(v), base_sz)); // movq base_sz(v), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(addr_rhs), base_sz), X86_RAX); // movq %rax, base_sz(addr_rhs)
                }
#endif
            } else {
                emit_scalar_to_complex(v, node->rhs->ty, node->ty->base, addr_rhs);
            }
            free_reg(v);
        }
        // Result allocated from fn_struct_ret_total (stable across calls/returns)
        int alloc = (complex_sz + 7) & ~7;
        fn_struct_ret_off += alloc;
        if (fn_struct_ret_off > fn_struct_ret_total)
            fn_struct_ret_total = fn_struct_ret_off;
        int result_off = current_fn_stack_size + fn_struct_ret_off;
        int result = alloc_reg();
#ifdef ARCH_ARM64
        asm_sub_reg_fp_imm(cg_sec, result, result_off); // sub result, x29, #result_off
#else
        asm_lea_rbp_reg(cg_sec, result, 8, result_off);
#endif
        if (is_flonum(node->ty->base)) {
#ifdef ARCH_ARM64
            int ftype = base_sz == 4 ? 0 : 1;
            int sf = base_sz == 4 ? 0 : 1;
            asm_ldr_fp(cg_sec, 0, addr_lhs, base_sz);
            asm_ldr_fp_off(cg_sec, 1, addr_lhs, base_sz, (uint32_t)base_sz);
            asm_ldr_fp(cg_sec, 2, addr_rhs, base_sz);
            asm_ldr_fp_off(cg_sec, 3, addr_rhs, base_sz, (uint32_t)base_sz);
            if (node->kind == ND_ADD || node->kind == ND_SUB) {
                if (node->kind == ND_ADD) {
                    arm64_fadd(cg_sec, ftype, 0, 0, 2);
                    arm64_fadd(cg_sec, ftype, 1, 1, 3);
                } else {
                    arm64_fsub(cg_sec, ftype, 0, 0, 2);
                    arm64_fsub(cg_sec, ftype, 1, 1, 3);
                }
            } else if (node->kind == ND_MUL) {
                // (a+bi)*(c+di) = (ac-bd) + (ad+bc)i
                int t = alloc_reg();
                arm64_fmov_f2i(cg_sec, sf, REG(t), 0);
                arm64_fmov_i2f(cg_sec, sf, 4, REG(t));
                free_reg(t);
                t = alloc_reg();
                arm64_fmov_f2i(cg_sec, sf, REG(t), 1);
                arm64_fmov_i2f(cg_sec, sf, 5, REG(t));
                free_reg(t);
                arm64_fmul(cg_sec, ftype, 4, 4, 2);
                arm64_fmul(cg_sec, ftype, 5, 5, 3);
                arm64_fsub(cg_sec, ftype, 4, 4, 5);
                arm64_fmul(cg_sec, ftype, 0, 0, 3);
                arm64_fmul(cg_sec, ftype, 1, 1, 2);
                arm64_fadd(cg_sec, ftype, 1, 0, 1);
                t = alloc_reg();
                arm64_fmov_f2i(cg_sec, sf, REG(t), 4);
                arm64_fmov_i2f(cg_sec, sf, 0, REG(t));
                free_reg(t);
            } else {
                // Complex float division via libgcc: d0/d1 = lhs, d2/d3 = rhs.
                int result_spill = spill_offset(result);
                asm_stur_fp(cg_sec, result, result_spill); // str x{result}, [x29, #-spill_offset]
                emit_direct_call(base_sz == 4 ? "__divsc3" : "__divdc3"); // bl __divsc3/__divdc3
                asm_ldur_fp(cg_sec, result, result_spill); // ldr x{result}, [x29, #-spill_offset]
                asm_str_fp(cg_sec, 0, result, base_sz);
                arm64_str_fp(cg_sec, base_sz == 4 ? 2 : 3, 1, REG(result), (uint32_t)base_sz);
                goto cx_arith_done;
            }
            if (node->kind != ND_DIV) {
                asm_str_fp(cg_sec, 0, result, base_sz);
                arm64_str_fp(cg_sec, base_sz == 4 ? 2 : 3, 1, REG(result), (uint32_t)base_sz);
            }
#else
            int sfx = base_sz; // 4 -> ss, 8 -> sd
            X86Mem m_lhs0 = x86_mem(REG(addr_lhs), 0);
            X86Mem m_lhs1 = x86_mem(REG(addr_lhs), base_sz);
            X86Mem m_rhs0 = x86_mem(REG(addr_rhs), 0);
            X86Mem m_rhs1 = x86_mem(REG(addr_rhs), base_sz);
            X86Mem m_res0 = x86_mem(REG(result), 0);
            X86Mem m_res1 = x86_mem(REG(result), base_sz);
            asm_mov_fp_rm(cg_sec, sfx, X86_XMM0, m_lhs0); // mov[s|s]s/d (addr_lhs), %xmm0
            asm_mov_fp_rm(cg_sec, sfx, X86_XMM1, m_lhs1); // mov[s|s]s/d base_sz(addr_lhs), %xmm1
            asm_mov_fp_rm(cg_sec, sfx, X86_XMM2, m_rhs0); // mov[s|s]s/d (addr_rhs), %xmm2
            asm_mov_fp_rm(cg_sec, sfx, X86_XMM3, m_rhs1); // mov[s|s]s/d base_sz(addr_rhs), %xmm3
            if (node->kind == ND_ADD || node->kind == ND_SUB) {
                if (node->kind == ND_ADD) {
                    asm_add_fp(cg_sec, sfx, X86_XMM0, X86_XMM2); // add[s|s]s/d %xmm2, %xmm0
                    asm_add_fp(cg_sec, sfx, X86_XMM1, X86_XMM3); // add[s|s]s/d %xmm3, %xmm1
                } else {
                    asm_sub_fp(cg_sec, sfx, X86_XMM0, X86_XMM2); // sub[s|s]s/d %xmm2, %xmm0
                    asm_sub_fp(cg_sec, sfx, X86_XMM1, X86_XMM3); // sub[s|s]s/d %xmm3, %xmm1
                }
            } else if (node->kind == ND_MUL) {
                // (a+bi)*(c+di) = (ac-bd) + (ad+bc)i
                // xmm0=a.r, xmm1=a.i, xmm2=b.r, xmm3=b.i
                asm_movapd_rr(cg_sec, X86_XMM4, X86_XMM0); // movapd %xmm0, %xmm4
                asm_movapd_rr(cg_sec, X86_XMM5, X86_XMM1); // movapd %xmm1, %xmm5
                asm_mul_fp(cg_sec, sfx, X86_XMM4, X86_XMM2); // mul[s|s]s/d %xmm2, %xmm4  -- a.r * b.r
                asm_mul_fp(cg_sec, sfx, X86_XMM5, X86_XMM3); // mul[s|s]s/d %xmm3, %xmm5  -- a.i * b.i
                asm_sub_fp(cg_sec, sfx, X86_XMM4, X86_XMM5); // sub[s|s]s/d %xmm5, %xmm4  -- ac - bd = real
                asm_mul_fp(cg_sec, sfx, X86_XMM0, X86_XMM3); // mul[s|s]s/d %xmm3, %xmm0  -- a.r * b.i
                asm_mul_fp(cg_sec, sfx, X86_XMM1, X86_XMM2); // mul[s|s]s/d %xmm2, %xmm1  -- a.i * b.r
                asm_add_fp(cg_sec, sfx, X86_XMM1, X86_XMM0); // add[s|s]s/d %xmm0, %xmm1  -- ad + bc = imag
                asm_movapd_rr(cg_sec, X86_XMM0, X86_XMM4); // movapd %xmm4, %xmm0
            } else {
                // (a+bi)/(c+di) = ((ac+bd)/(cc+dd)) + ((bc-ad)/(cc+dd))i
                // xmm0=a.r, xmm1=a.i, xmm2=b.r, xmm3=b.i
                asm_movapd_rr(cg_sec, X86_XMM6, X86_XMM2); // movapd %xmm2, %xmm6  -- save b.r
                asm_movapd_rr(cg_sec, X86_XMM7, X86_XMM3); // movapd %xmm3, %xmm7  -- save b.i
                asm_movapd_rr(cg_sec, X86_XMM4, X86_XMM0); // movapd %xmm0, %xmm4  -- save a.r
                asm_movapd_rr(cg_sec, X86_XMM5, X86_XMM1); // movapd %xmm1, %xmm5  -- save a.i
                // denom = b.r² + b.i²
                asm_mul_fp(cg_sec, sfx, X86_XMM2, X86_XMM2); // mul[s|s]s/d %xmm2, %xmm2  -- b.r²
                asm_mul_fp(cg_sec, sfx, X86_XMM3, X86_XMM3); // mul[s|s]s/d %xmm3, %xmm3  -- b.i²
                asm_add_fp(cg_sec, sfx, X86_XMM2, X86_XMM3); // add[s|s]s/d %xmm3, %xmm2  -- denom in xmm2
                // real = (a.r*b.r + a.i*b.i) / denom
                asm_mul_fp(cg_sec, sfx, X86_XMM4, X86_XMM6); // mul[s|s]s/d %xmm6, %xmm4  -- a.r * b.r
                asm_mul_fp(cg_sec, sfx, X86_XMM5, X86_XMM7); // mul[s|s]s/d %xmm7, %xmm5  -- a.i * b.i
                asm_add_fp(cg_sec, sfx, X86_XMM4, X86_XMM5); // add[s|s]s/d %xmm5, %xmm4  -- ac+bd
                asm_div_fp(cg_sec, sfx, X86_XMM4, X86_XMM2); // div[s|s]s/d %xmm2, %xmm4  -- real = (ac+bd)/denom
                // imag = (a.i*b.r - a.r*b.i) / denom
                asm_mul_fp(cg_sec, sfx, X86_XMM1, X86_XMM6); // mul[s|s]s/d %xmm6, %xmm1  -- a.i * b.r
                asm_mul_fp(cg_sec, sfx, X86_XMM0, X86_XMM7); // mul[s|s]s/d %xmm7, %xmm0  -- a.r * b.i
                asm_sub_fp(cg_sec, sfx, X86_XMM1, X86_XMM0); // sub[s|s]s/d %xmm0, %xmm1  -- bc-ad
                asm_div_fp(cg_sec, sfx, X86_XMM1, X86_XMM2); // div[s|s]s/d %xmm2, %xmm1  -- imag = (bc-ad)/denom
                asm_movapd_rr(cg_sec, X86_XMM0, X86_XMM4); // movapd %xmm4, %xmm0
            }
            asm_mov_fp_mr(cg_sec, sfx, m_res0, X86_XMM0); // mov[s|s]s/d %xmm0, (result)
            asm_mov_fp_mr(cg_sec, sfx, m_res1, X86_XMM1); // mov[s|s]s/d %xmm1, base_sz(result)
#endif
        } else {
#ifdef ARCH_ARM64
            arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), 0);
            if (complex_sz <= 8) {
                arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_rhs), 0);
                switch (node->kind) {
                case ND_ADD:
                    arm64_add_reg(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldrsw_uoff(cg_sec, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 4));
                    arm64_ldrsw_uoff(cg_sec, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_add_reg(cg_sec, 0, ARM64_X18, ARM64_X17, ARM64_X18, ARM64_LSL, 0);
                    arm64_ubfx(cg_sec, 1, ARM64_X16, ARM64_X16, 0, 32);
                    arm64_orr_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X18, ARM64_LSL, 32);
                    break;
                case ND_SUB:
                    arm64_sub_reg(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldrsw_uoff(cg_sec, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 4));
                    arm64_ldrsw_uoff(cg_sec, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_sub_reg(cg_sec, 0, ARM64_X18, ARM64_X17, ARM64_X18, ARM64_LSL, 0);
                    arm64_ubfx(cg_sec, 1, ARM64_X16, ARM64_X16, 0, 32);
                    arm64_orr_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X18, ARM64_LSL, 32);
                    break;
                case ND_MUL: {
                    // (a+bi)(c+di) = (ac-bd) + (bc+ad)i — integer arithmetic
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X16, REG(addr_lhs), 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X18, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X18);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 4));
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_mul(cg_sec, 0, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_sub_reg(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 4));
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X18, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 0, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X9, REG(addr_lhs), 0);
                    arm64_mul(cg_sec, 0, ARM64_X9, ARM64_X9, ARM64_X18);
                    arm64_add_reg(cg_sec, 0, ARM64_X17, ARM64_X17, ARM64_X9, ARM64_LSL, 0);
                    arm64_str_uoff(cg_sec, 2, ARM64_X16, REG(result), 0);
                    arm64_str_uoff(cg_sec, 2, ARM64_X17, REG(result), (uint32_t)(base_sz / 4));
                    goto cx_arith_done;
                }
                case ND_DIV: {
                    // (a+bi)/(c+di) = ((ac+bd)/(c²+d²)) + ((bc-ad)/(c²+d²))i
                    const char *dv = node->ty->base->is_unsigned ? "udiv" : "sdiv";
                    (void)dv;
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X16, REG(addr_rhs), 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_mul(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X16);
                    arm64_mul(cg_sec, 0, ARM64_X17, ARM64_X17, ARM64_X17);
                    arm64_add_reg(cg_sec, 0, ARM64_X9, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X16, REG(addr_lhs), 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 4));
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_mul(cg_sec, 0, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_add_reg(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    if (node->ty->base->is_unsigned)
                        arm64_udiv(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X9);
                    else
                        arm64_sdiv(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X9);
                    arm64_str_uoff(cg_sec, 2, ARM64_X16, REG(result), 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X16, REG(addr_lhs), (uint32_t)(base_sz / 4));
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_lhs), 0);
                    arm64_ldr_uoff(cg_sec, 2, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 4));
                    arm64_mul(cg_sec, 0, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_sub_reg(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    if (node->ty->base->is_unsigned)
                        arm64_udiv(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X9);
                    else
                        arm64_sdiv(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X9);
                    arm64_str_uoff(cg_sec, 2, ARM64_X16, REG(result), (uint32_t)(base_sz / 4));
                    goto cx_arith_done;
                }
                default:
                    __builtin_unreachable();
                }
                arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(result), 0);
            } else {
                arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_rhs), 0);
                switch (node->kind) {
                case ND_ADD:
                    arm64_adds_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_adc(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(result), 0);
                    arm64_str_uoff(cg_sec, 3, ARM64_X17, REG(result), (uint32_t)(base_sz / 8));
                    goto cx_arith_done;
                case ND_SUB:
                    arm64_subs_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_sbc(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(result), 0);
                    arm64_str_uoff(cg_sec, 3, ARM64_X17, REG(result), (uint32_t)(base_sz / 8));
                    goto cx_arith_done;
                case ND_MUL: {
                    // (a+bi)(c+di) = (ac-bd) + (bc+ad)i — 64-bit integer arithmetic
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X18);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_mul(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_sub_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X9, REG(addr_lhs), 0);
                    arm64_mul(cg_sec, 1, ARM64_X9, ARM64_X9, ARM64_X18);
                    arm64_add_reg(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X9, ARM64_LSL, 0);
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(result), 0);
                    arm64_str_uoff(cg_sec, 3, ARM64_X17, REG(result), (uint32_t)(base_sz / 8));
                    goto cx_arith_done;
                }
                case ND_DIV: {
                    const char *dv = node->ty->base->is_unsigned ? "udiv" : "sdiv";
                    (void)dv;
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_rhs), 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_mul(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X16);
                    arm64_mul(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X17);
                    arm64_add_reg(cg_sec, 1, ARM64_X9, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_lhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_mul(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_add_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    if (node->ty->base->is_unsigned)
                        arm64_udiv(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X9);
                    else
                        arm64_sdiv(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X9);
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(result), 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), (uint32_t)(base_sz / 8));
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_rhs), 0);
                    arm64_mul(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_lhs), 0);
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X18, REG(addr_rhs), (uint32_t)(base_sz / 8));
                    arm64_mul(cg_sec, 1, ARM64_X17, ARM64_X17, ARM64_X18);
                    arm64_sub_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0);
                    if (node->ty->base->is_unsigned)
                        arm64_udiv(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X9);
                    else
                        arm64_sdiv(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X9);
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(result), (uint32_t)(base_sz / 8));
                    goto cx_arith_done;
                }
                default:
                    __builtin_unreachable();
                }
            }
#else
            // Operate on each base_sz-byte component (real/imag) in a
            // W-byte register (W=4 for char/short/int bases, 8 for long
            // bases), sign- or zero-extending narrower components on load
            // and truncating on store. Scratch registers (rax, rcx, rdx,
            // r8, r9, rdi) are all outside the alloc_reg pool ({%r10, %r11,
            // %rbx, %r12-%r15, %rsi}), so they can never collide with
            // addr_lhs/addr_rhs/result or any live register from an
            // enclosing expression.
            bool base_unsigned = node->ty->base->is_unsigned;
            int W = base_sz <= 4 ? 4 : 8;
            const char *rax_w = W == 4 ? "%eax" : "%rax";
            const char *rdx_w = W == 4 ? "%edx" : "%rdx";
            const char *rcx_w = W == 4 ? "%ecx" : "%rcx";
            const char *r8_w = W == 4 ? "%r8d" : "%r8";
            const char *r9_w = W == 4 ? "%r9d" : "%r9";
            const char *rdi_w = W == 4 ? "%edi" : "%rdi";
#define CX_LOAD(memreg, off, dstw) do { \
    if (base_sz == W) \
        printf("  mov %d(%s), %s\n", off, memreg, dstw); \
    else if (base_sz == 1) \
        printf("  %s %d(%s), %s\n", base_unsigned ? "movzbl" : "movsbl", off, memreg, dstw); \
    else \
        printf("  %s %d(%s), %s\n", base_unsigned ? "movzwl" : "movswl", off, memreg, dstw); \
} while (0)
            CX_LOAD(reg64[addr_lhs], 0, rax_w); // a = lhs real
            CX_LOAD(reg64[addr_lhs], base_sz, rdx_w); // b = lhs imag
            CX_LOAD(reg64[addr_rhs], 0, rcx_w); // c = rhs real
            CX_LOAD(reg64[addr_rhs], base_sz, r8_w); // d = rhs imag
            const char *real_reg, *imag_reg;
            switch (node->kind) {
            case ND_ADD:
                x86_add_rr(cg_sec, W, X86_RAX, X86_RCX); // add %ecx/%rcx, %eax/%rax (real = a+c)
                x86_add_rr(cg_sec, W, X86_RDX, X86_R8); // add %r8d/%r8, %edx/%rdx (imag = b+d)
                real_reg = rax_w;
                imag_reg = rdx_w;
                break;
            case ND_SUB:
                x86_sub_rr(cg_sec, W, X86_RAX, X86_RCX); // sub %ecx/%rcx, %eax/%rax (real = a-c)
                x86_sub_rr(cg_sec, W, X86_RDX, X86_R8); // sub %r8d/%r8, %edx/%rdx (imag = b-d)
                real_reg = rax_w;
                imag_reg = rdx_w;
                break;
            case ND_MUL:
                // (a+bi)*(c+di) = (ac-bd) + (ad+bc)i
                printf("  mov %s, %s\n", rax_w, r9_w);
                printf("  mov %s, %s\n", rax_w, rdi_w);
                printf("  imul %s, %s\n", rcx_w, r9_w); // r9 = a*c
                printf("  imul %s, %s\n", r8_w, rdi_w); // rdi = b*d
                printf("  sub %s, %s\n", rdi_w, r9_w); // r9 = ac-bd (real)
                printf("  imul %s, %s\n", r8_w, rax_w); // rax = a*d
                printf("  imul %s, %s\n", rcx_w, rdx_w); // rdx = b*c
                printf("  add %s, %s\n", rdx_w, rax_w); // rax = ad+bc (imag)
                printf("  mov %s, %s\n", r9_w, rdx_w); // rdx = real result
                real_reg = rdx_w;
                imag_reg = rax_w;
                break;
            case ND_DIV:
                // (a+bi)/(c+di) = ((ac+bd)/(cc+dd)) + ((bc-ad)/(cc+dd))i
                printf("  mov %s, %s\n", rax_w, r9_w);
                printf("  mov %s, %s\n", rdx_w, rdi_w);
                printf("  imul %s, %s\n", rcx_w, r9_w); // r9 = a*c
                printf("  imul %s, %s\n", r8_w, rdi_w); // rdi = b*d
                printf("  add %s, %s\n", rdi_w, r9_w); // r9 = ac+bd (num_real)
                printf("  mov %s, %s\n", rdx_w, rdi_w); // rdi = b
                printf("  imul %s, %s\n", rcx_w, rdi_w); // rdi = b*c
                printf("  imul %s, %s\n", r8_w, rax_w); // rax = a*d
                printf("  sub %s, %s\n", rax_w, rdi_w); // rdi = bc-ad (num_imag)
                printf("  mov %s, %s\n", rcx_w, rax_w); // rax = c
                printf("  imul %s, %s\n", rcx_w, rax_w); // rax = c*c
                printf("  mov %s, %s\n", r8_w, rdx_w); // rdx = d
                printf("  imul %s, %s\n", r8_w, rdx_w); // rdx = d*d
                printf("  add %s, %s\n", rdx_w, rax_w); // rax = cc+dd (denom)
                printf("  mov %s, %s\n", rax_w, rcx_w); // rcx = denom
                printf("  mov %s, %s\n", r9_w, rax_w); // rax = num_real
                printf("  %s\n", base_unsigned ? "xor %edx, %edx" : (W == 8 ? "cqo" : "cdq"));
                printf("  %s %s\n", base_unsigned ? "div" : "idiv", rcx_w); // rax = real
                printf("  push %%rax\n"); // save real result
                printf("  mov %s, %s\n", rdi_w, rax_w); // rax = num_imag
                printf("  %s\n", base_unsigned ? "xor %edx, %edx" : (W == 8 ? "cqo" : "cdq"));
                printf("  %s %s\n", base_unsigned ? "div" : "idiv", rcx_w); // rax = imag
                printf("  pop %%rdx\n"); // rdx = real result
                real_reg = rdx_w;
                imag_reg = rax_w;
                break;
            default:
                __builtin_unreachable();
            }
            if (base_sz == W) {
                printf("  mov %s, (%s)\n", real_reg, reg64[result]);
                printf("  mov %s, %d(%s)\n", imag_reg, base_sz, reg64[result]);
            } else {
                printf("  mov %s, (%s)\n", x86_subreg(real_reg, base_sz), reg64[result]);
                printf("  mov %s, %d(%s)\n", x86_subreg(imag_reg, base_sz), base_sz, reg64[result]);
            }
#undef CX_LOAD
#endif
        }
#ifdef ARCH_ARM64
    cx_arith_done:
#endif
        free_reg(addr_lhs);
        free_reg(addr_rhs);
#ifdef ARCH_ARM64
        if (need_free_lhs) arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // add sp, sp, #complex_sz
        if (need_free_rhs) arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // add sp, sp, #complex_sz
#else
        if (need_free_lhs) asm_add_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // addq $N, %rsp
        if (need_free_rhs) asm_add_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // addq $N, %rsp
#endif
        return result;
    }

    if (node->kind == ND_REAL || node->kind == ND_IMAG) {
        // gen_addr adds the offset for ND_IMAG, so the returned address
        // already points to the real or imag part of the complex value.
        // This must be handled before the unconditional gen(node->lhs)
        // below, which would otherwise evaluate node->lhs (the whole
        // complex value) into a register that is never used or freed.
        int r = gen_addr(node);
        if (is_flonum(node->ty)) {
#ifdef ARCH_ARM64
            if (node->ty->size == 4) {
                asm_ldr_fp(cg_sec, ARM64_S0, r, 4); // ldr s0, [x{r}]
                asm_fcvt(cg_sec, 1, 0, ARM64_D0, ARM64_S0); // fcvt d0, s0
            } else {
                asm_ldr_fp(cg_sec, ARM64_D0, r, 8); // ldr d0, [x{r}]
            }
            asm_fmov_f2i(cg_sec, r, 0, 1); // fmov x{r}, d0
#else
            if (node->ty->size == 4) {
                x86_movss_rm(cg_sec, X86_XMM0, x86_mem(REG(r), 0)); // movss (r), %xmm0
                x86_cvtss2sd(cg_sec, X86_XMM0, X86_XMM0); // cvtss2sd %xmm0, %xmm0
            } else {
                x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(REG(r), 0)); // movsd (r), %xmm0
            }
            asm_movq_xmm_r(cg_sec, r, X86_XMM0); // movq %xmm0, r
#endif
        } else {
            emit_load(node->ty, r, r, 0);
        }
        return r;
    }

    VReg r_lhs = gen(node->lhs);


    // Float binary operations (must come before integer ops)
    if (is_flonum(node->ty) && (node->kind == ND_ADD || node->kind == ND_SUB || node->kind == ND_MUL || node->kind == ND_DIV)) {
        VReg r_rhs = gen(node->rhs);
#ifdef ARCH_ARM64
        bool rhs_in_x16 = false;
        if (r_lhs == r_rhs && (spilled_regs & (1 << r_lhs))) {
            // alloc_reg() reused r_lhs for rhs and spilled the original lhs.
            // Preserve rhs in x16, then reload lhs before feeding d0/d1.
            asm_mov_phy_reg(cg_sec, ARM64_X16, r_rhs, 1); // mov x16, x{r_rhs}
            asm_ldur_fp(cg_sec, r_lhs, spill_offset(r_lhs)); // ldr x{r_lhs}, [x29, #-spill_offset]
            spilled_regs &= ~(1 << r_lhs);
            rhs_in_x16 = true;
        }
        asm_fmov_i2f(cg_sec, 0, r_lhs, 1); // fmov d0, x{r_lhs}
        if (rhs_in_x16)
            arm64_fmov_i2f(cg_sec, 1, ARM64_D1, ARM64_X16); // fmov d1, x16
        else
            asm_fmov_i2f(cg_sec, 1, r_rhs, 1); // fmov d1, x{r_rhs}
        if (node->kind == ND_ADD) asm_fadd(cg_sec, 1); // fadd d0, d0, d1
        else if (node->kind == ND_SUB)
            asm_fsub(cg_sec, 1); // fsub d0, d0, d1
        else if (node->kind == ND_MUL)
            asm_fmul(cg_sec, 1); // fmul d0, d0, d1
        else if (node->kind == ND_DIV)
            asm_fdiv(cg_sec, 1); // fdiv d0, d0, d1
        if (node->ty->kind == TY_FLOAT) {
            asm_fcvt(cg_sec, 0, 1, 0, 0); // fcvt s0, d0 (opc=0=double->single)
            asm_fcvt(cg_sec, 1, 0, 0, 0); // asm_fcvt(1, 0, 0, 0)
        }
        asm_fmov_f2i(cg_sec, r_lhs, 0, 1); // asm_fmov_f2i(r_lhs, 0, 1)
        if (!rhs_in_x16 && r_rhs != r_lhs)
            free_reg(r_rhs);
#else
        asm_movq_r_xmm(cg_sec, X86_XMM0, r_lhs); // fcvt s0, d0
        asm_movq_r_xmm(cg_sec, X86_XMM1, r_rhs); // fcvt d0, s0
        free_reg(r_rhs);
        __attribute__((unused)) char *inst = "";
        if (node->kind == ND_ADD) inst = "addsd";
        else if (node->kind == ND_SUB)
            inst = "subsd";
        else if (node->kind == ND_MUL)
            inst = "mulsd";
        else if (node->kind == ND_DIV)
            inst = "divsd";
        if (node->kind == ND_ADD) asm_addsd(cg_sec); // %s %%xmm1, %%xmm0
        else if (node->kind == ND_SUB)
            asm_subsd(cg_sec); // cvtsd2ss %%xmm0, %%xmm0
        else if (node->kind == ND_MUL)
            asm_mulsd(cg_sec); // cvtss2sd %%xmm0, %%xmm0
        else if (node->kind == ND_DIV)
            asm_divsd(cg_sec); // movq %%xmm0, %s
        if (node->ty->kind == TY_FLOAT) {
            asm_cvtsd2ss(cg_sec); // %s %%xmm1, %%xmm0
            asm_cvtss2sd(cg_sec); // cvtsd2ss %%xmm0, %%xmm0
        }
        asm_movq_xmm_r(cg_sec, r_lhs, X86_XMM0); // cvtss2sd %%xmm0, %%xmm0
#endif
        return r_lhs;
    }

    // Float comparisons
    if ((node->kind == ND_EQ || node->kind == ND_NE || node->kind == ND_LT || node->kind == ND_LE) &&
        node->lhs->ty && node->rhs->ty && (is_flonum(node->lhs->ty) || is_flonum(node->rhs->ty))) {
        VReg r_rhs = gen(node->rhs);
#ifdef ARCH_ARM64
        bool rhs_in_x16 = false;
        if (r_lhs == r_rhs && (spilled_regs & (1 << r_lhs))) {
            // Preserve rhs in x16, then reload the spilled lhs for comparison.
            asm_mov_phy_reg(cg_sec, ARM64_X16, r_rhs, 1); // mov x16, x{r_rhs}
            asm_ldur_fp(cg_sec, r_lhs, spill_offset(r_lhs)); // ldr x{r_lhs}, [x29, #-spill_offset]
            spilled_regs &= ~(1 << r_lhs);
            rhs_in_x16 = true;
        }
        asm_fmov_i2f(cg_sec, 0, r_lhs, 1); // fmov d0, x{r_lhs}
        if (rhs_in_x16)
            arm64_fmov_i2f(cg_sec, 1, ARM64_D1, ARM64_X16); // fmov d1, x16
        else
            asm_fmov_i2f(cg_sec, 1, r_rhs, 1); // fmov d1, x{r_rhs}
        asm_fcmp(cg_sec, 1); // fcmp d0, d1
        if (node->kind == ND_EQ) asm_cset(cg_sec, r_lhs, ARM64_EQ); // cset rr_lhs
        else if (node->kind == ND_NE)
            asm_cset(cg_sec, r_lhs, ARM64_NE); // fcmp d0, d1
        else if (node->kind == ND_LT)
            asm_cset(cg_sec, r_lhs, ARM64_MI); // cset rr_lhs
        else if (node->kind == ND_LE)
            asm_cset(cg_sec, r_lhs, ARM64_LS); // cset rr_lhs
#else
        asm_movq_r_xmm(cg_sec, X86_XMM0, r_lhs); // movq %s, %%xmm0
        asm_movq_r_xmm(cg_sec, X86_XMM1, r_rhs); // movq %s, %%xmm1
        asm_ucomisd(cg_sec); // ucomisd %%xmm1, %%xmm0
        if (node->kind == ND_EQ) {
            asm_setcc(cg_sec, X86_RAX, X86_E); // sete %%al
            asm_setcc(cg_sec, X86_RCX, X86_NP); // setnp %%cl
            x86_and_rr(cg_sec, 1, X86_RAX, X86_RCX); // andb %%cl, %%al
        } else if (node->kind == ND_NE) {
            asm_setcc(cg_sec, X86_RAX, X86_NE); // setne %%al
            asm_setcc(cg_sec, X86_RCX, X86_P); // setp %%cl
            x86_or_rr(cg_sec, 1, X86_RAX, X86_RCX); // orb %%cl, %%al
        } else if (node->kind == ND_LT) {
            asm_setcc(cg_sec, X86_RAX, X86_B); // setb %%al
        } else if (node->kind == ND_LE) {
            asm_setcc(cg_sec, X86_RAX, X86_BE); // setbe %%al
        }
        asm_movzx_phys(cg_sec, r_lhs, X86_RAX, 4, 1); // movzbl %%al, %s
#endif
        if (r_rhs != r_lhs)
            free_reg(r_rhs);
        return r_lhs;
    }

    // Complex comparisons: compare both real and imag parts
    if ((node->kind == ND_EQ || node->kind == ND_NE) &&
        node->lhs->ty && is_complex(node->lhs->ty)) {
        // r_lhs (from the unconditional gen(node->lhs) above) is unused here
        // since we re-derive the address via gen_addr(); free it to avoid a
        // register leak.
        free_reg(r_lhs);
        int addr_lhs = gen_addr(node->lhs);
        int addr_rhs = gen_addr(node->rhs);
        int need_free_lhs = 0, need_free_rhs = 0;
        int complex_sz = node->lhs->ty->size;
        int base_sz = node->lhs->ty->base->size;
        int sz = complex_sz;
        if (addr_lhs < 0) {
            addr_lhs = alloc_reg();
#ifdef ARCH_ARM64
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // sub sp, sp, #complex_sz
            asm_mov_reg_sp(cg_sec, REG(addr_lhs)); // mov addr_lhs, sp
#else
            asm_sub_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // subq $N, %rsp
            x86_mov_rr(cg_sec, 8, REG(addr_lhs), X86_RSP); // movq %rsp, addr_lhs
#endif
            int v = gen(node->lhs);
            if (is_complex(node->lhs->ty)) {
                // gen() returned the address of the complex payload — copy it
#ifdef ARCH_ARM64
                arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(v), 0); // ldr x16, [v]
                arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), 0); // str x16, [addr_lhs]
                if (complex_sz > 8) {
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(v), (uint32_t)(base_sz / 8)); // ldr x16, [v, #base_sz]
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), (uint32_t)(base_sz / 8)); // str x16, [addr_lhs, #base_sz]
                }
#else
                asm_mov_reg_reg(cg_sec, 0, v, 8);
                asm_mov_reg_reg(cg_sec, addr_lhs, 0, 8);
                if (complex_sz > 8) {
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(v), base_sz)); // movq base_sz(v), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(addr_lhs), base_sz), X86_RAX); // movq %rax, base_sz(addr_lhs)
                }
#endif
            } else {
#ifdef ARCH_ARM64
                asm_str_reg_off(cg_sec, v, addr_lhs, 8, 0);
                if (complex_sz > 8)
                    arm64_str_uoff(cg_sec, 3, ARM64_XZR, REG(addr_lhs), (uint32_t)(base_sz / 8)); // str xzr, [addr_lhs, #base_sz]
#else
                asm_mov_reg_mem(cg_sec, v, addr_lhs, 8); // movq v, (addr_lhs)
                if (complex_sz > 8)
                    x86_mov_mi(cg_sec, 8, x86_mem(REG(addr_lhs), base_sz), 0); // movq $0, base_sz(addr_lhs)
#endif
            }
            free_reg(v);
            need_free_lhs = 1;
        }
        if (addr_rhs < 0) {
            addr_rhs = alloc_reg();
#ifdef ARCH_ARM64
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // sub sp, sp, #complex_sz
            asm_mov_reg_sp(cg_sec, REG(addr_rhs)); // mov addr_rhs, sp
#else
            asm_sub_rsp_imm(cg_sec, (complex_sz + 15) & ~15); // subq $N, %rsp
            x86_mov_rr(cg_sec, 8, REG(addr_rhs), X86_RSP); // movq %rsp, addr_rhs
#endif
            int v = gen(node->rhs);
            if (is_complex(node->rhs->ty)) {
                // gen() returned the address of the complex payload — copy it
#ifdef ARCH_ARM64
                arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(v), 0); // ldr x16, [v]
                arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(addr_rhs), 0); // str x16, [addr_rhs]
                if (complex_sz > 8) {
                    arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(v), (uint32_t)(base_sz / 8)); // ldr x16, [v, #base_sz]
                    arm64_str_uoff(cg_sec, 3, ARM64_X16, REG(addr_rhs), (uint32_t)(base_sz / 8)); // str x16, [addr_rhs, #base_sz]
                }
#else
                asm_mov_reg_reg(cg_sec, 0, v, 8);
                asm_mov_reg_reg(cg_sec, addr_rhs, 0, 8);
                if (complex_sz > 8) {
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(v), base_sz)); // movq base_sz(v), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(REG(addr_rhs), base_sz), X86_RAX); // movq %rax, base_sz(addr_rhs)
                }
#endif
            } else {
#ifdef ARCH_ARM64
                asm_str_reg_off(cg_sec, v, addr_rhs, 8, 0);
                if (complex_sz > 8)
                    arm64_str_uoff(cg_sec, 3, ARM64_XZR, REG(addr_rhs), (uint32_t)(base_sz / 8)); // str xzr, [addr_rhs, #base_sz]
#else
                asm_mov_reg_mem(cg_sec, v, addr_rhs, 8); // movq v, (addr_rhs)
                if (complex_sz > 8)
                    x86_mov_mi(cg_sec, 8, x86_mem(REG(addr_rhs), base_sz), 0); // movq $0, base_sz(addr_rhs)
#endif
            }
            free_reg(v);
            need_free_rhs = 1;
        }
        int result = alloc_reg();
#ifdef ARCH_ARM64
        if (sz <= 4) {
            asm_movq_zero(cg_sec, result);
            if (sz == 2) {
                arm64_ldrh_uoff(cg_sec, ARM64_X16, REG(addr_lhs), 0); // ldrh w16, [addr_lhs]
                arm64_ldrh_uoff(cg_sec, ARM64_X17, REG(addr_rhs), 0); // ldrh w17, [addr_rhs]
            } else {
                arm64_ldr_uoff(cg_sec, 2, ARM64_X16, REG(addr_lhs), 0); // ldr w16, [addr_lhs]
                arm64_ldr_uoff(cg_sec, 2, ARM64_X17, REG(addr_rhs), 0); // ldr w17, [addr_rhs]
            }
            arm64_eor_reg(cg_sec, 0, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0); // eor w16, w16, w17
            arm64_subs_imm(cg_sec, 0, ARM64_XZR, ARM64_X16, 0, 0); // cmp w16, #0
        } else {
            asm_ldr_reg_off(cg_sec, result, addr_lhs, 8, 0);
            arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_rhs), 0); // ldr x16, [addr_rhs]
            arm64_eor_reg(cg_sec, 1, REG(result), REG(result), ARM64_X16, ARM64_LSL, 0); // eor result, result, x16
            if (sz > 8) {
                arm64_ldr_uoff(cg_sec, 3, ARM64_X16, REG(addr_lhs), (uint32_t)(base_sz / 8)); // ldr x16, [addr_lhs, #base_sz]
                arm64_ldr_uoff(cg_sec, 3, ARM64_X17, REG(addr_rhs), (uint32_t)(base_sz / 8)); // ldr x17, [addr_rhs, #base_sz]
                arm64_eor_reg(cg_sec, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0); // eor x16, x16, x17
                arm64_orr_reg(cg_sec, 1, REG(result), REG(result), ARM64_X16, ARM64_LSL, 0); // orr result, result, x16
            }
            asm_cmp_zero(cg_sec, result, 8);
        }
        if (node->kind == ND_EQ)
            asm_cset(cg_sec, result, ARM64_EQ);
        else
            asm_cset(cg_sec, result, ARM64_NE);
#else
        if (sz <= 4) {
            asm_movl_zero(cg_sec, result);
            x86_mov_rm(cg_sec, 1, X86_RAX, x86_mem(REG(addr_lhs), 0)); // movb (addr_lhs), %al
            x86_xor_rm(cg_sec, 1, X86_RAX, x86_mem(REG(addr_rhs), 0)); // xorb (addr_rhs), %al
            asm_movzx_phys(cg_sec, result, X86_RAX, 4, 1); // movzbl %al, result
        } else {
            x86_mov_rm(cg_sec, 8, REG(result), x86_mem(REG(addr_lhs), 0)); // movq (addr_lhs), result
            x86_xor_rm(cg_sec, 8, REG(result), x86_mem(REG(addr_rhs), 0)); // xorq (addr_rhs), result
            if (sz > 8) {
                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(REG(addr_lhs), base_sz)); // movq base_sz(addr_lhs), %rax
                x86_xor_rm(cg_sec, 8, X86_RAX, x86_mem(REG(addr_rhs), base_sz)); // xorq base_sz(addr_rhs), %rax
                x86_or_rr(cg_sec, 8, REG(result), X86_RAX); // orq %rax, result
            }
        }
        if (node->kind == ND_EQ)
            asm_setcc(cg_sec, X86_RAX, X86_E);
        else
            asm_setcc(cg_sec, X86_RAX, X86_NE);
        asm_movzx_phys(cg_sec, result, X86_RAX, 4, 1);
#endif
        free_reg(addr_lhs);
        free_reg(addr_rhs);
#ifdef ARCH_ARM64
        if (need_free_lhs) arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // add sp, sp, #complex_sz
        if (need_free_rhs) arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, (complex_sz + 15) & ~15, 0); // add sp, sp, #complex_sz
#else
        if (need_free_lhs) asm_add_rsp_imm(cg_sec, (complex_sz + 15) & ~15);
        if (need_free_rhs) asm_add_rsp_imm(cg_sec, (complex_sz + 15) & ~15);
#endif
        return result;
    }

    // Fused Division/Modulo Optimization
    if (node->kind == ND_DIV || node->kind == ND_MOD) {
        int sz = op_size(node->ty);
        bool is_unsigned = use_unsigned(node->ty);
        VReg r_rhs = gen(node->rhs);
#ifdef ARCH_ARM64
        if (node->kind == ND_DIV) {
            if (is_unsigned)
                asm_udiv_reg_reg(cg_sec, r_lhs, r_rhs, sz); // udiv r_lhs, r_lhs, r_rhs
            else
                asm_sdiv_reg_reg(cg_sec, r_lhs, r_rhs, sz); // sdiv r_lhs, r_lhs, r_rhs
        } else {
            VReg tmp = alloc_reg();
            asm_mov_reg_reg(cg_sec, tmp, r_lhs, sz); // mov tmp, r_lhs
            if (is_unsigned)
                asm_udiv_reg_reg(cg_sec, tmp, r_rhs, sz); // udiv tmp, tmp, r_rhs
            else
                asm_sdiv_reg_reg(cg_sec, tmp, r_rhs, sz); // sdiv tmp, tmp, r_rhs
            asm_mul_reg_reg(cg_sec, tmp, r_rhs, sz); // mul tmp, tmp, rhs
            asm_sub_reg_reg(cg_sec, r_lhs, tmp, sz); // sub r_lhs, r_lhs, tmp
            free_reg(tmp);
        }
#else
        x86_mov_rr(cg_sec, sz, X86_RAX, REG(r_lhs)); // movq r_lhs, %rax
        if (is_unsigned) {
            x86_xor_rr(cg_sec, 4, X86_RDX, X86_RDX); // xorl %edx, %edx
        } else {
            if (sz == 8) asm_cqo(cg_sec); // cqo
            else
                asm_cdq(cg_sec); // cdq
        }
        if (is_unsigned)
            asm_div(cg_sec, r_rhs, sz); // div r_rhs
        else
            asm_idiv(cg_sec, r_rhs, sz); // idiv r_rhs
        if (node->kind == ND_DIV)
            x86_mov_rr(cg_sec, sz, REG(r_lhs), X86_RAX); // movq %rax, r_lhs
        else
            x86_mov_rr(cg_sec, sz, REG(r_lhs), X86_RDX); // movq %rdx, r_lhs
#endif
        free_reg(r_rhs);
        return r_lhs;
    }

    // Binary operators with potential immediate/memory optimization for RHS
    if (node->kind == ND_SHL || node->kind == ND_SHR) {
        int sz = op_size(node->ty);
#ifdef ARCH_ARM64
        int sf = (sz == 8) ? 1 : 0;
        if (node->rhs->kind == ND_NUM) {
            int s = (int)node->rhs->val;
            if (s >= sz * 8) {
                asm_movq_zero(cg_sec, r_lhs); // xor rr_lhs,rr_lhs (set zero)
            } else {
                if (node->kind == ND_SHL)
                    asm_shl_imm(cg_sec, r_lhs, sf ? 8 : 4, (uint8_t)(s)); // shl r_lhs, #s
                else if (use_unsigned(node->ty))
                    asm_shr_imm(cg_sec, r_lhs, sf ? 8 : 4, (uint8_t)(s)); // shr r_lhs, #s
                else
                    asm_sar_imm(cg_sec, r_lhs, sf ? 8 : 4, (uint8_t)(s)); // sar r_lhs, #s
            }
        } else {
            VReg r_rhs = gen(node->rhs);
            if (node->kind == ND_SHL)
                asm_shl_cl(cg_sec, r_lhs, sf ? 8 : 4, r_rhs); // lsl r_lhs, r_lhs, r_rhs
            else if (use_unsigned(node->ty))
                asm_shr_cl(cg_sec, r_lhs, sf ? 8 : 4, r_rhs); // lsr r_lhs, r_lhs, r_rhs
            else
                asm_sar_cl(cg_sec, r_lhs, sf ? 8 : 4, r_rhs); // asr r_lhs, r_lhs, r_rhs
            free_reg(r_rhs);
        }
#else
        if (node->rhs->kind == ND_NUM) {
            int imm = (int)node->rhs->val;
            if (node->kind == ND_SHL)
                asm_shl_imm(cg_sec, r_lhs, sz, (uint8_t)(imm)); // shl $(uint8_t)(imm), rr_lhs
            else if (use_unsigned(node->ty))
                asm_shr_imm(cg_sec, r_lhs, sz, (uint8_t)(imm)); // shr $(uint8_t)(imm), rr_lhs
            else
                asm_sar_imm(cg_sec, r_lhs, sz, (uint8_t)(imm)); // sar $(uint8_t)(imm), rr_lhs
        } else {
            VReg r_rhs = gen(node->rhs);
            x86_mov_rr(cg_sec, 4, X86_RCX, REG(r_rhs)); // movl %s, %ecx
            if (node->kind == ND_SHL)
                asm_shl_cl(cg_sec, r_lhs, sz, r_rhs); // shl cl, rr_lhs
            else if (use_unsigned(node->ty))
                asm_shr_cl(cg_sec, r_lhs, sz, r_rhs);
            else
                asm_sar_cl(cg_sec, r_lhs, sz, r_rhs);
            free_reg(r_rhs);
        }
#endif
        // Extended bitfield shift: mask result to bitfield width.
        // GCC truncates shifts of a >32-bit unsigned bitfield to its width.
        {
            int bw = 0;
            if (node->lhs && node->lhs->kind == ND_MEMBER && node->lhs->member &&
                node->lhs->member->bit_width > 32 && node->lhs->member->bit_width < 64 &&
                node->lhs->member->ty && node->lhs->member->ty->size >= 8 &&
                node->lhs->member->ty->is_unsigned)
                bw = node->lhs->member->bit_width;
            if (bw > 0) {
                unsigned long long mask = (1ULL << bw) - 1;
#ifdef ARCH_ARM64
                emit_mov_imm64(ARM64_X17, mask);
                asm_and_reg_phy(cg_sec, r_lhs, ARM64_X17, 8); // and r_lhs, r_lhs, x17
#else
                asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)mask); // movabsq $mask, %rax
                asm_and_rax(cg_sec, r_lhs, 8); // andq %rax, r_lhs
#endif
            }
        }
        return r_lhs;
    }

    if (node->kind == ND_ADD || node->kind == ND_SUB || node->kind == ND_MUL ||
        node->kind == ND_BITAND || node->kind == ND_BITXOR || node->kind == ND_BITOR ||
        node->kind == ND_EQ || node->kind == ND_NE || node->kind == ND_LT || node->kind == ND_LE) {
        char *inst = "";
        if (node->kind == ND_ADD) inst = "add";
        else if (node->kind == ND_SUB)
            inst = "sub";
        else if (node->kind == ND_MUL)
            inst =
#ifdef ARCH_ARM64
                "mul";
#else
                "imul";
#endif
        else if (node->kind == ND_BITAND)
            inst = "and";
        else if (node->kind == ND_BITXOR)
            inst =
#ifdef ARCH_ARM64
                "eor";
#else
                "xor";
#endif
        else if (node->kind == ND_BITOR)
            inst =
#ifdef ARCH_ARM64
                "orr";
#else
                "or";
#endif
        else
            inst = "cmp";

        int sz = (node->kind == ND_EQ || node->kind == ND_NE || node->kind == ND_LT || node->kind == ND_LE)
            ? op_size(node->lhs->ty)
            : op_size(node->ty);
        if (sz < op_size(node->rhs->ty))
            sz = op_size(node->rhs->ty);

        if (node->rhs->kind == ND_NUM && node->rhs->val == (int32_t)node->rhs->val) {
            // Skip identity operations: add 0, sub 0, mul 1, and ~0, or 0, xor 0
            int imm = (int)node->rhs->val;
            if ((node->kind == ND_MUL && imm == 1) ||
                ((node->kind == ND_ADD || node->kind == ND_SUB || node->kind == ND_BITOR || node->kind == ND_BITXOR) && imm == 0)) {
                // no-op, just return r_lhs
#ifdef ARCH_ARM64
            } else if (node->kind == ND_MUL && imm > 0 && (imm & (imm - 1)) == 0) {
                // Strength reduction: multiply by power of 2 → shift
                int shift = 0;
                int tmp = imm;
                while (tmp > 1) {
                    shift++;
                    tmp >>= 1;
                }
                asm_shl_imm(cg_sec, r_lhs, sz, (uint8_t)shift); // lsl r_lhs, r_lhs, #shift
            } else if (node->kind == ND_MUL) {
                // ARM64 mul doesn't take immediates; load into a scratch register
                VReg tmp = alloc_reg();
                emit_mov_imm(reg(tmp, sz), imm);
                asm_mul_reg_reg(cg_sec, r_lhs, tmp, sz); // mul r_lhs, r_lhs, tmp
                free_reg(tmp);
            } else if (!strcmp(inst, "cmp")) {
                if (imm >= 0 && imm <= 4095)
                    asm_cmp_imm(cg_sec, r_lhs, sz, imm); // cmp r_lhs, #imm
                else if (imm < 0 && imm >= -4095)
                    asm_cmn_imm(cg_sec, r_lhs, sz == 8 ? 1 : 0, -imm); // cmn r_lhs, #-imm
                else {
                    emit_mov_imm64(ARM64_X16, (uint64_t)(int64_t)imm);
                    asm_cmp_reg_phy(cg_sec, r_lhs, ARM64_X16, sz); // cmp r_lhs, x16
                }
            } else if (node->kind != ND_BITAND && node->kind != ND_BITOR && node->kind != ND_BITXOR &&
                       imm >= 0 && imm <= 4095) {
                if (!strcmp(inst, "add")) asm_add_imm(cg_sec, r_lhs, sz, imm); // add r_lhs, r_lhs, #imm
                else if (!strcmp(inst, "sub"))
                    asm_sub_imm(cg_sec, r_lhs, sz, imm); // sub r_lhs, r_lhs, #imm
            } else {
                VReg tmp = alloc_reg();
                emit_mov_imm(reg(tmp, sz), imm);
                if (!strcmp(inst, "add")) asm_add_reg_reg(cg_sec, r_lhs, tmp, sz); // add r_lhs, r_lhs, tmp
                else if (!strcmp(inst, "sub"))
                    asm_sub_reg_reg(cg_sec, r_lhs, tmp, sz); // sub r_lhs, r_lhs, tmp
                else if (!strcmp(inst, "and"))
                    asm_and_reg_reg(cg_sec, r_lhs, tmp, sz); // and r_lhs, r_lhs, tmp
                else if (!strcmp(inst, "orr"))
                    asm_or_reg_reg(cg_sec, r_lhs, tmp, sz); // orr r_lhs, r_lhs, tmp
                else if (!strcmp(inst, "eor"))
                    asm_eor_reg_reg(cg_sec, r_lhs, tmp, sz); // eor r_lhs, r_lhs, tmp
                free_reg(tmp);
            }
#else
            } else if (node->kind == ND_MUL && imm > 0 && (imm & (imm - 1)) == 0) {
                // Strength reduction: multiply by power of 2 → shift
                int shift = 0;
                int tmp = imm;
                while (tmp > 1) {
                    shift++;
                    tmp >>= 1;
                }
                asm_shl_imm(cg_sec, r_lhs, sz, (uint8_t)shift); // shl r_lhs, #shift
            } else {
                if (!strcmp(inst, "add")) asm_add_imm(cg_sec, r_lhs, sz, imm); // add r_lhs, #imm
                else if (!strcmp(inst, "sub"))
                    asm_sub_imm(cg_sec, r_lhs, sz, imm); // sub r_lhs, #imm
                else if (!strcmp(inst, "imul")) {
                    // FIXME arm
                    asm_imul_imm(cg_sec, r_lhs, r_lhs, sz, (int32_t)node->rhs->val); // imul $val, r_lhs, r_lhs
                } else if (!strcmp(inst, "and"))
                    asm_and_imm(cg_sec, r_lhs, sz, imm); // and r_lhs, #imm
                else if (!strcmp(inst, "or"))
                    asm_or_imm(cg_sec, r_lhs, sz, imm); // or r_lhs, #imm
                else if (!strcmp(inst, "xor"))
                    asm_xor_imm(cg_sec, r_lhs, sz, imm); // xor r_lhs, #imm
                else if (!strcmp(inst, "cmp"))
                    asm_cmp_imm(cg_sec, r_lhs, sz, imm); // cmp r_lhs, #imm
            }
#endif
        } else {
            VReg r_rhs = gen(node->rhs);
#ifdef ARCH_ARM64
            // Sign-extend rhs to 64 bits when operation is 64-bit but rhs was
            // computed as 32-bit signed. ARM64: use sxtw.
            if (sz == 8 && op_size(node->rhs->ty) == 4 && !use_unsigned(node->rhs->ty))
                asm_movsx(cg_sec, r_rhs, r_rhs, 8, 4); // movsx8->r_rhs rr_rhs, rr_rhs
            if (!strcmp(inst, "cmp")) {
                asm_cmp_reg_reg(cg_sec, r_lhs, r_rhs, sz); // cmp rr_rhs, rr_lhs
            } else {
                if (!strcmp(inst, "add")) asm_add_reg_reg(cg_sec, r_lhs, r_rhs, sz); // add rr_lhs, rr_rhs
                else if (!strcmp(inst, "sub"))
                    asm_sub_reg_reg(cg_sec, r_lhs, r_rhs, sz); // sub rr_lhs, rr_rhs
                else if (!strcmp(inst, "mul"))
                    asm_mul_reg_reg(cg_sec, r_lhs, r_rhs, sz); // imul rr_lhs, rr_rhs
                else if (!strcmp(inst, "and"))
                    asm_and_reg_reg(cg_sec, r_lhs, r_rhs, sz); // and rr_lhs, rr_rhs
                else if (!strcmp(inst, "eor"))
                    asm_eor_reg_reg(cg_sec, r_lhs, r_rhs, sz); // xor rr_lhs, rr_rhs
                else if (!strcmp(inst, "orr"))
                    asm_or_reg_reg(cg_sec, r_lhs, r_rhs, sz); // or rr_lhs, rr_rhs
            }
            // Pointer subtraction: divide byte difference by element size
            if (node->kind == ND_SUB && node->lhs->ty->base && node->rhs->ty->base) {
                int elem_sz = node->lhs->ty->base->size;
                if (elem_sz > 1) {
                    if ((elem_sz & (elem_sz - 1)) == 0) {
                        int shift = 0, tmp = elem_sz;
                        while (tmp > 1) {
                            shift++;
                            tmp >>= 1;
                        }
                        asm_sar_imm(cg_sec, r_lhs, sz, (uint8_t)shift); // asr r_lhs, r_lhs, #shift
                    } else {
                        VReg tmp = alloc_reg();
                        asm_mov_imm(cg_sec, tmp, sz, (int64_t)elem_sz); // mov tmp, #elem_sz
                        asm_sdiv_reg_reg(cg_sec, r_lhs, tmp, sz); // sdiv r_lhs, r_lhs, tmp
                        free_reg(tmp);
                    }
                }
            }
#else
            // Sign-extend rhs to 64 bits when operation is 64-bit but rhs was
            // computed as 32-bit signed (e.g. pointer + int, long + int).
            // 32-bit writes zero-extend in x86-64, so without this a negative
            // int would be treated as a large positive offset.
            if (sz == 8 && op_size(node->rhs->ty) == 4 && !use_unsigned(node->rhs->ty))
                asm_movsx(cg_sec, r_rhs, r_rhs, 8, 4);
            if (!strcmp(inst, "add")) asm_add_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            else if (!strcmp(inst, "sub"))
                asm_sub_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            else if (!strcmp(inst, "imul"))
                asm_mul_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            else if (!strcmp(inst, "and"))
                asm_and_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            else if (!strcmp(inst, "xor"))
                asm_eor_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            else if (!strcmp(inst, "or"))
                asm_or_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            else if (!strcmp(inst, "cmp"))
                asm_cmp_reg_reg(cg_sec, r_lhs, r_rhs, sz);
            // Pointer subtraction: divide byte difference by element size
            if (node->kind == ND_SUB && node->lhs->ty->base && node->rhs->ty->base) {
                int elem_sz = node->lhs->ty->base->size;
                if (elem_sz > 1) {
                    if ((elem_sz & (elem_sz - 1)) == 0) {
                        // Power of 2: use arithmetic shift right
                        int shift = 0;
                        int tmp = elem_sz;
                        while (tmp > 1) {
                            shift++;
                            tmp >>= 1;
                        }
                        asm_sar_imm(cg_sec, r_lhs, sz, (uint8_t)(shift)); // sar $(uint8_t)(shift), rr_lhs
                    } else {
                        // Non-power of 2: use idiv
                        x86_mov_rr(cg_sec, sz, X86_RAX, REG(r_lhs)); // movq r_lhs, %rax
                        if (sz == 8) asm_cqo(cg_sec); // cqo
                        else
                            asm_cdq(cg_sec); // cdq
                        x86_mov_ri(cg_sec, sz, X86_RCX, elem_sz); // movq $elem_sz, %rcx
                        x86_idiv_r(cg_sec, sz, X86_RCX); // idiv %rcx
                        x86_mov_rr(cg_sec, sz, REG(r_lhs), X86_RAX); // movq %rax, r_lhs
                    }
                }
            }
#endif
            free_reg(r_rhs);
        }

        if (node->kind == ND_EQ || node->kind == ND_NE || node->kind == ND_LT || node->kind == ND_LE) {
#ifdef ARCH_ARM64
            Arm64Cond cset_cond = ARM64_EQ;
            if (node->kind == ND_EQ) cset_cond = ARM64_EQ;
            else if (node->kind == ND_NE)
                cset_cond = ARM64_NE;
            else if (node->kind == ND_LT)
                cset_cond = use_unsigned_cmp(node) ? ARM64_LO : ARM64_LT;
            else if (node->kind == ND_LE)
                cset_cond = use_unsigned_cmp(node) ? ARM64_LS : ARM64_LE;
            asm_cset(cg_sec, r_lhs, cset_cond); // cset w{r_lhs}, cond
#else
            X86Cond cond = X86_E;
            if (node->kind == ND_EQ) cond = X86_E;
            else if (node->kind == ND_NE)
                cond = X86_NE;
            else if (node->kind == ND_LT)
                cond = use_unsigned_cmp(node) ? X86_B : X86_L;
            else if (node->kind == ND_LE)
                cond = use_unsigned_cmp(node) ? X86_BE : X86_LE;
            asm_setcc(cg_sec, REG(r_lhs), cond); // setcc rr_lhs
            asm_movzx(cg_sec, r_lhs, r_lhs, 4, 1); // movzx4->r_lhs rr_lhs, rr_lhs
#endif
        }
        // Extended bitfield arithmetic masking (GCC extension for unsigned long long : N).
        // When both operands involve extended (>= 8-byte underlying type) bitfield reads,
        // mask the result to the max bitfield width, matching GCC's behavior.
        if (node->kind == ND_MUL || node->kind == ND_ADD || node->kind == ND_SUB) {
            int bw = 0;
            Node *ops[2] = {node->lhs, node->rhs};
            for (int oi = 0; oi < 2; oi++) {
                Node *op = ops[oi];
                if (op && op->kind == ND_MEMBER && op->member &&
                    op->member->bit_width > 32 && op->member->bit_width < 64 &&
                    op->member->ty && op->member->ty->size >= 8 &&
                    op->member->ty->is_unsigned) {
                    if (op->member->bit_width > bw)
                        bw = op->member->bit_width;
                }
            }
            if (bw > 0) {
                unsigned long long mask = (1ULL << bw) - 1;
#ifdef ARCH_ARM64
                emit_mov_imm64(ARM64_X16, mask);
                asm_and_reg_phy(cg_sec, r_lhs, ARM64_X16, 8); // and rr_lhs, r16
#else
                asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)(mask)); // movabs $(uint64_t)(mask), rX86_RAX
                asm_and_rax(cg_sec, r_lhs, 8); // andq %rax, rr_lhs
#endif
            }
        }

        return r_lhs;
    }

    error("invalid expression %d", node->kind);
    return -1;
}


// Fast peephole parsing helpers (avoid sscanf in hot loop)

static const char *node_kind_name(NodeKind k) {
    switch (k) {
    case ND_ADD: return "ADD";
    case ND_SUB: return "SUB";
    case ND_MUL: return "MUL";
    case ND_DIV: return "DIV";
    case ND_MOD: return "MOD";
    case ND_SHL: return "SHL";
    case ND_SHR: return "SHR";
    case ND_BITAND: return "BITAND";
    case ND_BITXOR: return "BITXOR";
    case ND_BITOR: return "BITOR";
    case ND_EQ: return "EQ";
    case ND_NE: return "NE";
    case ND_LT: return "LT";
    case ND_LE: return "LE";
    case ND_ASSIGN: return "ASSIGN";
    case ND_POST_INC: return "POST_INC";
    case ND_POST_DEC: return "POST_DEC";
    case ND_ADDR: return "ADDR";
    case ND_DEREF: return "DEREF";
    case ND_CAST: return "CAST";
    case ND_BITNOT: return "BITNOT";
    case ND_FUNCALL: return "FUNCALL";
    case ND_LVAR: return "LVAR";
    case ND_NUM: return "NUM";
    case ND_RETURN: return "RETURN";
    case ND_IF: return "IF";
    case ND_FOR: return "FOR";
    case ND_DO: return "DO";
    case ND_SWITCH: return "SWITCH";
    case ND_CASE: return "CASE";
    case ND_BREAK: return "BREAK";
    case ND_CONTINUE: return "CONTINUE";
    case ND_GOTO: return "GOTO";
    case ND_GOTO_IND: return "GOTO_IND";
    case ND_LABEL: return "LABEL";
    case ND_LABEL_VAL: return "LABEL_VAL";
    case ND_STMT_EXPR: return "STMT_EXPR";
    case ND_BLOCK: return "BLOCK";
    case ND_EXPR_STMT: return "EXPR_STMT";
    case ND_NULL: return "NULL";
    case ND_STR: return "STR";
    case ND_MEMBER: return "MEMBER";
    case ND_LOGAND: return "LOGAND";
    case ND_LOGOR: return "LOGOR";
    case ND_COND: return "COND";
    case ND_COMMA: return "COMMA";
    case ND_SIZEOF: return "SIZEOF";
    case ND_FNUM: return "FNUM";
    case ND_NEG: return "NEG";
    case ND_NOT: return "NOT";
    case ND_ZERO_INIT: return "ZERO_INIT";
    case ND_ASM: return "ASM";
    case ND_VA_START: return "VA_START";
    case ND_VA_COPY: return "VA_COPY";
    case ND_VA_ARG: return "VA_ARG";
    case ND_VA_ARG_PACK: return "VA_ARG_PACK";
    case ND_ALLOCA: return "ALLOCA";
    case ND_ALLOCA_ZINIT: return "ALLOCA_ZINIT";
    case ND_CHAIN: return "CHAIN";
    case ND_ATOMIC_LOAD: return "ATOMIC_LOAD";
    case ND_ATOMIC_STORE: return "ATOMIC_STORE";
    case ND_ATOMIC_EXCHANGE: return "ATOMIC_EXCHANGE";
    case ND_ATOMIC_CAS: return "ATOMIC_CAS";
    case ND_ATOMIC_FENCE: return "ATOMIC_FENCE";
    case ND_ATOMIC_FETCH_OP: return "ATOMIC_FETCH_OP";
    default: return "UNKNOWN";
    }
}

static void dump_node(FILE *f, Node *node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) fputc(' ', f);
    fprintf(f, "%s", node_kind_name(node->kind));
    if (node->kind == ND_NUM) fprintf(f, " %lld", (long long)node->val);
    if (node->kind == ND_LVAR && node->var) fprintf(f, " '%s'", node->var->name ? node->var->name : "(anon)");
    if (node->kind == ND_FUNCALL && node->lhs && node->lhs->kind == ND_LVAR && node->lhs->var)
        fprintf(f, " %s", node->lhs->var->name);
    fprintf(f, "\n");
    if (node->kind == ND_BLOCK)
        for (Node *n = node->body; n; n = n->next) dump_node(f, n, depth + 2);
    if (node->lhs) dump_node(f, node->lhs, depth + 2);
    if (node->rhs) dump_node(f, node->rhs, depth + 2);
    if (node->then) dump_node(f, node->then, depth + 2);
    if (node->els) dump_node(f, node->els, depth + 2);
    if (node->cond) dump_node(f, node->cond, depth + 2);
    if (node->body) dump_node(f, node->body, depth + 2);
    if (node->init) dump_node(f, node->init, depth + 2);
    if (node->inc) dump_node(f, node->inc, depth + 2);
    if (node->kind == ND_CASE && node->lhs) dump_node(f, node->lhs, depth + 2);
}

void dump_ast(Program *prog) {
    fprintf(stderr, "=== AST dump ===\n");
    for (TLItem *item = prog->items; item; item = item->next) {
        if (item->kind == TL_FUNC && item->fn->body) {
            fprintf(stderr, "function %s:\n", item->fn->name);
            dump_node(stderr, item->fn->body, 2);
        }
    }
    fprintf(stderr, "=== end AST dump ===\n");
}

struct ObjFile *codegen(Program *prog) {
    init_builtin_names();
    // Reset label and fixup hashtables for new compilation unit
    cg_label_ht_reset();
    asm_fixup_ht_reset();

    // Initialize binary ObjFile for asm_* emission
    static ObjFile _obj;
    cg_obj = &_obj;
    memset(cg_obj, 0, sizeof(*cg_obj));
    objfile_init(cg_obj);
    cg_set_section(SEC_TEXT);
    cg_dry_run = false;
    all_items = prog->items;
    memset(func_htab, 0, sizeof(func_htab));
    for (TLItem *item = all_items; item; item = item->next)
        if (item->kind == TL_FUNC) {
            uint32_t h = func_hash_name(item->fn->name) % FUNC_HASH_SIZE;
            item->hash_next = func_htab[h];
            func_htab[h] = item;
        }
    all_strs = prog->strs;
    alloca_needed = false;
    last_debug_file = 0;
    last_debug_line = 0;
    // Assembly header
#ifndef ARCH_ARM64
    /* AT&T syntax is now the default */
#endif
#ifndef _WIN32
#if !defined(__APPLE__)
    /* .note.GNU-stack: default in ELF, no bytes emitted */
#endif
#endif

    // Emit data section for strings
    if (prog->globals || prog->strs || float_lits) {
        (void)0 /* section directive */;
        // Track emitted symbols to avoid duplicates (e.g. __asm__("same_name"))
        char **emitted_syms = NULL;
        int emitted_count = 0;
        for (LVar *var = prog->globals; var; var = var->next) {
            if (var->is_extern && !var->alias_target && !var->asm_name)
                continue;
            char *label = var->asm_name ? var->asm_name : var->name;
            if (var->is_function && !var->alias_target && !var->asm_name)
                continue;
            // Handle function aliases (__attribute__((alias)) or __asm__ renaming)
            if (var->is_function && !var->alias_target && var->asm_name) {
                // Skip if asm_name resolves to same symbol as C name (circular)
                if (strcmp(sym_name(var->name), var_sym_label(var)) == 0)
                    continue;
                // __asm__("target") on a function: alias the C name to the asm_name
                // The function body is emitted under asm_name; we need the C name to point to it.
                // Create a symbol alias by emitting both names at the same section offset.
                // The target symbol will be added when the function body is emitted below.
                continue;
            }
#ifdef _WIN32
            // Windows/MinGW uses emulated TLS via __emutls_get_address.
            if (var->is_tls) {
                if (!var->is_extern)
                    cg_emit_emutls_data(var);
                continue;
            }
#endif
            if (var->alias_target) {
                // __attribute__((alias("target")))
                // Defer alias resolution until after all symbols are emitted
                // Add as UNDEF for now; will be resolved later
                objfile_add_sym(cg_obj, asm_sym_name(sym_name(var->name)),
                                SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
                continue;
            }
            // If a global with this asm_name already emitted, skip (alias target)
            bool sym_already_emitted = false;
            const char *canon = asm_sym_name(var_sym_label(var));
            for (int i = 0; i < emitted_count; i++) {
                if (strcmp(emitted_syms[i], canon) == 0) {
                    sym_already_emitted = true;
                    break;
                }
            }
            if (sym_already_emitted)
                continue;
            if (var->asm_name) {
                // Check if asm_name matches another global that provides data
                // (with or without a leading underscore on the C name).
                LVar *existing = NULL;
                for (LVar *g = prog->globals; g; g = g->next) {
                    if (g != var && !g->is_extern && !g->is_function &&
                        (g->has_init || g->init_data)) {
                        if (strcmp(sym_name(g->name), var->asm_name) == 0 ||
                            (var->asm_name[0] == '_' &&
                             strcmp(sym_name(g->name), var->asm_name + 1) == 0)) {
                            existing = g;
                            break;
                        }
                    }
                }
                if (existing) {
                    // This is an alias via __asm__ — defer alias resolution
                    // until after all globals have been emitted.
                    continue;
                }
            }
            char **new_syms = realloc(emitted_syms, (emitted_count + 1) * sizeof(char *));
            if (!new_syms) {
                fprintf(stderr, "realloc failed\n");
                exit(1);
            }
            emitted_syms = new_syms;
            emitted_syms[emitted_count++] = (char *)canon;
            bool reserved = !var->asm_name && is_asm_reserved(var->name);
            char *safe_label = reserved ? format(".L_rcc_%s", var->name) : label;
            int is_bss = (!var->init_data && !var->relocs && !var->has_init) && !var->is_tls;
            const char *sym_name_str = asm_sym_name(sym_name(safe_label)); // .balign %d
            if (is_bss) {
                size_t align = var->ty->align > 1 ? var->ty->align : 1;
                size_t rem = cg_obj->bss_size % align;
                if (rem) cg_obj->bss_size += align - rem;
                size_t off = cg_obj->bss_size;
                if (!var->is_static)
                    objfile_add_sym(cg_obj, sym_name_str, var->is_tls ? SEC_TDATA : SEC_BSS, off, var->ty->size, SB_GLOBAL, var->is_tls ? ST_TLS : ST_OBJECT);
                else
                    objfile_add_sym(cg_obj, sym_name_str, var->is_tls ? SEC_TDATA : SEC_BSS, off, var->ty->size, SB_LOCAL, var->is_tls ? ST_TLS : ST_OBJECT);
                cg_label_ht_add(sym_name_str, off);
                if (reserved)
                    objfile_add_sym(cg_obj, asm_sym_name(sym_name(label)), var->is_tls ? SEC_TDATA : SEC_BSS, off, var->ty->size, var->is_static ? SB_LOCAL : SB_GLOBAL, var->is_tls ? ST_TLS : ST_OBJECT); // .globl %s
                cg_obj->bss_size += var->ty->size;
            } else {
                cg_set_section(var->is_tls ? SEC_TDATA : SEC_DATA);
                secbuf_align(cg_sec, var->ty->align > 1 ? var->ty->align : 1);
#ifdef __APPLE__
                // Apple ARM64: ensure at least 8-byte alignment for data symbols
                // (needed for proper relocation handling, fixes 119_random_stuff)
                secbuf_align(cg_sec, 8);
#endif
                size_t off = cg_sec->len;
#if defined(__APPLE__) && defined(ARCH_ARM64)
                if (var->is_tls) {
                    char *init_name = format("%s$tlv$init", label);
                    objfile_add_sym(cg_obj, init_name, SEC_TDATA, off, var->ty->size,
                                    SB_LOCAL, ST_OBJECT);
                    int tlv_boot = objfile_find_sym(cg_obj, "__tlv_bootstrap");
                    if (tlv_boot < 0)
                        tlv_boot = objfile_add_sym(cg_obj, "__tlv_bootstrap", SEC_UNDEF, 0, 0, SB_GLOBAL, ST_FUNC);
                    cg_set_section(SEC_THREAD_VARS);
                    secbuf_align(cg_sec, 8);
                    size_t desc_off = cg_sec->len;
                    secbuf_emit64le(cg_sec, 0);
                    objfile_add_reloc(cg_obj, SEC_THREAD_VARS, desc_off, tlv_boot, R_AARCH64_ABS64, 0);
                    secbuf_emit64le(cg_sec, 0);
                    int init_sidx = objfile_find_sym(cg_obj, init_name);
                    secbuf_emit64le(cg_sec, 0);
                    objfile_add_reloc(cg_obj, SEC_THREAD_VARS, desc_off + 16, init_sidx, R_AARCH64_ABS64, 0);
                    if (!var->is_static)
                        objfile_add_sym(cg_obj, sym_name_str, SEC_THREAD_VARS, desc_off, 24,
                                        SB_GLOBAL, ST_TLS);
                    else
                        objfile_add_sym(cg_obj, sym_name_str, SEC_THREAD_VARS, desc_off, 24,
                                        SB_LOCAL, ST_TLS);
                    cg_label_ht_add(sym_name_str, desc_off);
                    if (reserved)
                        objfile_add_sym(cg_obj, asm_sym_name(sym_name(label)), SEC_THREAD_VARS,
                                        desc_off, 24, var->is_static ? SB_LOCAL : SB_GLOBAL, ST_TLS);
                    cg_set_section(SEC_TDATA);
                } else
#endif
                {
                    if (!var->is_static)
                        objfile_add_sym(cg_obj, sym_name_str, var->is_tls ? SEC_TDATA : SEC_DATA, off, var->ty->size, SB_GLOBAL, var->is_tls ? ST_TLS : ST_OBJECT);
                    else
                        objfile_add_sym(cg_obj, sym_name_str, var->is_tls ? SEC_TDATA : SEC_DATA, off, var->ty->size, SB_LOCAL, var->is_tls ? ST_TLS : ST_OBJECT);
                    cg_label_ht_add(sym_name_str, off);
                    if (reserved)
                        objfile_add_sym(cg_obj, asm_sym_name(sym_name(label)), var->is_tls ? SEC_TDATA : SEC_DATA, off, var->ty->size, var->is_static ? SB_LOCAL : SB_GLOBAL, var->is_tls ? ST_TLS : ST_OBJECT); // %s:
                }
                if (var->is_tls && !var->has_init && !var->init_data && !var->relocs) {
                    for (int _zi = 0; _zi < var->ty->size; _zi++) secbuf_emit8(cg_sec, 0);
                }
                if (var->init_data || var->relocs) {
                    int pos = 0;
                    for (Reloc *rel = var->relocs; rel; rel = rel->next) {
                        for (; pos < rel->offset; pos++)
                            secbuf_emit8(cg_sec, (uint8_t)var->init_data[pos]); // .set %s, %s
                        size_t rel_off = cg_sec->len;
                        secbuf_emit64le(cg_sec, (uint64_t)rel->addend); // .quad addend (reloc addend embedded in data)
                        const char *rel_label = sym_name(rel->label);
                        int sidx;
                        // .L. labels are local text labels; use SB_LOCAL SEC_TEXT for section-relative reloc
                        bool is_local_label = (rel_label[0] == '.' && rel_label[1] == 'L' && rel_label[2] == '.');
                        sidx = objfile_find_sym(cg_obj, rel_label);
                        if (sidx < 0)
                            sidx = objfile_add_sym(cg_obj, rel_label,
                                                   is_local_label ? SEC_TEXT : SEC_UNDEF, 0, 0,
                                                   is_local_label ? SB_LOCAL : SB_GLOBAL, ST_NOTYPE);
#ifdef ARCH_ARM64
                        objfile_add_reloc(cg_obj, SEC_DATA, rel_off, sidx, R_AARCH64_ABS64, (int64_t)rel->addend);
#else
                        objfile_add_reloc(cg_obj, SEC_DATA, rel_off, sidx, R_X86_64_64, (int64_t)rel->addend);
#endif
                        pos += 8;
                    }
                    for (; pos < var->init_size; pos++)
                        secbuf_emit8(cg_sec, (uint8_t)var->init_data[pos]); // .quad %s%+d
                    if (var->ty->size > var->init_size) {
                        size_t pad = var->ty->size - var->init_size;
                        secbuf_reserve(cg_sec, pad);
                        memset(cg_sec->data + cg_sec->len, 0, pad);
                        cg_sec->len += pad;
                    }
                } else if (var->has_init) {
                    if (var->ty->size == 1)
                        secbuf_emit8(cg_sec, (uint8_t)var->init_val); // .quad %s
                    else if (var->ty->size == 2)
                        secbuf_emit16le(cg_sec, (uint16_t)var->init_val); // .quad %s%+d
                    else if (var->ty->size == 4)
                        secbuf_emit32le(cg_sec, (uint32_t)var->init_val); // .quad %s
                    else
                        secbuf_emit64le(cg_sec, (uint64_t)var->init_val); // .byte %u
                }
            }
        }
        // Resolve __asm__("name") aliases that refer to another global.
        // The matching is done with/without a leading underscore so that
        // e.g. `extern struct xx7 y __asm__("_z7")` aliases the C global `z7`
        // on targets where C symbols are prefixed with an underscore.
        for (LVar *var = prog->globals; var; var = var->next) {
            if (!var->is_extern || !var->asm_name)
                continue;
            LVar *existing = NULL;
            for (LVar *g = prog->globals; g; g = g->next) {
                if (g != var && !g->is_extern && !g->is_function &&
                    (g->has_init || g->init_data)) {
                    if (strcmp(sym_name(g->name), var->asm_name) == 0 ||
                        (var->asm_name[0] == '_' &&
                         strcmp(sym_name(g->name), var->asm_name + 1) == 0)) {
                        existing = g;
                        break;
                    }
                }
            }
            if (existing) {
                const char *target_name = asm_sym_name(sym_name(existing->name));
                const char *alias_name = asm_sym_name(var->asm_name);
                int tidx = objfile_find_sym(cg_obj, target_name);
                if (tidx >= 0 && cg_obj->syms[tidx].section != SEC_UNDEF) {
                    int aidx = objfile_find_sym(cg_obj, alias_name);
                    if (aidx < 0) {
                        objfile_add_sym(cg_obj, alias_name,
                                        cg_obj->syms[tidx].section,
                                        cg_obj->syms[tidx].offset,
                                        cg_obj->syms[tidx].size,
                                        cg_obj->syms[tidx].bind,
                                        cg_obj->syms[tidx].type);
                    } else if (cg_obj->syms[aidx].section == SEC_UNDEF) {
                        cg_obj->syms[aidx].section = cg_obj->syms[tidx].section;
                        cg_obj->syms[aidx].offset = cg_obj->syms[tidx].offset;
                        cg_obj->syms[aidx].size = cg_obj->syms[tidx].size;
                        cg_obj->syms[aidx].bind = cg_obj->syms[tidx].bind;
                        cg_obj->syms[aidx].type = cg_obj->syms[tidx].type;
                    }
                }
            }
        }
        free(emitted_syms);
        cg_set_section(SEC_RODATA);
        for (StrLit *s = prog->strs; s; s = s->next) {
            cg_def_label_sec(format(".LC%d", s->id), SEC_RODATA); // .zero %d
            if (s->prefix != 0) {
                // Wide string: decode UTF-8 and emit wide characters
                char *p = s->str;
                while (*p) {
                    char *next;
                    uint32_t c = decode_utf8(&next, p);
                    if (next == p) {
                        p++;
                        continue;
                    } // invalid UTF-8, skip
                    p = next;
                    if (s->elem_size == 2) {
                        // UTF-16: Windows wchar_t is UCS-2 (2 bytes).
                        // Code points above U+FFFF need a surrogate pair.
                        if (c > 0xFFFF) {
                            uint32_t sc = c - 0x10000;
                            secbuf_emit16le(cg_sec, (uint16_t)(0xD800 + (sc >> 10))); // .2byte %u
                            secbuf_emit16le(cg_sec, (uint16_t)(0xDC00 + (sc & 0x3FF))); // .2byte %u
                        } else {
                            secbuf_emit16le(cg_sec, (uint16_t)c); // .2byte %u
                        }
                    } else {
                        secbuf_emit32le(cg_sec, c); // .4byte %u
                    }
                }
                if (s->elem_size == 2)
                    secbuf_emit16le(cg_sec, 0); // .2byte 0
                else
                    secbuf_emit32le(cg_sec, 0); // .4byte 0
            } else {
                secbuf_emitbuf(cg_sec, s->str, s->len); // .byte %u
                secbuf_emit8(cg_sec, 0); // .byte 0
            }
        }
        cg_set_section(SEC_TEXT);
    }

    for (TLItem *item = prog->items; item; item = item->next) {
        if (item->kind == TL_ASM) {
            // Emit global-scope asm through the assembler
            // Parse label: "vide: ret" → label "vide", instruction "ret"
            const char *tp = item->asm_str;
            char label_buf[256];
            const char *label_end = strchr(tp, ':');
            if (label_end && label_end > tp) {
                size_t lbl_len = (size_t)(label_end - tp);
                if (lbl_len < sizeof(label_buf)) {
                    memcpy(label_buf, tp, lbl_len);
                    label_buf[lbl_len] = '\0';
                    // Skip whitespace between label and possible insn
                    const char *insn = label_end + 1;
                    insn += strspn(insn, " \t");
                    // Define label at current text position
                    objfile_add_sym(cg_obj, asm_sym_name(label_buf), SEC_TEXT,
                                    cg_sec->len, 0, SB_GLOBAL, ST_FUNC);
                    // If there's an instruction after the label, assemble it
                    if (*insn) {
                        // Simple instruction dispatch through the assembler
                        char mnem[64], ops[256];
                        int n = 0;
                        sscanf(insn, "%63s", mnem);
                        // Handle TCC-specific {$} → immediate $N
                        const char *rest = insn + strlen(mnem);
                        while (*rest == ' ' || *rest == '\t') rest++;
                        for (const char *s = rest; *s && n < (int)sizeof(ops) - 1; s++) {
                            if (s[0] == '{' && s[1] == '$' && s[2] == '}') {
                                ops[n++] = '$';
                                s += 3;
                                while (*s >= '0' && *s <= '9' && n < (int)sizeof(ops) - 1)
                                    ops[n++] = *s++;
                                if (*s && n < (int)sizeof(ops) - 1)
                                    ops[n++] = *s;
                                s--;
                            } else {
                                ops[n++] = *s;
                            }
                        }
                        ops[n] = '\0';
                        // Encode known instructions directly
#ifdef ARCH_ARM64
                        if (strcmp(mnem, "ret") == 0)
                            arm64_ret(cg_sec, ARM64_X30);
                        else if (strcmp(mnem, "nop") == 0)
                            arm64_nop(cg_sec);
                        else if (strcmp(mnem, "brk") == 0)
                            secbuf_emit32le(cg_sec, 0xd4200000u); // brk #0
#else
                        if (strcmp(mnem, "ret") == 0)
                            x86_ret(cg_sec);
                        else if (strcmp(mnem, "nop") == 0)
                            x86_nop(cg_sec);
                        else if (strcmp(mnem, "int3") == 0)
                            secbuf_emit8(cg_sec, 0xcc);
#endif
                        // For other instructions, fall through to nothing (assembler needed)
                    }
                }
            }
            continue;
        }
        Function *fn = item->fn;
        current_fn = fn->name;
        current_fn_def = fn;
        current_fn_stack_size = fn->stack_size;
        memset(spill_slot, 0, sizeof(spill_slot));
        next_spill_slot = 8;
#ifndef ARCH_ARM64
        init_spill_slots();
#endif
        fn_struct_ret_off = 0;
        fn_struct_ret_total = 0;

        // Pass 1: Generate dummy function body to discover register usage
        SecBuf _dummy;
        secbuf_init(&_dummy);
        SecBuf *saved_sec = cg_sec;
        cg_sec = &_dummy;
        cg_dry_run = true;
        used_regs = 0;
        ever_used_regs = 0;
        spilled_regs = 0;
        spill_count = 0;
        memset(reg_owner, 0, sizeof(reg_owner));
        ctrl_depth = 0;
        fn_uses_alloca = false;
        last_debug_file = 0;
        last_debug_line = 0;

        // Save params to locals (emitted to body buffer, will be after prologue)
        // ARM64: handled in the Pass 2 prologue instead
#ifndef ARCH_ARM64
        int param_xmm_index = 0;
        int stack_param_index = 0;
        int param_index = fn->ty->return_ty && (fn->ty->return_ty->kind == TY_STRUCT || fn->ty->return_ty->kind == TY_UNION || (is_complex(fn->ty->return_ty)
#ifdef _WIN32
                                                                                                                                && fn->ty->return_ty->size > 8
#endif
                                                                                                                                ))
            ? 1
            : 0;
#ifdef _WIN32
        int max_param_regs = 4;
        X86Reg cg_x86_paramreg[] = {X86_RCX, X86_RDX, X86_R8, X86_R9};
#else
        X86Reg cg_x86_paramreg[] = {X86_RDI, X86_RSI, X86_RDX, X86_RCX, X86_R8, X86_R9};
        int max_param_regs = 6;
        int max_param_xmm = 8;
#endif
        // Physical param register tables for prologue
        for (LVar *var = fn->params; var; var = var->param_next) {
#ifdef _WIN32
            if (is_flonum(var->ty)) {
                if (param_index < max_param_regs) {
                    if (var->ty->size == 4) {
                        asm_cvtsd2ss_xmm_rbp(cg_sec, param_index, var->offset); // cvtsd2ss xmm_i, xmm0; movss xmm0, -(off)(%rbp)
                    } else {
                        asm_movsd_xmm_rbp(cg_sec, param_index, var->offset); // movsd xmm_i, -(off)(%rbp)
                    }
                    param_index++;
                    param_xmm_index++;
                } else {
                    // Float on stack (arg position >= 4 in Win64)
                    int stack_off = 48 + stack_param_index * 8;
                    if (var->ty->size == 4) {
                        asm_movss_rm_rbp(cg_sec, 0, stack_off); // movss stack_off(%rbp), xmm0
                        asm_movss_mr_rbp(cg_sec, 0, var->offset); // movss xmm0, -(off)(%rbp)
                    } else {
                        asm_movsd_rm_rbp(cg_sec, 0, stack_off); // movsd stack_off(%rbp), xmm0
                        asm_movsd_mr_rbp(cg_sec, 0, var->offset); // movsd xmm0, -(off)(%rbp)
                    }
                    stack_param_index++;
                }
            } else if (param_index < max_param_regs) {
                int psz = var->ty->size == 1 ? 1 : var->ty->size == 2 ? 2
                    : var->ty->size <= 4                              ? 4
                                                                      : 8;
                // Structs > 8 bytes (and 16-byte _Complex double) are passed by
                // pointer; copy the pointee to the local stack slot.
                if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION || is_complex(var->ty)) && var->ty->size > 8) {
                    int c = ++rcc_label_count;
                    asm_mov_preg_r11(cg_sec, cg_x86_paramreg[param_index]); // movq preg, %r11
                    x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size); // movq $size, %r10
                    cg_def_label(format(".L.pcopy.%d", c)); // .L.pcopy.%d:\n", c
                    x86_cmp_ri(cg_sec, 8, X86_R10, 0); // cmpq $0, %r10
                    size_t jze1 = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, jze1, format(".L.pcopy_end.%d", c), 0);
                    asm_movb_r11_r10_al(cg_sec, -1); // movb -1(%r11,%r10), %%al
                    asm_movb_al_rbp_r10(cg_sec, var->offset); // movb %%al, -(off)-1(%rbp,%r10) !!
                    x86_sub_ri(cg_sec, 8, X86_R10, 1); // subq $1, %r10
                    size_t jmp1 = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, jmp1, format(".L.pcopy.%d", c), 0);
                    cg_def_label(format(".L.pcopy_end.%d", c));
                } else if (var->ty->kind == TY_INT128) {
                    // __int128 is passed by pointer (like a large struct) on Windows x64.
                    // param_regs64[param_index] holds a pointer; dereference it.
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(cg_x86_paramreg[param_index], 0)); // movq (preg), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX); // movq %rax, -off(%rbp)
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(cg_x86_paramreg[param_index], 8)); // movq 8(preg), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), X86_RAX); // movq %rax, -(off-8)(%rbp)
                } else {
                    asm_mov_rbp(cg_sec, cg_x86_paramreg[param_index], psz, var->offset); // mov preg, -(off)(%rbp)
                }
                param_index++;
            } else {
                // Stack argument on Windows (shadow space = 32 bytes)
                int stack_off = 48 + stack_param_index * 8;
                if (is_flonum(var->ty)) {
                    if (var->ty->size == 4) {
                        asm_movss_rm_rbp(cg_sec, 0, stack_off); // movss stack_off(%rbp), xmm0
                        asm_movss_mr_rbp(cg_sec, 0, var->offset); // movss xmm0, -(off)(%rbp)
                    } else {
                        asm_movsd_rm_rbp(cg_sec, 0, stack_off); // movsd stack_off(%rbp), xmm0
                        asm_movsd_mr_rbp(cg_sec, 0, var->offset); // movsd xmm0, -(off)(%rbp)
                    }
                } else if (var->ty->kind == TY_INT128) {
                    // __int128 on stack: slot holds a pointer; dereference it.
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off)); // movq stack_off(%rbp), %rax
                    x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RAX, 0)); // movq (%rax), %rcx  (lo)
                    asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, var->offset);
                    x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RAX, 8)); // movq 8(%rax), %rcx  (hi)
                    asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, var->offset - 8);
                } else if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION || is_complex(var->ty)) && var->ty->size > 8) {
                    // Structs > 8 bytes (and 16-byte _Complex double) are passed
                    // by pointer even on the stack; load the pointer and copy
                    // the pointee into the local slot.
                    int c = ++rcc_label_count;
                    x86_mov_rm(cg_sec, 8, X86_R11, x86_mem(X86_RBP, stack_off)); // movq stack_off(%rbp), %r11
                    x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size); // movq $size, %r10
                    cg_def_label(format(".L.pcopy.%d", c));
                    x86_cmp_ri(cg_sec, 8, X86_R10, 0); // cmpq $0, %r10
                    size_t jze3 = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, jze3, format(".L.pcopy_end.%d", c), 0);
                    asm_movb_r11_r10_al(cg_sec, -1); // movb -1(%r11,%r10), %%al
                    asm_movb_al_rbp_r10(cg_sec, var->offset); // movb %%al, -(off)-1(%rbp,%r10)
                    x86_sub_ri(cg_sec, 8, X86_R10, 1); // subq $1, %r10
                    size_t jmp3 = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, jmp3, format(".L.pcopy.%d", c), 0);
                    cg_def_label(format(".L.pcopy_end.%d", c));
                } else {
                    int psz = var->ty->size == 1 ? 1 : var->ty->size == 2 ? 2
                        : var->ty->size <= 4                              ? 4
                                                                          : 8;
                    // tmpreg = sized %rax
                    asm_mov_rbp_tmpreg(cg_sec, stack_off, psz); // mov stack_off(%rbp), tmpreg
                    asm_mov_tmpreg_rbp(cg_sec, var->offset, psz); // mov tempreg, -(off)(%rbp)
                }
                stack_param_index++;
            }
#else
            if (is_flonum(var->ty)) {
                if (param_xmm_index < max_param_xmm) {
                    if (var->ty->size == 4) {
                        asm_cvtsd2ss_xmm_rbp(cg_sec, param_xmm_index, var->offset); // cvtsd2ss xmm_i; movss xmm0, -(off)(%rbp)
                    } else {
                        asm_movsd_xmm_rbp(cg_sec, param_xmm_index, var->offset); // movsd xmm_i, -(off)(%rbp)
                    }
                    param_xmm_index++;
                } else {
                    int stack_off2 = 16 + stack_param_index * 8;
                    if (var->ty->size == 4) {
                        asm_movss_rm_rbp(cg_sec, 0, stack_off2); // movss off2(%rbp), xmm0
                        asm_movss_mr_rbp(cg_sec, 0, var->offset); // movss xmm0, -(off2)(%rbp)
                    } else {
                        asm_movsd_rm_rbp(cg_sec, 0, stack_off2); // movsd off2(%rbp), xmm0
                        asm_movsd_mr_rbp(cg_sec, 0, var->offset); // movsd xmm0, -(off2)(%rbp)
                    }
                    stack_param_index++;
                }
            } else if (var->ty->kind == TY_INT128) {
#ifdef _WIN32
                // Windows x64: __int128 is passed by pointer in a single GP register.
                // Dereference the pointer to copy the 16-byte value to the local slot.
                if (param_index < max_param_regs) {
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(cg_x86_paramreg[param_index], 0)); // movq (preg), %rax  (lo)
                    asm_mov_phyreg_rbp(cg_sec, X86_RAX, 8, var->offset);
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(cg_x86_paramreg[param_index], 8)); // movq 8(preg), %rax  (hi)
                    asm_mov_phyreg_rbp(cg_sec, X86_RAX, 8, var->offset - 8);
                    param_index++;
                } else {
                    // Stack: pointer is in the shadow-space-extended stack slot.
                    int stack_off = 48 + stack_param_index * 8;
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off)); // movq stack_off(%rbp), %rax
                    x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RAX, 0)); // movq (%rax), %rcx  (lo)
                    asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, var->offset);
                    x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RAX, 8)); // movq 8(%rax), %rcx  (hi)
                    asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, var->offset - 8);
                    stack_param_index++;
                }
#else
                // x86-64 SysV: int128 lo in param_regs64[N], hi in param_regs64[N+1]
                if (param_index + 1 < max_param_regs) {
                    x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), cg_x86_paramreg[param_index]); // movq param, -off(%rbp)
                    x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), cg_x86_paramreg[param_index + 1]); // movq param+1, -(off-8)(%rbp)
                    param_index += 2;
                } else {
                    // On stack: two consecutive 8-byte slots at [rbp + 16 + k*8]
                    int stack_off = 16 + stack_param_index * 8;
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off)); // movq stack_off(%rbp), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX); // movq %rax, -off(%rbp)
                    x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off + 8)); // movq stack_off+8(%rbp), %rax
                    x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), X86_RAX); // movq %rax, -(off-8)(%rbp)
                    stack_param_index += 2;
                }
#endif
            } else if (is_complex(var->ty)) {
                // Complex types: {real, imag} like struct of two base-type elements
                // _Complex float  (8 bytes):  one SSE eightbyte   → xmm0
                // _Complex double (16 bytes): two SSE eightbytes  → xmm0, xmm1
                // _Complex int    (8 bytes):  one INTEGER eightbyte → GP reg
                int base_sz = var->ty->base ? var->ty->base->size : 4;
#ifdef _WIN32
                // Win64: an 8-byte _Complex (float or int base) is passed by
                // value in a single GP register, like any other 8-byte
                // aggregate — XMM is only used for native float/double
                // scalars, not for _Complex.
                bool win_small_complex = var->ty->size <= 8;
#else
                bool win_small_complex = false;
#endif
                if (is_flonum(var->ty->base) && !win_small_complex) {
                    // Float complex: pass in xmm registers
                    if (var->ty->size <= 8) {
                        // _Complex float: 8 bytes in one xmm reg
                        if (param_xmm_index < max_param_xmm) {
                            asm_movsd_xmm_rbp(cg_sec, param_xmm_index, var->offset); // movsd xmm_i, -off(%rbp)
                            param_xmm_index++;
                        } else {
                            asm_movsd_rm_rbp(cg_sec, 0, 16 + stack_param_index * 8); // movsd stack_off(%rbp), %xmm0
                            asm_movsd_mr_rbp(cg_sec, 0, var->offset); // movsd %xmm0, -off(%rbp)
                            stack_param_index++;
                        }
                    } else {
                        // _Complex double: 16 bytes, two xmm regs
                        if (param_xmm_index + 1 < max_param_xmm) {
                            asm_movsd_xmm_rbp(cg_sec, param_xmm_index, var->offset); // movsd xmm_i, -off(%rbp)
                            asm_movsd_xmm_rbp(cg_sec, param_xmm_index + 1, var->offset - base_sz); // movsd xmm_i+1, -(off-base_sz)(%rbp)
                            param_xmm_index += 2;
                        } else {
                            int stack_off = 16 + stack_param_index * 8;
                            asm_movsd_rm_rbp(cg_sec, 0, stack_off); // movsd stack_off(%rbp), %xmm0
                            asm_movsd_mr_rbp(cg_sec, 0, var->offset); // movsd %xmm0, -off(%rbp)
                            asm_movsd_rm_rbp(cg_sec, 0, stack_off + 8); // movsd stack_off+8(%rbp), %xmm0
                            asm_movsd_mr_rbp(cg_sec, 0, var->offset - base_sz); // movsd %xmm0, -(off-base_sz)(%rbp)
                            stack_param_index += 2;
                        }
                    }
                } else {
                    // Integer complex: pass in GP registers
                    if (var->ty->size <= 8) {
                        // _Complex int: 8 bytes in one GP reg
                        if (param_index < max_param_regs) {
                            asm_mov_rbp(cg_sec, cg_x86_paramreg[param_index], 8, var->offset); // movq preg, -off(%rbp)
                            param_index++;
                        } else {
                            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, 16 + stack_param_index * 8)); // movq stack_off(%rbp), %rax
                            x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX); // movq %rax, -off(%rbp)
                            stack_param_index++;
                        }
                    } else {
                        // _Complex long long: 16 bytes in two GP regs
                        if (param_index + 1 < max_param_regs) {
                            asm_mov_rbp(cg_sec, cg_x86_paramreg[param_index], 8, var->offset); // movq preg, -off(%rbp)
                            asm_mov_rbp(cg_sec, cg_x86_paramreg[param_index + 1], 8, var->offset - base_sz); // movq preg+1, -(off-base_sz)(%rbp)
                            param_index += 2;
                        } else {
                            int stack_off = 16 + stack_param_index * 8;
                            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off)); // movq stack_off(%rbp), %rax
                            x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX); // movq %rax, -off(%rbp)
                            x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off + 8)); // movq stack_off+8(%rbp), %rax
                            x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - base_sz)), X86_RAX); // movq %rax, -(off-base_sz)(%rbp)
                            stack_param_index += 2;
                        }
                    }
                }
            } else if (param_index < max_param_regs) {
                // X86Reg preg = lin_gp_regs[param_index];
                int psz = var->ty->size == 1 ? 1 : var->ty->size == 2 ? 2
                    : var->ty->size <= 4                              ? 4
                                                                      : 8;
                if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && var->ty->size > 8) {
                    int c = ++rcc_label_count;
                    asm_mov_preg_r11(cg_sec, cg_x86_paramreg[param_index]); // movq preg, %r11
                    x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size); // movq $size, %r10
                    cg_def_label(format(".L.pcopy.%d", c));
                    x86_cmp_ri(cg_sec, 8, X86_R10, 0); // cmpq $0, %r10
                    size_t jze2 = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, jze2, format(".L.pcopy_end.%d", c), 0);
                    asm_movb_r11_r10_al(cg_sec, -1); // movb -1(%r11,%r10), %%al
                    asm_movb_al_rbp_r10(cg_sec, var->offset); // movb %%al, -(off)-1(%rbp,%r10)
                    x86_sub_ri(cg_sec, 8, X86_R10, 1); // subq $1, %r10
                    size_t jmp2 = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, jmp2, format(".L.pcopy.%d", c), 0);
                    cg_def_label(format(".L.pcopy_end.%d", c));
                } else {
                    asm_mov_rbp(cg_sec, cg_x86_paramreg[param_index], psz, var->offset); // mov preg, -(off)(%rbp)
                }
                param_index++;
            } else {
                if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && var->ty->size > 8) {
                    int c = ++rcc_label_count;
                    int stack_off2 = 16 + stack_param_index * 8;
                    x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size); // movq $size, %r10
                    cg_def_label(format(".L.param.%d", c));
                    x86_cmp_ri(cg_sec, 8, X86_R10, 0); // cmpq $0, %r10
                    size_t jze3 = asm_jcc_label(cg_sec, X86_E);
                    asm_fixup_add(cg_sec, jze3, format(".L.param_end.%d", c), 0);
                    asm_movb_rbp_r10_al(cg_sec, stack_off2); // movb stack_off-1(%rbp,%r10), %%al
                    asm_movb_al_rbp_r10(cg_sec, var->offset); // movb %%al, -(off)-1(%rbp,%r10)
                    x86_sub_ri(cg_sec, 8, X86_R10, 1); // subq $1, %r10
                    size_t jmp3 = asm_jmp_label(cg_sec);
                    asm_fixup_add(cg_sec, jmp3, format(".L.param.%d", c), 0);
                    cg_def_label(format(".L.param_end.%d", c));
                } else {
                    int stack_off2 = 16 + stack_param_index * 8;
                    int psz = var->ty->size == 1 ? 1 : var->ty->size == 2 ? 2
                        : var->ty->size <= 4                              ? 4
                                                                          : 8;
                    asm_mov_rbp_tmpreg(cg_sec, stack_off2, psz); // mov stack_off(%rbp), %rax
                    asm_mov_tmpreg_rbp(cg_sec, var->offset, psz); // mov %rax, -(off)(%rbp)
                }
                stack_param_index++;
            }
#endif
        }
#endif /* !ARCH_ARM64 */

#ifndef ARCH_ARM64
        // Save arg registers if function is variadic
        if (fn->is_variadic) {
            int gp_count = param_index;
            int va_fp = param_xmm_index;
#ifdef _WIN32
            // Windows x64 ABI: 4 GP regs (rcx,rdx,r8,r9), 4 XMM regs (xmm0-3)
            va_reg_save_ofs = current_fn_stack_size + 96;
            va_gp_start = gp_count * 8;
            va_fp_start = va_fp * 16 + 32;
            va_st_start = 48 + stack_param_index * 8;
            // va_st_start = 48 + (gp_count > 4 ? (gp_count - 4) : 0) * 8;
            switch (gp_count) {
            case 0: asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, va_reg_save_ofs); /* fallthrough */ /* movq %rcx, -%d(%rbp) */
            case 1: asm_mov_phyreg_rbp(cg_sec, X86_RDX, 8, va_reg_save_ofs - 8); /* fallthrough */ /* movq %rdx, -%d(%rbp) */
            case 2: asm_mov_phyreg_rbp(cg_sec, X86_R8, 8, va_reg_save_ofs - 16); /* fallthrough */ /* movq %r8, -%d(%rbp) */
            case 3: asm_mov_phyreg_rbp(cg_sec, X86_R9, 8, va_reg_save_ofs - 24); /* movq %r9, -%d(%rbp) */
            }
            // Save all 4 xmm regs unconditionally (caller puts FP in both GP and XMM)
            asm_movaps_rbp_xmm(cg_sec, 0, va_reg_save_ofs - 32); /* movaps %xmm0, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 1, va_reg_save_ofs - 48); /* movaps %xmm1, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 2, va_reg_save_ofs - 64); /* movaps %xmm2, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 3, va_reg_save_ofs - 80); /* movaps %xmm3, -%d(%rbp) */
#else
            va_reg_save_ofs = current_fn_stack_size + 176;
            va_gp_start = gp_count * 8;
            va_fp_start = va_fp * 16 + 48;
            va_st_start = 16 + stack_param_index * 8;

            switch (gp_count) {
            case 0: asm_mov_phyreg_rbp(cg_sec, X86_RDI, 8, va_reg_save_ofs); /* fallthrough */ /* movq %rdi, -%d(%rbp) */
            case 1: asm_mov_phyreg_rbp(cg_sec, X86_RSI, 8, va_reg_save_ofs - 8); /* fallthrough */ /* movq %rsi, -%d(%rbp) */
            case 2: asm_mov_phyreg_rbp(cg_sec, X86_RDX, 8, va_reg_save_ofs - 16); /* fallthrough */ /* movq %rdx, -%d(%rbp) */
            case 3: asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, va_reg_save_ofs - 24); /* fallthrough */ /* movq %rcx, -%d(%rbp) */
            case 4: asm_mov_phyreg_rbp(cg_sec, X86_R8, 8, va_reg_save_ofs - 32); /* fallthrough */ /* movq %r8, -%d(%rbp) */
            case 5: asm_mov_phyreg_rbp(cg_sec, X86_R9, 8, va_reg_save_ofs - 40); /* movq %r9, -%d(%rbp) */
            }
            if (va_fp < 8) {
                x86_test_rr(cg_sec, 1, X86_RAX, X86_RAX); // testb %%al, %%al
                {
                    size_t o = asm_jcc_label(cg_sec, X86_E); // jcc label
                    asm_fixup_add(cg_sec, o, format(".L.x%d", rcc_label_count), 1);
                }
                switch (va_fp) {
                case 0: asm_movaps_rbp_xmm(cg_sec, 0, va_reg_save_ofs - 48); /* fallthrough */ /* movaps %xmm0, -%d(%rbp) */
                case 1: asm_movaps_rbp_xmm(cg_sec, 1, va_reg_save_ofs - 64); /* fallthrough */ /* movaps %xmm1, -%d(%rbp) */
                case 2: asm_movaps_rbp_xmm(cg_sec, 2, va_reg_save_ofs - 80); /* fallthrough */ /* movaps %xmm2, -%d(%rbp) */
                case 3: asm_movaps_rbp_xmm(cg_sec, 3, va_reg_save_ofs - 96); /* fallthrough */ /* movaps %xmm3, -%d(%rbp) */
                case 4: asm_movaps_rbp_xmm(cg_sec, 4, va_reg_save_ofs - 112); /* fallthrough */ /* movaps %xmm4, -%d(%rbp) */
                case 5: asm_movaps_rbp_xmm(cg_sec, 5, va_reg_save_ofs - 128); /* fallthrough */ /* movaps %xmm5, -%d(%rbp) */
                case 6: asm_movaps_rbp_xmm(cg_sec, 6, va_reg_save_ofs - 144); /* fallthrough */ /* movaps %xmm6, -%d(%rbp) */
                case 7: asm_movaps_rbp_xmm(cg_sec, 7, va_reg_save_ofs - 160); /* movaps %xmm7, -%d(%rbp) */
                }
                cg_def_label(format(".L.x%d", rcc_label_count++)); // .L.x%d:
            }
#endif
        }
#else
        // Compute va_list init values for ARM64 (register saves are in Pass 2)
        if (fn->is_variadic) {
            int gp_param = 0;
            int fp_param = 0;
            int stack_param = 0;
            for (LVar *var = fn->params; var; var = var->param_next) {
                if (is_flonum(var->ty)) {
                    if (fp_param < 8) fp_param++;
                    else
                        stack_param++;
                } else if (gp_param < 8) {
                    gp_param++;
                } else {
                    stack_param++;
                }
            }
#ifdef __APPLE__
            // Apple ARM64: unnamed variadic args go on the stack, not in registers.
            // Set va offsets past the register save area so va_arg immediately
            // reads from the overflow (stack) area.
            va_gp_start = 64;
            va_fp_start = 192;
#else
            // AArch64 ABI: gr_offs = -(8 - gp_param) * 8, vr_offs = -(8 - fp_param) * 16
            va_gp_start = -(8 - gp_param) * 8;
            va_fp_start = -(8 - fp_param) * 16;
#endif
            va_st_start = 16 + stack_param * 8;
        }
#endif

        for (Node *n = fn->body; n; n = n->next) {
            VReg r = gen(n);
            if (r != -1) free_reg(r);
        }
        // Pass 2: Emit binary prologue, body, epilogue
        cg_sec = saved_sec;
        secbuf_free(&_dummy);
        cg_dry_run = false;
        used_regs = 0;
        spilled_regs = 0;
        spill_count = 0;
        memset(reg_owner, 0, sizeof(reg_owner));
        fn_struct_ret_off = 0; // reset for Pass 2 (fn_struct_ret_total already computed)
        cg_label_ht_reset();
        asm_fixup_ht_reset();

#ifdef ARCH_ARM64
        // === ARM64 prologue ===
        // Callee-saved: x19-x28 are callee-saved among our 12 allocatable regs
        // x19-x24 = indices 6-11 in our reg arrays
        // If the function has cleanup vars and returns non-void, we use x19 to
        // preserve the return value across cleanup calls; mark x19 as used now
        // so the prologue saves/restores it correctly.
        bool fn_ret_nonvoid = fn->ty->return_ty && fn->ty->return_ty->kind != TY_VOID;
        {
            for (LVar *var = fn->locals; var; var = var->next) {
                if (var_has_cleanup(var) && fn_ret_nonvoid) {
                    ever_used_regs |= (1 << 6); // x19 will be used in cleanup section
                    break;
                }
            }
        }
        int callee_mask = ((ever_used_regs >> 6) & 63);
        int n_callee_saved = 0;
        for (int j = 0; j < 6; j++)
            if (callee_mask & (1 << j)) n_callee_saved++;

        int need = fn->stack_size + fn_struct_ret_total + 32;
        // Include register spill slots in frame (discovered during dry run)
        if (need < next_spill_slot)
            need = next_spill_slot;
        int va_save_size = 0;
#ifndef __APPLE__
        // AAPCS64 (Linux): save all GP and FP arg regs for variadic functions
        if (fn->is_variadic) va_save_size = 192;
#endif
        if (va_save_size > 0)
            need += va_save_size;
        int frame_size = need + 16 + n_callee_saved * 8;
        // Round up to 16-byte alignment
        frame_size = (frame_size + 15) & ~15;
        // va_reg_save_ofs not used on ARM64; reg_save_area is stored as sp in va_start

        // Symbol linkage
        bool has_noninline_decl = false;
        bool had_extern_decl = false;
        if (fn->is_inline && !fn->is_extern) {
            for (LVar *g = prog->globals; g; g = g->next) {
                if (g->is_function && g->name == fn->name) {
                    if (g->has_init) has_noninline_decl = true;
                    if (g->is_extern && !g->is_weak) had_extern_decl = true;
                    break;
                }
            }
        }
        char *fn_label = fn->name;
        if (is_asm_reserved(fn->name))
            fn_label = format(".L_rcc_%s", fn->name);
        bool fn_exported = !fn->is_static && (!fn->is_inline || fn->is_extern || has_noninline_decl || had_extern_decl);
        if (fn->is_weak) {
            cg_weak_label(asm_sym_name(sym_name(fn->name))); // .weak_definition %s
        } else if (fn_exported)
            cg_global_label(asm_sym_name(sym_name(fn->name))); // .globl %s
        else
            cg_def_label(asm_sym_name(sym_name(fn_label))); // .weak %s

        // Stack frame: stp fp,lr; mov fp,sp; sub sp,sp,#frame_size
        asm_stp_fp_lr(cg_sec); // stp x29, x30, [sp, #-16]!
        asm_mov_fp_sp(cg_sec); // mov x29, sp
        if (frame_size <= 4095)
            arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_SP, frame_size, 0); // sub sp, sp, #frame_size
        else {
            emit_mov_imm64(ARM64_X16, (uint64_t)frame_size); // mov x16, #frame_size
            arm64_sub_extreg(cg_sec, 1, ARM64_SP, ARM64_SP, ARM64_X16, ARM64_UXTX, 0); // sub sp, sp, x16
        }

        // Save variadic argument registers at the bottom of the frame (sp)
        // Skip on Apple ARM64: variadic args are already on the caller's stack.
        if (fn->is_variadic && va_save_size > 0) {
            asm_stp_sp(cg_sec, ARM64_X0, ARM64_X1, 0, false, false); // stp x0, x1, [sp]
            asm_stp_sp(cg_sec, ARM64_X2, ARM64_X3, 2, false, false); // stp x2, x3, [sp, #16]
            asm_stp_sp(cg_sec, ARM64_X4, ARM64_X5, 4, false, false); // stp x4, x5, [sp, #32]
            asm_stp_sp(cg_sec, ARM64_X6, ARM64_X7, 6, false, false); // stp x6, x7, [sp, #48]
            asm_stp_q_sp(cg_sec, 0, 1, 64); // stp q0, q1, [sp, #64]
            asm_stp_q_sp(cg_sec, 2, 3, 96); // stp q2, q3, [sp, #96]
            asm_stp_q_sp(cg_sec, 4, 5, 128); // stp q4, q5, [sp, #128]
            asm_stp_q_sp(cg_sec, 6, 7, 160); // stp q6, q7, [sp, #160]
            // Save original sp so va_start can find the reg_save_area even after alloca
            arm64_add_imm(cg_sec, 1, ARM64_X16, ARM64_SP, 0, 0); // mov x16, sp
            arm64_stur(cg_sec, 3, ARM64_X16, ARM64_X29, -8); // stur x16, [x29, #-8]
        }

        // Save callee-saved regs
        int cs_off = (fn->is_variadic && va_save_size > 0) ? va_save_size + 16 : 16;
        for (int j = 0; j < 6; j++) {
            if (callee_mask & (1 << j)) {
                arm64_str_uoff(cg_sec, 3, (Arm64Reg)(ARM64_X19 + j), ARM64_SP, cs_off / 8); // str x{cs}, [sp, #cs_off]
                cs_off += 8;
            }
        }

        // Save hidden struct return pointer if needed
        if (fn->ty->return_ty && (fn->ty->return_ty->kind == TY_STRUCT || fn->ty->return_ty->kind == TY_UNION || fn->ty->return_ty->kind == TY_COMPLEX)) {
            int retbuf_offset = 0;
            for (LVar *var = fn->locals; var; var = var->next) {
                if (var->name && var->name == kw_retbuf) {
                    retbuf_offset = var->offset;
                    break;
                }
            }
            if (retbuf_offset <= 4095)
                arm64_stur(cg_sec, 3, ARM64_X8, ARM64_X29, -retbuf_offset); // stur x8, [x29, #-retbuf_offset]
            else {
                int v = retbuf_offset;
                emit_mov_imm64(ARM64_X16, (uint64_t)v); // mov x16, #v
                arm64_sub_reg(cg_sec, 1, ARM64_X16, ARM64_X29, ARM64_X16, ARM64_LSL, 0); // sub x16, x29, x16
                arm64_str_uoff(cg_sec, 3, ARM64_X8, ARM64_X16, 0); // str x8, [x16]
            }
        }

        // Save incoming params to stack slots
        int gp_param = 0;
        int fp_param = 0;
        int stack_param = 0;
        {
            for (LVar *var = fn->params; var; var = var->param_next) {
                int hfa_elem_size = 0;
                int hfa_count = (!is_flonum(var->ty) && (var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION))
                    ? arm64_hfa_count(var->ty, &hfa_elem_size)
                    : 0;
                if (hfa_count > 0 && fp_param + hfa_count <= 8) {
                    if (var->offset <= 4095)
                        asm_sub_x16_fp_imm(cg_sec, var->offset); // sub x16, x29, #var->offset
                    else {
                        int v = var->offset;
                        emit_mov_imm64(ARM64_X16, (uint64_t)v); // mov x16, #v
                        asm_sub_x16_fp_x16(cg_sec); // sub x16, x29, x16
                    }
                    for (int j = 0; j < hfa_count; j++) {
                        int off = j * hfa_elem_size;
                        if (hfa_elem_size == 4)
                            asm_str_s_x16_off(cg_sec, fp_param + j, off); // str s{fp_param+j}, [x16, #off]
                        else
                            asm_str_d_x16_off(cg_sec, fp_param + j, off); // str d{fp_param+j}, [x16, #off]
                    }
                    fp_param += hfa_count;
                } else if (hfa_count > 0) {
                    // HFA struct overflowed V registers — copy from caller's stack frame
                    int spoff = 16 + stack_param * 8;
                    int sz = var->ty->size;
                    // Use byte-copy from caller's stack to local slot
                    int c = ++rcc_label_count;
                    if (var->offset <= 4095)
                        arm64_sub_imm(cg_sec, 1, ARM64_X13, ARM64_X29, var->offset, 0); // sub x13, x29, #var->offset
                    else {
                        int v = var->offset;
                        asm_mov_imm(cg_sec, 13, 8, v & 0xffff);
                        v >>= 16;
                        int s = 16;
                        while (v) {
                            asm_movk(cg_sec, 13, 1, (uint16_t)(v & 0xffff), s);
                            v >>= 16;
                            s += 16;
                        }
                        arm64_sub_reg(cg_sec, 1, ARM64_X13, ARM64_X29, ARM64_X13, ARM64_LSL, 0); // sub x13, x29, x13
                    }
                    // x11 = source (caller's stack)
                    arm64_add_imm(cg_sec, 1, ARM64_X11, ARM64_X29, spoff, 0); // add x11, x29, #spoff
                    asm_mov_imm_phy(cg_sec, ARM64_X9, 8, sz); // mov x9, #sz
                    cg_label_ht_add(format(".L.pcopy.%d", c), cg_sec->len); // .L.pcopy.c:
                    arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X9, 0, 0); // cmp x9, #0
                    {
                        size_t _off = asm_jcc_label(cg_sec, ARM64_EQ);
                        asm_fixup_add(cg_sec, _off, format(".L.pcopy_end.%d", c), 1);
                    } // b.eq .L.pcopy_end.c
                    arm64_sub_imm(cg_sec, 1, ARM64_X9, ARM64_X9, 1, 0); // sub x9, x9, #1
                    asm_ldrb_w16_x9_phy(cg_sec, ARM64_X11); // ldrb w16, [x11, x9]
                    asm_strb_w16_x9_phy(cg_sec, ARM64_X13); // strb w16, [x13, x9]
                    {
                        size_t _off = asm_jmp_label(cg_sec);
                        asm_fixup_add(cg_sec, _off, format(".L.pcopy.%d", c), 0);
                    } // b .L.pcopy.c
                    cg_label_ht_add(format(".L.pcopy_end.%d", c), cg_sec->len);
                    asm_fixup_resolve(cg_sec, format(".L.pcopy_end.%d", c), cg_sec->len); // .L.pcopy_end.c:
                    stack_param += (sz + 7) / 8;
                } else if (is_flonum(var->ty)) {
                    if (fp_param < 8) {
                        if (var->ty->size == 4) {
                            if (fn->ty->is_oldstyle) {
                                asm_fcvt_s0_d(cg_sec, fp_param); // fcvt s0, d{fp_param}
                                asm_str_s0_fp_neg(cg_sec, var->offset); // str s0, [x29, #-offset]
                            } else {
                                asm_str_s_fp_neg(cg_sec, fp_param, var->offset); // str s{fp_param}, [x29, #-offset]
                            }
                        } else
                            asm_str_d_fp_neg(cg_sec, fp_param, var->offset); // str d{fp_param}, [x29, #-offset]
                        fp_param++;
                    }
                } else if (var->ty->kind == TY_INT128) {
                    // int128: two consecutive GP registers
                    if (gp_param + 1 < 8) {
                        /* golden:
                           printf("  str %s, [%s, #-%d]\n", gpreg[gp_param], FRAME_PTR, var->offset);
                           printf("  str %s, [%s, #-%d]\n", gpreg[gp_param + 1], FRAME_PTR, var->offset - 8);
                           GNU as converts str x{n}, [x29, #-offset] to stur */
                        arm64_stur(cg_sec, 1, (Arm64Reg)gp_param, ARM64_X29, -var->offset); // stur x{gp_param}, [x29, #-offset]
                        arm64_stur(cg_sec, 1, (Arm64Reg)(gp_param + 1), ARM64_X29, -(var->offset - 8)); // stur x{gp_param+1}, [x29, #-(offset-8)]
                        gp_param += 2;
                    } else {
                        // int128 on stack: two 8-byte slots from caller's frame
                        int spoff = 16 + stack_param * 8;
                        arm64_ldr_uoff(cg_sec, 3, ARM64_X11, ARM64_X29, (uint32_t)(spoff / 8)); // ldr x11, [x29, #spoff]
                        arm64_stur(cg_sec, 1, ARM64_X11, ARM64_X29, -var->offset); // stur x11, [x29, #-var->offset]
                        arm64_ldr_uoff(cg_sec, 3, ARM64_X11, ARM64_X29, (uint32_t)((spoff + 8) / 8)); // ldr x11, [x29, #spoff+8]
                        arm64_stur(cg_sec, 1, ARM64_X11, ARM64_X29, -(var->offset - 8)); // stur x11, [x29, #-(var->offset-8)]
                        stack_param += 2;
                    }
                } else if (gp_param < 8) {
                    int sf = var->ty->size <= 4 ? 0 : 1; // word or dword
                    if (is_complex(var->ty) && var->ty->size > 8) {
                        // _Complex double (16 bytes): two consecutive GP registers
                        if (gp_param + 1 < 8) {
                            arm64_stur(cg_sec, sf, (Arm64Reg)gp_param, ARM64_X29, -var->offset); // stur x{gp_param}, [x29, #-offset]
                            arm64_stur(cg_sec, sf, (Arm64Reg)(gp_param + 1), ARM64_X29, -(var->offset - 8)); // stur x{gp_param+1}, [x29, #-(offset-8)]
                            gp_param += 2;
                            continue;
                        } else {
                            // complex > 8 bytes on stack
                            int spoff = 16 + stack_param * 8;
                            arm64_ldr_uoff(cg_sec, 3, ARM64_X11, ARM64_X29, (uint32_t)(spoff / 8)); // ldr x11, [x29, #spoff]
                            arm64_stur(cg_sec, 1, ARM64_X11, ARM64_X29, -var->offset); // stur x11, [x29, #-offset]
                            arm64_ldr_uoff(cg_sec, 3, ARM64_X11, ARM64_X29, (uint32_t)((spoff + 8) / 8)); // ldr x11, [x29, #spoff+8]
                            arm64_stur(cg_sec, 1, ARM64_X11, ARM64_X29, -(var->offset - 8)); // stur x11, [x29, #-(offset-8)]
                            stack_param += 2;
                            continue;
                        }
                    }
                    if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION) && var->ty->size > 8) {

                        int c = ++rcc_label_count;
                        arm64_add_imm(cg_sec, 1, ARM64_X16, gp_param, 0, 0); // mov x16, x{gp_param}
                        if (var->offset <= 4095)
                            arm64_sub_imm(cg_sec, 1, ARM64_X17, ARM64_X29, var->offset, 0); // sub x17, x29, #var->offset
                        else {
                            emit_mov_imm64(ARM64_X17, (uint64_t)var->offset); // mov x17, #var->offset
                            arm64_sub_reg(cg_sec, 1, ARM64_X17, ARM64_X29, ARM64_X17, ARM64_LSL, 0); // sub x17, x29, x17
                        }
                        emit_mov_imm64(ARM64_X9, (uint64_t)var->ty->size); // mov x9, #var->ty->size
                        cg_def_label(format(".L.param_copy.%d", c));
                        arm64_subs_imm(cg_sec, 1, ARM64_XZR, ARM64_X9, 0, 0); // cmp x9, #0
                        size_t cj = asm_jcc_label(cg_sec, ARM64_EQ); // beq .L.param_copy_end.%d
                        asm_fixup_add(cg_sec, cj, format(".L.param_copy_end.%d", c), 1);
                        arm64_sub_imm(cg_sec, 1, ARM64_X9, ARM64_X9, 1, 0); // sub x9, x9, #1
                        asm_ldur_phy(cg_sec, ARM64_X18, ARM64_X16, 0, 0); // ldurb w18, [x16]
                        asm_stur_phy(cg_sec, ARM64_X18, ARM64_X17, 0, 0); // sturb w18, [x17]
                        arm64_add_imm(cg_sec, 1, ARM64_X16, ARM64_X16, 1, 0); // add x16, x16, #1
                        arm64_add_imm(cg_sec, 1, ARM64_X17, ARM64_X17, 1, 0); // add x17, x17, #1
                        size_t cj2 = asm_jmp_label(cg_sec); // b .L.param_copy.%d
                        asm_fixup_add(cg_sec, cj2, format(".L.param_copy.%d", c), 0);
                        cg_def_label(format(".L.param_copy_end.%d", c));
                    } else {
                        int sf = var->ty->size <= 4 ? 2 : 3;
                        asm_stur_phy(cg_sec, gp_param, ARM64_X29, sf, -var->offset); // stur x/w{gp_param}, [x29, #-offset]
                    }
                    gp_param++;
                } else {
                    // Stack argument — load from caller's frame using correct width
                    int spoff = 16 + stack_param * 8;
                    if (is_flonum(var->ty)) {
                        if (var->ty->size == 4) {
                            asm_ldr_s0_fp_off(cg_sec, spoff); // ldr s0, [x29, #spoff]
                            asm_str_s0_fp_neg(cg_sec, var->offset); // str s0, [x29, #-offset]
                        } else {
                            asm_ldr_d0_fp_off(cg_sec, spoff); // ldr d0, [x29, #spoff]
                            asm_str_d_fp_neg(cg_sec, 0, var->offset); // str d0, [x29, #-offset]
                        }
                    } else if (var->ty->size <= 1) {
                        asm_ldrb_w11_fp_off(cg_sec, spoff); // ldrb w11, [x29, #spoff]
                        asm_strb_w11_fp_neg(cg_sec, var->offset); // strb w11, [x29, #-offset]
                    } else if (var->ty->size <= 2) {
                        asm_ldrh_w11_fp_off(cg_sec, spoff); // ldrh w11, [x29, #spoff]
                        asm_strh_w11_fp_neg(cg_sec, var->offset); // strh w11, [x29, #-offset]
                    } else if (var->ty->size <= 4) {
                        asm_ldr_w11_fp_off(cg_sec, spoff); // ldr w11, [x29, #spoff]
                        asm_str_w11_fp_neg(cg_sec, var->offset); // str w11, [x29, #-offset]
                    } else {
                        asm_ldr_x11_fp_off(cg_sec, spoff); // ldr x11, [x29, #spoff]
                        asm_str_x11_fp_neg(cg_sec, var->offset); // str x11, [x29, #-offset]
                    }
                    stack_param++;
                }
            }
        }


        // Re-run gen() to emit binary body
        {
            uint64_t _t0 = opt_time ? cg_now_us() : 0;
            for (Node *n = fn->body; n; n = n->next) {
                asm_peep_node_start(cg_sec); // .L.return.%s:
                VReg r = gen(n);
                asm_peep_node_end(cg_sec); // .L.return.%s:
                if (r != -1) free_reg(r);
            }
            if (opt_time)
                time_peep_us += cg_now_us() - _t0;
        }

        // === ARM64 epilogue ===
        cg_def_label(format(".L.return.%s", fn->name));

        // Cleanup calls
        bool has_cleanup = false;
        for (LVar *var = fn->locals; var; var = var->next)
            if (var_has_cleanup(var)) {
                has_cleanup = true;
                break;
            }
        if (has_cleanup && fn_ret_nonvoid) {
            // Save x0 (return value) to x19 (callee-saved) before cleanup calls.
            // x19 is guaranteed saved/restored by the prologue (see ever_used_regs |= above).
            asm_mov_phy_phy(cg_sec, ARM64_X19, ARM64_X0, 1); // mov x19, x0 (save return value)
        }
        for (LVar *var = fn->locals; var; var = var->next)
            if (var_has_cleanup(var))
                emit_cleanup_var(var);
        if (has_cleanup && fn_ret_nonvoid) {
            // Restore x0 from x19 after cleanup calls
            asm_mov_phy_phy(cg_sec, ARM64_X0, ARM64_X19, 1); // mov x0, x19 (restore return value)
        }

        // VLA or alloca may have moved sp; restore to fixed frame position
        // before reading callee-saved regs (stored at sp+16..sp+n at frame entry)
        if (fn->dealloc_vla || fn_uses_alloca) {
            if (frame_size <= 4095)
                arm64_sub_imm(cg_sec, 1, ARM64_SP, ARM64_X29, frame_size, 0); // sub sp, x29, #frame_size
            else {
                emit_mov_imm64(ARM64_X16, (uint64_t)frame_size); // mov x16, #frame_size
                arm64_sub_extreg(cg_sec, 1, ARM64_SP, ARM64_X29, ARM64_X16, ARM64_UXTX, 0); // sub sp, x29, x16
            }
        }

        // Restore callee-saved
        cs_off = 16;
        for (int j = 0; j < 6; j++) {
            if (callee_mask & (1 << j)) {
                arm64_ldr_uoff(cg_sec, 3, (Arm64Reg)(ARM64_X19 + j), ARM64_SP, cs_off / 8); // ldr x{cs}, [sp, #cs_off]
                cs_off += 8;
            }
        }

        if (frame_size <= 4095)
            arm64_add_imm(cg_sec, 1, ARM64_SP, ARM64_SP, frame_size, 0); // add sp, sp, #frame_size
        else {
            emit_mov_imm64(ARM64_X16, (uint64_t)frame_size); // mov x16, #frame_size
            arm64_add_extreg(cg_sec, 1, ARM64_SP, ARM64_SP, ARM64_X16, ARM64_UXTX, 0); // add sp, sp, x16
        }
        asm_ldp_fp_lr(cg_sec); // ldp x29, x30, [sp], #16
        asm_ret(cg_sec); // ret

#else
        // === x86_64 prologue ===
        // Determine which callee-saved regs need saving.
        // Windows: rbx,r12,r13,r14,r15,rsi (indices 2-7)
        // Linux SysV: rbx,r12,r13,r14,r15 only (rsi is caller-saved)
#ifdef _WIN32
        int callee_count = 6;
#else
        int callee_count = 5;
#endif
        int callee_mask = (ever_used_regs >> 2) & ((1 << callee_count) - 1);
        int n_pushes = 0;
        for (int j = 0; j < callee_count; j++)
            if (callee_mask & (1 << j)) n_pushes++;

        // Calculate stack frame size
        // Total space below rbp must cover: locals + spills + shadow (32)
        int need = fn->stack_size + fn_struct_ret_total + 32;
        if (fn->is_variadic)
            need = va_reg_save_ofs;
        // Reserve space for register spill slots
        if (need < next_spill_slot)
            need = next_spill_slot;
        int push_bytes = n_pushes * 8;
        int sub_amount = need - push_bytes;
        if (sub_amount < 32) sub_amount = 32;
        // Fix 16-byte alignment
        if ((push_bytes + sub_amount) % 16 != 0) sub_amount += 8;

        // Emit prologue - handle is_weak, inline, and static linkage.
        // For inline functions, check:
        // 1. If there's a non-inline declaration (has_init) in globals
        // 2. If prior non-weak declaration had extern (LVar is_extern=true)
        bool has_noninline_decl = false;
        bool had_extern_decl = false;
        if (fn->is_inline && !fn->is_extern) {
            for (LVar *g = prog->globals; g; g = g->next) {
                if (g->is_function && g->name == fn->name) {
                    if (g->has_init)
                        has_noninline_decl = true;
                    // Only consider non-weak extern declarations
                    if (g->is_extern && !g->is_weak)
                        had_extern_decl = true;
                    break;
                }
            }
        }
        char *fn_label = fn->name;
        if (is_asm_reserved(fn->name))
            fn_label = format(".L_rcc_%s", fn->name);
        bool fn_exported = !fn->is_static && (!fn->is_inline || fn->is_extern || has_noninline_decl || had_extern_decl);
        if (fn->is_weak) {
            cg_weak_label(asm_sym_name(sym_name(fn->name))); // .weak_definition %s
        } else if (fn_exported) {
            cg_global_label(asm_sym_name(sym_name(fn->name))); // .globl %s
        } else {
            cg_def_label(asm_sym_name(sym_name(fn_label))); // .weak %s
        }
#ifdef _WIN32
        uw_begin(); // .seh_proc
#endif
        asm_push(cg_sec, X86_RBP); // push rX86_RBP
#ifdef _WIN32
        uw_pushreg(X86_RBP); // .seh_pushreg %rbp
#endif
        asm_mov_rsp_rbp(cg_sec); // movq %rsp, %rbp

        // Only push callee-saved registers that were actually used
        for (int j = 0; j < callee_count; j++) {
            if (callee_mask & (1 << j)) {
                asm_push(cg_sec, REG(j + 2)); // push rREG(j + 2)
#ifdef _WIN32
                uw_pushreg(REG(j + 2)); // .seh_pushreg %s
#endif
            }
        }
#ifdef _WIN32
        // Windows requires probing the stack one page at a time when growing
        // it by more than a page: a single large `sub %rsp` can jump over the
        // guard page straight into unmapped memory. mingw-w64 and MSVC call
        // ___chkstk_ms to touch each page in turn.
        if (sub_amount >= 4096) {
            x86_mov_ri(cg_sec, 4, X86_RAX, sub_amount); // movl $sub_amount, %eax
            emit_direct_call("___chkstk_ms"); // call ___chkstk_ms
            x86_sub_rr(cg_sec, 8, X86_RSP, X86_RAX); // subq %rax, %rsp
        } else
#endif
            asm_sub_rsp_imm(cg_sec, sub_amount); // subq $sub_amount, %rsp
#ifdef _WIN32
        uw_stackalloc(sub_amount); // .seh_stackalloc sub_amount
        uw_endprologue(); // .seh_endprologue
#endif

        // Save variadic argument registers to the reg_save_area
        // (must happen before param saves, which may clobber xmm0 via cvtsd2ss)
        if (fn->is_variadic) {
#ifdef _WIN32
            // Windows x64 ABI: save rcx, rdx, r8, r9 and xmm0-xmm3
            int gp_count = va_gp_start / 8;
            switch (gp_count) {
            case 0: asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, va_reg_save_ofs); /* fallthrough */ /* movq %rcx, -%d(%rbp) */
            case 1: asm_mov_phyreg_rbp(cg_sec, X86_RDX, 8, va_reg_save_ofs - 8); /* fallthrough */ /* movq %rdx, -%d(%rbp) */
            case 2: asm_mov_phyreg_rbp(cg_sec, X86_R8, 8, va_reg_save_ofs - 16); /* fallthrough */ /* movq %r8, -%d(%rbp) */
            case 3: asm_mov_phyreg_rbp(cg_sec, X86_R9, 8, va_reg_save_ofs - 24); /* movq %r9, -%d(%rbp) */
            }
            asm_movaps_rbp_xmm(cg_sec, 0, va_reg_save_ofs - 32); /* movaps %xmm0, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 1, va_reg_save_ofs - 48); /* movaps %xmm1, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 2, va_reg_save_ofs - 64); /* movaps %xmm2, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 3, va_reg_save_ofs - 80); /* movaps %xmm3, -%d(%rbp) */
#else
            // Linux: save all 6 GP registers: rdi, rsi, rdx, rcx, r8, r9
            asm_mov_phyreg_rbp(cg_sec, X86_RDI, 8, va_reg_save_ofs); /* movq %rdi, -%d(%rbp) */
            asm_mov_phyreg_rbp(cg_sec, X86_RSI, 8, va_reg_save_ofs - 8); /* movq %rsi, -%d(%rbp) */
            asm_mov_phyreg_rbp(cg_sec, X86_RDX, 8, va_reg_save_ofs - 16); /* movq %rdx, -%d(%rbp) */
            asm_mov_phyreg_rbp(cg_sec, X86_RCX, 8, va_reg_save_ofs - 24); /* movq %rcx, -%d(%rbp) */
            asm_mov_phyreg_rbp(cg_sec, X86_R8, 8, va_reg_save_ofs - 32); /* movq %r8, -%d(%rbp) */
            asm_mov_phyreg_rbp(cg_sec, X86_R9, 8, va_reg_save_ofs - 40); /* movq %r9, -%d(%rbp) */
            // Save all 8 XMM registers: xmm0-xmm7
            asm_movaps_rbp_xmm(cg_sec, 0, va_reg_save_ofs - 48); /* movaps %xmm0, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 1, va_reg_save_ofs - 64); /* movaps %xmm1, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 2, va_reg_save_ofs - 80); /* movaps %xmm2, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 3, va_reg_save_ofs - 96); /* movaps %xmm3, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 4, va_reg_save_ofs - 112); /* movaps %xmm4, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 5, va_reg_save_ofs - 128); /* movaps %xmm5, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 6, va_reg_save_ofs - 144); /* movaps %xmm6, -%d(%rbp) */
            asm_movaps_rbp_xmm(cg_sec, 7, va_reg_save_ofs - 160); /* movaps %xmm7, -%d(%rbp) */
#endif
        }

        // Save incoming params from ABI regs to stack slots
        {
#ifdef _WIN32
            X86Reg greg[] = {X86_RCX, X86_RDX, X86_R8, X86_R9};
            int max_gp = 4;
#else
            X86Reg greg[] = {X86_RDI, X86_RSI, X86_RDX, X86_RCX, X86_R8, X86_R9};
            int max_gp = 6;
#endif
            int gp = fn->ty->return_ty && (fn->ty->return_ty->kind == TY_STRUCT || fn->ty->return_ty->kind == TY_UNION || (fn->ty->return_ty->kind == TY_COMPLEX
#ifdef _WIN32
                                                                                                                           && fn->ty->return_ty->size > 8
#endif
                                                                                                                           ))
                ? 1
                : 0;
            int xfp = 0;
            int stack_param_index = 0;
            for (LVar *var = fn->params; var; var = var->param_next) {
#ifndef _WIN32
                // Linux SysV: _Complex float/double are passed in SSE regs,
                // _Complex integer types in GP regs, matching the call-site
                // classification in gen_funcall.
                if (is_complex(var->ty)) {
                    bool cfloat = var->ty->base && is_flonum(var->ty->base);
                    if (cfloat) {
                        int base_sz = var->ty->base ? var->ty->base->size : 8;
                        if (var->ty->size <= 8) {
                            // _Complex float (or other <=8-byte floating complex):
                            // packed in one xmm reg; store the whole 8 bytes.
                            if (xfp < 8) {
                                x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -var->offset), (X86XmmReg)xfp);
                                xfp++;
                            } else {
                                int stack_off2 = 16 + stack_param_index * 8;
                                x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(X86_RBP, stack_off2));
                                x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -var->offset), X86_XMM0);
                                stack_param_index++;
                            }
                        } else {
                            // _Complex double: real in xmm{xfp}, imag in xmm{xfp+1}
                            if (xfp + 1 < 8) {
                                x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -var->offset), (X86XmmReg)xfp);
                                x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -(var->offset - base_sz)), (X86XmmReg)(xfp + 1));
                                xfp += 2;
                            } else {
                                int stack_off2 = 16 + stack_param_index * 8;
                                if (stack_param_index & 1) stack_param_index++;
                                stack_off2 = 16 + stack_param_index * 8;
                                x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(X86_RBP, stack_off2));
                                x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -var->offset), X86_XMM0);
                                x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(X86_RBP, stack_off2 + base_sz));
                                x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -(var->offset - base_sz)), X86_XMM0);
                                stack_param_index += 2;
                            }
                        }
                    } else {
                        // Integer _Complex: one or two GP regs
                        if (var->ty->size <= 8) {
                            if (gp < max_gp) {
                                x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), greg[gp]);
                                gp++;
                            } else {
                                int stack_off2 = 16 + stack_param_index * 8;
                                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off2));
                                x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX);
                                stack_param_index++;
                            }
                        } else {
                            if (gp + 1 < max_gp) {
                                int base_sz = var->ty->base ? var->ty->base->size : 8;
                                x86_mov_mr(cg_sec, base_sz <= 4 ? 4 : 8, x86_mem(X86_RBP, -var->offset), greg[gp]);
                                x86_mov_mr(cg_sec, base_sz <= 4 ? 4 : 8, x86_mem(X86_RBP, -(var->offset - base_sz)), greg[gp + 1]);
                                gp += 2;
                            } else {
                                int stack_off2 = 16 + stack_param_index * 8;
                                if (stack_param_index & 1) stack_param_index++;
                                stack_off2 = 16 + stack_param_index * 8;
                                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off2));
                                x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX);
                                x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off2 + 8));
                                x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), X86_RAX);
                                stack_param_index += 2;
                            }
                        }
                    }
                    continue;
                }
                // Linux SysV: __int128 occupies two consecutive GP registers/stack slots
                if (var->ty->kind == TY_INT128) {
                    if (gp + 1 < max_gp) {
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), greg[gp]); // movq greg[gp], -off(%rbp)
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), greg[gp + 1]); // movq greg[gp+1], -(off-8)(%rbp)
                        gp += 2;
                    } else {
                        if (stack_param_index & 1) stack_param_index++;
                        int stack_off2 = 16 + stack_param_index * 8;
                        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off2)); // movq stack_off(%rbp), %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX); // movq %rax, -off(%rbp)
                        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off2 + 8)); // movq stack_off+8(%rbp), %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), X86_RAX); // movq %rax, -(off-8)(%rbp)
                        stack_param_index += 2;
                    }
                    continue;
                }
#endif
#ifdef _WIN32
                // Win64: __int128 is passed by pointer (like large struct)
                if (var->ty->kind == TY_INT128) {
                    if (gp < max_gp) {
                        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(greg[gp], 0)); // movq (greg[gp]), %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RAX); // movq %rax, -off(%rbp)
                        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(greg[gp], 8)); // movq 8(greg[gp]), %rax
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), X86_RAX); // movq %rax, -(off-8)(%rbp)
                        gp++;
                    } else {
                        int stack_off2 = 48 + stack_param_index * 8;
                        x86_mov_rm(cg_sec, 8, X86_RAX, x86_mem(X86_RBP, stack_off2)); // movq stack_off(%rbp), %rax
                        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RAX, 0)); // movq (%rax), %rcx (lo)
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -var->offset), X86_RCX); // movq %rcx, -off(%rbp)
                        x86_mov_rm(cg_sec, 8, X86_RCX, x86_mem(X86_RAX, 8)); // movq 8(%rax), %rcx (hi)
                        x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -(var->offset - 8)), X86_RCX); // movq %rcx, -(off-8)(%rbp)
                        stack_param_index++;
                    }
                    continue;
                }
                // Win64: _Complex > 8 bytes passed by pointer like large struct
                bool is_large_complex = is_complex(var->ty) && var->ty->size > 8;
                if (is_large_complex && gp < max_gp) {
                    // Complex > 8 in GP reg: copy pointee to local slot
                    int c = ++rcc_label_count;
                    x86_mov_rr(cg_sec, 8, X86_R11, greg[gp]); // movq greg[gp], %r11
                    x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size); // movq $size, %r10
                    cg_def_label(format(".L.param2.%d", c));
                    x86_cmp_ri(cg_sec, 8, X86_R10, 0);
                    size_t js = cg_sec->len;
                    x86_jcc_rel32(cg_sec, X86_E, 0);
                    asm_fixup_add(cg_sec, js, format(".L.param2_end.%d", c), 1);
                    asm_movb_r11_r10_al(cg_sec, -1);
                    asm_movb_al_rbp_r10(cg_sec, var->offset);
                    x86_sub_ri(cg_sec, 8, X86_R10, 1);
                    size_t jp = cg_sec->len;
                    x86_jmp_rel32(cg_sec, 0);
                    asm_fixup_add(cg_sec, jp, format(".L.param2.%d", c), 0);
                    cg_def_label(format(".L.param2_end.%d", c));
                    gp++;
                    continue;
                }
#endif
                if (!is_flonum(var->ty) && gp < max_gp && !((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION || is_complex(var->ty)) && var->ty->size > 8)) {
                    int sz = var->ty->size <= 4 ? 4 : 8;
                    x86_mov_mr(cg_sec, sz, x86_mem(X86_RBP, -var->offset), greg[gp]); // %s:
                    gp++;
                } else if (is_flonum(var->ty) &&
#ifdef _WIN32
                           gp < max_gp // Win64: float and int share the same 4-reg slots
#else
                           xfp < 8
#endif
                ) {
                    // Save float/double param from xmm register to stack slot
#ifdef _WIN32
                    int xmm_idx = gp; // Win64: float uses slot gp (combined counter)
#else
                    int xmm_idx = xfp;
#endif
                    if (var->ty->size <= 4) {
                        x86_cvtsd2ss(cg_sec, X86_XMM0, (X86XmmReg)xmm_idx); // cvtsd2ss %xmm{n}, %xmm0
                        x86_movss_mr(cg_sec, x86_mem(X86_RBP, -var->offset), X86_XMM0); // movss %xmm0, -off(%rbp)
                    } else {
                        x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -var->offset), (X86XmmReg)xmm_idx); // movsd %xmm{n}, -off(%rbp)
                    }
#ifdef _WIN32
                    gp++; // Win64: advance combined position counter
#endif
                    xfp++;
                } else if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION || is_complex(var->ty)) && var->ty->size > 8 && gp < max_gp) {
                    int c = ++rcc_label_count;
                    x86_mov_rr(cg_sec, 8, X86_R11, greg[gp]);
                    x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size);
                    cg_def_label(format(".L.param2.%d", c));
                    x86_cmp_ri(cg_sec, 8, X86_R10, 0);
                    size_t js = cg_sec->len;
                    x86_jcc_rel32(cg_sec, X86_E, 0);
                    asm_fixup_add(cg_sec, js, format(".L.param2_end.%d", c), 1);
                    asm_movb_r11_r10_al(cg_sec, -1);
                    asm_movb_al_rbp_r10(cg_sec, var->offset);
                    x86_sub_ri(cg_sec, 8, X86_R10, 1);
                    size_t jp = cg_sec->len;
                    x86_jmp_rel32(cg_sec, 0);
                    asm_fixup_add(cg_sec, jp, format(".L.param2.%d", c), 0);
                    cg_def_label(format(".L.param2_end.%d", c));
                    gp++;
                } else {
                    // Stack argument — passed above return address
#ifdef _WIN32
                    int stack_off2 = 48 + stack_param_index * 8;
#else
                    int stack_off2 = 16 + stack_param_index * 8;
#endif
                    if (is_flonum(var->ty)) {
                        if (var->ty->size <= 4) {
                            x86_movss_rm(cg_sec, X86_XMM0, x86_mem(X86_RBP, stack_off2)); // movss stack_off2(%rbp), xmm0
                            x86_movss_mr(cg_sec, x86_mem(X86_RBP, -var->offset), X86_XMM0); // movss xmm0, -var->offset(%rbp)
                        } else {
                            x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(X86_RBP, stack_off2)); // movsd stack_off2(%rbp), xmm0
                            x86_movsd_mr(cg_sec, x86_mem(X86_RBP, -var->offset), X86_XMM0); // movsd xmm0, -var->offset(%rbp)
                        }
                    } else if ((var->ty->kind == TY_STRUCT || var->ty->kind == TY_UNION || is_complex(var->ty)) && var->ty->size > 8) {
                        // Structs > 8 bytes (and 16-byte _Complex double) are passed
                        // by pointer even on the stack; load the pointer and copy
                        // the pointee into the local slot.
                        int c = ++rcc_label_count;
                        x86_mov_rm(cg_sec, 8, X86_R11, x86_mem(X86_RBP, stack_off2)); // movq stack_off2(%rbp), %r11
                        x86_mov_ri(cg_sec, 8, X86_R10, var->ty->size);
                        cg_def_label(format(".L.param2.%d", c));
                        x86_cmp_ri(cg_sec, 8, X86_R10, 0);
                        size_t js2 = cg_sec->len;
                        x86_jcc_rel32(cg_sec, X86_E, 0);
                        asm_fixup_add(cg_sec, js2, format(".L.param2_end.%d", c), 1);
                        asm_movb_r11_r10_al(cg_sec, -1); // movb -1(%r11,%r10), %%al
                        asm_movb_al_rbp_r10(cg_sec, var->offset);
                        x86_sub_ri(cg_sec, 8, X86_R10, 1);
                        size_t jp2 = cg_sec->len;
                        x86_jmp_rel32(cg_sec, 0);
                        asm_fixup_add(cg_sec, jp2, format(".L.param2.%d", c), 0);
                        cg_def_label(format(".L.param2_end.%d", c));
                    } else {
                        int psz = var->ty->size <= 4 ? 4 : 8;
                        x86_mov_rm(cg_sec, psz, X86_RAX, x86_mem(X86_RBP, stack_off2)); // mov stack_off2(%rbp), %rax
                        x86_mov_mr(cg_sec, psz, x86_mem(X86_RBP, -var->offset), X86_RAX); // mov %rax, -var->offset(%rbp)
                    }
                    stack_param_index++;
                }
            }
#ifdef _WIN32
            // Fix va_st_start using correct stack_param_index from pass 2
            va_st_start = 48 + stack_param_index * 8;
#endif
        }

        if (fn->ty->return_ty && (fn->ty->return_ty->kind == TY_STRUCT || fn->ty->return_ty->kind == TY_UNION || (fn->ty->return_ty->kind == TY_COMPLEX
#ifdef _WIN32
                                                                                                                  && fn->ty->return_ty->size > 8
#endif
                                                                                                                  ))) {
            int retbuf_offset = 0;
            for (LVar *var = fn->locals; var; var = var->next) {
                if (var->name && var->name == kw_retbuf) {
                    retbuf_offset = var->offset;
                    break;
                }
            }
            x86_mov_mr(cg_sec, 8, x86_mem(X86_RBP, -retbuf_offset),
#ifdef _WIN32
                       X86_RCX
#else
                       X86_RDI
#endif
            ); // mov retreg, -retbuf_offset(%rbp)
        }

        // Re-run gen() to emit binary body
        {
            uint64_t _t0 = opt_time ? cg_now_us() : 0;
            for (Node *n = fn->body; n; n = n->next) {
                asm_peep_node_start(cg_sec); // mov %s, -%d(%rbp)
                VReg r = gen(n);
                asm_peep_node_end(cg_sec); // .L.return.%s:
                if (r != -1) free_reg(r);
            }
            if (opt_time)
                time_peep_us += cg_now_us() - _t0;
        }

        // Emit epilogue
        cg_def_label(format(".L.return.%s", fn->name)); // mov %s, -%d(%rbp)

        // Emit __cleanup__ calls (LIFO: locals list is in reverse declaration order)
        bool has_cleanup = false;
        for (LVar *var = fn->locals; var; var = var->next)
            if (var_has_cleanup(var)) {
                has_cleanup = true;
                break;
            }
        if (has_cleanup)
            asm_mov_phyreg_rbp(cg_sec, X86_RAX, 8, spill_offset(0)); // mov [rbp-spill_offset(0], X86_RAX)
        for (LVar *var = fn->locals; var; var = var->next) {
            if (var_has_cleanup(var))
                emit_cleanup_var(var);
        }
        if (has_cleanup)
            asm_mov_rbp(cg_sec, X86_RAX, 8, spill_offset(0)); // mov [rbp-spill_offset(0], RAX)
        if (fn->dealloc_vla)
            asm_lea_rbp(cg_sec, X86_RSP, 8, n_pushes * 8); // lea [rbp-8], %rsp
        else if (fn_uses_alloca)
            asm_lea_rbp(cg_sec, X86_RSP, 8, n_pushes * 8); // lea [rbp-8], %rsp
        else
            asm_add_rsp_imm(cg_sec, sub_amount); // addq $sub_amount, %rsp
        for (int j = callee_count - 1; j >= 0; j--) {
            if (callee_mask & (1 << j))
                asm_pop(cg_sec, REG(j + 2)); // pop rREG(j + 2)
        }
        asm_pop(cg_sec, X86_RBP); // popq %rbp
        asm_ret(cg_sec); // ret
#ifdef _WIN32
        uw_endproc(); // .seh_endproc
#endif
#endif
    }
    // Emit constructor/destructor entries
    bool has_ctor = false, has_dtor = false;
    for (TLItem *item = prog->items; item; item = item->next) {
        if (item->kind == TL_FUNC) {
            if (item->fn->is_constructor) has_ctor = true;
            if (item->fn->is_destructor) has_dtor = true;
        }
    }
    if (has_dtor) {
#if defined(__APPLE__)
        // macOS: emit destructor stubs that call __cxa_atexit at startup
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->is_destructor) {
                cg_set_section(SEC_TEXT);
                cg_def_label(format("___GLOBAL_dtor_%s", item->fn->name));
                asm_stp_fp_lr(cg_sec);
                asm_mov_fp_sp(cg_sec);
                { // adrp x0, dtor_label@PAGE; add x0, x0, dtor_label@PAGEOFF
                    const char *lbl = asm_sym_name(sym_name(item->fn->name));
                    int sidx = objfile_find_sym(cg_obj, lbl);
                    if (sidx < 0) sidx = objfile_add_sym(cg_obj, lbl, SEC_UNDEF, 0, 0, SB_LOCAL, ST_NOTYPE);
                    size_t a_off = cg_sec->len;
                    asm_adrp(cg_sec, ARM64_X0);
                    objfile_add_reloc(cg_obj, SEC_TEXT, a_off, sidx, R_AARCH64_ADR_PREL_PG_HI21, 0);
                    size_t d_off = cg_sec->len;
                    asm_add_rd_rd_0(cg_sec, ARM64_X0);
                    objfile_add_reloc(cg_obj, SEC_TEXT, d_off, sidx, R_AARCH64_ADD_ABS_LO12_NC, 0);
                }
                asm_mov_imm_phy(cg_sec, ARM64_X1, 8, 0);
                { // adrp x2, ___dso_handle@PAGE; add x2, x2, ___dso_handle@PAGEOFF
                    const char *lbl = "___dso_handle";
                    int sidx = objfile_find_sym(cg_obj, lbl);
                    if (sidx < 0) sidx = objfile_add_sym(cg_obj, lbl, SEC_UNDEF, 0, 0, SB_LOCAL, ST_NOTYPE);
                    size_t a_off = cg_sec->len;
                    asm_adrp(cg_sec, ARM64_X2);
                    objfile_add_reloc(cg_obj, SEC_TEXT, a_off, sidx, R_AARCH64_ADR_PREL_PG_HI21, 0);
                    size_t d_off = cg_sec->len;
                    asm_add_rd_rd_0(cg_sec, ARM64_X2);
                    objfile_add_reloc(cg_obj, SEC_TEXT, d_off, sidx, R_AARCH64_ADD_ABS_LO12_NC, 0);
                }
                size_t call_off = asm_call_label(cg_sec);
                int cxa_sidx = objfile_find_sym(cg_obj, "___cxa_atexit");
                if (cxa_sidx < 0)
                    cxa_sidx = objfile_add_sym(cg_obj, "___cxa_atexit", SEC_UNDEF, 0, 0, SB_GLOBAL, ST_FUNC);
                objfile_add_reloc(cg_obj, SEC_TEXT, call_off, cxa_sidx, R_AARCH64_CALL26, 0);
                asm_ldp_fp_lr(cg_sec);
                asm_ret(cg_sec);
            }
        }
#endif
    }
#if defined(__APPLE__)
    if (has_ctor || has_dtor) {
        cg_set_section(SEC_INIT_ARRAY);
        secbuf_align(cg_sec, 8);
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->is_constructor) {
                size_t off = cg_sec->len;
                secbuf_emit64le(cg_sec, 0); // .quad constructor
                int sidx = objfile_find_sym(cg_obj, asm_sym_name(sym_name(item->fn->name)));
                if (sidx < 0)
                    sidx = objfile_add_sym(cg_obj, asm_sym_name(sym_name(item->fn->name)), SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
                objfile_add_reloc(cg_obj, SEC_INIT_ARRAY, off, sidx, R_AARCH64_ABS64, 0);
            }
        }
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->is_destructor) {
                size_t off = cg_sec->len;
                secbuf_emit64le(cg_sec, 0);
                const char *stub = format("___GLOBAL_dtor_%s", item->fn->name);
                int sidx = objfile_find_sym(cg_obj, stub);
                if (sidx < 0)
                    sidx = objfile_add_sym(cg_obj, stub, SEC_UNDEF, 0, 0, SB_LOCAL, ST_NOTYPE);
                objfile_add_reloc(cg_obj, SEC_INIT_ARRAY, off, sidx, R_AARCH64_ABS64, 0);
            }
        }
    }
#else
    if (has_ctor) {
        cg_set_section(SEC_INIT_ARRAY);
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->is_constructor) {
                size_t off = cg_sec->len;
                secbuf_emit64le(cg_sec, 0); // .quad %s
                int sidx = objfile_find_sym(cg_obj, asm_sym_name(sym_name(item->fn->name)));
                if (sidx < 0)
                    sidx = objfile_add_sym(cg_obj, asm_sym_name(sym_name(item->fn->name)), SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
#ifdef ARCH_ARM64
                objfile_add_reloc(cg_obj, SEC_INIT_ARRAY, off, sidx, R_AARCH64_ABS64, 0);
#else
                objfile_add_reloc(cg_obj, SEC_INIT_ARRAY, off, sidx, R_X86_64_64, 0);
#endif
            }
        }
#if defined(__APPLE__)
        // Register destructor stubs in __mod_init_func too
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->is_destructor)
                (void)0 /* directive: .quad */;
        }
#endif
    }
#if !defined(__APPLE__)
    if (has_dtor) {
        cg_set_section(SEC_FINI_ARRAY);
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->is_destructor) {
                size_t off = cg_sec->len;
                secbuf_emit64le(cg_sec, 0); // .quad %s
                int sidx = objfile_find_sym(cg_obj, asm_sym_name(sym_name(item->fn->name)));
                if (sidx < 0)
                    sidx = objfile_add_sym(cg_obj, asm_sym_name(sym_name(item->fn->name)), SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
#ifdef ARCH_ARM64
                objfile_add_reloc(cg_obj, SEC_FINI_ARRAY, off, sidx, R_AARCH64_ABS64, 0);
#else
                objfile_add_reloc(cg_obj, SEC_FINI_ARRAY, off, sidx, R_X86_64_64, 0);
#endif
            }
        }
    }
#endif
#endif
    if (alloca_needed)
        emit_alloca();

    // Emit float literal constants after all functions
    if (float_lits) {
        cg_set_section(SEC_RODATA);
        (void)0 /* directive: .balign */;
        for (FloatLit *fl = float_lits; fl; fl = fl->next) {
            cg_def_label_sec(format(".LF%d", fl->id), SEC_RODATA);
            if (fl->size == 4) {
                float f = (float)fl->val;
                uint32_t bits;
                memcpy(&bits, &f, 4);
                secbuf_emit32le(cg_sec, bits);
            } else {
                uint64_t bits;
                memcpy(&bits, &fl->val, 8);
                secbuf_emit64le(cg_sec, bits);
            }
        }
    }
    // Resolve pending aliases after all symbols are emitted
    for (LVar *var = prog->globals; var; var = var->next) {
        if (!var->alias_target) continue;
        const char *alias_name = asm_sym_name(sym_name(var->name));
        const char *target_name = asm_sym_name(sym_name(var->alias_target));
        int tidx = objfile_find_sym(cg_obj, target_name);
        if (tidx >= 0 && cg_obj->syms[tidx].section != SEC_UNDEF) {
            // Update existing UNDEF alias symbol to match target
            int aidx = objfile_find_sym(cg_obj, alias_name);
            if (aidx >= 0 && cg_obj->syms[aidx].section == SEC_UNDEF) {
                cg_obj->syms[aidx].section = cg_obj->syms[tidx].section;
                cg_obj->syms[aidx].offset = cg_obj->syms[tidx].offset;
                cg_obj->syms[aidx].size = cg_obj->syms[tidx].size;
                cg_obj->syms[aidx].bind = cg_obj->syms[tidx].bind;
                cg_obj->syms[aidx].type = cg_obj->syms[tidx].type;
            } else if (aidx < 0) {
                objfile_add_sym(cg_obj, alias_name,
                                cg_obj->syms[tidx].section,
                                cg_obj->syms[tidx].offset,
                                cg_obj->syms[tidx].size,
                                cg_obj->syms[tidx].bind,
                                cg_obj->syms[tidx].type);
            }
        }
    }
    // Flush DWARF debug line info
    if (objfile_has_debug(cg_obj))
        objfile_flush_debug_line(cg_obj, cg_obj->text.len);
    return cg_obj;
}
