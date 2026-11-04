// ir.h -- SSA IR for the -O2 backend (x86-64 SysV first).
//
// Pipeline, per eligible function (see TODO "-O2 performance"):
//   ir_build()   AST -> IR, SSA built on the fly (Braun et al.), scalar locals
//                whose address is never taken become SSA values (ir_build.c)
//   ir_opt()     scalar fixpoint (fold/simplify/DCE/GVN/memopt), then loop
//                rotation, induction-variable widening (+ scaled strength
//                reduction), invariant-load hoisting and LICM   (ir_opt.c)
//   ir_emit_x86() address+imm folding, phi elimination, linear-scan register
//                allocation, instruction selection, emit into cg_sec (ir_x86.c)
// Functions ir_build() rejects (returns NULL) are compiled by the old
// AST-walking codegen. Once ir_build() succeeds the rest MUST NOT fail.
//
// Value model
//   Every IRInstr has an id (its index in IRFunc.vals). Operands a/b/c name
//   other instrs by id; IR_NONE (-1) = unused. An instr whose type is IRT_VOID
//   (store, jmp, br, ret) produces no value. Constants are ordinary instrs
//   (ICONST/FCONST); the backend folds them into immediates and
//   rematerializes them instead of giving them registers.
//
// Types: IRT_I32 / IRT_I64 / IRT_F32 / IRT_F64. char/short/_Bool are I32
// (zero/sign-extended on load, normalized with SEXT*/ZEXT*). Pointers are I64.
// Integer ops are 2's complement and wrap; signedness is in the opcode.
// Register convention for emitters: the upper 32 bits of a register holding an
// I32 value are UNSPECIFIED (use 32-bit instruction forms; SEXT32/ZEXT32 are
// the only way to get a defined 64-bit value). SEXT8/16 and ZEXT8/16 yield a
// value normalized within the low 32 bits. Addresses (base/index) are I64.
#ifndef RCC_IR_H
#define RCC_IR_H

#include "rcc.h"

#define IR_NONE (-1)

typedef enum { IRT_VOID,
               IRT_I32,
               IRT_I64,
               IRT_F32,
               IRT_F64 } IRType;

