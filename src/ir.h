// ir.h -- SSA IR for the -O2 backend (x86-64 SysV first).
//
// Pipeline, per eligible function (see TODO "-O2 performance"):
//   ir_build()   AST -> IR, SSA built on the fly (Braun et al.), scalar locals
//                whose address is never taken become SSA values (ir_build.c)
//   ir_opt()     fold / simplify / DCE / CFG cleanup           (ir_opt.c)
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
    IR_CALL,

    // ---- SSA ----
    IR_PHI, // args[i] = value flowing in from block->preds[i]; nargs == npreds

    // ---- terminators (last instr of every block, nothing after) ----
    IR_JMP, // -> t
    IR_BR, // a (I32) != 0 ? t : f
    IR_RET, // a = return value or IR_NONE
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

typedef struct IRBlock IRBlock;

typedef struct IRInstr {
    uint8_t op; // IROp
    uint8_t type; // IRType of the result
    uint8_t cc; // IRCC for CMP/FCMP
    uint8_t mw; // IRMem for LOAD/STORE
    uint8_t msign; // LOAD of I8/I16: 1 = sign-extend, 0 = zero-extend
    uint8_t scale; // LOAD/STORE index scale: 1, 2, 4, 8
    uint8_t variadic; // CALL
    int32_t id; // index in IRFunc.vals
    int32_t a, b, c; // operand value ids, IR_NONE if unused
    int64_t imm;
    double fimm;
    const char *sym; // GADDR / direct CALL target (assembler-level name)
    int32_t *args; // CALL args, PHI incoming values
    int nargs;
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
    int nparams; // incoming ABI args, all scalar I32/I64/F32/F64
    IRType *param_types;
    bool has_calls;
    int frame_size; // bytes of address-taken locals/arrays = fn->stack_size
} IRFunc;

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
bool ir_has_side_effects(const IRInstr *i); // STORE/CALL/terminators/div (may trap)
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

// ---------------------------------------------------------------- ir_opt.c
// Constant folding, algebraic simplification, copy/phi simplification, DCE,
// unreachable-block removal, branch folding, empty-block threading, block
// merging, dominator-tree GVN of pure instrs and LICM of pure non-trapping
// instrs (which may add loop preheader blocks). Output block ids are dense and
// loop_depth is refreshed. Unused LOADs are deleted; STORE/CALL/trapping divides
// are never removed, merged or reordered.
// Must preserve exact C-visible semantics (incl. wrap, trap-capable divides).
void ir_opt(IRFunc *f);

// ---------------------------------------------------------------- ir_x86.c
// Register allocation + isel + emit for x86-64 SysV into cg_sec. Defines the
// function symbol (via cg_ir_begin_fn), emits prologue/body/epilogue, patches
// the symbol size (cg_ir_end_fn). Infallible once called.
void ir_emit_x86(Program *prog, IRFunc *f, bool emitting_local_copy);

// ----------------------------------------------------------- codegen.c hooks
// Entry point called from codegen()'s per-function loop at -O2+; returns true
// if the function was compiled through the IR (caller skips the old path).
// The helpers ir_x86.c needs from codegen.c are declared in cg_ir.h.
bool cg_ir_try_function(Program *prog, Function *fn, bool emitting_local_copy);

extern bool opt_fdump_ir; // -fdump-ir: ir_dump() every function to stderr after ir_opt
extern bool opt_fno_ir; // -fno-ir: never use the IR backend

#endif
