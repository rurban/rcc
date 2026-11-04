// ir_arm64.c -- register allocation, instruction selection and emission (AArch64:
// AAPCS64 on Linux, Darwin arm64).
//
// ir_emit_arm64() runs these stages on an already built + optimized IRFunc, mirroring
// ir_x86.c (which documents the shared design in more detail):
//   1. CFG prep: a call to a noreturn function ends its block, unreachable preds are
//      dropped, critical edges into blocks with phis are split, loop depths, loop-friendly
//      block layout.
//   2. Isel prep on the IR: UNDEF -> 0, address folding into LOAD/STORE (base + index<<scale
//      + disp, sp-relative frame slots), single-use LOADs folded into the extension that
//      consumes them, CMP/FCMP+BR fusion, use counts + DCE. Constants and symbol/frame
//      addresses are rematerialized.
//   3. Liveness -> live ranges with holes -> linear scan with loop-weighted spill costs and
//      register hints (phis, two-address ops, incoming/outgoing ABI registers).
//   4. Frame layout, prologue, per-instruction emission, phi elimination by parallel moves.
//
// Registers: x14-x17 (T3 T2 T0 T1) and v28-v31 are scratch, never allocated; x18 is the
// Darwin platform register and never touched; x29/x30 hold the frame record. Allocatable:
// x0-x13 (caller-saved), x19-x28 (callee-saved), v0-v7 + v16-v27 (caller-saved), v8-v15
// (callee-saved: AAPCS64 only preserves their low 64 bits, which is all an F32/F64 needs).
// Values live across a call get callee-saved registers or a spill slot.
//
// Frame (the AST backend's convention: x29 = sp after `stp x29, x30, [sp, #-16]!`):
//   [x29+16 ..)          incoming stack arguments (8-byte slots)
//   [x29, x29+16)        saved x29 / x30
//   x29 - FRAMEADDR.imm  address-taken locals (IRFunc.frame_size region)
//   below: callee-saved save area, spill slots, then the outgoing stack argument area at
//   [sp, sp+outgoing). sp = x29 - F is 16-byte aligned for the whole body (no alloca in IR
//   functions), so every slot is addressed sp-relative with a small positive offset.
// A leaf that needs none of it is frameless (no prologue at all).
#include "rcc.h"
#ifdef ARCH_ARM64
#include "cg_ir.h"
#include <limits.h>
#include <math.h>

#define T0 ARM64_X16 // dest scratch (the result of a spilled def)
#define T1 ARM64_X17 // operand a scratch; parallel-move cycle breaker
#define T2 ARM64_X15 // operand b scratch / displacement temp
#define T3 ARM64_X14 // operand c / address / quotient temp
#define XT0 30
#define XT1 31 // also the FP parallel-move cycle breaker
#define XT2 29
#define XT3 28
#define R_S0 T0
#define R_S1 T1
#define X_S0 XT0
#define X_S1 XT1
#define SPREG ARM64_SP
#define ZR ARM64_XZR

enum { RC_G = 0,
       RC_X = 1 };
enum { LK_NONE,
       LK_REG,
       LK_SPILL,
       LK_REMAT };

typedef struct {
    uint8_t kind;
    int8_t reg;
    int32_t slot; // LK_SPILL: the value lives at [x29 - slot] (negative: incoming stack arg [x29 - slot])
} Loc;

typedef struct {
    int from, to; // half-open position range [from, to)
} Range;

typedef struct {
    int start, end; // hull of the ranges
    Range *rg; // live ranges, ascending, disjoint
    int nrg, caprg;
    int64_t cost; // loop-weighted number of defs + uses
    int uses;
    int hint[3]; // values whose register is a good choice for this one, -1 = none
    int ipos; // instruction slot
    int slotidx; // spill slot index within its class pool, -1 = none
    int16_t stk; // PARAM passed on the stack: [x29 + stk], 0 = in a register
    int8_t abi; // PARAM passed in a register / value wanted in an ABI register: its number, -1 = none
    uint8_t dead; // removed by isel prep (not emitted)
    uint8_t keep; // GADDR held in a register (used in a deeper loop than its def), not rematerialized
} VInfo;

#define BIT(r) (1u << (r))
#define MASK_CALLEE_G 0x1ff80000u // x19-x28
#define MASK_G_ALL (0x3fffu | MASK_CALLEE_G) // x0-x13 + x19-x28
#define MASK_CALLEE_X 0x0000ff00u // v8-v15
#define MASK_X_ALL (0x000000ffu | 0x0fff0000u | MASK_CALLEE_X) // v0-v7, v16-v27, v8-v15

// Caller-saved registers first: a callee-saved register costs a save/restore pair.
static const int8_t gpr_alloc_order[] = {9, 10, 11, 12, 13, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                                         19, 20, 21, 22, 23, 24, 25, 26, 27, 28};
static const int8_t gpr_callee_order[] = {19, 20, 21, 22, 23, 24, 25, 26, 27, 28};
static const int8_t xmm_alloc_order[] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                                         7, 6, 5, 4, 3, 2, 1, 0, 8, 9, 10, 11, 12, 13, 14, 15};
static const int8_t xmm_callee_order[] = {8, 9, 10, 11, 12, 13, 14, 15};

static struct {
    IRFunc *f;
    const char *sym;
    int serial;
    int nv;
    VInfo *vi;
    Loc *loc;
    int nord;
    IRBlock **ord; // layout order of reachable blocks
    int *oidx; // block id -> index in ord, -1 if unreachable
    int *bslot; // block id -> entry slot (phis are defined at its def position)
    int *bterm; // block id -> slot of its terminator
    uint8_t *tramp; // block id -> empty forwarding block, never emitted
    const char **lbl; // block id -> label
    int ncall, *callpos; // use positions of every CALL, ascending
    int nspill[2]; // spill slots per class pool
    int ncs, cs_reg[10]; // callee-saved GPRs used
    int ncx, cx_reg[8]; // callee-saved FP registers used (d8-d15)
    int locals, cs_base, spill_base, outgoing, frame;
    bool frameless; // no frame record, locals, spills or saved registers
    bool far; // function may exceed the +-1MB conditional branch range: branch through `b`
    int *mu; // value -> uses as an address operand in its own block (isel prep)
    int *pabi; // param index -> ABI register number / -1
    int *pstk; // param index -> incoming stack offset from x29 / 0
    int *pcls; // param index -> IRAggClass (0 = scalar)
    int *paggoff; // materialized aggregate param: x29 offset of its frame temp
    int *ctemp; // CALL value id -> x29 offset of its aggregate-return temp
    int aggbase; // x29 offset where the aggregate temps start (above the locals)
    int aggtotal; // bytes of aggregate temps
} be;

static SecBuf *sec(void) { return cg_sec; }
#define S sec()

static void *xalloc(size_t n) {
    void *p = calloc(1, n ? n : 1);
    if (!p) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    return p;
}
#define NEW(T, n) ((T *)xalloc(sizeof(T) * (size_t)(n)))

static inline IRInstr *V(int id) { return be.f->vals[id]; }
static inline Loc *LOC(int v) { return &be.loc[v]; }
static inline bool tflt(int t) { return t == IRT_F32 || t == IRT_F64; }
static inline int tsz(int t) { return (t == IRT_I32 || t == IRT_F32) ? 4 : 8; }
static inline int rc_of(int v) { return tflt(V(v)->type) ? RC_X : RC_G; }
static inline bool is_remat_op(int op) {
    return op == IR_ICONST || op == IR_FCONST || op == IR_GADDR || op == IR_FRAMEADDR;
}
// An aggregate parameter (VALUE8 / HFA) arrives in registers or stack slots; the prologue stores
// it into a frame temp and the IR_PARAM value is that temp's address (like a FRAMEADDR).
static inline bool agg_param(const IRInstr *i) {
    if (i->op != IR_PARAM || !be.pcls) return false;
    int c = be.pcls[i->imm];
    return c == IRAGG_VALUE8 || c == IRAGG_HFA4 || c == IRAGG_HFA8;
}
// Values rematerialized at each use instead of living in a register. A GADDR
// flagged `keep` (see count_uses) is an ordinary allocated value.
static inline bool remat_instr(const IRInstr *i) {
    return (is_remat_op(i->op) && !(i->op == IR_GADDR && be.vi[i->id].keep)) || agg_param(i);
}

// Use slots of an instruction: CALL = target + args, PHI = args, else a b c.
static int nuse_slots(const IRInstr *i) {
    if (i->op == IR_PHI) return i->nargs;
    if (i->op == IR_CALL) return 1 + i->nargs;
    return 3;
}
static int use_at(const IRInstr *i, int k) {
    if (i->op == IR_PHI) return i->args[k];
    if (i->op == IR_CALL) return k == 0 ? i->a : i->args[k - 1];
    return k == 0 ? i->a : k == 1 ? i->b
                                  : i->c;
}

// ------------------------------------------------------------------ 1. CFG


// Block whose control flow ends in a noreturn call (see cut_noreturn).
static bool is_cold(const IRBlock *b) {
    int k = b->nins - 1;
    if (k < 1 || b->ins[k]->op != IR_UNREACHABLE) return false;
    while (--k >= 0 && b->ins[k]->op == IR_NOP) {}
    return k >= 0 && b->ins[k]->op == IR_CALL && b->ins[k]->noret;
}

// A call to a noreturn function ends its block: whatever follows it (merged
// successor code, the original terminator) never runs. Cutting the block there
// leaves it with no successors, so nothing is live across the call, no phi
// copies are made for it and values need no callee-saved register just to
// survive a call that never comes back. The upstream passes keep seeing the
// plain call + terminator they were given.
static void cut_noreturn(IRFunc *f) {
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        int k = 0;
        while (k < b->nins && !(b->ins[k]->op == IR_CALL && b->ins[k]->noret)) k++;
        if (k == b->nins || (k + 2 == b->nins && b->ins[k + 1]->op == IR_UNREACHABLE)) continue;
        IRBlock *ss[2];
        int ns = ir_succs(b, ss);
        for (int s = 0; s < ns; s++) ir_remove_pred_edge(ss[s], b);
        for (int j = k + 1; j < b->nins; j++) {
            IRInstr *d = b->ins[j];
            d->op = IR_NOP;
            d->type = IRT_VOID;
            d->a = d->b = d->c = IR_NONE;
            d->blk = NULL;
        }
        b->nins = k + 1;
        ir_block_append(b, ir_instr_new(f, IR_UNREACHABLE, IRT_VOID));
    }
}

static void cfg_prep(IRFunc *f) {
    cut_noreturn(f);
    int n = f->nblocks;
    IRBlock **rpo = NEW(IRBlock *, n);
    uint8_t *reach = NEW(uint8_t, n);
    int cnt = ir_rpo(f, rpo);
    for (int i = 0; i < cnt; i++) reach[rpo[i]->id] = 1;
    for (int i = 0; i < cnt; i++) {
        IRBlock *b = rpo[i];
        for (int k = 0; k < b->npreds;) {
            if (reach[b->preds[k]->id]) k++;
            else
                ir_remove_pred_edge(b, b->preds[k]);
        }
    }
    // Phi copies are placed at the end of the predecessor, so a predecessor
    // with two successors needs a private forwarding block for such an edge.
    for (int i = 0; i < cnt; i++) {
        IRBlock *s = rpo[i];
        if (!s->nphis) continue;
        for (int k = 0; k < s->npreds; k++) {
            IRBlock *p = s->preds[k];
            IRBlock *ss[2];
            if (ir_succs(p, ss) < 2) continue;
            IRBlock *nb = ir_block_new(f);
            IRInstr *j = ir_instr_new(f, IR_JMP, IRT_VOID);
            j->t = s;
            ir_block_append(nb, j);
            ir_add_pred(nb, p);
            IRInstr *t = p->ins[p->nins - 1];
            if (t->t == s) t->t = nb;
            else
                t->f = nb;
            s->preds[k] = nb;
        }
    }
    free(rpo);
    free(reach);
}

// Loop rotation by layout: a loop header that ends in a conditional branch
// with one successor inside the loop is moved behind the loop's last block.
// Entry jumps to it; the latch falls into it; the taken branch is the back
// edge, so an iteration executes one branch instead of two.
static void rotate_loops(void) {
    int n = be.nord, nb = be.f->nblocks;
    uint8_t *in = NEW(uint8_t, nb);
    IRBlock **work = NEW(IRBlock *, nb);
    IRBlock **hdr = NEW(IRBlock *, n);
    int nh = 0;
    for (int i = 1; i < n; i++) {
        IRBlock *h = be.ord[i];
        for (int k = 0; k < h->npreds; k++)
            if (be.oidx[h->preds[k]->id] >= i) {
                hdr[nh++] = h;
                break;
            }
    }
    for (int hh = nh - 1; hh >= 0; hh--) { // inner loops first
        IRBlock *h = hdr[hh];
        IRInstr *t = h->ins[h->nins - 1];
        if (t->op != IR_BR) continue;
        int hi = be.oidx[h->id], wp = 0;
        memset(in, 0, (size_t)nb);
        in[h->id] = 1;
        for (int k = 0; k < h->npreds; k++) {
            IRBlock *p = h->preds[k];
            if (be.oidx[p->id] >= hi && !in[p->id]) {
                in[p->id] = 1;
                work[wp++] = p;
            }
        }
        while (wp) {
            IRBlock *x = work[--wp];
            for (int k = 0; k < x->npreds; k++) {
                IRBlock *p = x->preds[k];
                if (!in[p->id]) {
                    in[p->id] = 1;
                    work[wp++] = p;
                }
            }
        }
        if (in[t->t->id] == in[t->f->id]) continue;
        int last = hi;
        for (int i = hi + 1; i < n; i++)
            if (in[be.ord[i]->id]) last = i;
        if (last == hi) continue;
        memmove(&be.ord[hi], &be.ord[hi + 1], sizeof(IRBlock *) * (size_t)(last - hi));
        be.ord[last] = h;
        for (int i = hi; i <= last; i++) be.oidx[be.ord[i]->id] = i;
    }
    free(in);
    free(work);
    free(hdr);
}