typedef enum {
    // ---- leaves (no operands) ----
    IR_ICONST, // imm = value (I32: sign-extended 32-bit; I64: full)
    IR_FCONST, // fimm = value (F32 values are exactly representable as float)
    IR_PARAM, // imm = index of the incoming ABI argument (0-based, in
    // declaration order); type = its class. Only in the entry block.
    IR_FRAMEADDR, // I64: address of frame slot, imm = LVar->offset (the slot
    // is [rbp - imm], same convention as the old codegen)
    IR_GADDR, // I64: address of symbol `sym` + imm (RIP-relative lea); msign = 1: the
    // address is loaded from the symbol's GOT slot instead (-fPIC / extern / weak data)
    IR_UNDEF, // any type; value is unspecified (uninitialized local read)

    // ---- integer arithmetic: a (op) b, same type as result (I32 or I64) ----
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_SDIV,
    IR_UDIV,
    IR_SREM,
    IR_UREM, // divisor 0 / INT_MIN/-1 trap in hardware
    IR_AND,
    IR_OR,
    IR_XOR,
    IR_SHL,
    IR_SHR,
    IR_SAR, // b = I32 count; hardware masks it (C UB >= width)
    IR_NEG,
    IR_NOT, // unary on a

    // ---- float arithmetic: F32 or F64 ----
    IR_FADD,
    IR_FSUB,
    IR_FMUL,
    IR_FDIV,
    IR_FNEG, // unary on a

    // ---- comparisons: result I32 = 0/1; cc selects the predicate ----
    IR_CMP, // a, b integer (both I32 or both I64); cc in IRCC_*
    IR_FCMP, // a, b float (same type); cc in IRCC_F*; unordered => 0 except NE

    // ---- conversions (result type is the instr type) ----
    IR_SEXT8,
    IR_SEXT16, // I32 -> I32: sign-extend low 8/16 bits in place
    IR_ZEXT8,
    IR_ZEXT16, // I32 -> I32: zero-extend low 8/16 bits in place
    IR_SEXT32, // I32 -> I64 sign-extend
    IR_ZEXT32, // I32 -> I64 zero-extend
    IR_TRUNC, // I64 -> I32
    IR_SITOF, // I32/I64 (signed) -> F32/F64
    IR_UITOF, // I64 (unsigned) -> F32/F64. u32 sources are ZEXT32'd + SITOF.
    IR_FTOSI, // F32/F64 -> I32/I64, truncating toward zero
    IR_FTOU64, // F32/F64 -> I64 interpreted as unsigned 64-bit
    IR_FEXT, // F32 -> F64
    IR_FTRUNC, // F64 -> F32

    // ---- memory ----
    // LOAD : result = *(a + b*scale + imm); a = base (I64), b = index (I64) or
    //        IR_NONE. mw/msign give the memory width and, for widths < 32,
    //        the extension to I32. Builder emits b = IR_NONE, scale = 1,
    //        imm = 0; the backend folds ADD/MUL/SHL/FRAMEADDR chains into
    //        (base, index, scale, disp).
    // STORE: *(a + b*scale + imm) = c  (truncating to mw); type = IRT_VOID.
    IR_LOAD,
    IR_STORE,

    // ---- calls ----
    // CALL: direct if sym != NULL, else indirect through a (I64).
    // args[0..nargs) are value ids in C argument order; their IR types decide
    // the SysV class (I32/I64 -> GPR, F32/F64 -> XMM; overflow -> stack, 8-byte
    // slots). `variadic` => set %al = number of XMM registers used.
    // Result type = IRInstr.type (IRT_VOID, I32, I64, F32, F64).
    // Clobbers every caller-saved register.
    //
    // Aggregate arguments (struct/union/vector): arg_class[i] != IRAGG_NONE
    // means args[i] is the ADDRESS of an argument of arg_class[i] bytes and the
    // backend must pass it per that class (see IRAggClass). arg_class is NULL
    // or all-NONE when every argument is scalar.
    //
    // ret_agg: 0 = scalar/void result. > 0: VALUE16-class aggregate return of
    // ret_agg bytes; the result value is the ADDRESS of a backend temp holding
    // the bytes the backend stored from RAX[:RDX]. < 0: SRET-class return of
    // -ret_agg bytes; the backend allocates the temp and passes its address
    // itself as the hidden first GP argument (GP slot 0); result = temp address.
    IR_CALL,

    // ---- SSA ----
    IR_PHI, // args[i] = value flowing in from block->preds[i]; nargs == npreds

    // ---- terminators (last instr of every block, nothing after) ----
    IR_JMP, // -> t
    IR_BR, // a (I32) != 0 ? t : f
    // a = return value or IR_NONE. When the function returns an aggregate
    // (IRFunc.ret_agg > 0), a is the ADDRESS of the result bytes: for
    // ret_sret the backend moves that address into RAX (it IS the hidden
    // return pointer), otherwise (VALUE16 class) it loads RAX=[a], RDX=[a+8].
    IR_RET,
    IR_UNREACHABLE,

    IR_NOP, // removed instr (passes may leave these; emitters skip them)
    IR_NUM_OPS
} IROp;

typedef enum {
    IRCC_EQ,
    IRCC_NE,
    IRCC_SLT,
    IRCC_SLE,
    IRCC_SGT,
    IRCC_SGE,
    IRCC_ULT,
    IRCC_ULE,
    IRCC_UGT,
    IRCC_UGE,
    // float predicates, C semantics (any NaN operand => false, except NE => true)
    IRCC_FEQ,
    IRCC_FNE,
    IRCC_FLT,
    IRCC_FLE,
    IRCC_FGT,
    IRCC_FGE
} IRCC;

typedef enum { IRM_I8,
               IRM_I16,
               IRM_I32,
               IRM_I64,
               IRM_F32,
               IRM_F64 } IRMem;

// How an aggregate argument or parameter is passed, mirroring the AST
// codegen's simplification of the target ABI EXACTLY (caller and callee must agree or the
// ABI breaks). Computed once by ir_agg_class() from the C type. The set of classes a target
// produces differs: x86-64 SysV uses VALUE8 / VALUE16 / PTR / MEMORY; AArch64 uses
// VALUE8 / HFA4 / HFA8 / PTR (see ir_agg_class in ir_core.c).
typedef enum {
    IRAGG_NONE = 0, // not an aggregate: ordinary scalar value argument
    IRAGG_VALUE8, // struct/union/vector <= 8 bytes: raw value, 1 GP reg or 1 stack slot
    IRAGG_VALUE16, // struct/union 9..16B, all-integer leaves: raw value, 2 consecutive
    // GP regs (only if 2 are available) or 2+ stack slots
    IRAGG_PTR, // passed by pointer: 1 GP reg or 1 stack slot holding the address
    // (x86-64: 9..16B with a float/complex member; vectors > 8 bytes. AArch64: every
    // non-HFA struct/union > 8 bytes)
    IRAGG_MEMORY, // struct/union > 16B: raw bytes in outgoing stack slots, 0 GP regs
    IRAGG_HFA4, // AArch64 homogeneous float aggregate (1-4 floats): one V register per element
    // (all or none, else the raw bytes in 8-byte stack slots)
    IRAGG_HFA8 // AArch64 homogeneous double aggregate (1-4 doubles), same placement
} IRAggClass;
// arg_class / param_class entries encode the size: entry = (bytes << 3) | class,
// 0 = scalar. The only >8-aligned case in the AST codegen (long double/vector
// members) never reaches the IR builder, so backends need no align encoding:
// VALUE16 stack placement assumes align <= 8 (no odd-slot padding).

