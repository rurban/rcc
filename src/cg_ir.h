// cg_ir.h -- the parts of codegen.c the IR backend (ir_x86.c) needs.
// All are thin wrappers around codegen.c statics; x86-64 only.
#ifndef RCC_CG_IR_H
#define RCC_CG_IR_H

#include "ir.h"
#include "codegen_asm.h" // cg_sec, SecBuf, asm_* helpers

// Start emitting `fn`: resets per-function label/fixup/branch-site tables,
// defines the function symbol (global/weak/local, visibility) at the current
// .text offset and returns that offset. *sym_out = final symbol name.
size_t cg_ir_begin_fn(Program *prog, Function *fn, bool emitting_local_copy, const char **sym_out);
// Finish: patch the symbol size, run -O1 jump threading over the branch sites.
void cg_ir_end_fn(const char *sym, size_t start_off);

// Conditional / unconditional jump to a label defined (possibly later) with
// cg_ir_label(); backpatched through the asm fixup table. `cond` is an X86Cond.
size_t cg_ir_jcc(int cond, const char *label);
void cg_ir_jmp(const char *label);
// Define a local label at the current offset (resolves pending fixups).
void cg_ir_label(const char *name);
// call rel32 to a symbol (adds the relocation / resolves same-TU labels).
void cg_ir_call(const char *name);
// Assembler-level symbol for a global/function LVar (use for IR_GADDR.sym).
const char *cg_ir_var_sym(LVar *v);
// Name of an AST node kind (a NodeKind value), for eligibility diagnostics.
const char *cg_ir_node_kind_name(int kind);
// How IR_GADDR must reach a global/function LVar: -1 unsupported (register
// variable, TLS, alias), 0 plain rip-relative lea, 1 GOT load (set
// IRInstr.msign = 1). Declares weak symbols as a side effect.
int cg_ir_global_access(LVar *v);
#ifdef _WIN32
// Win64 SEH unwind records (.seh_pushreg / .seh_setframe / .seh_stackalloc / .seh_endprologue).
// cg_ir_begin_fn() opens the function's unwind entry and cg_ir_end_fn() closes it; a function
// that never calls these (frameless leaf) gets an entry with no codes. `reg` is an X86Reg.
void cg_ir_uw_pushreg(int reg);
void cg_ir_uw_setframe(int reg);
void cg_ir_uw_stackalloc(int size);
void cg_ir_uw_endprologue(void);
#endif
#ifdef ARCH_ARM64
// ARM64: x<rd> = address of symbol `label` (ADRP + ADD, or the GOT slot when `got` is set /
// on Darwin), with the same relocations the AST backend emits.
void cg_ir_sym_addr(int rd, const char *label, int got);
// ARM64: cbz/cbnz (sf = 1: x register) to a label, backpatched like cg_ir_jcc().
void cg_ir_cbz(int sf, int reg, bool nonzero, const char *label);
// ARM64: tbz/tbnz to a label; bit is 0..63.
void cg_ir_tbz(int reg, int bit, bool nonzero, const char *label);
// ARM64: symbol index of an assembler-level label (created on first use), and whether the label
// is defined in this object and non-weak (reachable without the GOT).
int cg_ir_sym_index(const char *label);
bool cg_ir_sym_direct(const char *label);
// True when a direct page-offset ld/st relocation at label+addend is naturally aligned.
bool cg_ir_sym_ldst_direct(const char *label, int64_t addend, int size);
#endif
// Emit `lea rip-relative` of a symbol/label into x86 register `r` (X86Reg).
// (asm_lea_rip_reg from codegen_asm.h is already exported.)

#endif