// Layout: reverse post-order of a DFS that visits the shallower-loop-depth
// successor first, so loop bodies precede their exits and loops stay
// contiguous; on ties the false edge goes first so the true edge falls through.
static void layout(IRFunc *f) {
    int n = f->nblocks;
    IRBlock **stack = NEW(IRBlock *, n);
    IRBlock **post = NEW(IRBlock *, n);
    IRBlock **succ = NEW(IRBlock *, 2 * n);
    int *ns = NEW(int, n), *nx = NEW(int, n);
    uint8_t *seen = NEW(uint8_t, n);
    int sp = 0, np = 0;
#define INIT_SUCC(blk) \
    do { \
        IRBlock *_s[2]; \
        int _n = ir_succs((blk), _s); \
        ns[(blk)->id] = _n; \
        if (_n == 2 && _s[0]->loop_depth < _s[1]->loop_depth) { \
            succ[2 * (blk)->id] = _s[0]; \
            succ[2 * (blk)->id + 1] = _s[1]; \
        } else if (_n == 2) { \
            succ[2 * (blk)->id] = _s[1]; \
            succ[2 * (blk)->id + 1] = _s[0]; \
        } else if (_n == 1) { \
            succ[2 * (blk)->id] = _s[0]; \
        } \
    } while (0)
    stack[sp++] = f->blocks[0];
    seen[0] = 1;
    INIT_SUCC(f->blocks[0]);
    while (sp) {
        IRBlock *b = stack[sp - 1];
        if (nx[b->id] < ns[b->id]) {
            IRBlock *c = succ[2 * b->id + nx[b->id]++];
            if (!seen[c->id]) {
                seen[c->id] = 1;
                INIT_SUCC(c);
                stack[sp++] = c;
            }
        } else {
            post[np++] = b;
            sp--;
        }
    }
#undef INIT_SUCC
    be.nord = np;
    be.ord = NEW(IRBlock *, np);
    be.oidx = NEW(int, n);
    for (int i = 0; i < n; i++) be.oidx[i] = -1;
    for (int i = 0; i < np; i++) {
        be.ord[i] = post[np - 1 - i];
        be.oidx[be.ord[i]->id] = i;
    }
    rotate_loops();
    // Cold (noreturn) blocks go behind everything else, keeping their order:
    // the hot path stays contiguous and the cold code stays out of the way.
    int nh = 1;
    for (int i = 1; i < np; i++)
        if (!is_cold(be.ord[i])) post[nh++] = be.ord[i];
    post[0] = be.ord[0];
    int nc = nh;
    for (int i = 1; i < np; i++)
        if (is_cold(be.ord[i])) post[nc++] = be.ord[i];
    memcpy(be.ord, post, sizeof(IRBlock *) * (size_t)np);
    for (int i = 0; i < np; i++) be.oidx[be.ord[i]->id] = i;
    free(stack);
    free(post);
    free(succ);
    free(ns);
    free(nx);
    free(seen);
}

// ------------------------------------------------------------- 2. isel prep

static void count_uses(void) {
    for (int v = 0; v < be.nv; v++) {
        be.vi[v].uses = 0;
        be.vi[v].keep = 0;
    }
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (i->op == IR_NOP || be.vi[i->id].dead) continue;
            for (int u = 0, nu = nuse_slots(i); u < nu; u++) {
                int v = use_at(i, u);
                if (v == IR_NONE) continue;
                be.vi[v].uses++;
                // A global's address used in a deeper loop than its (hoisted) def is
                // worth a register: re-materializing it costs a lea per iteration.
                if (V(v)->op == IR_GADDR && b->loop_depth > V(v)->blk->loop_depth) be.vi[v].keep = 1;
            }
        }
    }
}

static bool const_of(int v, int64_t *c) {
    IRInstr *d = V(v);
    if (d->op != IR_ICONST) return false;
    *c = d->type == IRT_I32 ? (int64_t)(int32_t)d->imm : d->imm;
    return true;
}

static bool fits32(int64_t v) { return v >= INT32_MIN && v <= INT32_MAX; }

// v = y << k or y * 2^k (I64): the index scale it contributes, 0 if it is not one.
static int scaled_by(int v, int *base) {
    IRInstr *d = V(v);
    int64_t c;
    if (d->type != IRT_I64) return 0;
    if (d->op == IR_SHL && const_of(d->b, &c) && c >= 1 && c <= 3) {
        *base = d->a;
        return 1 << c;
    }
    if (d->op == IR_MUL) {
        if (const_of(d->b, &c) && (c == 2 || c == 4 || c == 8)) {
            *base = d->a;
            return (int)c;
        }
        if (const_of(d->a, &c) && (c == 2 || c == 4 || c == 8)) {
            *base = d->b;
            return (int)c;
        }
    }
    return 0;
}


// Fold ADD/SUB/SHL/MUL/FRAMEADDR chains of a LOAD/STORE address into
// (base, index, scale, disp). m->aux = 1 means the base is the frame (a FRAMEADDR
// folded in, m->a == IR_NONE, disp relative to x29). Folded operands must live in the
// same block so operand lifetimes are not stretched across loops; FRAMEADDR is a frame
// constant that is rematerialized anyway, so it folds from any block.
static void fold_addr(IRInstr *m, bool split) {
    IRBlock *blk = m->blk;
    int base = m->a, idx = m->b, scale = m->scale ? m->scale : 1;
    int64_t disp = m->imm;
    int kind = m->aux;
    for (int iter = 0; iter < 8; iter++) {
        bool ch = false;
        int64_t c;
        if (idx != IR_NONE) {
            IRInstr *d = V(idx);
            int ib;
            int sc;
            if (const_of(idx, &c)) {
                if (fits32(disp + c * scale)) {
                    disp += c * scale;
                    idx = IR_NONE;
                    ch = true;
                }
            } else if (d->blk == blk && (sc = scaled_by(idx, &ib)) && scale * sc <= 8) {
                idx = ib;
                scale *= sc;
                ch = true;
            } else if (d->blk == blk && d->op == IR_ADD && d->type == IRT_I64) {
                if (const_of(d->b, &c) && fits32(disp + c * scale)) {
                    disp += c * scale;
                    idx = d->a;
                    ch = true;
                } else if (const_of(d->a, &c) && fits32(disp + c * scale)) {
                    disp += c * scale;
                    idx = d->b;
                    ch = true;
                }
            } else if (m->cc == 0 && (d->op == IR_SEXT32 || d->op == IR_ZEXT32) && d->type == IRT_I64 &&
                       !const_of(d->a, &c)) {
                // [base, w_idx, sxtw/uxtw #scale]: the extension is free in the addressing mode
                m->cc = d->op == IR_SEXT32 ? 1 : 2;
                idx = d->a;
                ch = true;
            }
        }
        if (base != IR_NONE) {
            IRInstr *d = V(base);
            if (d->op == IR_GADDR) {
                // A direct (non-GOT) symbol with no index becomes ADRP + [page, :lo12:sym + disp]
                // (kind 2, symbol in m->sym): the load/store takes the low 12 bits itself.
                static const uint8_t mwsz[] = {1, 2, 4, 8, 4, 8};
                int64_t off = disp + d->imm;
                if (!d->msign && kind == 0 && idx == IR_NONE && off >= 0 && fits32(off) &&
                    cg_ir_sym_ldst_direct(d->sym, off, mwsz[m->mw])) {
                    disp = off;
                    m->sym = d->sym;
                    kind = 2;
                    base = IR_NONE;
                    ch = true;
                }
            } else if (d->op == IR_FRAMEADDR) {
                if (fits32(disp - d->imm)) {
                    disp -= d->imm;
                    kind = 1;
                    base = IR_NONE;
                    ch = true;
                }
            } else if (agg_param(d)) { // address of a materialized aggregate parameter: a frame slot
                if (fits32(disp - be.paggoff[d->imm])) {
                    disp -= be.paggoff[d->imm];
                    kind = 1;
                    base = IR_NONE;
                    ch = true;
                }
            } else if (d->blk == blk && d->op == IR_ADD && d->type == IRT_I64) {
                if (const_of(d->b, &c) && fits32(disp + c)) {
                    disp += c;
                    base = d->a;
                    ch = true;
                } else if (const_of(d->a, &c) && fits32(disp + c)) {
                    disp += c;
                    base = d->b;
                    ch = true;
                } else if (split && idx == IR_NONE && d->blk == blk &&
                           (be.vi[d->id].uses == 1 || be.vi[d->id].uses == be.mu[d->id])) {
                    int ib;
                    if (scaled_by(d->a, &ib) && !scaled_by(d->b, &ib)) {
                        base = d->b;
                        idx = d->a;
                    } else {
                        base = d->a;
                        idx = d->b;
                    }
                    scale = 1;
                    ch = true;
                }
            } else if (d->blk == blk && d->op == IR_SUB && d->type == IRT_I64 && const_of(d->b, &c) &&
                       fits32(disp - c)) {
                disp -= c;
                base = d->a;
                ch = true;
            }
        }
        if (!ch) break;
    }
    m->a = base;
    m->b = idx;
    m->scale = idx == IR_NONE ? 1 : (uint8_t)scale;
    m->imm = disp;
    m->aux = kind;
}

static bool pure_op(const IRInstr *i) { return i->type != IRT_VOID && !ir_has_side_effects(i); }

// DCE: pure instructions with no remaining uses (use counts must be current).
static void dce(void) {
    int *work = NEW(int, be.f->nvals + 1);
    int nw = 0;
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (i->op != IR_NOP && !be.vi[i->id].dead && pure_op(i) && be.vi[i->id].uses == 0) work[nw++] = i->id;
        }
    }
    while (nw) {
        int v = work[--nw];
        IRInstr *i = V(v);
        if (be.vi[v].dead) continue;
        be.vi[v].dead = 1;
        for (int u = 0, nu = nuse_slots(i); u < nu; u++) {
            int o = use_at(i, u);
            if (o == IR_NONE) continue;
            if (--be.vi[o].uses == 0 && pure_op(V(o)) && !be.vi[o].dead) work[nw++] = o;
        }
    }
    free(work);
}

// Operand-swapped predicate: a cc b  <=>  b cc_swap[cc] a.
static const uint8_t cc_swap[] = {
    [IRCC_EQ] = IRCC_EQ,
    [IRCC_NE] = IRCC_NE,
    [IRCC_SLT] = IRCC_SGT,
    [IRCC_SLE] = IRCC_SGE,
    [IRCC_SGT] = IRCC_SLT,
    [IRCC_SGE] = IRCC_SLE,
    [IRCC_ULT] = IRCC_UGT,
    [IRCC_ULE] = IRCC_UGE,
    [IRCC_UGT] = IRCC_ULT,
    [IRCC_UGE] = IRCC_ULE,
    [IRCC_FEQ] = IRCC_FEQ,
    [IRCC_FNE] = IRCC_FNE,
    [IRCC_FLT] = IRCC_FGT,
    [IRCC_FLE] = IRCC_FGE,
    [IRCC_FGT] = IRCC_FLT,
    [IRCC_FGE] = IRCC_FLE,
};

enum {
    ALUF_NONE,
    ALUF_SHIFT,
    ALUF_SXTW,
    ALUF_UXTW,
    ALUF_MADD,
    ALUF_MSUB,
};

static bool foldable_alu_producer(const IRInstr *d, const IRInstr *use) {
    return d->blk == use->blk && be.vi[d->id].uses == 1 && !be.vi[d->id].dead;
}

// Fold AArch64's free register shift/extension and multiply-add/subtract forms into integer ALU
// instructions. aux records the form; cc records a folded shift amount; c is the addend for MADD
// or the minuend for MSUB. DCE removes the now-single-use producer.
static void fold_alu_operands(void) {
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (be.vi[i->id].dead) continue;
            int op = i->op;
            if ((i->type != IRT_I32 && i->type != IRT_I64) ||
                (op != IR_ADD && op != IR_SUB && op != IR_AND && op != IR_OR && op != IR_XOR))
                continue;
            i->aux = ALUF_NONE;
            i->cc = 0;

            // a + b*c / a - b*c
            if (op == IR_ADD || op == IR_SUB) {
                int mv = op == IR_SUB ? i->b : i->a;
                IRInstr *m = V(mv);
                if (m->op != IR_MUL || m->type != i->type || !foldable_alu_producer(m, i)) {
                    if (op == IR_ADD) {
                        mv = i->b;
                        m = V(mv);
                    }
                }
                if (m->op == IR_MUL && m->type == i->type && foldable_alu_producer(m, i)) {
                    int addend = op == IR_SUB ? i->a : (mv == i->a ? i->b : i->a);
                    i->a = m->a;
                    i->b = m->b;
                    i->c = addend;
                    i->aux = op == IR_ADD ? ALUF_MADD : ALUF_MSUB;
                    be.vi[mv].uses--;
                    be.vi[m->a].uses++;
                    be.vi[m->b].uses++;
                    continue;
                }
            }

            // Commutative operations may put the foldable producer on either side.
            int fv = i->b;
            IRInstr *d = V(fv);
            if (op != IR_SUB && !foldable_alu_producer(d, i)) {
                fv = i->a;
                d = V(fv);
                if (foldable_alu_producer(d, i)) {
                    i->a = i->b;
                    i->b = fv;
                }
            }
            if (!foldable_alu_producer(d, i)) continue;
            int64_t sh;
            if (d->op == IR_SHL && d->type == i->type && const_of(d->b, &sh)) {
                i->b = d->a;
                i->aux = ALUF_SHIFT;
                i->cc = (uint8_t)((uint64_t)sh & (i->type == IRT_I64 ? 63 : 31));
            } else if ((op == IR_ADD || op == IR_SUB) && i->type == IRT_I64 &&
                       (d->op == IR_SEXT32 || d->op == IR_ZEXT32) && V(d->a)->type == IRT_I32) {
                i->b = d->a;
                i->aux = d->op == IR_SEXT32 ? ALUF_SXTW : ALUF_UXTW;
            } else {
                continue;
            }
            be.vi[fv].uses--;
            be.vi[i->b].uses++;
        }
    }
}

static bool one_bit_and(int v, IRBlock *b, int *src_out, int *bit_out) {
    IRInstr *a = V(v);
    if (a->op != IR_AND || a->blk != b || be.vi[a->id].uses != 1) return false;
    int src = IR_NONE;
    int64_t dummy, mask = 0;
    if (const_of(a->b, &mask) && !const_of(a->a, &dummy)) src = a->a;
    else if (const_of(a->a, &mask) && !const_of(a->b, &dummy))
        src = a->b;
    uint64_t bits = a->type == IRT_I32 ? (uint32_t)mask : (uint64_t)mask;
    if (src == IR_NONE || !bits || (bits & (bits - 1))) return false;
    *src_out = src;
    *bit_out = __builtin_ctzll(bits);
    return true;
}