typedef struct IRBlock IRBlock;

typedef struct IRInstr {
    uint8_t op; // IROp
    uint8_t type; // IRType of the result
    uint8_t cc; // IRCC for CMP/FCMP
    uint8_t mw; // IRMem for LOAD/STORE
    uint8_t msign; // LOAD of I8/I16: 1 = sign-extend, 0 = zero-extend
    uint8_t scale; // LOAD/STORE index scale: 1, 2, 4, 8
    uint8_t variadic; // CALL
    uint8_t noret; // CALL: the callee is noreturn (abort, exit, __assert_fail, ...). Only the
    // x86 backend reads it: it cuts the block after the call (no successors, nothing live
    // across it) and may build the stack frame inside that cold block alone. Every other pass
    // keeps treating the call as an ordinary one followed by the block's terminator.
    uint8_t vol; // LOAD/STORE: a C `volatile` access (C11 5.1.2.3: an observable side effect). It is
    // executed exactly once per evaluation, at its declared width mw, in program order relative to
    // every other volatile access, call and store: never removed (even with an unused result),
    // duplicated, split, widened, merged, forwarded, hoisted, sunk or reordered. Passes treat it
    // as a barrier for memory-op motion that also kills every memory fact, like a call (see
    // ir_has_side_effects(); ir_opt.c memopt/dse/sroa/hoist_loads/rotation and ir_x86.c's load
    // folding each carry the rule). Zero for every other instr.
    int32_t id; // index in IRFunc.vals
    int32_t a, b, c; // operand value ids, IR_NONE if unused
    int64_t imm;
    double fimm;
    const char *sym; // GADDR / direct CALL target (assembler-level name)
    int32_t *args; // CALL args, PHI incoming values
    int nargs;
    int32_t *arg_class; // CALL: per-arg (bytes<<3)|IRAggClass, 0 = scalar value
    int32_t ret_agg; // CALL: <0 sret of -ret_agg bytes, >0 two-reg return of ret_agg bytes
    int32_t var_from; // CALL: > 0 = the callee is a prototyped variadic function and args[var_from-1 ..]
    // are its unnamed (variadic) arguments (Darwin/ARM64 passes those on the stack); 0 = none
    IRBlock *t, *f; // JMP/BR targets
    IRBlock *blk; // owning block (NULL when dead)
    int line; // source line for -g / diagnostics, 0 if none
    // scratch for passes (not preserved between passes)
    int32_t aux;
} IRInstr;

struct IRBlock {
    int id; // index in IRFunc.blocks
    IRInstr **phis; // phis come first in execution order
    int nphis, capphis;
    IRInstr **ins; // non-phi instrs; the last one is the terminator
    int nins, capins;
    IRBlock **preds;
    int npreds, cappreds;
    int loop_depth; // filled by ir_analyze_loops(), 0 = not in a loop
    // scratch for passes
    int aux;
    void *auxp;
    bool sealed, filled; // Braun construction state (ir_build.c)
};

typedef struct IRFunc {
    Function *fn; // source function (names, flags, ty)
    IRBlock **blocks; // blocks[0] is the entry block
    int nblocks, capblocks;
    IRInstr **vals; // id -> instr (never shrinks; dead instrs become IR_NOP)
    int nvals, capvals;
    IRType ret_type; // IRT_VOID if none
    int32_t ret_agg; // >0: returns a struct/union of this many bytes (RET.a = address)
    int32_t ret_sret; // ret_agg > 0 and the ABI uses a hidden return pointer:
    // incoming parameter 0 is that pointer (I64, IRAGG_NONE)
    int nparams; // incoming ABI args (including the hidden sret pointer, if any)
    IRType *param_types;
    int32_t *param_class; // per-param (bytes<<3)|IRAggClass, 0 = scalar; >0 means
    // the IR_PARAM value is the ADDRESS of the bytes
    bool has_calls;
    int frame_size; // bytes of address-taken locals/arrays = fn->stack_size
} IRFunc;