static void prep_ir(void) {
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_UNDEF) {
                if (tflt(i->type)) {
                    i->op = IR_FCONST;
                    i->fimm = 0.0;
                } else {
                    i->op = IR_ICONST;
                    i->imm = 0;
                }
            } else if (i->op == IR_LOAD || i->op == IR_STORE) {
                i->aux = 0;
                i->cc = 0; // index extension (set by fold_addr): 0 none, 1 sxtw, 2 uxtw
                i->sym = NULL; // aux == 2: the symbol
            } else if (i->op == IR_BR) {
                i->mw = 0; // set to 1/2 when a CMP/FCMP is fused into the branch
                i->b = i->c = IR_NONE;
            }
        }
    }
    count_uses();
    // Uses of a value as base/index of a LOAD/STORE in its own block: if all of
    // its uses are such, folding the add into every address removes it.
    be.mu = NEW(int, be.nv + 1);
    for (int pass = 0; pass < 2; pass++) {
        // Pass 0 folds constants/scales; pass 1 additionally splits reg+reg
        // adds, judged on the use counts that pass 0 left behind.
        if (pass) {
            count_uses();
            dce(); // address adds folded away in pass 0 must not inflate use counts
        }
        memset(be.mu, 0, sizeof(int) * (size_t)(be.nv + 1));
        for (int bi = 0; bi < be.nord; bi++) {
            IRBlock *b = be.ord[bi];
            for (int k = 0; k < b->nins; k++) {
                IRInstr *m = b->ins[k];
                if (m->op != IR_LOAD && m->op != IR_STORE) continue;
                if (m->a != IR_NONE && V(m->a)->blk == b) be.mu[m->a]++;
                if (m->b != IR_NONE && V(m->b)->blk == b) be.mu[m->b]++;
            }
        }
        for (int bi = 0; bi < be.nord; bi++) {
            IRBlock *b = be.ord[bi];
            for (int k = 0; k < b->nins; k++)
                if (b->ins[k]->op == IR_LOAD || b->ins[k]->op == IR_STORE) fold_addr(b->ins[k], pass);
        }
    }
    count_uses();
    // Fold a single-use LOAD into the extension/truncation that consumes it
    // (ldrsw/ldrsb/ldrh/... zero or sign extend for free) when nothing in between
    // can write memory or trap.
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *e = b->ins[k];
            if (e->a == IR_NONE || be.vi[e->id].dead) continue;
            IRInstr *l = V(e->a);
            if (l->op != IR_LOAD || l->vol || l->blk != b || be.vi[l->id].uses != 1 || tflt(l->type)) continue; // vol: the fold can narrow the access
            uint8_t mw, sgn = 0;
            switch (e->op) {
            case IR_SEXT32:
            case IR_ZEXT32:
                if (l->type != IRT_I32 || l->mw != IRM_I32) continue;
                mw = IRM_I32;
                sgn = e->op == IR_SEXT32;
                break;
            case IR_SEXT8:
            case IR_ZEXT8:
                if (l->type != IRT_I32 || l->mw > IRM_I32) continue;
                mw = IRM_I8;
                sgn = e->op == IR_SEXT8;
                break;
            case IR_SEXT16:
            case IR_ZEXT16:
                if (l->type != IRT_I32 || l->mw < IRM_I16 || l->mw > IRM_I32) continue;
                mw = IRM_I16;
                sgn = e->op == IR_SEXT16;
                break;
            case IR_TRUNC:
                if (l->type != IRT_I64 || l->mw != IRM_I64) continue;
                mw = IRM_I32;
                break;
            default:
                continue;
            }
            int lk = k - 1;
            while (lk >= 0 && b->ins[lk] != l && !ir_has_side_effects(b->ins[lk])) lk--;
            if (lk < 0 || b->ins[lk] != l) continue;
            e->op = IR_LOAD;
            e->a = l->a;
            e->b = l->b;
            e->c = IR_NONE;
            e->scale = l->scale;
            e->imm = l->imm;
            e->aux = l->aux;
            e->cc = l->cc;
            e->sym = l->sym;
            e->mw = mw;
            e->msign = sgn;
            be.vi[l->id].uses--;
            if (e->a != IR_NONE) be.vi[e->a].uses++;
            if (e->b != IR_NONE) be.vi[e->b].uses++;
        }
    }
    fold_alu_operands();
    // Fuse a single-use compare with the BR that ends its block.
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        IRInstr *br = b->ins[b->nins - 1];
        if (br->op != IR_BR) continue;
        int src, bit;
        IRInstr *c = V(br->a);
        if (one_bit_and(br->a, b, &src, &bit)) {
            be.vi[br->a].uses--;
            be.vi[src].uses++;
            br->mw = 3; // TBZ/TBNZ
            br->cc = IRCC_NE;
            br->a = src;
            br->b = IR_NONE;
            br->imm = bit;
            continue;
        }
        if ((c->op != IR_CMP && c->op != IR_FCMP) || c->blk != b || be.vi[c->id].uses != 1) continue;
        if (c->op == IR_CMP && (c->cc == IRCC_EQ || c->cc == IRCC_NE)) {
            int av = IR_NONE;
            int64_t z;
            if (const_of(c->b, &z) && z == 0) av = c->a;
            else if (const_of(c->a, &z) && z == 0)
                av = c->b;
            if (av != IR_NONE && one_bit_and(av, b, &src, &bit)) {
                br->mw = 3; // TBZ/TBNZ
                br->cc = c->cc;
                br->a = src;
                br->b = IR_NONE;
                br->imm = bit;
                be.vi[c->id].uses--;
                be.vi[src].uses++;
                continue;
            }
        }
        br->mw = c->op == IR_CMP ? 1 : 2;
        br->cc = c->cc;
        br->b = c->b;
        br->a = c->a;
        be.vi[c->id].uses--;
        be.vi[c->a].uses++;
        be.vi[c->b].uses++;
    }
    dce(); // the fused compares are dead now
    free(be.mu);
    be.mu = NULL;
}


// ------------------------------------------------------ 3. liveness + alloc

static bool needs_loc(int v) {
    const IRInstr *i = V(v);
    return !be.vi[v].dead && be.vi[v].uses > 0 && i->type != IRT_VOID && !remat_instr(i) &&
        i->op != IR_NOP;
}

// Instructions that produce machine code (params arrive in registers, consts
// and addresses are rematerialized at their uses).
static bool emitted(const IRInstr *i) {
    return i->op != IR_NOP && i->op != IR_PARAM && i->op != IR_PHI && !remat_instr(i) &&
        !be.vi[i->id].dead;
}

// ------------------------------------------------------ ABI classification

// Callee side: walk the parameter list with the same GP / FP / stack-slot counting the caller
// uses (see arg_place). Mirrors the AST prologue: scalars in x0-x7 / v0-v7, an HFA aggregate in
// consecutive V registers (all or none), any other aggregate <= 8 bytes as a raw value in one GP
// register, larger aggregates as a pointer in one GP register; whatever does not fit goes to
// 8-byte stack slots (an HFA that does not fit takes (size+7)/8 of them). A hidden aggregate
// return pointer arrives in x8 and uses no argument slot.
static void classify_params(void) {
    IRFunc *f = be.f;
    be.pabi = NEW(int, f->nparams + 1);
    be.pstk = NEW(int, f->nparams + 1);
    be.pcls = NEW(int, f->nparams + 1);
    be.paggoff = NEW(int, f->nparams + 1);
    int ng = 0, nx = 0, nst = 0;
    for (int k = 0; k < f->nparams; k++) {
        be.pabi[k] = -1;
        int e = f->param_class ? f->param_class[k] : 0;
        int cls = e & 7, szb = e >> 3;
        be.pcls[k] = cls;
        if (k == 0 && f->ret_sret) {
            be.pabi[k] = ARM64_X8;
        } else if (cls == IRAGG_HFA4 || cls == IRAGG_HFA8) {
            int cnt = szb / (cls == IRAGG_HFA4 ? 4 : 8);
            if (nx + cnt <= 8) {
                be.pabi[k] = nx;
                nx += cnt;
            } else {
                be.pstk[k] = 16 + 8 * nst;
                nst += (szb + 7) / 8;
            }
        } else if (tflt(f->param_types[k])) {
            if (nx < 8) be.pabi[k] = nx++;
            else
                be.pstk[k] = 16 + 8 * nst++;
        } else {
            if (ng < 8) be.pabi[k] = ng++;
            else
                be.pstk[k] = 16 + 8 * nst++;
        }
    }
}

// Caller side: place argument k of CALL c with the same walk. *rg = register number (x0-x7 or
// v0-v7; the first V register of an HFA) or -1 with *slot = the first outgoing 8-byte stack
// slot. Scalars and pointer-class aggregates take one slot; an HFA / VALUE8 aggregate is
// handed over as raw bytes by the emitter. Darwin passes the unnamed arguments of a prototyped
// variadic call on the stack (AST backend: gen_funcall); AAPCS64 on Linux passes them like
// named ones.
static void arg_place(const IRInstr *c, int k, int *ng, int *nx, int *nst, int *rg, int *slot) {
    *rg = -1;
    *slot = -1;
    int e = c->arg_class ? c->arg_class[k] : 0;
    int cls = e & 7, szb = e >> 3;
#ifdef __APPLE__
    if (c->var_from > 0 && k >= c->var_from - 1) {
        *slot = (*nst)++;
        return;
    }
#endif
    if (cls == IRAGG_HFA4 || cls == IRAGG_HFA8) {
        int cnt = szb / (cls == IRAGG_HFA4 ? 4 : 8);
        if (*nx + cnt <= 8) {
            *rg = *nx;
            *nx += cnt;
        } else {
            *slot = *nst;
            *nst += (szb + 7) / 8;
        }
    } else if (cls == IRAGG_NONE && tflt(V(c->args[k])->type)) {
        if (*nx < 8) *rg = (*nx)++;
        else
            *slot = (*nst)++;
    } else {
        if (*ng < 8) *rg = (*ng)++;
        else
            *slot = (*nst)++;
    }
}

static inline void bs_set(uint64_t *b, int i) { b[i >> 6] |= 1ull << (i & 63); }
static inline bool bs_get(const uint64_t *b, int i) { return (b[i >> 6] >> (i & 63)) & 1; }

static void add_range(int v, int from, int to) {
    VInfo *x = &be.vi[v];
    if (x->nrg) {
        Range *r = &x->rg[x->nrg - 1];
        if (to >= r->from) {
            if (from < r->from) r->from = from;
            if (to > r->to) r->to = to;
            return;
        }
    }
    if (x->nrg == x->caprg) {
        x->caprg = x->caprg ? 2 * x->caprg : 4;
        x->rg = realloc(x->rg, sizeof(Range) * (size_t)x->caprg);
        if (!x->rg) {
            fprintf(stderr, "rcc: out of memory\n");
            exit(1);
        }
    }
    x->rg[x->nrg++] = (Range){from, to};
}

// v is defined at `pos`: its lowest range (the one reaching back to the block
// start B0) now begins there.
static void set_from(int v, int B0, int pos) {
    VInfo *x = &be.vi[v];
    if (x->nrg && x->rg[x->nrg - 1].from == B0) x->rg[x->nrg - 1].from = pos;
    else
        add_range(v, pos, pos + 1);
}

static void add_hint(int v, int w) {
    if (w < 0 || v == w || !needs_loc(v) || !needs_loc(w) || rc_of(v) != rc_of(w)) return;
    VInfo *x = &be.vi[v];
    for (int k = 0; k < 3; k++) {
        if (x->hint[k] == w) return;
        if (x->hint[k] < 0) {
            x->hint[k] = w;
            return;
        }
    }
}

static void build_intervals(void) {
    IRFunc *f = be.f;
    int nb = f->nblocks;
    be.bslot = NEW(int, nb);
    be.bterm = NEW(int, nb);
    int slot = 0;
    for (int v = 0; v < be.nv; v++) {
        be.vi[v].start = INT_MAX;
        be.vi[v].end = -1;
        be.vi[v].ipos = -1;
        be.vi[v].slotidx = -1;
        be.vi[v].abi = -1;
        be.vi[v].hint[0] = be.vi[v].hint[1] = be.vi[v].hint[2] = -1;
    }
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        be.bslot[b->id] = slot++;
        for (int k = 0; k < b->nins; k++)
            if (emitted(b->ins[k])) {
                be.vi[b->ins[k]->id].ipos = slot;
                IRInstr *ki = b->ins[k];
                if (ki->op == IR_CALL) be.ncall++;
                slot++;
            }
        be.bterm[b->id] = slot - 1;
    }
    be.callpos = NEW(int, be.ncall + 1);
    int nc = 0;
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++)
            if (emitted(b->ins[k]) && b->ins[k]->op == IR_CALL) be.callpos[nc++] = 2 * be.vi[b->ins[k]->id].ipos;
    }
    // Compact index for values that are live across blocks (used outside their
    // defining block); block-local values need no dataflow, their ranges come
    // out of the backward scan below.
    uint8_t *glob = NEW(uint8_t, be.nv + 1);
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (i->op == IR_NOP || be.vi[i->id].dead) continue;
            for (int u = 0, nu = nuse_slots(i); u < nu; u++) {
                int o = use_at(i, u);
                if (o == IR_NONE || !needs_loc(o)) continue;
                IRBlock *ub = i->op == IR_PHI ? b->preds[u] : b; // phi operands are read at the pred's end
                if (V(o)->blk != ub) glob[o] = 1;
            }
        }
    }
    int *ci = NEW(int, be.nv + 1);
    int *civ = NEW(int, be.nv + 1);
    int nl = 0;
    for (int v = 0; v < be.nv; v++) {
        ci[v] = -1;
        if (glob[v]) {
            ci[v] = nl;
            civ[nl++] = v;
        }
    }
    free(glob);
    int W = (nl + 63) / 64;
    if (W == 0) W = 1;
    size_t sz = (size_t)be.nord * (size_t)W;
    uint64_t *use = NEW(uint64_t, sz), *def = NEW(uint64_t, sz), *in = NEW(uint64_t, sz), *out = NEW(uint64_t, sz);
#define BS(a, bi) ((a) + (size_t)(bi) * (size_t)W)
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        uint64_t *U = BS(use, bi), *D = BS(def, bi);
        for (int k = 0; k < b->nphis; k++)
            if (ci[b->phis[k]->id] >= 0) bs_set(D, ci[b->phis[k]->id]);
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_NOP || be.vi[i->id].dead) continue;
            for (int u = 0, nu = nuse_slots(i); u < nu; u++) {
                int o = use_at(i, u);
                if (o != IR_NONE && ci[o] >= 0 && !bs_get(D, ci[o])) bs_set(U, ci[o]);
            }
            if (ci[i->id] >= 0) bs_set(D, ci[i->id]);
        }
        IRBlock *ss[2];
        int nsu = ir_succs(b, ss);
        for (int s = 0; s < nsu; s++) {
            IRBlock *sb = ss[s];
            int pk = 0;
            while (pk < sb->npreds && sb->preds[pk] != b) pk++;
            for (int k = 0; k < sb->nphis && pk < sb->npreds; k++) {
                IRInstr *p = sb->phis[k];
                int o = p->args[pk];
                if (needs_loc(p->id) && o != IR_NONE && ci[o] >= 0 && !bs_get(D, ci[o])) bs_set(U, ci[o]);
            }
        }
    }
    for (bool changed = true; changed;) {
        changed = false;
        for (int bi = be.nord - 1; bi >= 0; bi--) {
            IRBlock *b = be.ord[bi];
            uint64_t *O = BS(out, bi), *I = BS(in, bi), *U = BS(use, bi), *D = BS(def, bi);
            IRBlock *ss[2];
            int nsu = ir_succs(b, ss);
            for (int s = 0; s < nsu; s++) {
                uint64_t *SI = BS(in, be.oidx[ss[s]->id]);
                for (int w = 0; w < W; w++) O[w] |= SI[w];
            }
            for (int w = 0; w < W; w++) {
                uint64_t ni = U[w] | (O[w] & ~D[w]);
                if (ni != I[w]) {
                    I[w] = ni;
                    changed = true;
                }
            }
        }
    }
    // Live ranges, built backwards over the layout so each value's ranges are
    // discovered in descending position order (Wimmer & Franz style).
    for (int bi = be.nord - 1; bi >= 0; bi--) {
        IRBlock *b = be.ord[bi];
        int B0 = 2 * be.bslot[b->id], E = 2 * be.bterm[b->id] + 2;
        int64_t w = (int64_t)1 << (3 * (b->loop_depth > 8 ? 8 : b->loop_depth));
        uint64_t *O = BS(out, bi);
        for (int x = 0; x < nl; x++)
            if (bs_get(O, x)) add_range(civ[x], B0, E);
        // Phi operands are read by the parallel copy in front of the terminator.
        IRBlock *ss[2];
        int nsu = ir_succs(b, ss);
        for (int s = 0; s < nsu; s++) {
            IRBlock *sb = ss[s];
            int pk = 0;
            while (pk < sb->npreds && sb->preds[pk] != b) pk++;
            for (int k = 0; k < sb->nphis && pk < sb->npreds; k++) {
                IRInstr *p = sb->phis[k];
                int o = p->args[pk];
                if (!needs_loc(p->id) || !needs_loc(o)) continue;
                add_range(o, B0, E - 1);
                be.vi[o].cost += w;
            }
        }
        for (int k = b->nins - 1; k >= 0; k--) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_NOP || be.vi[i->id].dead || i->op == IR_PARAM) continue;
            int pos = 2 * be.vi[i->id].ipos;
            if (needs_loc(i->id)) {
                set_from(i->id, B0, pos + 1);
                be.vi[i->id].cost += w;
            }
            for (int u = 0, nu = nuse_slots(i); u < nu; u++) {
                int o = use_at(i, u);
                if (o == IR_NONE || !needs_loc(o)) continue;
                add_range(o, B0, pos + 1);
                be.vi[o].cost += w;
            }
        }
        for (int k = 0; k < b->nphis; k++) {
            int p = b->phis[k]->id;
            if (!needs_loc(p)) continue;
            set_from(p, B0, B0 + 1);
            be.vi[p].cost += w;
        }
        if (bi == 0) {
            for (int k = 0; k < b->nins; k++) {
                IRInstr *i = b->ins[k];
                if (i->op != IR_PARAM || !needs_loc(i->id)) continue;
                set_from(i->id, B0, B0 + 1);
                be.vi[i->id].abi = (int8_t)be.pabi[i->imm];
                be.vi[i->id].stk = (int16_t)be.pstk[i->imm];
                be.vi[i->id].cost += w;
            }
        }
    }
    for (int v = 0; v < be.nv; v++) {
        VInfo *x = &be.vi[v];
        for (int a = 0, z = x->nrg - 1; a < z; a++, z--) {
            Range t = x->rg[a];
            x->rg[a] = x->rg[z];
            x->rg[z] = t;
        }
        if (x->nrg) {
            x->start = x->rg[0].from;
            x->end = x->rg[x->nrg - 1].to;
        }
    }
#undef BS
    // Register hints: phis with their operands, two-address ops with the first
    // operand (the destination wants to reuse it when it dies).
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nphis; k++) {
            IRInstr *p = b->phis[k];
            for (int a = 0; a < p->nargs; a++) {
                add_hint(p->id, p->args[a]);
                add_hint(p->args[a], p->id);
            }
        }
    }
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_NOP || be.vi[i->id].dead) continue;
            switch (i->op) {
            case IR_ADD:
            case IR_MUL:
            case IR_AND:
            case IR_OR:
            case IR_XOR:
            case IR_FADD:
            case IR_FMUL:
                add_hint(i->id, i->a);
                add_hint(i->id, i->b);
                break;
            case IR_SUB:
            case IR_SHL:
            case IR_SHR:
            case IR_SAR:
            case IR_FSUB:
            case IR_FDIV:
            case IR_NEG:
            case IR_NOT:
            case IR_FNEG:
            case IR_SEXT8:
            case IR_SEXT16:
            case IR_ZEXT8:
            case IR_ZEXT16:
            case IR_SEXT32:
            case IR_ZEXT32:
            case IR_TRUNC:
            case IR_FEXT:
            case IR_FTRUNC:
                add_hint(i->id, i->a);
                break;
            case IR_CALL: { // an argument computed into its ABI register needs no move at the call
                int ng = 0, nx = 0, nst = 0;
                for (int a = 0; a < i->nargs; a++) {
                    int v = i->args[a];
                    int rg, slot;
                    arg_place(i, a, &ng, &nx, &nst, &rg, &slot);
                    int cls = i->arg_class ? i->arg_class[a] & 7 : 0; // raw VALUE8/HFA bytes are loaded from memory
                    if (rg >= 0 && cls != IRAGG_VALUE8 && cls != IRAGG_HFA4 && cls != IRAGG_HFA8 && needs_loc(v) &&
                        be.vi[v].abi < 0 && !be.vi[v].stk)
                        be.vi[v].abi = (int8_t)rg;
                }
                // the result is born in x0 / v0
                if (needs_loc(i->id) && be.vi[i->id].abi < 0) be.vi[i->id].abi = 0;
                break;
            }
            case IR_RET: // the returned value belongs in x0 / v0
                if (i->a != IR_NONE && needs_loc(i->a) && be.vi[i->a].abi < 0 && !be.vi[i->a].stk) be.vi[i->a].abi = 0;
                break;
            default:
                break;
            }
        }
    }
    free(ci);
    free(civ);
    free(use);
    free(def);
    free(in);
    free(out);
}

// Does v live across some call, i.e. is it live both at the call's use and at
// its def position (arguments die at the use, results are born at the def)?
static bool crosses_call(int v) {
    const VInfo *x = &be.vi[v];
    for (int k = 0; k < x->nrg; k++) {
        int lo = 0, hi = be.ncall;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (be.callpos[mid] >= x->rg[k].from) hi = mid;
            else
                lo = mid + 1;
        }
        if (lo < be.ncall && be.callpos[lo] + 2 <= x->rg[k].to) return true;
    }
    return false;
}

static bool covers(int v, int p) {
    const VInfo *x = &be.vi[v];
    int lo = 0, hi = x->nrg;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (x->rg[mid].to <= p) lo = mid + 1;
        else
            hi = mid;
    }
    return lo < x->nrg && x->rg[lo].from <= p;
}

static bool intersects(int a, int b) {
    const VInfo *x = &be.vi[a], *y = &be.vi[b];
    int i = 0, j = 0;
    while (i < x->nrg && j < y->nrg) {
        if (x->rg[i].to <= y->rg[j].from) i++;
        else if (y->rg[j].to <= x->rg[i].from)
            j++;
        else
            return true;
    }
    return false;
}

static int cmp_start(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    if (be.vi[x].start != be.vi[y].start) return be.vi[x].start < be.vi[y].start ? -1 : 1;
    return x - y;
}

static void list_del(int *l, int *n, int k) { l[k] = l[--*n]; }

// Linear scan over intervals with lifetime holes. An interval that spills is
// spilled for its whole life (no splitting); eviction picks the cheapest set
// of conflicting intervals by loop-weighted use count.
static void regalloc(void) {
    int *ids = NEW(int, be.nv + 1);
    int n = 0;
    for (int v = 0; v < be.nv; v++) {
        Loc *l = LOC(v);
        l->kind = LK_NONE;
        if (!be.vi[v].dead && be.vi[v].uses > 0 && remat_instr(V(v))) l->kind = LK_REMAT;
        else if (needs_loc(v) && be.vi[v].nrg)
            ids[n++] = v;
    }
    qsort(ids, (size_t)n, sizeof(int), cmp_start);
    int *act = NEW(int, n + 1), *inact = NEW(int, n + 1);
    int nact = 0, nin = 0;
    for (int ii = 0; ii < n; ii++) {
        int v = ids[ii];
        int c = rc_of(v);
        VInfo *x = &be.vi[v];
        int p = x->start;
        for (int k = 0; k < nact;) {
            int a = act[k];
            if (LOC(a)->kind != LK_REG || be.vi[a].end <= p) {
                list_del(act, &nact, k);
            } else if (!covers(a, p)) {
                inact[nin++] = a;
                list_del(act, &nact, k);
            } else {
                k++;
            }
        }
        for (int k = 0; k < nin;) {
            int a = inact[k];
            if (LOC(a)->kind != LK_REG || be.vi[a].end <= p) {
                list_del(inact, &nin, k);
            } else if (covers(a, p)) {
                act[nact++] = a;
                list_del(inact, &nin, k);
            } else {
                k++;
            }
        }
        uint32_t allowed;
        if (c == RC_G) {
            allowed = crosses_call(v) ? MASK_CALLEE_G : MASK_G_ALL;
        } else {
            allowed = crosses_call(v) ? MASK_CALLEE_X : MASK_X_ALL;
        }
        uint32_t blocked = 0;
        for (int k = 0; k < nact; k++)
            if (rc_of(act[k]) == c) blocked |= 1u << LOC(act[k])->reg;
        for (int k = 0; k < nin; k++)
            if (rc_of(inact[k]) == c && intersects(inact[k], v)) blocked |= 1u << LOC(inact[k])->reg;
        uint32_t avail = allowed & ~blocked;
        int pick = -1;
        if (x->abi >= 0 && (avail >> x->abi & 1)) pick = x->abi;
        for (int k = 0; pick < 0 && k < 3; k++) {
            int h = x->hint[k];
            if (h < 0 || LOC(h)->kind != LK_REG) continue;
            if (avail >> LOC(h)->reg & 1) pick = LOC(h)->reg;
        }
        if (pick < 0) {
            const int8_t *order = c == RC_G ? gpr_alloc_order : xmm_alloc_order;
            size_t no = c == RC_G ? sizeof gpr_alloc_order : sizeof xmm_alloc_order;
            for (size_t k = 0; k < no; k++)
                if (avail >> order[k] & 1) {
                    pick = order[k];
                    break;
                }
        }
        if (pick < 0 && allowed) {
            // Evict every conflicting interval of the cheapest register if
            // that is cheaper than spilling this one.
            int64_t best_cost = 0;
            int best = -1;
            for (int r = 0; r < 32; r++) {
                if (!(allowed >> r & 1)) continue;
                int64_t sum = 0;
                for (int k = 0; k < nact; k++)
                    if (rc_of(act[k]) == c && LOC(act[k])->reg == r) sum += be.vi[act[k]].cost;
                for (int k = 0; k < nin; k++)
                    if (rc_of(inact[k]) == c && LOC(inact[k])->reg == r && intersects(inact[k], v))
                        sum += be.vi[inact[k]].cost;
                if (best < 0 || sum < best_cost) {
                    best = r;
                    best_cost = sum;
                }
            }
            if (best >= 0 && best_cost < x->cost) {
                for (int k = 0; k < nact; k++)
                    if (rc_of(act[k]) == c && LOC(act[k])->reg == best) LOC(act[k])->kind = LK_SPILL;
                for (int k = 0; k < nin; k++)
                    if (rc_of(inact[k]) == c && LOC(inact[k])->reg == best && intersects(inact[k], v))
                        LOC(inact[k])->kind = LK_SPILL;
                pick = best;
            }
        }
        if (pick < 0) {
            LOC(v)->kind = LK_SPILL;
            continue;
        }
        LOC(v)->kind = LK_REG;
        LOC(v)->reg = (int8_t)pick;
        act[nact++] = v;
    }
    // Spill slots, shared between non-overlapping intervals of the same class.
    int *slot_end = NEW(int, n + 1);
    for (int c = 0; c < 2; c++) {
        int nslots = 0;
        for (int ii = 0; ii < n; ii++) {
            int v = ids[ii];
            if (rc_of(v) != c || LOC(v)->kind != LK_SPILL) continue;
            if (be.vi[v].stk) {
                LOC(v)->slot = -be.vi[v].stk;
                continue;
            }
            int s = -1;
            for (int k = 0; k < nslots; k++)
                if (slot_end[k] <= be.vi[v].start) {
                    s = k;
                    break;
                }
            if (s < 0) s = nslots++;
            slot_end[s] = be.vi[v].end;
            be.vi[v].slotidx = s;
        }
        be.nspill[c] = nslots;
    }
    free(slot_end);
    free(act);
    free(inact);
    free(ids);
}

// ------------------------------------------------------------ 4. emission

static const char *blabel(IRBlock *b) {
    if (!be.lbl[b->id]) be.lbl[b->id] = format(".L.ir.%s.%d.%d", be.sym, be.serial, b->id);
    return be.lbl[b->id];
}

static IRBlock *resolve(IRBlock *b) {
    for (int g = 0; g < be.f->nblocks && be.tramp[b->id]; g++) b = b->ins[b->nins - 1]->t;
    return b;
}

// ------------------------------------------------------------ 4. emission helpers

static inline void emit32(uint32_t w) { secbuf_emit32le(S, w); }

static void mov_rr(int sf, Arm64Reg rd, Arm64Reg rn) {
    if (rd == rn) return; // upper half of an I32 is unspecified anyway
    arm64_orr_reg(S, sf, rd, ZR, rn, ARM64_LSL, 0);
}

// 64-bit (sf) or zero-extended 32-bit constant into rd.
static void mov_imm(Arm64Reg rd, int sf, int64_t val) {
    uint64_t v = sf ? (uint64_t)val : (uint64_t)(uint32_t)val;
    int nh = sf ? 4 : 2, nz = 0, nf = 0, fz = -1, ff = -1;
    for (int i = 0; i < nh; i++) {
        unsigned h = (unsigned)((v >> (16 * i)) & 0xffff);
        if (h) {
            nz++;
            if (fz < 0) fz = i;
        }
        if (h != 0xffff) {
            nf++;
            if (ff < 0) ff = i;
        }
    }
    if (nz == 0) {
        arm64_movz(S, sf, rd, 0, 0);
        return;
    }
    if (nz == 1) {
        arm64_movz(S, sf, rd, (uint16_t)((v >> (16 * fz)) & 0xffff), (uint16_t)(16 * fz));
        return;
    }
    if (nf == 0) {
        arm64_movn(S, sf, rd, 0, 0);
        return;
    }
    if (nf == 1) {
        arm64_movn(S, sf, rd, (uint16_t)((~v >> (16 * ff)) & 0xffff), (uint16_t)(16 * ff));
        return;
    }
    int N, r, s;
    if (arm64_encode_logic_imm(sf, v, &N, &r, &s)) {
        arm64_orr_imm(S, sf, rd, ZR, v);
        return;
    }
    if (nz <= nf) {
        arm64_movz(S, sf, rd, (uint16_t)((v >> (16 * fz)) & 0xffff), (uint16_t)(16 * fz));
        for (int i = fz + 1; i < nh; i++) {
            unsigned h = (unsigned)((v >> (16 * i)) & 0xffff);
            if (h) arm64_movk(S, sf, rd, (uint16_t)h, (uint16_t)(16 * i));
        }
    } else {
        arm64_movn(S, sf, rd, (uint16_t)((~v >> (16 * ff)) & 0xffff), (uint16_t)(16 * ff));
        for (int i = ff + 1; i < nh; i++) {
            unsigned h = (unsigned)((v >> (16 * i)) & 0xffff);
            if (h != 0xffff) arm64_movk(S, sf, rd, (uint16_t)h, (uint16_t)(16 * i));
        }
    }
}