// Classify an aggregate argument/parameter type per the IRAggClass rules
// above; IRAGG_NONE for scalars. Must match the AST codegen's decisions.
int ir_agg_class(Type *ty);
// True when a struct/union return goes in RAX[:RDX] (all-integer, <= 16B,
// not a vector); otherwise it uses the hidden return pointer. Mirrors
// codegen.c's struct_returns_in_gp_regs().
bool ir_ret_two_reg(Type *ty);

// ---------------------------------------------------------------- ir_build.c
// Returns NULL when the function is not eligible (caller falls back to the old
// codegen). The result is verified (ir_verify) in debug builds.
IRFunc *ir_build(Program *prog, Function *fn);

// ---------------------------------------------------------------- ir_core.c
// Construction and bookkeeping shared by all passes.
IRFunc *ir_func_new(Function *fn);
void ir_func_free(IRFunc *f);
IRBlock *ir_block_new(IRFunc *f); // appended to f->blocks
IRInstr *ir_instr_new(IRFunc *f, IROp op, IRType type); // not yet in a block
void ir_block_append(IRBlock *b, IRInstr *i); // before/after terminator rules: appends; phis go to ir_block_add_phi
void ir_block_add_phi(IRBlock *b, IRInstr *phi);
void ir_block_prepend(IRBlock *b, IRInstr *i); // insert at start of non-phi list
void ir_block_remove(IRBlock *b, IRInstr *i); // unlink from b, mark IR_NOP
void ir_add_pred(IRBlock *b, IRBlock *pred);
// Replace every use of value `from` by `to` in all blocks (operands, call args,
// phi args). Does not delete `from`.
void ir_replace_uses(IRFunc *f, int from, int to);
bool ir_is_terminator(IROp op);
bool ir_has_side_effects(const IRInstr *i); // STORE/CALL/terminators/div (may trap)/volatile LOAD
int ir_succs(IRBlock *b, IRBlock *out[2]); // number of successors
// Recomputes preds from terminators (keeps phi args aligned: callers that
// reorder preds must permute phi args too, so use ir_remove_pred_edge below).
void ir_remove_pred_edge(IRBlock *b, IRBlock *pred); // drops pred + phi arg
// Reverse post-order of reachable blocks, entry first. Returns count.
int ir_rpo(IRFunc *f, IRBlock **out);
// Sets IRBlock.loop_depth from back edges (RPO + dominators or simple natural
// loop discovery). Needed by the register allocator's spill weights.
void ir_analyze_loops(IRFunc *f);
void ir_dump(IRFunc *f, FILE *out); // textual form, used by -fdump-ir
// Checks SSA/CFG invariants (defs dominate uses, phi arity == npreds, one
// terminator per block, operand types). Aborts with a message on violation.
void ir_verify(IRFunc *f);

// ---------------------------------------------------------------- ir_inline.c
// Inline direct calls to small callees into `caller`, before ir_opt().
// `lookup(sym, ud)` returns the callee's as-built IRFunc (read-only template,
// shared; never modified or freed here) or NULL when `sym` must stay a call
// (not in this TU, weak, interposable, or otherwise disqualified by the host).
// The pass decides profitability and legality of the IR itself: see ir_inline.c.
// Leaves `caller` verifiable and `caller->has_calls` correct.
void ir_inline(IRFunc *caller, IRFunc *(*lookup)(const char *sym, void *ud), void *ud);
// Turn self-recursive tail calls of `f` into a loop (run after ir_inline, before
// ir_opt). Only direct scalar calls of f's own symbol whose result is returned
// unchanged (through empty jmp blocks / a `phi; ret phi` merge) qualify, and only
// in functions without IR_FRAMEADDR. Adds a loop-header block with one phi per
// changed param and refreshes has_calls and loop_depth. Callers must additionally
// skip functions whose self-call may bind elsewhere (weak, or interposable under -fPIC).
void ir_tailrec(IRFunc *f);