// add/sub immediate operand: v (>= 0) as imm12 or imm12 << 12.
static bool arith_imm(uint64_t v, int *imm, int *sh) {
    if (v < 4096) {
        *imm = (int)v;
        *sh = 0;
        return true;
    }
    if (v < (1u << 24) && (v & 0xfff) == 0) {
        *imm = (int)(v >> 12);
        *sh = 1;
        return true;
    }
    return false;
}

// rd = rn + imm (64-bit). rn may be SP. `tmp` is clobbered when the immediate needs a
// register; it must differ from rn.
static void add_imm64(Arm64Reg rd, Arm64Reg rn, int64_t imm, Arm64Reg tmp) {
    if (imm == 0) {
        if (rd != rn) arm64_add_imm(S, 1, rd, rn, 0, 0);
        return;
    }
    bool neg = imm < 0;
    uint64_t u = neg ? 0 - (uint64_t)imm : (uint64_t)imm;
    int i12, sh;
    if (arith_imm(u, &i12, &sh)) {
        if (neg) arm64_sub_imm(S, 1, rd, rn, i12, (uint32_t)sh);
        else
            arm64_add_imm(S, 1, rd, rn, i12, (uint16_t)sh);
    } else if (u < (1u << 24)) {
        int hi = (int)(u >> 12), lo = (int)(u & 0xfff);
        if (neg) {
            arm64_sub_imm(S, 1, rd, rn, hi, 1);
            if (lo) arm64_sub_imm(S, 1, rd, rd, lo, 0);
        } else {
            arm64_add_imm(S, 1, rd, rn, hi, 1);
            if (lo) arm64_add_imm(S, 1, rd, rd, lo, 0);
        }
    } else {
        mov_imm(tmp, 1, imm);
        arm64_add_extreg(S, 1, rd, rn, tmp, ARM64_UXTX, 0);
    }
}

// Load/store with a base, optional index (index << sh, extended per `ext`: 0 = 64-bit register,
// 1 = SXTW, 2 = UXTW of a W register) and displacement. szl = log2 of the access size; v =
// FP/SIMD register; opc: 0 store, 1 load, 2 sign-extending load into an X register, 3
// sign-extending load into a W register. With `sym`, `base` holds the ADRP page of the symbol
// and the access is [base, :lo12:sym + disp] (disp >= 0 and a multiple of the access size; the
// relocation carries it as addend). Temps: T3 holds a computed address, T2 a materialized
// displacement (callers load their index operand into T2 only, and it is dead by the time the
// displacement needs a temp).
static void ldst_x(int szl, int v, int opc, int rt, Arm64Reg base, Arm64Reg idx, int ext, int sh, int64_t disp,
                   const char *sym) {
    if (sym) {
        size_t off = S->len;
        emit32(((uint32_t)szl << 30) | 0x39000000u | ((uint32_t)v << 26) | ((uint32_t)opc << 22) |
               ((uint32_t)base << 5) | (uint32_t)rt);
        static const uint32_t lo12[] = {R_AARCH64_LDST8_ABS_LO12_NC, R_AARCH64_LDST16_ABS_LO12_NC,
                                        R_AARCH64_LDST32_ABS_LO12_NC, R_AARCH64_LDST64_ABS_LO12_NC};
        objfile_add_reloc(cg_obj, SEC_TEXT, off, cg_ir_sym_index(sym), lo12[szl], disp);
        return;
    }
    if (idx != (Arm64Reg)-1) {
        if (disp == 0 && (sh == 0 || sh == szl)) {
            emit32(((uint32_t)szl << 30) | 0x38200800u | ((uint32_t)v << 26) | ((uint32_t)opc << 22) |
                   ((uint32_t)idx << 16) | ((ext == 1 ? 6u : ext == 2 ? 2u
                                                                      : 3u)
                                            << 13) |
                   ((uint32_t)(sh != 0) << 12) | ((uint32_t)base << 5) | (uint32_t)rt);
            return;
        }
        arm64_add_extreg(S, 1, T3, base, idx, ext == 1 ? ARM64_SXTW : ext == 2 ? ARM64_UXTW
                                                                               : ARM64_UXTX,
                         sh);
        base = T3;
    }
    int size = 1 << szl;
    if (disp >= 0 && disp % size == 0 && disp / size < 4096) {
        emit32(((uint32_t)szl << 30) | 0x39000000u | ((uint32_t)v << 26) | ((uint32_t)opc << 22) |
               ((uint32_t)(disp / size) << 10) | ((uint32_t)base << 5) | (uint32_t)rt);
    } else if (disp >= -256 && disp <= 255) {
        emit32(((uint32_t)szl << 30) | 0x38000000u | ((uint32_t)v << 26) | ((uint32_t)opc << 22) |
               (((uint32_t)disp & 0x1ff) << 12) | ((uint32_t)base << 5) | (uint32_t)rt);
    } else {
        // A GPR load can build its own address in the destination register, leaving T2/T3 (possibly
        // holding other operands of the instruction being emitted) alone.
        Arm64Reg t = (opc != 0 && !v && (Arm64Reg)rt != base) ? (Arm64Reg)rt : T3;
        add_imm64(t, base, disp, t == T3 ? T2 : t);
        emit32(((uint32_t)szl << 30) | 0x39000000u | ((uint32_t)v << 26) | ((uint32_t)opc << 22) |
               ((uint32_t)t << 5) | (uint32_t)rt);
    }
}
#define ldst(szl, v, opc, rt, base, idx, sh, disp) ldst_x(szl, v, opc, rt, base, idx, 0, sh, disp, NULL)
#define NOIDX ((Arm64Reg)-1)

static inline int64_t slot_off(int slot) { return (int64_t)be.frame - slot; } // sp-relative offset

static void ld_slot_g(Arm64Reg rd, int slot, int ty) { ldst(tsz(ty) == 4 ? 2 : 3, 0, 1, rd, SPREG, NOIDX, 0, slot_off(slot)); }
static void st_slot_g(Arm64Reg rs, int slot, int ty) { ldst(tsz(ty) == 4 ? 2 : 3, 0, 0, rs, SPREG, NOIDX, 0, slot_off(slot)); }
static void ld_slot_x(int xd, int slot, int ty) { ldst(tsz(ty) == 4 ? 2 : 3, 1, 1, xd, SPREG, NOIDX, 0, slot_off(slot)); }
static void st_slot_x(int xs, int slot, int ty) { ldst(tsz(ty) == 4 ? 2 : 3, 1, 0, xs, SPREG, NOIDX, 0, slot_off(slot)); }

static void fmov_dd(int rd, int rn) {
    if (rd != rn) emit32(0x1e604000u | ((uint32_t)rn << 5) | (uint32_t)rd);
}

// FMOV imm8 encodable value (the 8-bit float format shared by single and double).
static bool fp_imm8(double v, int *out) {
    for (int i = 0; i < 256; i++) {
        int s = i >> 7, b = (i >> 6) & 1, cd = (i >> 4) & 3, efgh = i & 15;
        int e = b ? cd - 3 : cd + 1;
        double x = ldexp(1.0 + efgh / 16.0, e);
        if (s) x = -x;
        if (x == v && signbit(x) == signbit(v)) {
            *out = i;
            return true;
        }
    }
    return false;
}

static bool const_of(int v, int64_t *c);

// Materialize a rematerializable value (constant / address) into GPR rd. May use T3.
static void mat_g(int v, Arm64Reg rd) {
    IRInstr *d = V(v);
    switch (d->op) {
    case IR_ICONST:
        mov_imm(rd, d->type == IRT_I64, d->type == IRT_I64 ? d->imm : (int64_t)(int32_t)d->imm);
        break;
    case IR_GADDR:
        cg_ir_sym_addr(rd, d->sym, d->msign);
        if (d->imm) add_imm64(rd, rd, d->imm, T3);
        break;
    case IR_FRAMEADDR:
        add_imm64(rd, SPREG, slot_off((int)d->imm), T3);
        break;
    case IR_PARAM: // materialized aggregate parameter: the value is its frame temp
        add_imm64(rd, SPREG, slot_off(be.paggoff[d->imm]), T3);
        break;
    default:
        abort();
    }
}

// Materialize an FP constant into v<xd>. Uses T3.
static void mat_x(int v, int xd) {
    IRInstr *d = V(v);
    bool dbl = d->type == IRT_F64;
    int imm8;
    uint64_t bits;
    if (dbl) memcpy(&bits, &d->fimm, 8);
    else {
        float f = (float)d->fimm;
        uint32_t b32;
        memcpy(&b32, &f, 4);
        bits = b32;
    }
    if (bits == 0) arm64_fmov_i2f(S, dbl, (Arm64Reg)xd, ZR);
    else if (fp_imm8(dbl ? d->fimm : (double)(float)d->fimm, &imm8))
        arm64_fmov_imm(S, dbl, (Arm64Reg)xd, (uint8_t)imm8);
    else {
        mov_imm(T3, dbl, (int64_t)bits);
        arm64_fmov_i2f(S, dbl, (Arm64Reg)xd, T3);
    }
}

// Operand access. use_g returns the register holding v, loading it into `tmp` when it is
// spilled or rematerialized.
static Arm64Reg use_g(int v, Arm64Reg tmp) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) return (Arm64Reg)l->reg;
    if (l->kind == LK_SPILL) ld_slot_g(tmp, l->slot, V(v)->type);
    else
        mat_g(v, tmp);
    return tmp;
}
// Like use_g, but the constant 0 becomes the zero register (only for operand positions where
// register 31 means XZR, never for add/sub immediate, extended-register or address operands).
static Arm64Reg use_g0(int v, Arm64Reg tmp) {
    int64_t c;
    if (LOC(v)->kind == LK_REMAT && const_of(v, &c) && c == 0) return ZR;
    return use_g(v, tmp);
}
static void load_g(Arm64Reg rd, int v) {
    Arm64Reg r = use_g(v, rd);
    mov_rr(V(v)->type == IRT_I64, rd, r);
}
static Arm64Reg def_g(int v) { return LOC(v)->kind == LK_REG ? (Arm64Reg)LOC(v)->reg : T0; }
static void fin_g(int v, Arm64Reg r) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) mov_rr(V(v)->type == IRT_I64, (Arm64Reg)l->reg, r);
    else if (l->kind == LK_SPILL)
        st_slot_g(r, l->slot, V(v)->type);
}

static int use_x(int v, int tmp) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) return l->reg;
    if (l->kind == LK_SPILL) ld_slot_x(tmp, l->slot, V(v)->type);
    else
        mat_x(v, tmp);
    return tmp;
}
static void load_x(int xd, int v) {
    int r = use_x(v, xd);
    fmov_dd(xd, r);
}
static int def_x(int v) { return LOC(v)->kind == LK_REG ? LOC(v)->reg : XT0; }
static void fin_x(int v, int x) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) fmov_dd(l->reg, x);
    else if (l->kind == LK_SPILL)
        st_slot_x(x, l->slot, V(v)->type);
}

// ------------------------------------------------------ parallel moves

// ------------------------------------------------------ parallel moves

enum { PL_G,
       PL_X,
       PL_M,
       PL_C };
typedef struct {
    uint8_t k;
    int r; // register number (PL_G / PL_X)
    int off; // PL_M: [rbp - off]
    int val; // PL_C: remat value id
} PL;
typedef struct {
    PL d, s;
    int ty;
} Mv;

static PL pl_reg(int cls, int r) { return (PL){cls == RC_X ? PL_X : PL_G, r, 0, 0}; }
static PL pl_of(int v) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) return pl_reg(rc_of(v), l->reg);
    if (l->kind == LK_SPILL) return (PL){PL_M, 0, l->slot, 0};
    return (PL){PL_C, 0, 0, v};
}
static bool pl_eq(PL a, PL b) {
    if (a.k != b.k || a.k == PL_C) return false;
    return a.k == PL_M ? a.off == b.off : a.r == b.r;
}

static void mv_emit(PL d, PL s, int ty) {
    int sz = tsz(ty);
    switch (d.k) {
    case PL_G:
        if (s.k == PL_G) mov_rr(sz == 8, d.r, s.r);
        else if (s.k == PL_M)
            ld_slot_g(d.r, s.off, ty);
        else
            mat_g(s.val, d.r);
        break;
    case PL_X:
        if (s.k == PL_X) fmov_dd(d.r, s.r);
        else if (s.k == PL_M)
            ld_slot_x(d.r, s.off, ty);
        else
            mat_x(s.val, d.r);
        break;
    default:
        if (s.k == PL_G) {
            st_slot_g(s.r, d.off, ty);
        } else if (s.k == PL_X) {
            st_slot_x(s.r, d.off, ty);
        } else if (s.k == PL_M) {
            ld_slot_g(R_S0, s.off, ty);
            st_slot_g(R_S0, d.off, ty);
        } else if (tflt(ty)) {
            mat_x(s.val, X_S0);
            st_slot_x(X_S0, d.off, ty);
        } else {
            st_slot_g(use_g0(s.val, R_S0), d.off, ty);
        }
        break;
    }
}

// Performs all moves as if simultaneously; destinations are distinct. Cycles
// are broken through r11 / xmm15 (never allocated, never a move endpoint).
static void pm_run(Mv *m, int n) {
    int *readers = NEW(int, n), *stack = NEW(int, n);
    uint8_t *done = NEW(uint8_t, n);
    int left = 0, sp = 0;
    for (int i = 0; i < n; i++) {
        if (pl_eq(m[i].d, m[i].s)) done[i] = 1;
        else
            left++;
    }
    for (int i = 0; i < n; i++) {
        if (done[i]) continue;
        for (int j = 0; j < n; j++)
            if (!done[j] && j != i && pl_eq(m[j].s, m[i].d)) readers[i]++;
    }
    for (int i = 0; i < n; i++)
        if (!done[i] && readers[i] == 0) stack[sp++] = i;
    while (left) {
        if (sp) {
            int i = stack[--sp];
            mv_emit(m[i].d, m[i].s, m[i].ty);
            done[i] = 1;
            left--;
            for (int k = 0; k < n; k++)
                if (!done[k] && pl_eq(m[k].d, m[i].s) && --readers[k] == 0) stack[sp++] = k;
        } else {
            int i = 0;
            while (done[i]) i++;
            PL d = m[i].d;
            PL t = tflt(m[i].ty) ? pl_reg(RC_X, X_S1) : pl_reg(RC_G, R_S1);
            // The saved node may be read at another width: copy all 64 bits.
            mv_emit(t, d, tflt(m[i].ty) ? IRT_F64 : IRT_I64);
            for (int j = 0; j < n; j++)
                if (!done[j] && pl_eq(m[j].s, d)) {
                    m[j].s = t;
                    readers[i]--;
                }
            stack[sp++] = i;
        }
    }
    free(readers);
    free(stack);
    free(done);
}

// Moves for the phis of `s` along the edge p -> s.
static int edge_moves(IRBlock *p, IRBlock *s, Mv **out) {
    int k = 0;
    while (k < s->npreds && s->preds[k] != p) k++;
    Mv *m = NEW(Mv, s->nphis + 1);
    int n = 0;
    for (int i = 0; i < s->nphis && k < s->npreds; i++) {
        IRInstr *ph = s->phis[i];
        if (!needs_loc(ph->id) || LOC(ph->id)->kind == LK_NONE) continue;
        m[n++] = (Mv){pl_of(ph->id), pl_of(ph->args[k]), ph->type};
    }
    *out = m;
    return n;
}

static int edge_real_moves(IRBlock *p, IRBlock *s) {
    Mv *m;
    int n = edge_moves(p, s, &m), r = 0;
    for (int i = 0; i < n; i++)
        if (!pl_eq(m[i].d, m[i].s)) r++;
    free(m);
    return r;
}

// ------------------------------------------------------ instruction selection

static const uint8_t cc_arm[] = {
    [IRCC_EQ] = ARM64_EQ,
    [IRCC_NE] = ARM64_NE,
    [IRCC_SLT] = ARM64_LT,
    [IRCC_SLE] = ARM64_LE,
    [IRCC_SGT] = ARM64_GT,
    [IRCC_SGE] = ARM64_GE,
    [IRCC_ULT] = ARM64_LO,
    [IRCC_ULE] = ARM64_LS,
    [IRCC_UGT] = ARM64_HI,
    [IRCC_UGE] = ARM64_HS,
    // After fcmp/fcmpe an unordered result is N=0 Z=0 C=1 V=1, so MI / LS / GT / GE are all false
    // for NaN, EQ is false and NE true: exactly the C semantics. The complement condition
    // (cc ^ 1) is the exact complement on every flag state, so inverted branches are right too.
    [IRCC_FEQ] = ARM64_EQ,
    [IRCC_FNE] = ARM64_NE,
    [IRCC_FLT] = ARM64_MI,
    [IRCC_FLE] = ARM64_LS,
    [IRCC_FGT] = ARM64_GT,
    [IRCC_FGE] = ARM64_GE,
};

static void emit_mul3(int sf, bool sub, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Reg ra) {
    emit32(((uint32_t)sf << 31) | 0x1b000000u | (sub ? 0x8000u : 0) | ((uint32_t)rm << 16) |
           ((uint32_t)ra << 10) | ((uint32_t)rn << 5) | (uint32_t)rd);
}

static int log2i(uint64_t v) {
    int n = 0;
    while (v > 1) {
        v >>= 1;
        n++;
    }
    return n;
}

static void emit_alu(IRInstr *i) {
    int sf = i->type == IRT_I64;
    int op = i->op, a = i->a, b = i->b;
    int64_t c, c2;
    if (i->aux == ALUF_NONE && op != IR_SUB && const_of(a, &c) && !const_of(b, &c2)) {
        int t = a;
        a = b;
        b = t;
    }
    Arm64Reg rd = def_g(i->id);
    if (i->aux == ALUF_MADD || i->aux == ALUF_MSUB) {
        Arm64Reg ra = use_g(a, T1);
        Arm64Reg rb = use_g0(b, T2);
        Arm64Reg rc = use_g(i->c, T3);
        if (i->aux == ALUF_MADD) arm64_madd(S, sf, rd, ra, rb, rc);
        else
            arm64_msub(S, sf, rd, ra, rb, rc);
        fin_g(i->id, rd);
        return;
    }
    Arm64Reg ra = use_g(a, T1);
    if (i->aux == ALUF_NONE && const_of(b, &c)) {
        int64_t cv = sf ? c : (int64_t)(int32_t)c;
        uint64_t uv = sf ? (uint64_t)cv : (uint64_t)(uint32_t)cv;
        bool done = true;
        if (op == IR_ADD || op == IR_SUB) {
            int64_t k = op == IR_ADD ? cv : (int64_t)(0 - (uint64_t)cv);
            if (!sf) k = (int32_t)k;
            bool neg = k < 0;
            uint64_t u = neg ? 0 - (uint64_t)k : (uint64_t)k;
            int i12, sh;
            if (arith_imm(u, &i12, &sh)) {
                if (neg) arm64_sub_imm(S, sf, rd, ra, i12, (uint32_t)sh);
                else
                    arm64_add_imm(S, sf, rd, ra, i12, (uint16_t)sh);
            } else {
                done = false;
            }
        } else if (op == IR_MUL) {
            if (cv > 0 && (cv & (cv - 1)) == 0) {
                if (cv == 1) mov_rr(sf, rd, ra);
                else
                    arm64_lsl_imm(S, sf, rd, ra, log2i((uint64_t)cv));
            } else {
                done = false;
            }
        } else { // AND / OR / XOR
            int N, r, s;
            if (arm64_encode_logic_imm(sf, uv, &N, &r, &s)) {
                if (op == IR_AND) arm64_and_imm(S, sf, rd, ra, uv);
                else if (op == IR_OR)
                    arm64_orr_imm(S, sf, rd, ra, uv);
                else
                    arm64_eor_imm(S, sf, rd, ra, uv);
            } else {
                done = false;
            }
        }
        if (done) {
            fin_g(i->id, rd);
            return;
        }
    }
    Arm64Reg rb = use_g0(b, T2);
    if (i->aux == ALUF_SXTW || i->aux == ALUF_UXTW) {
        Arm64Ext ext = i->aux == ALUF_SXTW ? ARM64_SXTW : ARM64_UXTW;
        if (op == IR_ADD) arm64_add_extreg(S, 1, rd, ra, rb, ext, 0);
        else
            arm64_sub_extreg(S, 1, rd, ra, rb, ext, 0);
    } else {
        int sh = i->aux == ALUF_SHIFT ? i->cc : 0;
        switch (op) {
        case IR_ADD: arm64_add_reg(S, sf, rd, ra, rb, ARM64_LSL, sh); break;
        case IR_SUB: arm64_sub_reg(S, sf, rd, ra, rb, ARM64_LSL, sh); break;
        case IR_MUL: arm64_mul(S, sf, rd, ra, rb); break;
        case IR_AND: arm64_and_reg(S, sf, rd, ra, rb, ARM64_LSL, sh); break;
        case IR_OR: arm64_orr_reg(S, sf, rd, ra, rb, ARM64_LSL, sh); break;
        default: arm64_eor_reg(S, sf, rd, ra, rb, ARM64_LSL, sh); break;
        }
    }
    fin_g(i->id, rd);
}

static void emit_shift(IRInstr *i) {
    int sf = i->type == IRT_I64;
    int64_t c;
    Arm64Reg rd = def_g(i->id);
    Arm64Reg ra = use_g(i->a, T1);
    if (const_of(i->b, &c)) {
        int n = (int)(c & (sf ? 63 : 31));
        if (i->op == IR_SHL) arm64_lsl_imm(S, sf, rd, ra, n);
        else if (i->op == IR_SHR)
            arm64_lsr_imm(S, sf, rd, ra, n);
        else
            arm64_asr_imm(S, sf, rd, ra, n);
    } else {
        Arm64Reg rb = use_g(i->b, T2);
        if (i->op == IR_SHL) arm64_lsl_reg(S, sf, rd, ra, rb);
        else if (i->op == IR_SHR)
            arm64_lsr_reg(S, sf, rd, ra, rb);
        else
            arm64_asr_reg(S, sf, rd, ra, rb);
    }
    fin_g(i->id, rd);
}

static void emit_div(IRInstr *i) {
    int sf = i->type == IRT_I64;
    Arm64Reg rd = def_g(i->id);
    Arm64Reg ra = use_g(i->a, T1);
    Arm64Reg rb = use_g(i->b, T2);
    bool sgn = i->op == IR_SDIV || i->op == IR_SREM;
    bool rem = i->op == IR_SREM || i->op == IR_UREM;
    Arm64Reg q = rem ? T3 : rd;
    if (sgn) arm64_sdiv(S, sf, q, ra, rb);
    else
        arm64_udiv(S, sf, q, ra, rb);
    if (rem) emit_mul3(sf, true, rd, q, rb, ra); // rd = ra - q * rb
    fin_g(i->id, rd);
}

static void emit_unary(IRInstr *i) {
    int sf = i->type == IRT_I64;
    Arm64Reg rd = def_g(i->id);
    Arm64Reg ra = use_g(i->a, T1);
    if (i->op == IR_NEG) arm64_sub_reg(S, sf, rd, ZR, ra, ARM64_LSL, 0);
    else
        arm64_mvn(S, sf, rd, ra, ARM64_LSL, 0);
    fin_g(i->id, rd);
}

// Sets the flags for integer compare `a cc b`; returns the ARM condition that holds when it is true.
static int emit_icmp(int a, int b, int cc) {
    int sf = V(a)->type == IRT_I64;
    int64_t c, c2;
    if (const_of(a, &c) && !const_of(b, &c2)) {
        int t = a;
        a = b;
        b = t;
        cc = cc_swap[cc];
    }
    Arm64Reg ra = use_g(a, T1);
    if (const_of(b, &c)) {
        int64_t cv = sf ? c : (int64_t)(int32_t)c;
        int i12, sh;
        bool neg = cv < 0;
        uint64_t u = neg ? 0 - (uint64_t)cv : (uint64_t)cv;
        // cmn a, #k is cmp a, #-k with identical C and V (k != INT_MIN, which arith_imm rejects)
        if (arith_imm(u, &i12, &sh)) {
            if (neg) arm64_adds_imm(S, sf, ZR, ra, i12, (uint32_t)sh);
            else
                arm64_subs_imm(S, sf, ZR, ra, i12, (uint32_t)sh);
            return cc_arm[cc];
        }
    }
    Arm64Reg rb = use_g0(b, T2);
    arm64_subs_reg(S, sf, ZR, ra, rb, ARM64_LSL, 0);
    return cc_arm[cc];
}

static bool fzero(int v) { return V(v)->op == IR_FCONST && V(v)->fimm == 0.0; }

// fcmpe (ordered compares signal on a NaN operand, like the fixed x86 side) or the quiet fcmp
// for ==/!=; compares against 0.0 use the immediate form.
static int emit_fcmp(int a, int b, int cc) {
    int ft = V(a)->type == IRT_F64;
    if (fzero(a) && !fzero(b)) {
        int t = a;
        a = b;
        b = t;
        cc = cc_swap[cc];
    }
    uint32_t e = (cc == IRCC_FEQ || cc == IRCC_FNE) ? 0 : 0x10;
    int ra = use_x(a, XT1);
    if (fzero(b)) {
        emit32(0x1e202008u | e | ((uint32_t)ft << 22) | ((uint32_t)ra << 5));
    } else {
        int rb = use_x(b, XT2);
        emit32(0x1e202000u | e | ((uint32_t)ft << 22) | ((uint32_t)rb << 16) | ((uint32_t)ra << 5));
    }
    return cc_arm[cc];
}

static void emit_cmp_value(IRInstr *i) {
    int cc = i->op == IR_CMP ? emit_icmp(i->a, i->b, i->cc) : emit_fcmp(i->a, i->b, i->cc);
    Arm64Reg rd = def_g(i->id);
    arm64_cset(S, 0, rd, (Arm64Cond)cc);
    fin_g(i->id, rd);
}

static void emit_fbin(IRInstr *i) {
    int ft = i->type == IRT_F64;
    int rd = def_x(i->id);
    int ra = use_x(i->a, XT1);
    int rb = use_x(i->b, XT2);
    switch (i->op) {
    case IR_FADD: arm64_fadd(S, ft, (Arm64Reg)rd, (Arm64Reg)ra, (Arm64Reg)rb); break;
    case IR_FSUB: arm64_fsub(S, ft, (Arm64Reg)rd, (Arm64Reg)ra, (Arm64Reg)rb); break;
    case IR_FMUL: arm64_fmul(S, ft, (Arm64Reg)rd, (Arm64Reg)ra, (Arm64Reg)rb); break;
    default: arm64_fdiv(S, ft, (Arm64Reg)rd, (Arm64Reg)ra, (Arm64Reg)rb); break;
    }
    fin_x(i->id, rd);
}

static void emit_fneg(IRInstr *i) {
    int rd = def_x(i->id);
    int ra = use_x(i->a, XT1);
    arm64_fneg(S, i->type == IRT_F64, (Arm64Reg)rd, (Arm64Reg)ra);
    fin_x(i->id, rd);
}

static void emit_conv(IRInstr *i) {
    switch (i->op) {
    case IR_SEXT8:
    case IR_SEXT16:
    case IR_ZEXT8:
    case IR_ZEXT16:
    case IR_SEXT32:
    case IR_ZEXT32:
    case IR_TRUNC: {
        Arm64Reg rd = def_g(i->id);
        Arm64Reg ra = use_g(i->a, T1);
        switch (i->op) {
        case IR_SEXT8: arm64_sxtb(S, 0, rd, ra); break;
        case IR_SEXT16: arm64_sxth(S, 0, rd, ra); break;
        case IR_ZEXT8: arm64_uxtb(S, rd, ra); break;
        case IR_ZEXT16: arm64_uxth(S, rd, ra); break;
        case IR_SEXT32: arm64_sxtw(S, rd, ra); break;
        case IR_ZEXT32: arm64_orr_reg(S, 0, rd, ZR, ra, ARM64_LSL, 0); break; // mov w, w
        default: mov_rr(0, rd, ra); break; // TRUNC: the low half is already in place
        }
        fin_g(i->id, rd);
        break;
    }
    case IR_SITOF:
    case IR_UITOF: {
        int rd = def_x(i->id);
        Arm64Reg ra = use_g(i->a, T1);
        int sf = V(i->a)->type == IRT_I64, ft = i->type == IRT_F64;
        if (i->op == IR_SITOF) arm64_scvtf(S, sf, ft, (Arm64Reg)rd, ra);
        else
            arm64_ucvtf(S, 1, ft, (Arm64Reg)rd, ra);
        fin_x(i->id, rd);
        break;
    }
    case IR_FTOSI:
    case IR_FTOU64: {
        Arm64Reg rd = def_g(i->id);
        int ra = use_x(i->a, XT1);
        int sf = i->type == IRT_I64, ft = V(i->a)->type == IRT_F64;
        if (i->op == IR_FTOSI) arm64_fcvtzs(S, sf, ft, rd, (Arm64Reg)ra);
        else
            arm64_fcvtzu(S, 1, ft, rd, (Arm64Reg)ra);
        fin_g(i->id, rd);
        break;
    }
    case IR_FEXT: { // fcvt d, s
        int rd = def_x(i->id);
        int ra = use_x(i->a, XT1);
        arm64_fcvt(S, 1, 0, (Arm64Reg)rd, (Arm64Reg)ra);
        fin_x(i->id, rd);
        break;
    }
    default: { // IR_FTRUNC: fcvt s, d
        int rd = def_x(i->id);
        int ra = use_x(i->a, XT1);
        arm64_fcvt(S, 0, 1, (Arm64Reg)rd, (Arm64Reg)ra);
        fin_x(i->id, rd);
        break;
    }
    }
}