// ---------------------------------------------------------------- ir_opt.c
// Scalar fixpoint: constant folding, algebraic simplification, copy/phi
// simplification, DCE, unreachable-block removal, branch folding (incl. branches
// on values a dominating edge already decided), empty-block threading, block
// merging, dominator-tree GVN of pure instrs, and memopt + sroa (below).
// Loop passes (after the fixpoint; loops from dominance back edges, irreducible
// loops skipped): loop rotation turns a header-tested loop into a guarded
// do-while (the header's test is copied into the preheader; SSA uses of header
// values are re-merged with phis); iv_widen turns an I32 induction variable whose
// wrap is provably unobservable (a tiny sound range analysis over dominating
// conditional edges proves the increment cannot wrap) into an I64 phi, removing
// per-iteration sign extensions, and folds the scaled variant (sext(i) * C with
// a stride C the addressing modes cannot absorb) into a pointer-style IV;
// iv_strength replaces an I32 `iv * m` (m loop-invariant, `+ j` folded in) by an I32
// add recurrence, exact modulo 2^32 without any range fact;
// hoist_loads moves a loop-invariant load into the preheader when no store in the
// loop may alias it, the loop has no call, volatile access or trapping divide, and
// the load's block dominates every exit; licm hoists invariant pure instrs into
// (created) loop preheaders. Memory (volatile LOAD/STOREs, IRInstr.vol, are never
// touched and kill every fact and pending store like a call; the rest are plain
// accesses): redundant loads
// and store->load pairs are forwarded over a bounded availability dataflow (facts
// merge at joins when every predecessor holds one, with a phi of the per-predecessor
// values when they differ; loops are analysed optimistically to a stable fixpoint, so
// a loop-carried accumulator cell becomes a header phi of its entry and back-edge
// values),
// same-address overwritten stores and stores to never-read non-escaping frame
// slots are removed; unused plain LOADs are deleted. CALLs, volatile accesses and
// trapping divides are never removed, merged or reordered, nor is a store moved past
// an aliasing access. A frame slot with a volatile access counts as escaped (it is
// neither promoted by sroa nor stripped of stores).
// sroa (same fixpoint, after memopt): a frame slot whose address never escapes and
// whose every access decomposes to that root plus a constant displacement is promoted
// to per-displacement SSA cells, provided the cells are pairwise disjoint byte
// intervals (any overlap -- differing widths over the same bytes, or equal-width
// intervals straddling each other -- keeps the slot in memory); loads rewrite to the
// cell value and stores vanish, so the slot's FRAMEADDR goes dead and the backend
// drops the slot.
// Cells merge at multi-predecessor joins with phis; a load with no stored value on its
// reaching path bails the slot (all-or-nothing per slot). Must preserve exact C-visible
// semantics (incl. wrap, trap-capable divides).
// Output block ids are dense and loop_depth is refreshed.
void ir_opt(IRFunc *f);

// ---------------------------------------------------------------- ir_x86.c
// Register allocation + isel + emit for x86-64 SysV into cg_sec. Defines the
// function symbol (via cg_ir_begin_fn), emits prologue/body/epilogue, patches
// the symbol size (cg_ir_end_fn). Infallible once called.
void ir_emit_x86(Program *prog, IRFunc *f, bool emitting_local_copy);

// ---------------------------------------------------------------- ir_arm64.c
// AArch64 counterpart of ir_emit_x86() (AAPCS64 / Darwin arm64). Returns false, BEFORE
// emitting any code, when the function uses something the emitter does not model
// (the caller then falls back to the AST backend); true once the function is emitted.
bool ir_emit_arm64(Program *prog, IRFunc *f, bool emitting_local_copy);

#ifdef ARCH_ARM64
// AArch64: homogeneous float aggregate test (codegen.c arm64_hfa_count): number of elements
// (1-4, 0 = not an HFA); *elem_size = 4 or 8.
int cg_ir_arm64_hfa(Type *ty, int *elem_size);
#endif
// ----------------------------------------------------------- codegen.c hooks
// Entry point called from codegen()'s per-function loop at -O2+; returns true
// if the function was compiled through the IR (caller skips the old path).
// The helpers ir_x86.c needs from codegen.c are declared in cg_ir.h.
bool cg_ir_try_function(Program *prog, Function *fn, bool emitting_local_copy);

extern bool opt_fdump_ir; // -fdump-ir: ir_dump() every function to stderr after ir_opt
extern bool opt_fno_ir; // -fno-ir: never use the IR backend
extern bool opt_fno_ir_inline; // -fno-ir-inline: skip ir_inline()

#endif