typedef struct {
    Arm64Reg base, idx;
    int ext, sh;
    int64_t disp;
    const char *sym; // base holds the ADRP page of this symbol; disp is the :lo12: addend
} Addr;

static void adrp_sym(Arm64Reg rd, const char *sym) {
    size_t off = S->len;
    arm64_adrp(S, rd, 0);
    objfile_add_reloc(cg_obj, SEC_TEXT, off, cg_ir_sym_index(sym), R_AARCH64_ADR_PREL_PG_HI21, 0);
}

// Operand registers of a folded LOAD/STORE address: base in T1 / index in T2 when spilled.
static Addr addr_of(const IRInstr *m) {
    Addr a;
    memset(&a, 0, sizeof a);
    a.idx = NOIDX;
    if (m->aux == 1) {
        a.base = SPREG;
        a.disp = (int64_t)be.frame + m->imm; // imm is relative to x29
    } else if (m->aux == 2) {
#ifdef __APPLE__
        // Mach-O page-offset relocations carry no addend: a displacement is added separately.
        if (m->imm != 0) {
            cg_ir_sym_addr(T1, m->sym, 0);
            a.base = T1;
            a.disp = m->imm;
            return a;
        }
#endif
        adrp_sym(T1, m->sym);
        a.base = T1;
        a.disp = m->imm;
        a.sym = m->sym;
        return a;
    } else {
        a.base = use_g(m->a, T1);
        a.disp = m->imm;
    }
    if (m->b != IR_NONE) {
        a.idx = use_g(m->b, T2);
        a.ext = m->cc;
        a.sh = log2i(m->scale ? m->scale : 1);
    }
    return a;
}

static const uint8_t mw_szl[] = {[IRM_I8] = 0, [IRM_I16] = 1, [IRM_I32] = 2, [IRM_I64] = 3, [IRM_F32] = 2, [IRM_F64] = 3};

static void emit_load(IRInstr *i) {
    int szl = mw_szl[i->mw];
    Addr a = addr_of(i);
    if (tflt(i->type)) {
        int rd = def_x(i->id);
        ldst_x(szl, 1, 1, rd, a.base, a.idx, a.ext, a.sh, a.disp, a.sym);
        fin_x(i->id, rd);
        return;
    }
    Arm64Reg rd = def_g(i->id);
    int opc = 1;
    if (i->msign && szl < (i->type == IRT_I64 ? 3 : 2)) opc = i->type == IRT_I64 ? 2 : 3;
    ldst_x(szl, 0, opc, rd, a.base, a.idx, a.ext, a.sh, a.disp, a.sym);
    fin_g(i->id, rd);
}

static void emit_store(IRInstr *i) {
    int szl = mw_szl[i->mw];
    int vt = V(i->c)->type;
    if (tflt(vt)) {
        int rv = use_x(i->c, XT0);
        Addr a = addr_of(i);
        ldst_x(szl, 1, 0, rv, a.base, a.idx, a.ext, a.sh, a.disp, a.sym);
        return;
    }
    Arm64Reg rv = use_g0(i->c, T0);
    Addr a = addr_of(i);
    ldst_x(szl, 0, 0, rv, a.base, a.idx, a.ext, a.sh, a.disp, a.sym);
}

// rd = the n (1..8) bytes at [base + disp], zero-extended, without reading past them. `tmp` is a
// scratch register distinct from rd and base.
static void ld_bytes_exact(Arm64Reg rd, Arm64Reg base, int64_t disp, int n, Arm64Reg tmp) {
    if (n == 1 || n == 2 || n == 4 || n == 8) {
        ldst(n == 8 ? 3 : n == 4 ? 2
                 : n == 2        ? 1
                                 : 0,
             0, 1, rd, base, NOIDX, 0, disp);
        return;
    }
    int off = 0;
    while (off < n) {
        int rem = n - off, szl = rem >= 4 ? 2 : rem >= 2 ? 1
                                                         : 0;
        if (off == 0) {
            ldst(szl, 0, 1, rd, base, NOIDX, 0, disp);
        } else {
            ldst(szl, 0, 1, tmp, base, NOIDX, 0, disp + off);
            arm64_orr_reg(S, 1, rd, rd, tmp, ARM64_LSL, 8 * off);
        }
        off += 1 << szl;
    }
}

// Outgoing stack argument slot s: [sp + 8*s].
static void emit_stack_arg(int a, int slot) {
    int t = V(a)->type;
    if (tflt(t)) {
        int r = use_x(a, XT0);
        ldst(tsz(t) == 4 ? 2 : 3, 1, 0, r, SPREG, NOIDX, 0, 8 * slot);
    } else {
        Arm64Reg r = use_g0(a, T0);
        ldst(tsz(t) == 4 ? 2 : 3, 0, 0, r, SPREG, NOIDX, 0, 8 * slot);
    }
}

static bool raw_agg(int cls) { return cls == IRAGG_VALUE8 || cls == IRAGG_HFA4 || cls == IRAGG_HFA8; }

// Outgoing area needed by CALL c: argument stack slots *nst, then staging slots *nstage for the
// raw aggregates that end up in registers (their bytes are copied there first because the
// parallel move of the scalar arguments may overwrite the registers holding their addresses).
static void call_layout(const IRInstr *c, int *nst_out, int *nstage_out) {
    int ng = 0, nx = 0, nst = 0, nstage = 0;
    for (int k = 0; k < c->nargs; k++) {
        int rg, slot;
        arg_place(c, k, &ng, &nx, &nst, &rg, &slot);
        int e = c->arg_class ? c->arg_class[k] : 0, cls = e & 7;
        if (rg >= 0 && raw_agg(cls)) nstage += cls == IRAGG_VALUE8 ? 1 : (e >> 3) / (cls == IRAGG_HFA4 ? 4 : 8);
    }
    *nst_out = nst;
    *nstage_out = nstage;
}

static void emit_call(IRInstr *c) {
    int n = c->nargs, nst, nstage;
    call_layout(c, &nst, &nstage);
    int stbase = nst, ng = 0, nx = 0, nst2 = 0, nm = 0, stage = 0;
    Mv *mv = NEW(Mv, n + 2);
    int *rgs = NEW(int, n + 1), *stg = NEW(int, n + 1);
    for (int k = 0; k < n; k++) {
        int slot;
        arg_place(c, k, &ng, &nx, &nst2, &rgs[k], &slot);
        int a = c->args[k], t = V(a)->type;
        int e = c->arg_class ? c->arg_class[k] : 0, cls = e & 7, szb = e >> 3;
        stg[k] = -1;
        if (!raw_agg(cls)) { // scalar or pointer-class aggregate (the value is the pointer)
            if (rgs[k] >= 0) mv[nm++] = (Mv){pl_reg(tflt(t) ? RC_X : RC_G, rgs[k]), pl_of(a), t};
            else
                emit_stack_arg(a, slot);
            continue;
        }
        Arm64Reg ar = use_g(a, T1); // address of the argument bytes
        if (cls == IRAGG_VALUE8) {
            ld_bytes_exact(T0, ar, 0, szb, T2);
            if (rgs[k] >= 0) {
                stg[k] = stage++;
                ldst(3, 0, 0, T0, SPREG, NOIDX, 0, 8 * (stbase + stg[k]));
            } else {
                ldst(3, 0, 0, T0, SPREG, NOIDX, 0, 8 * slot);
            }
        } else { // HFA
            int elem = cls == IRAGG_HFA4 ? 4 : 8, cnt = szb / elem;
            if (rgs[k] >= 0) { // elements go to V registers after the parallel move: stage them
                stg[k] = stage;
                stage += cnt;
                for (int j = 0; j < cnt; j++) {
                    ldst(elem == 4 ? 2 : 3, 0, 1, T0, ar, NOIDX, 0, (int64_t)j * elem);
                    ldst(3, 0, 0, T0, SPREG, NOIDX, 0, 8 * (stbase + stg[k] + j));
                }
            } else { // the raw bytes in consecutive 8-byte slots
                for (int j = 0; j < (szb + 7) / 8; j++) {
                    ldst(szb - 8 * j >= 8 ? 3 : 2, 0, 1, T0, ar, NOIDX, 0, 8 * j);
                    ldst(3, 0, 0, T0, SPREG, NOIDX, 0, 8 * (slot + j));
                }
            }
        }
    }
    // The indirect target goes through x9: not an argument register and never a pm temporary.
    if (!c->sym) mv[nm++] = (Mv){pl_reg(RC_G, ARM64_X9), pl_of(c->a), IRT_I64};
    pm_run(mv, nm);
    // A hidden aggregate result pointer goes in x8 (no argument register is involved).
    if (c->ret_agg < 0) add_imm64(ARM64_X8, SPREG, slot_off(be.ctemp[c->id]), T3);
    for (int k = 0; k < n; k++) { // raw aggregates into their ABI registers
        if (stg[k] < 0) continue;
        int e = c->arg_class[k], cls = e & 7, szb = e >> 3;
        if (cls == IRAGG_VALUE8) {
            ldst(szb <= 4 ? 2 : 3, 0, 1, rgs[k], SPREG, NOIDX, 0, 8 * (stbase + stg[k]));
        } else {
            int elem = cls == IRAGG_HFA4 ? 4 : 8, cnt = szb / elem;
            for (int j = 0; j < cnt; j++)
                ldst(elem == 4 ? 2 : 3, 1, 1, rgs[k] + j, SPREG, NOIDX, 0, 8 * (stbase + stg[k] + j));
        }
    }
    if (c->sym) cg_ir_call(c->sym);
    else
        arm64_blr(S, ARM64_X9);
    if (c->noret) emit32(0xd4200020u); // brk #1: a declared noreturn callee must not return
    if (c->ret_agg != 0) {
        if (LOC(c->id)->kind != LK_NONE) {
            int tmp = be.ctemp[c->id];
            if (c->ret_agg > 0) { // x0[:x1] hold the bytes: spill them to the temp
                ldst(3, 0, 0, ARM64_X0, SPREG, NOIDX, 0, slot_off(tmp));
                if (c->ret_agg > 8) ldst(3, 0, 0, ARM64_X1, SPREG, NOIDX, 0, slot_off(tmp) + 8);
            }
            Arm64Reg rd = def_g(c->id);
            add_imm64(rd, SPREG, slot_off(tmp), T3);
            fin_g(c->id, rd);
        }
    } else if (c->type != IRT_VOID && LOC(c->id)->kind != LK_NONE) {
        if (tflt(c->type)) fin_x(c->id, 0);
        else
            fin_g(c->id, ARM64_X0);
    }
    free(mv);
    free(rgs);
    free(stg);
}

static void emit_epilogue(void) {
    if (be.frameless) {
        arm64_ret(S, ARM64_X30);
        return;
    }
    for (int k = 0; k < be.ncs;) {
        int64_t off = slot_off(be.cs_base + 8 * (k + 1));
        if (k + 1 < be.ncs && off % 8 == 0 && off / 8 >= -63 && off / 8 <= 64) {
            arm64_ldp(S, 1, be.cs_reg[k + 1], be.cs_reg[k], SPREG, (int32_t)(off / 8 - 1), false, false);
            k += 2;
        } else {
            ldst(3, 0, 1, be.cs_reg[k], SPREG, NOIDX, 0, off);
            k++;
        }
    }
    for (int k = 0; k < be.ncx;) {
        int64_t off = slot_off(be.cs_base + 8 * (be.ncs + k + 1));
        if (k + 1 < be.ncx && off % 8 == 0 && off / 8 >= -63 && off / 8 <= 64) {
            arm64_ldp_fp(S, 1, be.cx_reg[k + 1], be.cx_reg[k], SPREG, (int32_t)(off / 8 - 1), false, false);
            k += 2;
        } else {
            ldst(3, 1, 1, be.cx_reg[k], SPREG, NOIDX, 0, off);
            k++;
        }
    }
    arm64_add_imm(S, 1, SPREG, ARM64_X29, 0, 0); // mov sp, x29
    arm64_ldp(S, 1, ARM64_X29, ARM64_X30, SPREG, 2, false, true); // ldp x29, x30, [sp], #16
    arm64_ret(S, ARM64_X30);
}

static void jmp_to(IRBlock *t, IRBlock *nxt) {
    if (t != nxt) cg_ir_jmp(blabel(t));
}

// A branch condition: flags + ARM condition, cbz/cbnz, or tbz/tbnz on a register.
typedef struct {
    bool cb, tb;
    int sf, bit;
    Arm64Reg reg;
    bool nz; // cb/tb: true when the tested register/bit is nonzero
    int cc;
} Cond;

static void jc(Cond c, bool invert, const char *label) {
    bool long_branch = be.far || (c.tb && be.f->nvals > 1000);
    if (long_branch) { // skip over an unconditional jump on the complement
        if (c.tb) {
            if (c.nz != invert) arm64_tbz(S, c.reg, c.bit, 2);
            else
                arm64_tbnz(S, c.reg, c.bit, 2);
        } else if (c.cb) {
            if (c.nz != invert) arm64_cbz(S, c.sf, c.reg, 2);
            else
                arm64_cbnz(S, c.sf, c.reg, 2);
        } else {
            arm64_bcond(S, (Arm64Cond)((invert ? c.cc ^ 1 : c.cc) ^ 1), 2);
        }
        cg_ir_jmp(label);
        return;
    }
    if (c.tb) cg_ir_tbz(c.reg, c.bit, c.nz != invert, label);
    else if (c.cb)
        cg_ir_cbz(c.sf, c.reg, c.nz != invert, label);
    else
        cg_ir_jcc(invert ? c.cc ^ 1 : c.cc, label);
}

static void emit_br(IRInstr *br, IRBlock *nxt) {
    IRBlock *T = resolve(br->t), *F = resolve(br->f);
    Cond c = {0};
    int64_t imm;
    if (br->mw == 3) {
        c.tb = true;
        c.reg = use_g(br->a, T1);
        c.bit = (int)br->imm;
        c.nz = br->cc == IRCC_NE;
    } else if (br->mw == 1 && (br->cc == IRCC_EQ || br->cc == IRCC_NE) &&
               const_of(br->b, &imm) && imm == 0 && !const_of(br->a, &imm)) {
        c.cb = true;
        c.sf = V(br->a)->type == IRT_I64;
        c.reg = use_g(br->a, T1);
        c.nz = br->cc == IRCC_NE;
    } else if (br->mw == 1) {
        c.cc = emit_icmp(br->a, br->b, br->cc);
    } else if (br->mw == 2) {
        c.cc = emit_fcmp(br->a, br->b, br->cc);
    } else {
        if (const_of(br->a, &imm)) {
            jmp_to((int32_t)imm ? T : F, nxt);
            return;
        }
        c.cb = true;
        c.sf = 0;
        c.reg = use_g(br->a, T1);
        c.nz = true;
    }
    if (T == F) {
        jmp_to(T, nxt);
    } else if (F == nxt) {
        jc(c, false, blabel(T));
    } else if (T == nxt) {
        jc(c, true, blabel(F));
    } else if (be.oidx[F->id] < be.oidx[T->id]) { // the backward edge gets the conditional jump
        jc(c, true, blabel(F));
        cg_ir_jmp(blabel(T));
    } else {
        jc(c, false, blabel(T));
        cg_ir_jmp(blabel(F));
    }
}

static void emit_ins(IRBlock *b, IRInstr *i, IRBlock *nxt) {
    switch (i->op) {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
        emit_alu(i);
        break;
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
        emit_div(i);
        break;
    case IR_SHL:
    case IR_SHR:
    case IR_SAR:
        emit_shift(i);
        break;
    case IR_NEG:
    case IR_NOT:
        emit_unary(i);
        break;
    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
        emit_fbin(i);
        break;
    case IR_FNEG:
        emit_fneg(i);
        break;
    case IR_CMP:
    case IR_FCMP:
        emit_cmp_value(i);
        break;
    case IR_SEXT8:
    case IR_SEXT16:
    case IR_ZEXT8:
    case IR_ZEXT16:
    case IR_SEXT32:
    case IR_ZEXT32:
    case IR_TRUNC:
    case IR_SITOF:
    case IR_UITOF:
    case IR_FTOSI:
    case IR_FTOU64:
    case IR_FEXT:
    case IR_FTRUNC:
        emit_conv(i);
        break;
    case IR_LOAD:
        emit_load(i);
        break;
    case IR_STORE:
        emit_store(i);
        break;
    case IR_GADDR: { // only reached for a `keep` GADDR (others are rematerialized)
        Arm64Reg rd = def_g(i->id);
        mat_g(i->id, rd);
        fin_g(i->id, rd);
        break;
    }
    case IR_CALL:
        emit_call(i);
        break;
    case IR_JMP: {
        Mv *m;
        int n = edge_moves(b, i->t, &m);
        pm_run(m, n);
        free(m);
        jmp_to(resolve(i->t), nxt);
        break;
    }
    case IR_BR:
        emit_br(i, nxt);
        break;
    case IR_RET:
        if (i->a != IR_NONE) {
            if (be.f->ret_agg > 0 && !be.f->ret_sret) { // x0[:x1] = the bytes at the returned address
                load_g(T1, i->a);
                int sz = be.f->ret_agg;
                ld_bytes_exact(ARM64_X0, T1, 0, sz < 8 ? sz : 8, T2);
                if (sz > 8) ld_bytes_exact(ARM64_X1, T1, 8, sz - 8, T2);
            } else if (tflt(V(i->a)->type)) {
                load_x(0, i->a);
            } else {
                load_g(ARM64_X0, i->a); // scalars, and the hidden return pointer (a IS that pointer)
            }
        }
        emit_epilogue();
        break;
    case IR_UNREACHABLE:
        emit32(0xd4200020u); // brk #1
        break;
    default:
        abort();
    }
}

static void frame_layout(void) {
    IRFunc *f = be.f;
    uint32_t used = 0, usedx = 0;
    for (int v = 0; v < be.nv; v++)
        if (LOC(v)->kind == LK_REG) {
            if (rc_of(v) == RC_G) used |= BIT(LOC(v)->reg);
            else
                usedx |= BIT(LOC(v)->reg);
        }
    for (size_t k = 0; k < sizeof gpr_callee_order; k++)
        if (used >> gpr_callee_order[k] & 1) be.cs_reg[be.ncs++] = gpr_callee_order[k];
    for (size_t k = 0; k < sizeof xmm_callee_order; k++)
        if (usedx >> xmm_callee_order[k] & 1) be.cx_reg[be.ncx++] = xmm_callee_order[k];
    // The frame below x29: locals (sized from the FRAMEADDRs that survived inlining and the
    // optimizer), aggregate temps (agg_layout), callee-saved registers, spill slots, outgoing area.
    be.locals = be.aggbase + be.aggtotal;
    be.cs_base = be.locals;
    be.spill_base = be.cs_base + 8 * (be.ncs + be.ncx);
    int nslots = be.nspill[RC_G] + be.nspill[RC_X];
    for (int v = 0; v < be.nv; v++) {
        VInfo *x = &be.vi[v];
        if (LOC(v)->kind != LK_SPILL || x->slotidx < 0) continue;
        int idx = x->slotidx + (rc_of(v) == RC_X ? be.nspill[RC_G] : 0);
        LOC(v)->slot = be.spill_base + 8 * (idx + 1);
    }
    bool has_call = false;
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_CALL || be.vi[i->id].dead) continue;
            has_call = true;
            int nst, nstage;
            call_layout(i, &nst, &nstage);
            if (8 * (nst + nstage) > be.outgoing) be.outgoing = 8 * (nst + nstage);
        }
    }
    int total = be.spill_base + 8 * nslots + be.outgoing;
    be.frame = (total + 15) & ~15;
    // A leaf that touches neither the frame, the callee-saved registers nor the stack
    // arguments needs no frame record at all.
    be.frameless = !has_call && be.frame == 0;
    for (int k = 0; k < f->nparams; k++)
        if (be.pstk[k]) be.frameless = false;
}

static void emit_prologue(void) {
    IRFunc *f = be.f;
    if (!be.frameless) {
        arm64_stp(S, 1, ARM64_X29, ARM64_X30, SPREG, -2, true, false); // stp x29, x30, [sp, #-16]!
        arm64_add_imm(S, 1, ARM64_X29, SPREG, 0, 0); // mov x29, sp
        if (be.frame) add_imm64(SPREG, SPREG, -(int64_t)be.frame, T3);
        for (int k = 0; k < be.ncs;) {
            int64_t off = slot_off(be.cs_base + 8 * (k + 1));
            if (k + 1 < be.ncs && off % 8 == 0 && off / 8 >= -63 && off / 8 <= 64) {
                arm64_stp(S, 1, be.cs_reg[k + 1], be.cs_reg[k], SPREG, (int32_t)(off / 8 - 1), false, false);
                k += 2;
            } else {
                ldst(3, 0, 0, be.cs_reg[k], SPREG, NOIDX, 0, off);
                k++;
            }
        }
        for (int k = 0; k < be.ncx;) {
            int64_t off = slot_off(be.cs_base + 8 * (be.ncs + k + 1));
            if (k + 1 < be.ncx && off % 8 == 0 && off / 8 >= -63 && off / 8 <= 64) {
                arm64_stp_fp(S, 1, be.cx_reg[k + 1], be.cx_reg[k], SPREG, (int32_t)(off / 8 - 1), false, false);
                k += 2;
            } else {
                ldst(3, 1, 0, be.cx_reg[k], SPREG, NOIDX, 0, off);
                k++;
            }
        }
    }
    IRBlock *entry = f->blocks[0];
    // Aggregate parameters arrive in registers / stack slots: store them into their frame temps
    // (before the parallel move below overwrites any argument register); the IR_PARAM value is
    // the temp's address.
    for (int k = 0; k < entry->nins; k++) {
        IRInstr *p = entry->ins[k];
        if (!agg_param(p)) continue;
        int idx = (int)p->imm, cls = be.pcls[idx], szb = f->param_class[idx] >> 3;
        int64_t to = slot_off(be.paggoff[idx]);
        if (cls == IRAGG_VALUE8) {
            if (be.pstk[idx]) {
                ldst(3, 0, 1, T0, SPREG, NOIDX, 0, slot_off(-be.pstk[idx]));
                ldst(3, 0, 0, T0, SPREG, NOIDX, 0, to);
            } else {
                ldst(szb <= 4 ? 2 : 3, 0, 0, be.pabi[idx], SPREG, NOIDX, 0, to);
            }
        } else {
            int elem = cls == IRAGG_HFA4 ? 4 : 8, cnt = szb / elem;
            if (be.pstk[idx]) {
                for (int j = 0; j < (szb + 7) / 8; j++) {
                    ldst(3, 0, 1, T0, SPREG, NOIDX, 0, slot_off(-be.pstk[idx]) + 8 * j);
                    ldst(3, 0, 0, T0, SPREG, NOIDX, 0, to + 8 * j);
                }
            } else {
                for (int j = 0; j < cnt; j++)
                    ldst(elem == 4 ? 2 : 3, 1, 0, be.pabi[idx] + j, SPREG, NOIDX, 0, to + (int64_t)j * elem);
            }
        }
    }
    Mv *mv = NEW(Mv, entry->nins + 1);
    int nm = 0;
    for (int k = 0; k < entry->nins; k++) {
        IRInstr *p = entry->ins[k];
        if (p->op != IR_PARAM || !needs_loc(p->id) || LOC(p->id)->kind == LK_NONE) continue;
        int idx = (int)p->imm;
        if (be.pstk[idx]) {
            if (LOC(p->id)->kind == LK_SPILL) continue; // lives in its incoming slot
            mv[nm++] = (Mv){pl_of(p->id), (PL){PL_M, 0, -be.pstk[idx], 0}, p->type};
        } else {
            mv[nm++] = (Mv){pl_of(p->id), pl_reg(tflt(p->type) ? RC_X : RC_G, be.pabi[idx]), p->type};
        }
    }
    pm_run(mv, nm);
    free(mv);
}

// Why the emitter cannot take `f` (NULL = it can). Checked before anything is emitted.
static const char *arm64_ineligible(IRFunc *f) {
    if (f->nvals > 1500000) return "function too large"; // pathological: even `b` (+-128MB) could fall short
    for (int k = 0; k < f->nparams; k++) {
        int cls = f->param_class ? f->param_class[k] & 7 : 0;
        if (cls != IRAGG_NONE && cls != IRAGG_VALUE8 && cls != IRAGG_PTR && cls != IRAGG_HFA4 && cls != IRAGG_HFA8)
            return "aggregate parameter class";
    }
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; b && k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_CALL) continue;
            for (int a = 0; i->arg_class && a < i->nargs; a++) {
                int cls = i->arg_class[a] & 7;
                if (i->var_from > 0 && a >= i->var_from - 1 && cls != IRAGG_NONE)
                    return "variadic aggregate call";
                if (cls != IRAGG_NONE && cls != IRAGG_VALUE8 && cls != IRAGG_PTR && cls != IRAGG_HFA4 && cls != IRAGG_HFA8)
                    return "aggregate call argument class";
            }
        }
    }
    return NULL;
}

// Frame temps for aggregates, laid out right above the locals: one per raw (VALUE8 / HFA)
// aggregate parameter and one per aggregate-returning call, each rounded up to 16 bytes. An
// offset o addresses bytes [x29 - o, x29 - o + size), the FRAMEADDR convention. Done before isel
// prep so address folding can use the final offsets.
static void agg_layout(void) {
    IRFunc *f = be.f;
    int lbytes = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; b && k < b->nins; k++)
            if (b->ins[k]->op == IR_FRAMEADDR && b->ins[k]->imm > lbytes) lbytes = (int)b->ins[k]->imm;
    }
    be.aggbase = (lbytes + 15) & ~15;
    be.ctemp = NEW(int, be.nv + 1);
    int tot = 0;
    for (int k = 0; k < f->nparams; k++) {
        int cls = be.pcls[k];
        if (cls != IRAGG_VALUE8 && cls != IRAGG_HFA4 && cls != IRAGG_HFA8) continue;
        int szb = f->param_class[k] >> 3;
        tot += (szb + 15) & ~15;
        be.paggoff[k] = be.aggbase + tot;
    }
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_CALL || !i->ret_agg) continue;
            int szb = i->ret_agg < 0 ? -i->ret_agg : i->ret_agg;
            tot += (szb + 15) & ~15;
            be.ctemp[i->id] = be.aggbase + tot;
        }
    }
    be.aggtotal = tot;
}

bool ir_emit_arm64(Program *prog, IRFunc *f, bool emitting_local_copy) {
    static int serial;
    const char *why = arm64_ineligible(f);
    if (why) {
        if (opt_fdump_ir) fprintf(stderr, "ir: %s: not eligible (%s)\n", f->fn->name, why);
        return false;
    }
    const char *sym;
    size_t start = cg_ir_begin_fn(prog, f->fn, emitting_local_copy, &sym);
    memset(&be, 0, sizeof be);
    be.f = f;
    be.sym = sym;
    be.serial = serial++;
    be.far = f->nvals > 12000;

    cfg_prep(f);
    ir_analyze_loops(f);
    layout(f);
    be.nv = f->nvals;
    be.vi = NEW(VInfo, be.nv + 1);
    be.loc = NEW(Loc, be.nv + 1);
    be.lbl = NEW(const char *, f->nblocks);
    be.tramp = NEW(uint8_t, f->nblocks);
    classify_params();
    agg_layout();
    prep_ir();
    build_intervals();
    regalloc();
    // A kept global address that lost its register is rematerialized after all:
    // an adrp+add at the use is cheaper than a reload from a spill slot.
    for (int v = 0; v < be.nv; v++)
        if (be.vi[v].keep && LOC(v)->kind == LK_SPILL) {
            be.vi[v].keep = 0;
            LOC(v)->kind = LK_REMAT;
        }
    frame_layout();

    // Blocks that only forward (no emitted instruction, no phi moves) are
    // skipped: branches go straight to their target.
    for (int bi = 1; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        IRInstr *t = b->ins[b->nins - 1];
        if (t->op != IR_JMP || t->t == b) continue;
        bool plain = true;
        for (int k = 0; k + 1 < b->nins; k++)
            if (emitted(b->ins[k])) plain = false;
        for (int k = 0; k < b->nphis; k++)
            if (needs_loc(b->phis[k]->id)) plain = false;
        if (plain && edge_real_moves(b, t->t) == 0) be.tramp[b->id] = 1;
    }
    for (int bi = 1; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        if (!be.tramp[b->id]) continue;
        IRBlock *c = b;
        for (int g = 0; g < f->nblocks && be.tramp[c->id]; g++) c = c->ins[c->nins - 1]->t;
        if (be.tramp[c->id]) be.tramp[b->id] = 0; // forwarding cycle: keep a real block
    }

    emit_prologue();
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        if (be.tramp[b->id]) continue;
        IRBlock *nxt = NULL;
        for (int k = bi + 1; k < be.nord && !nxt; k++)
            if (!be.tramp[be.ord[k]->id]) nxt = be.ord[k];
        cg_ir_label(blabel(b));
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_NOP || (!emitted(i))) continue;
            emit_ins(b, i, nxt);
        }
    }
    cg_ir_end_fn(sym, start);

    for (int v = 0; v < be.nv; v++) free(be.vi[v].rg);
    free(be.vi);
    free(be.loc);
    free(be.ord);
    free(be.oidx);
    free(be.pcls);
    free(be.paggoff);
    free(be.ctemp);
    free(be.bslot);
    free(be.bterm);
    free(be.tramp);
    free(be.lbl);
    free(be.callpos);
    free(be.pabi);
    free(be.pstk);
    return true;
}

#endif /* ARCH_ARM64 */
