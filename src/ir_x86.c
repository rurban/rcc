// ir_x86.c -- register allocation, instruction selection and emission (x86-64 SysV).
//
// ir_emit_x86() runs these stages on an already built + optimized IRFunc:
//   1. CFG prep: drop unreachable preds, split critical edges into blocks with
//      phis (so phi copies always sit at the end of a single-successor block),
//      loop depths, loop-friendly block layout (loop headers rotated behind
//      their bodies so the back edge is the only branch of an iteration).
//   2. Isel prep on the IR: UNDEF -> 0, address folding into LOAD/STORE
//      (base + index*scale + disp, rbp-relative frame slots), single-use LOADs
//      folded into extensions and into ALU/FP memory operands, CMP/FCMP+BR
//      fusion, use counts + DCE. Constants/addresses are rematerialized.
//   3. Liveness -> live ranges with holes per value -> linear scan with
//      loop-weighted spill costs and register hints (phis, two-address ops,
//      incoming/outgoing ABI registers).
//   4. Frame layout, prologue, per-instruction emission, phi elimination by
//      parallel moves at predecessor ends.
//
// Registers: rax r11 and xmm14 xmm15 are scratch (never allocated); rsi rdi
// rdx rcx r8 r9 r10 + rbx r12-r15 and xmm0-xmm13 are allocatable (rcx/rdx are
// blocked around variable shifts and divides). Intervals live across a call
// get callee-saved GPRs only; XMM intervals live across a call are spilled for
// their whole life. A spilled value lives in its frame slot everywhere; every
// use loads it into a scratch register.
//
// Frame (rbp-relative, rsp 16-byte aligned at every call):
//   [rbp-locals ..rbp)          address-taken locals (IRFunc.frame_size)
//   below: callee-saved save area, spill slots, then the outgoing stack args
//   area at [rsp .. rsp+outgoing).
#include "cg_ir.h"
#include <limits.h>

#define R_S0 X86_RAX
#define R_S1 X86_R11
#define X_S0 X86_XMM14
#define X_S1 X86_XMM15

enum { RC_G = 0,
       RC_X = 1 };
enum { LK_NONE,
       LK_REG,
       LK_SPILL,
       LK_REMAT };

typedef struct {
    uint8_t kind;
    int8_t reg;
    int32_t slot; // LK_SPILL: the value lives at [rbp - slot] (negative: incoming stack arg)
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
    int16_t stk; // PARAM passed on the stack: [rbp + stk], 0 = in a register
    int8_t abi; // PARAM passed in a register: its register number, -1 = none
    uint8_t dead; // removed by isel prep (not emitted)
} VInfo;

static const int8_t gpr_alloc_order[] = {X86_RSI, X86_RDI, X86_R8, X86_R9, X86_R10, X86_RDX,
                                         X86_RCX, X86_RBX, X86_R12, X86_R13, X86_R14, X86_R15};
static const int8_t gpr_callee_order[] = {X86_RBX, X86_R12, X86_R13, X86_R14, X86_R15};
static const int8_t xmm_alloc_order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static const X86Reg abi_gpr[6] = {X86_RDI, X86_RSI, X86_RDX, X86_RCX, X86_R8, X86_R9};
#define MASK_CALLEE ((1u << X86_RBX) | (1u << X86_R12) | (1u << X86_R13) | (1u << X86_R14) | (1u << X86_R15))
#define MASK_G_ALL (MASK_CALLEE | (1u << X86_RSI) | (1u << X86_RDI) | (1u << X86_R8) | (1u << X86_R9) | \
                    (1u << X86_R10) | (1u << X86_RCX) | (1u << X86_RDX))
#define MASK_X_ALL 0x3fffu

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
    int *blk[2], nblk[2]; // use positions of instructions clobbering rcx / rdx
    int nspill[2]; // spill slots per class pool
    int ncs, cs_reg[5]; // callee-saved registers used
    int locals, cs_base, spill_base, outgoing, frame;
    bool frameless; // leaf without frame pointer, locals, spills or saved registers
    int nlocal_lbl;
    int *mu; // value -> uses as an address operand in its own block (isel prep)
    int *pabi; // param index -> ABI register / -1
    int *pstk; // param index -> incoming stack offset / 0
    int *p2nd; // VALUE16 param: second GP register / -1
    int *pcls; // param index -> IRAggClass
    int *paggoff; // materialized aggregate param: rbp offset of its frame temp
    int *ctemp; // CALL value id -> rbp offset of its aggregate-return temp
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
static inline X86Mem off_mem(int off) { return x86_mem(X86_RBP, -(int64_t)off); }
static inline X86Mem slot_mem(int v) { return off_mem(be.loc[v].slot); }

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

static void cfg_prep(IRFunc *f) {
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
    free(stack);
    free(post);
    free(succ);
    free(ns);
    free(nx);
    free(seen);
}

// ------------------------------------------------------------- 2. isel prep

static void count_uses(void) {
    for (int v = 0; v < be.nv; v++) be.vi[v].uses = 0;
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (i->op == IR_NOP || be.vi[i->id].dead) continue;
            for (int u = 0, nu = nuse_slots(i); u < nu; u++) {
                int v = use_at(i, u);
                if (v != IR_NONE) be.vi[v].uses++;
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
// (base, index, scale, disp). m->aux = 1 means the base is rbp (a.k.a. a
// FRAMEADDR folded in, m->a == IR_NONE). Folded operands must live in the same
// block so operand lifetimes are not stretched across loops.
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
            }
        }
        if (base != IR_NONE) {
            IRInstr *d = V(base);
            if (d->op == IR_FRAMEADDR) {
                if (fits32(disp - d->imm)) {
                    disp -= d->imm;
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
                } else if (split && idx == IR_NONE &&
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
            } else if (i->op == IR_BR) {
                i->mw = 0;
                i->b = IR_NONE;
            } else if ((i->op >= IR_ADD && i->op <= IR_XOR) || (i->op >= IR_FADD && i->op <= IR_FDIV)) {
                i->mw = 0; // set to 1 by the memory-operand fusion below
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
    // (movsxd/movzx/mov from memory) when nothing in between can write memory
    // or trap.
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *e = b->ins[k];
            if (e->a == IR_NONE || be.vi[e->id].dead) continue;
            IRInstr *l = V(e->a);
            if (l->op != IR_LOAD || l->blk != b || be.vi[l->id].uses != 1 || tflt(l->type)) continue;
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
            e->mw = mw;
            e->msign = sgn;
            be.vi[l->id].uses--;
            if (e->a != IR_NONE) be.vi[e->a].uses++;
            if (e->b != IR_NONE) be.vi[e->b].uses++;
        }
    }
    // Fold a single-use LOAD that is the second operand of an ALU/FP op into
    // a memory operand. The op takes the load's address in b (base) / c
    // (index) / scale / imm / aux and is flagged with mw = 1.
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *e = b->ins[k];
            if (be.vi[e->id].dead) continue;
            bool comm = e->op == IR_ADD || e->op == IR_AND || e->op == IR_OR || e->op == IR_XOR ||
                e->op == IR_FADD || e->op == IR_FMUL;
            bool fp = e->op >= IR_FADD && e->op <= IR_FDIV;
            if (!comm && e->op != IR_SUB && !fp) continue;
            int lv[2] = {e->b, comm ? e->a : IR_NONE};
            for (int s = 0; s < 2; s++) {
                if (lv[s] == IR_NONE || e->a == e->b) continue;
                IRInstr *l = V(lv[s]);
                if (l->op != IR_LOAD || l->blk != b || be.vi[l->id].uses != 1 || l->type != e->type) continue;
                if (fp ? l->mw != (e->type == IRT_F32 ? IRM_F32 : IRM_F64)
                       : l->mw != (e->type == IRT_I32 ? IRM_I32 : IRM_I64))
                    continue;
                int lk = k - 1;
                while (lk >= 0 && b->ins[lk] != l && !ir_has_side_effects(b->ins[lk])) lk--;
                if (lk < 0 || b->ins[lk] != l) continue;
                if (s == 1) e->a = e->b; // commutative: the load was the left operand
                e->b = l->a;
                e->c = l->b;
                e->scale = l->scale;
                e->imm = l->imm;
                e->aux = l->aux;
                e->mw = 1;
                be.vi[l->id].uses--;
                if (e->b != IR_NONE) be.vi[e->b].uses++;
                if (e->c != IR_NONE) be.vi[e->c].uses++;
                break;
            }
        }
    }
    // Fuse a single-use compare with the BR that ends its block.
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        IRInstr *br = b->ins[b->nins - 1];
        if (br->op != IR_BR) continue;
        IRInstr *c = V(br->a);
        if ((c->op != IR_CMP && c->op != IR_FCMP) || c->blk != b || be.vi[c->id].uses != 1) continue;
        br->mw = c->op == IR_CMP ? 1 : 2;
        br->cc = c->cc;
        br->b = c->b;
        br->a = c->a;
        be.vi[c->id].uses--;
        be.vi[c->a].uses++;
        be.vi[c->b].uses++;
    }
    dce();
    free(be.mu);
    be.mu = NULL;
}

// ------------------------------------------------------ 3. liveness + alloc

static bool needs_loc(int v) {
    const IRInstr *i = V(v);
    return !be.vi[v].dead && be.vi[v].uses > 0 && i->type != IRT_VOID && !is_remat_op(i->op) &&
        i->op != IR_NOP;
}

// Instructions that produce machine code (params arrive in registers, consts
// and addresses are rematerialized at their uses).
static bool emitted(const IRInstr *i) {
    return i->op != IR_NOP && i->op != IR_PARAM && i->op != IR_PHI && !is_remat_op(i->op) &&
        !be.vi[i->id].dead;
}

// Callee side: walk the parameter list with the same GP / XMM / stack-slot
// counting the caller uses (see call_arg_slot). Aggregate entries encode
// (bytes << 3) | IRAggClass; IRAGG_PTR params are plain pointer values and
// classified exactly like scalar GP arguments.
static void classify_params(void) {
    IRFunc *f = be.f;
    be.pabi = NEW(int, f->nparams + 1);
    be.pstk = NEW(int, f->nparams + 1);
    be.p2nd = NEW(int, f->nparams + 1);
    be.pcls = NEW(int, f->nparams + 1);
    be.paggoff = NEW(int, f->nparams + 1);
    int ng = 0, nx = 0, nst = 0;
    for (int k = 0; k < f->nparams; k++) {
        be.pabi[k] = -1;
        be.p2nd[k] = -1;
        int e = f->param_class ? f->param_class[k] : 0;
        int cls = e & 7;
        int szb = e >> 3;
        be.pcls[k] = cls;
        if (!cls || cls == IRAGG_PTR) {
            if (tflt(f->param_types[k])) {
                if (nx < 8) be.pabi[k] = nx++;
                else
                    be.pstk[k] = 16 + 8 * nst++;
            } else {
                if (ng < 6) be.pabi[k] = abi_gpr[ng++];
                else
                    be.pstk[k] = 16 + 8 * nst++;
            }
        } else if (cls == IRAGG_VALUE8) {
            if (ng < 6) be.pabi[k] = abi_gpr[ng++];
            else
                be.pstk[k] = 16 + 8 * nst++;
        } else if (cls == IRAGG_VALUE16) {
            if (ng + 1 < 6) {
                be.pabi[k] = abi_gpr[ng];
                be.p2nd[k] = abi_gpr[ng + 1];
                ng += 2;
            } else {
                be.pstk[k] = 16 + 8 * nst;
                nst += (szb + 7) / 8;
            }
        } else { // IRAGG_MEMORY: raw bytes in incoming stack slots, 0 GP regs
            be.pstk[k] = 16 + 8 * nst;
            nst += (szb + 7) / 8;
        }
    }
}

// Caller side: advance the ABI counters for argument k of CALL instr i.
// *rg / *rg2 receive the destination GP register (or XMM index for scalar
// floats) or -1; *slot0 / *nslot describe outgoing stack placement. A
// register-placed IRAGG_VALUE8 / IRAGG_VALUE16 still needs the backend to
// load the bytes (the IR value is an ADDRESS): those go through a staging
// slot, counted by the caller via the return value (extra staging slots).
static int call_arg_slot(IRInstr *i, int k, int *ng, int *nx, int *nst, int *rg, int *rg2,
                         int *slot0, int *nslot) {
    int e = i->arg_class ? i->arg_class[k] : 0;
    int cls = e & 7;
    int szb = e >> 3;
    *rg = -1;
    *rg2 = -1;
    *slot0 = *nst;
    *nslot = 0;
    if (!cls) {
        if (tflt(V(i->args[k])->type)) {
            if (*nx < 8) *rg = (*nx)++;
            else
                (*nst)++;
        } else {
            if (*ng < 6) *rg = abi_gpr[(*ng)++];
            else
                (*nst)++;
        }
        return 0;
    }
    switch (cls) {
    case IRAGG_VALUE8:
        if (*ng < 6) {
            *rg = abi_gpr[(*ng)++];
            return 1;
        }
        *nslot = 1;
        (*nst)++;
        return 0;
    case IRAGG_VALUE16:
        if (*ng + 1 < 6) {
            *rg = abi_gpr[*ng];
            *rg2 = abi_gpr[*ng + 1];
            *ng += 2;
            return 2;
        }
        *nslot = (szb + 7) / 8;
        *nst += *nslot;
        return 0;
    case IRAGG_PTR: // the value IS the pointer: like a scalar GP argument
        if (*ng < 6) *rg = abi_gpr[(*ng)++];
        else {
            *nslot = 1;
            (*nst)++;
        }
        return 0;
    default: // IRAGG_MEMORY
        *nslot = (szb + 7) / 8;
        *nst += *nslot;
        return 0;
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
    be.blk[0] = NEW(int, be.nv + 1);
    be.blk[1] = NEW(int, be.nv + 1);
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
                if (ki->op == IR_SDIV || ki->op == IR_UDIV || ki->op == IR_SREM || ki->op == IR_UREM)
                    be.blk[1][be.nblk[1]++] = 2 * slot;
                else if ((ki->op == IR_SHL || ki->op == IR_SHR || ki->op == IR_SAR) && !const_of(ki->b, &(int64_t){0}))
                    be.blk[0][be.nblk[0]++] = 2 * slot;
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
    classify_params();
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
                int ng = i->ret_agg < 0 ? 1 : 0, nx = 0, nst = 0; // sret: hidden pointer takes GP slot 0
                for (int a = 0; a < i->nargs; a++) {
                    int v = i->args[a];
                    int rg, rg2, slot0, nsl;
                    call_arg_slot(i, a, &ng, &nx, &nst, &rg, &rg2, &slot0, &nsl);
                    int cls = i->arg_class ? i->arg_class[a] & 7 : 0;
                    // VALUE8 / VALUE16 args are loaded through memory at the
                    // call; only scalar and PTR values want the ABI register.
                    if (rg >= 0 && cls != IRAGG_VALUE8 && cls != IRAGG_VALUE16 &&
                        needs_loc(v) && be.vi[v].abi < 0 && !be.vi[v].stk)
                        be.vi[v].abi = (int8_t)rg;
                }
                break;
            }
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

// rcx / rdx are allocatable but variable shifts (count in cl) and divides
// (rdx:rax) clobber them: a value live across such an instruction (including
// its operands and result) must not live there. blk[0] = rcx, blk[1] = rdx.
static bool blocked_reg(int v, int r) {
    int k = r == X86_RCX ? 0 : 1;
    const int *p = be.blk[k];
    int n = be.nblk[k];
    const VInfo *x = &be.vi[v];
    for (int g = 0; n && g < x->nrg; g++) {
        int lo = 0, hi = n;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (p[mid] >= x->rg[g].from - 1) hi = mid;
            else
                lo = mid + 1;
        }
        if (lo < n && p[lo] < x->rg[g].to) return true;
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
        if (!be.vi[v].dead && be.vi[v].uses > 0 && is_remat_op(V(v)->op)) l->kind = LK_REMAT;
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
            allowed = crosses_call(v) ? MASK_CALLEE : MASK_G_ALL;
            if (blocked_reg(v, X86_RCX)) allowed &= ~(1u << X86_RCX);
            if (blocked_reg(v, X86_RDX)) allowed &= ~(1u << X86_RDX);
        } else {
            allowed = crosses_call(v) ? 0 : MASK_X_ALL;
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
            for (int r = 0; r < 16; r++) {
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

static bool imm32(int v, int64_t *out) {
    int64_t c;
    if (!const_of(v, &c) || !fits32(c)) return false;
    *out = c;
    return true;
}

// lea r, label(%rip) with the same relocation asm_lea_rip_reg() records (that
// helper takes a codegen virtual register index, not a physical register).
static void lea_rip(X86Reg r, const char *label) {
    size_t off = S->len;
    X86Mem m = {X86_RIP, X86_NOREG, 1, 0, 0};
    x86_lea(S, 8, r, m);
    int sidx = objfile_find_sym(cg_obj, label);
    if (sidx < 0) sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, label[0] == '.' ? SB_LOCAL : SB_GLOBAL, ST_NOTYPE);
    objfile_add_reloc(cg_obj, SEC_TEXT, off + 3, sidx, R_X86_64_PC32, -4);
}

// mov r, label@GOTPCREL(%rip): the address of an extern/weak/-fPIC symbol.
static void got_rip(X86Reg r, const char *label) {
    size_t off = S->len;
    X86Mem m = {X86_RIP, X86_NOREG, 1, 0, 0};
    x86_mov_rm(S, 8, r, m);
    int sidx = objfile_find_sym(cg_obj, label);
    if (sidx < 0) sidx = objfile_add_sym(cg_obj, label, SEC_UNDEF, 0, 0, SB_GLOBAL, ST_NOTYPE);
    objfile_add_reloc(cg_obj, SEC_TEXT, off + 3, sidx, R_X86_64_GOTPCREL, -4);
}

static void mat_g(int v, X86Reg r) {
    IRInstr *d = V(v);
    switch (d->op) {
    case IR_ICONST: {
        int64_t c = d->imm;
        if (c == 0) x86_xor_rr(S, 4, r, r); // callers never keep flags live across a materialization
        else if (d->type == IRT_I32)
            x86_mov_ri(S, 4, r, (int32_t)c);
        else if (c >= 0 && c <= 0xffffffffLL)
            x86_mov_ri(S, 4, r, (int32_t)(uint32_t)c);
        else if (fits32(c))
            x86_mov_ri(S, 8, r, c);
        else
            x86_movabs(S, r, (uint64_t)c);
        break;
    }
    case IR_FCONST: { // the bit pattern
        if (d->type == IRT_F32) {
            float fv = (float)d->fimm;
            uint32_t bits;
            memcpy(&bits, &fv, 4);
            x86_mov_ri(S, 4, r, (int32_t)bits);
        } else {
            uint64_t bits;
            memcpy(&bits, &d->fimm, 8);
            if (bits <= 0xffffffffull) x86_mov_ri(S, 4, r, (int32_t)(uint32_t)bits);
            else
                x86_movabs(S, r, bits);
        }
        break;
    }
    case IR_GADDR:
        if (d->msign) got_rip(r, d->sym);
        else
            lea_rip(r, d->sym);
        if (d->imm) x86_lea(S, 8, r, x86_mem(r, d->imm));
        break;
    case IR_FRAMEADDR:
        x86_lea(S, 8, r, off_mem((int)d->imm));
        break;
    case IR_PARAM: // materialized aggregate param: the value is its frame temp
        x86_lea(S, 8, r, off_mem(be.paggoff[d->imm]));
        break;
    default:
        abort();
    }
}

static void mat_x(int v, X86XmmReg x, X86Reg tmp) {
    IRInstr *d = V(v);
    uint64_t bits;
    if (d->type == IRT_F32) {
        float fv = (float)d->fimm;
        uint32_t b32;
        memcpy(&b32, &fv, 4);
        bits = b32;
    } else {
        memcpy(&bits, &d->fimm, 8);
    }
    if (bits == 0) {
        x86_xorps(S, x, x);
        return;
    }
    mat_g(v, tmp);
    if (d->type == IRT_F32) x86_movd_r_xmm(S, x, tmp);
    else
        x86_movq_r_xmm(S, x, tmp);
}

static void ld_x(int ty, X86XmmReg x, X86Mem m) {
    if (ty == IRT_F32) x86_movss_rm(S, x, m);
    else
        x86_movsd_rm(S, x, m);
}
static void st_x(int ty, X86Mem m, X86XmmReg x) {
    if (ty == IRT_F32) x86_movss_mr(S, m, x);
    else
        x86_movsd_mr(S, m, x);
}

// Integer operand access. use_g: a register holding v (its own, or `tmp`).
static X86Reg use_g(int v, X86Reg tmp) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) return (X86Reg)l->reg;
    if (l->kind == LK_SPILL) x86_mov_rm(S, tsz(V(v)->type), tmp, slot_mem(v));
    else
        mat_g(v, tmp);
    return tmp;
}
static void load_g(X86Reg rd, int v) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) {
        if (l->reg != rd) x86_mov_rr(S, tsz(V(v)->type), rd, (X86Reg)l->reg);
    } else if (l->kind == LK_SPILL) {
        x86_mov_rm(S, tsz(V(v)->type), rd, slot_mem(v));
    } else {
        mat_g(v, rd);
    }
}
static X86Reg def_g(int v) { return LOC(v)->kind == LK_REG ? (X86Reg)LOC(v)->reg : R_S0; }
static void fin_g(int v, X86Reg r) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) {
        if (l->reg != r) x86_mov_rr(S, tsz(V(v)->type), (X86Reg)l->reg, r);
    } else if (l->kind == LK_SPILL) {
        x86_mov_mr(S, tsz(V(v)->type), slot_mem(v), r);
    }
}

static X86XmmReg use_x(int v, X86XmmReg tmp) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) return (X86XmmReg)l->reg;
    if (l->kind == LK_SPILL) ld_x(V(v)->type, tmp, slot_mem(v));
    else
        mat_x(v, tmp, R_S1);
    return tmp;
}
static void load_x(X86XmmReg xd, int v) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) {
        if (l->reg != (int)xd) x86_movaps(S, xd, (X86XmmReg)l->reg);
    } else if (l->kind == LK_SPILL) {
        ld_x(V(v)->type, xd, slot_mem(v));
    } else {
        mat_x(v, xd, R_S1);
    }
}
static X86XmmReg def_x(int v) { return LOC(v)->kind == LK_REG ? (X86XmmReg)LOC(v)->reg : X_S0; }
static void fin_x(int v, X86XmmReg x) {
    Loc *l = LOC(v);
    if (l->kind == LK_REG) {
        if (l->reg != (int)x) x86_movaps(S, (X86XmmReg)l->reg, x);
    } else if (l->kind == LK_SPILL) {
        st_x(V(v)->type, slot_mem(v), x);
    }
}

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
        if (s.k == PL_G) x86_mov_rr(S, sz, d.r, s.r);
        else if (s.k == PL_M)
            x86_mov_rm(S, sz, d.r, off_mem(s.off));
        else
            mat_g(s.val, d.r);
        break;
    case PL_X:
        if (s.k == PL_X) x86_movaps(S, d.r, s.r);
        else if (s.k == PL_M)
            ld_x(ty, d.r, off_mem(s.off));
        else
            mat_x(s.val, d.r, R_S0);
        break;
    default:
        if (s.k == PL_G) {
            x86_mov_mr(S, sz, off_mem(d.off), s.r);
        } else if (s.k == PL_X) {
            st_x(ty, off_mem(d.off), s.r);
        } else if (s.k == PL_M) {
            x86_mov_rm(S, sz, R_S0, off_mem(s.off));
            x86_mov_mr(S, sz, off_mem(d.off), R_S0);
        } else {
            int64_t c;
            if (V(s.val)->op == IR_ICONST && imm32(s.val, &c)) {
                x86_mov_mi(S, sz, off_mem(d.off), (int32_t)c);
            } else {
                mat_g(s.val, R_S0);
                x86_mov_mr(S, sz, off_mem(d.off), R_S0);
            }
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

typedef void (*AluRR)(SecBuf *, int, X86Reg, X86Reg);
typedef void (*AluRI)(SecBuf *, int, X86Reg, int32_t);
typedef void (*AluRM)(SecBuf *, int, X86Reg, X86Mem);
static const struct {
    AluRR rr;
    AluRI ri;
    AluRM rm;
    bool comm;
} alu_tab[IR_NUM_OPS] = {
    [IR_ADD] = {x86_add_rr, x86_add_ri, x86_add_rm, true},
    [IR_SUB] = {x86_sub_rr, x86_sub_ri, x86_sub_rm, false},
    [IR_AND] = {x86_and_rr, x86_and_ri, x86_and_rm, true},
    [IR_OR] = {x86_or_rr, x86_or_ri, x86_or_rm, true},
    [IR_XOR] = {x86_xor_rr, x86_xor_ri, x86_xor_rm, true},
    [IR_MUL] = {x86_imul_rr, NULL, NULL, true},
};

static void emit_mul_imm(IRInstr *i, int a, int64_t imm) {
    int sz = tsz(i->type);
    X86Reg rd = def_g(i->id);
    X86Reg ra = LOC(a)->kind == LK_REG ? (X86Reg)LOC(a)->reg : X86_NOREG;
    if (imm > 0 && (imm & (imm - 1)) == 0) {
        load_g(rd, a);
        int k = 0;
        while ((1ll << k) < imm) k++;
        if (k) x86_shl_ri(S, sz, rd, (uint8_t)k);
    } else if ((imm == 3 || imm == 5 || imm == 9) && ra != X86_NOREG) {
        x86_lea(S, sz, rd, x86_mem_idx(ra, ra, (int)imm - 1, 0));
    } else {
        X86Reg src = ra;
        if (src == X86_NOREG) {
            load_g(rd, a);
            src = rd;
        }
        x86_imul_rri(S, sz, rd, src, (int32_t)imm);
    }
    fin_g(i->id, rd);
}

static X86Mem mem_of(int a, int b, int scale, int64_t disp, int kind, X86Reg tb, X86Reg ti) {
    X86Reg base = kind == 1 ? X86_RBP : use_g(a, tb);
    X86Reg idx = b != IR_NONE ? use_g(b, ti) : X86_NOREG;
    return x86_mem_idx(base, idx, scale, disp);
}
// ALU op whose second operand is a memory operand (a folded single-use LOAD).
static void emit_alu_rm(IRInstr *i) {
    const int sz = tsz(i->type), id = i->id;
    X86Reg rd = def_g(id);
    X86Mem m = mem_of(i->b, i->c, i->scale, i->imm, i->aux, R_S1, R_S0);
    // A scratch index forces the destination out of rax (dst spilled): fold the address.
    if (rd == R_S0 && m.index == R_S0) {
        x86_lea(S, 8, R_S1, m);
        m = x86_mem(R_S1, 0);
    }
    X86Reg work = rd;
    if (rd == m.base || rd == m.index) // loading `a` would clobber the address
        work = (m.base != R_S0 && m.index != R_S0) ? R_S0 : R_S1;
    load_g(work, i->a);
    alu_tab[i->op].rm(S, sz, work, m);
    fin_g(id, work);
}

static void emit_alu(IRInstr *i) {
    if (i->mw == 1) {
        emit_alu_rm(i);
        return;
    }
    const int sz = tsz(i->type), op = i->op, id = i->id;
    int a = i->a, b = i->b;
    int64_t imm;
    const bool comm = alu_tab[op].comm;
    if (comm && V(a)->op == IR_ICONST && V(b)->op != IR_ICONST) {
        int t = a;
        a = b;
        b = t;
    }
    X86Reg rd = def_g(id);
    X86Reg ra = LOC(a)->kind == LK_REG ? (X86Reg)LOC(a)->reg : X86_NOREG;
    if (imm32(b, &imm)) {
        if (op == IR_MUL) {
            emit_mul_imm(i, a, imm);
            return;
        }
        if ((op == IR_ADD || (op == IR_SUB && imm != INT32_MIN)) && ra != X86_NOREG && ra != rd) {
            x86_lea(S, sz, rd, x86_mem(ra, op == IR_ADD ? imm : -imm));
            fin_g(id, rd);
            return;
        }
        load_g(rd, a);
        if (imm != 0 || op == IR_AND) alu_tab[op].ri(S, sz, rd, (int32_t)imm);
        fin_g(id, rd);
        return;
    }
    X86Reg rb = LOC(b)->kind == LK_REG ? (X86Reg)LOC(b)->reg : X86_NOREG;
    if (op == IR_ADD && ra != X86_NOREG && rb != X86_NOREG && rd != ra && rd != rb) {
        x86_lea(S, sz, rd, x86_mem_idx(ra, rb, 1, 0));
        fin_g(id, rd);
        return;
    }
    X86Reg work = rd;
    if (rb != X86_NOREG && rb == work && ra != work) {
        if (comm) {
            int t = a;
            a = b;
            b = t;
        } else {
            // rd = a - rd  ==>  neg rd; add rd, a
            x86_neg_r(S, sz, rd);
            if (V(a)->op == IR_ICONST && imm32(a, &imm)) {
                x86_add_ri(S, sz, rd, (int32_t)imm);
            } else if (LOC(a)->kind == LK_SPILL) {
                x86_add_rm(S, sz, rd, slot_mem(a));
            } else {
                X86Reg t = use_g(a, R_S1);
                x86_add_rr(S, sz, rd, t);
            }
            fin_g(id, rd);
            return;
        }
    }
    load_g(work, a);
    Loc *lb = LOC(b);
    if (lb->kind == LK_REG) alu_tab[op].rr(S, sz, work, (X86Reg)lb->reg);
    else if (lb->kind == LK_SPILL && alu_tab[op].rm)
        alu_tab[op].rm(S, sz, work, slot_mem(b));
    else
        alu_tab[op].rr(S, sz, work, use_g(b, R_S1));
    fin_g(id, work);
}

static void emit_shift(IRInstr *i) {
    const int sz = tsz(i->type), id = i->id;
    int64_t imm;
    X86Reg rd = def_g(id);
    if (imm32(i->b, &imm)) {
        load_g(rd, i->a);
        uint8_t cnt = (uint8_t)(imm & (sz * 8 - 1));
        if (cnt) {
            if (i->op == IR_SHL) x86_shl_ri(S, sz, rd, cnt);
            else if (i->op == IR_SHR)
                x86_shr_ri(S, sz, rd, cnt);
            else
                x86_sar_ri(S, sz, rd, cnt);
        }
    } else {
        load_g(X86_RCX, i->b);
        load_g(rd, i->a);
        if (i->op == IR_SHL) x86_shl_rcl(S, sz, rd);
        else if (i->op == IR_SHR)
            x86_shr_rcl(S, sz, rd);
        else
            x86_sar_rcl(S, sz, rd);
    }
    fin_g(id, rd);
}

static void emit_div(IRInstr *i) {
    const int sz = tsz(i->type);
    bool sgn = i->op == IR_SDIV || i->op == IR_SREM;
    bool quo = i->op == IR_SDIV || i->op == IR_UDIV;
    X86Reg dv = LOC(i->b)->kind == LK_REG ? (X86Reg)LOC(i->b)->reg : R_S1;
    if (dv == R_S1) load_g(R_S1, i->b);
    load_g(R_S0, i->a);
    if (sgn) {
        if (sz == 4) x86_cdq(S);
        else
            x86_cqo(S);
        x86_idiv_r(S, sz, dv);
    } else {
        x86_xor_rr(S, 4, X86_RDX, X86_RDX);
        x86_div_r(S, sz, dv);
    }
    fin_g(i->id, quo ? R_S0 : X86_RDX);
}

static void emit_unary(IRInstr *i) {
    const int sz = tsz(i->type);
    X86Reg rd = def_g(i->id);
    load_g(rd, i->a);
    if (i->op == IR_NEG) x86_neg_r(S, sz, rd);
    else
        x86_not_r(S, sz, rd);
    fin_g(i->id, rd);
}

static const uint8_t cc_x86[] = {
    [IRCC_EQ] = X86_E,
    [IRCC_NE] = X86_NE,
    [IRCC_SLT] = X86_L,
    [IRCC_SLE] = X86_LE,
    [IRCC_SGT] = X86_G,
    [IRCC_SGE] = X86_GE,
    [IRCC_ULT] = X86_B,
    [IRCC_ULE] = X86_BE,
    [IRCC_UGT] = X86_A,
    [IRCC_UGE] = X86_AE,
};
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
};

// Emits `cmp a, b` and returns the x86 condition that holds iff `a cc b`.
static int emit_icmp(int a, int b, int cc) {
    int sz = tsz(V(a)->type);
    int64_t imm;
    if (V(a)->op == IR_ICONST && V(b)->op != IR_ICONST) {
        int t = a;
        a = b;
        b = t;
        cc = cc_swap[cc];
    }
    if (imm32(b, &imm)) {
        if (LOC(a)->kind == LK_SPILL) {
            x86_cmp_mi(S, sz, slot_mem(a), (int32_t)imm);
        } else {
            X86Reg ra = use_g(a, R_S0);
            if (imm == 0) x86_test_rr(S, sz, ra, ra);
            else
                x86_cmp_ri(S, sz, ra, (int32_t)imm);
        }
    } else if (LOC(b)->kind == LK_REG) {
        if (LOC(a)->kind == LK_SPILL) {
            x86_cmp_mr(S, sz, slot_mem(a), (X86Reg)LOC(b)->reg);
        } else {
            X86Reg ra = use_g(a, R_S0);
            x86_cmp_rr(S, sz, ra, (X86Reg)LOC(b)->reg);
        }
    } else if (LOC(b)->kind == LK_SPILL) {
        X86Reg ra = use_g(a, R_S0);
        x86_cmp_rm(S, sz, ra, slot_mem(b));
    } else {
        X86Reg rb = use_g(b, R_S1);
        X86Reg ra = use_g(a, R_S0);
        x86_cmp_rr(S, sz, ra, rb);
    }
    return cc_x86[cc];
}

typedef struct {
    int cc;
    int pf; // 0: cc alone; 1: cc && !PF (equal); 2: cc || PF (not equal)
} FCond;

static FCond emit_fcmp(int a, int b, int cc) {
    if (cc == IRCC_FLT || cc == IRCC_FLE) {
        int t = a;
        a = b;
        b = t;
        cc = cc == IRCC_FLT ? IRCC_FGT : IRCC_FGE;
    }
    X86XmmReg xa = use_x(a, X_S0), xb = use_x(b, X_S1);
    if (V(a)->type == IRT_F32) x86_ucomiss(S, xa, xb);
    else
        x86_ucomisd(S, xa, xb);
    switch (cc) {
    case IRCC_FGT: return (FCond){X86_A, 0};
    case IRCC_FGE: return (FCond){X86_AE, 0};
    case IRCC_FEQ: return (FCond){X86_E, 1};
    default: return (FCond){X86_NE, 2};
    }
}

static void emit_cmp_value(IRInstr *i) {
    if (i->op == IR_CMP) {
        int cc = emit_icmp(i->a, i->b, i->cc);
        X86Reg rd = def_g(i->id);
        x86_setcc(S, (X86Cond)cc, rd);
        x86_movzx(S, 4, 1, rd, rd);
        fin_g(i->id, rd);
        return;
    }
    FCond fc = emit_fcmp(i->a, i->b, i->cc);
    X86Reg rd = fc.pf ? R_S0 : def_g(i->id);
    x86_setcc(S, (X86Cond)fc.cc, rd);
    if (fc.pf) {
        x86_setcc(S, fc.pf == 1 ? X86_NP : X86_P, R_S1);
        if (fc.pf == 1) x86_and_rr(S, 4, R_S0, R_S1);
        else
            x86_or_rr(S, 4, R_S0, R_S1);
    }
    x86_movzx(S, 4, 1, rd, rd);
    fin_g(i->id, rd);
}

// SSE scalar arithmetic with a memory source: encoded as movss/movsd (same
// layout) with the opcode byte rewritten, the encoder has no rm form for them.
static void fp_arith_rm(int ty, uint8_t opc, X86XmmReg x, X86Mem m) {
    size_t off = S->len;
    ld_x(ty, x, m);
    S->data[off + 2 + ((S->data[off + 1] & 0xf0) == 0x40)] = opc;
}

static void emit_fbin_rm(IRInstr *i) {
    static const uint8_t opc[] = {0x58, 0x5c, 0x59, 0x5e}; // FADD FSUB FMUL FDIV
    X86XmmReg rd = def_x(i->id);
    load_x(rd, i->a);
    X86Mem m = mem_of(i->b, i->c, i->scale, i->imm, i->aux, R_S0, R_S1);
    fp_arith_rm(i->type, opc[i->op - IR_FADD], rd, m);
    fin_x(i->id, rd);
}

static void emit_fbin(IRInstr *i) {
    if (i->mw == 1) {
        emit_fbin_rm(i);
        return;
    }
    typedef void (*F)(SecBuf *, X86XmmReg, X86XmmReg);
    static const F tab_d[] = {[IR_FADD - IR_FADD] = x86_addsd, x86_subsd, x86_mulsd, x86_divsd};
    static const F tab_s[] = {[IR_FADD - IR_FADD] = x86_addss, x86_subss, x86_mulss, x86_divss};
    F fn = (i->type == IRT_F32 ? tab_s : tab_d)[i->op - IR_FADD];
    bool comm = i->op == IR_FADD || i->op == IR_FMUL;
    int a = i->a, b = i->b;
    X86XmmReg rd = def_x(i->id);
    int ra = LOC(a)->kind == LK_REG ? LOC(a)->reg : -1, rb = LOC(b)->kind == LK_REG ? LOC(b)->reg : -1;
    X86XmmReg work = rd;
    if (rb >= 0 && rb == (int)work && ra != (int)work) {
        if (comm) {
            int t = a;
            a = b;
            b = t;
        } else {
            work = X_S0;
        }
    }
    load_x(work, a);
    fn(S, work, use_x(b, X_S1));
    fin_x(i->id, work);
}

static void emit_fneg(IRInstr *i) {
    X86XmmReg rd = def_x(i->id);
    load_x(rd, i->a);
    if (i->type == IRT_F32) {
        x86_mov_ri(S, 4, R_S1, (int32_t)0x80000000u);
        x86_movd_r_xmm(S, X_S1, R_S1);
        x86_xorps(S, rd, X_S1);
    } else {
        x86_movabs(S, R_S1, 0x8000000000000000ull);
        x86_movq_r_xmm(S, X_S1, R_S1);
        x86_xorpd(S, rd, X_S1);
    }
    fin_x(i->id, rd);
}

static const char *local_label(void) { return format(".L.ir.%s.%d.t%d", be.sym, be.serial, be.nlocal_lbl++); }

static void emit_conv(IRInstr *i) {
    int id = i->id, a = i->a;
    switch (i->op) {
    case IR_SEXT8:
    case IR_SEXT16:
    case IR_ZEXT8:
    case IR_ZEXT16:
    case IR_SEXT32: {
        X86Reg rd = def_g(id);
        bool sx = i->op == IR_SEXT8 || i->op == IR_SEXT16 || i->op == IR_SEXT32;
        int dsz = i->op == IR_SEXT32 ? 8 : 4;
        int ssz = (i->op == IR_SEXT8 || i->op == IR_ZEXT8) ? 1 : (i->op == IR_SEXT32 ? 4 : 2);
        Loc *l = LOC(a);
        if (l->kind == LK_SPILL) {
            if (sx) x86_movsx_rm(S, dsz, ssz, rd, slot_mem(a));
            else
                x86_movzx_rm(S, dsz, ssz, rd, slot_mem(a));
        } else {
            X86Reg ra = use_g(a, rd);
            if (sx) x86_movsx(S, dsz, ssz, rd, ra);
            else
                x86_movzx(S, dsz, ssz, rd, ra);
        }
        fin_g(id, rd);
        break;
    }
    case IR_ZEXT32: {
        X86Reg rd = def_g(id);
        Loc *l = LOC(a);
        if (l->kind == LK_REG) x86_mov_rr(S, 4, rd, (X86Reg)l->reg);
        else if (l->kind == LK_SPILL)
            x86_mov_rm(S, 4, rd, slot_mem(a));
        else
            mat_g(a, rd);
        fin_g(id, rd);
        break;
    }
    case IR_TRUNC: {
        X86Reg rd = def_g(id);
        Loc *l = LOC(a);
        if (l->kind == LK_REG) {
            if (l->reg != rd) x86_mov_rr(S, 4, rd, (X86Reg)l->reg);
        } else if (l->kind == LK_SPILL) {
            x86_mov_rm(S, 4, rd, slot_mem(a));
        } else {
            mat_g(a, rd);
        }
        fin_g(id, rd);
        break;
    }
    case IR_SITOF: {
        X86XmmReg rd = def_x(id);
        X86Reg ra = use_g(a, R_S0);
        int ssz = tsz(V(a)->type);
        x86_xorps(S, rd, rd);
        if (i->type == IRT_F32) x86_cvtsi2ss(S, ssz, rd, ra);
        else
            x86_cvtsi2sd(S, ssz, rd, ra);
        fin_x(id, rd);
        break;
    }
    case IR_UITOF: {
        bool f32 = i->type == IRT_F32;
        X86XmmReg rd = def_x(id);
        const char *big = local_label(), *done = local_label();
        load_g(R_S0, a);
        x86_xorps(S, rd, rd);
        x86_test_rr(S, 8, R_S0, R_S0);
        cg_ir_jcc(X86_S, big);
        if (f32) x86_cvtsi2ss(S, 8, rd, R_S0);
        else
            x86_cvtsi2sd(S, 8, rd, R_S0);
        cg_ir_jmp(done);
        cg_ir_label(big);
        x86_mov_rr(S, 8, R_S1, R_S0);
        x86_shr_ri(S, 8, R_S0, 1);
        x86_and_ri(S, 4, R_S1, 1);
        x86_or_rr(S, 8, R_S0, R_S1);
        if (f32) {
            x86_cvtsi2ss(S, 8, rd, R_S0);
            x86_addss(S, rd, rd);
        } else {
            x86_cvtsi2sd(S, 8, rd, R_S0);
            x86_addsd(S, rd, rd);
        }
        cg_ir_label(done);
        fin_x(id, rd);
        break;
    }
    case IR_FTOSI: {
        X86Reg rd = def_g(id);
        X86XmmReg xs = use_x(a, X_S0);
        int dsz = tsz(i->type);
        if (V(a)->type == IRT_F32) x86_cvttss2si(S, dsz, rd, xs);
        else
            x86_cvttsd2si(S, dsz, rd, xs);
        fin_g(id, rd);
        break;
    }
    case IR_FTOU64: {
        bool f32 = V(a)->type == IRT_F32;
        const char *big = local_label(), *done = local_label();
        X86XmmReg xs = use_x(a, X_S0);
        if (f32) {
            x86_mov_ri(S, 4, R_S1, 0x5f000000); // 2^63 as float
            x86_movd_r_xmm(S, X_S1, R_S1);
            x86_ucomiss(S, xs, X_S1);
        } else {
            x86_movabs(S, R_S1, 0x43e0000000000000ull); // 2^63 as double
            x86_movq_r_xmm(S, X_S1, R_S1);
            x86_ucomisd(S, xs, X_S1);
        }
        cg_ir_jcc(X86_AE, big);
        if (f32) x86_cvttss2si(S, 8, R_S0, xs);
        else
            x86_cvttsd2si(S, 8, R_S0, xs);
        cg_ir_jmp(done);
        cg_ir_label(big);
        if (xs != X_S0) x86_movaps(S, X_S0, xs);
        if (f32) {
            x86_subss(S, X_S0, X_S1);
            x86_cvttss2si(S, 8, R_S0, X_S0);
        } else {
            x86_subsd(S, X_S0, X_S1);
            x86_cvttsd2si(S, 8, R_S0, X_S0);
        }
        x86_movabs(S, R_S1, 0x8000000000000000ull);
        x86_xor_rr(S, 8, R_S0, R_S1);
        cg_ir_label(done);
        fin_g(id, R_S0);
        break;
    }
    case IR_FEXT: {
        X86XmmReg rd = def_x(id);
        x86_cvtss2sd(S, rd, use_x(a, X_S0));
        fin_x(id, rd);
        break;
    }
    case IR_FTRUNC: {
        X86XmmReg rd = def_x(id);
        x86_cvtsd2ss(S, rd, use_x(a, X_S0));
        fin_x(id, rd);
        break;
    }
    default:
        abort();
    }
}

// Memory operand of a LOAD/STORE; scratch registers hold spilled/remat parts.
static X86Mem build_mem(const IRInstr *m, X86Reg tb, X86Reg ti) {
    return mem_of(m->a, m->b, m->scale, m->imm, m->aux, tb, ti);
}

static void emit_load(IRInstr *i) {
    X86Mem m = build_mem(i, R_S0, R_S1);
    if (tflt(i->type)) {
        X86XmmReg rd = def_x(i->id);
        ld_x(i->type, rd, m);
        fin_x(i->id, rd);
        return;
    }
    X86Reg rd = def_g(i->id);
    switch (i->mw) {
    case IRM_I8:
        if (i->msign) x86_movsx_rm(S, 4, 1, rd, m);
        else
            x86_movzx_rm(S, 4, 1, rd, m);
        break;
    case IRM_I16:
        if (i->msign) x86_movsx_rm(S, 4, 2, rd, m);
        else
            x86_movzx_rm(S, 4, 2, rd, m);
        break;
    case IRM_I32:
        if (i->type == IRT_I64 && i->msign) x86_movsx_rm(S, 8, 4, rd, m);
        else
            x86_mov_rm(S, 4, rd, m);
        break;
    default:
        x86_mov_rm(S, 8, rd, m);
        break;
    }
    fin_g(i->id, rd);
}

static void emit_store(IRInstr *i) {
    X86Mem m = build_mem(i, R_S0, R_S1);
    int c = i->c;
    // A base and an index that both sit in scratch registers leave none for
    // the value: fold the address first.
    if (m.index != X86_NOREG && (m.base == R_S0 || m.base == R_S1) && (m.index == R_S0 || m.index == R_S1)) {
        x86_lea(S, 8, R_S0, m);
        m = x86_mem(R_S0, 0);
    }
    X86Reg vt = (m.base == R_S0 || m.index == R_S0) ? R_S1 : R_S0; // scratch for the value
    if (i->mw == IRM_F32 || i->mw == IRM_F64) {
        X86XmmReg x = X_S0;
        if (LOC(c)->kind == LK_REMAT) mat_x(c, X_S0, vt);
        else
            x = use_x(c, X_S0);
        st_x(i->mw == IRM_F32 ? IRT_F32 : IRT_F64, m, x);
        return;
    }
    int sz = i->mw == IRM_I8 ? 1 : i->mw == IRM_I16 ? 2
        : i->mw == IRM_I32                          ? 4
                                                    : 8;
    int64_t imm;
    if (imm32(c, &imm)) {
        x86_mov_mi(S, sz, m, (int32_t)imm);
        return;
    }
    X86Reg r = use_g(c, vt);
    // Byte stores from sil/dil would encode dh/bh without a REX prefix.
    if (sz == 1 && (r == X86_RSI || r == X86_RDI)) {
        x86_mov_rr(S, 4, vt, r);
        r = vt;
    }
    x86_mov_mr(S, sz, m, r);
}

static void emit_call(IRInstr *c) {
    int ng = c->ret_agg < 0 ? 1 : 0, nx = 0, nst = 0; // sret: hidden result pointer takes GP slot 0
    Mv *mv = NEW(Mv, c->nargs + 1);
    int *prg = NEW(int, c->nargs), *prg2 = NEW(int, c->nargs);
    int *pslot = NEW(int, c->nargs), *pnsl = NEW(int, c->nargs);
    int nm = 0, stage = 0;
    for (int k = 0; k < c->nargs; k++)
        stage += call_arg_slot(c, k, &ng, &nx, &nst, &prg[k], &prg2[k], &pslot[k], &pnsl[k]);
    int stbase = nst; // staging slots sit above the stack argument copies
    stage = 0;
    for (int k = 0; k < c->nargs; k++) {
        int a = c->args[k], t = V(a)->type;
        int e = c->arg_class ? c->arg_class[k] : 0;
        int cls = e & 7, szb = e >> 3;
        if (!cls || cls == IRAGG_PTR) { // scalar / pointer value
            if (prg[k] >= 0) {
                mv[nm++] = (Mv){pl_reg(tflt(t) ? RC_X : RC_G, prg[k]), pl_of(a), t};
            } else {
                X86Mem m = x86_mem(X86_RSP, 8 * pslot[k]);
                int64_t imm;
                if (tflt(t)) {
                    st_x(t, m, use_x(a, X_S0));
                } else if (imm32(a, &imm)) {
                    x86_mov_mi(S, tsz(t), m, (int32_t)imm);
                } else {
                    x86_mov_mr(S, tsz(t), m, use_g(a, R_S0));
                }
            }
            continue;
        }
        X86Reg ar = use_g(a, R_S0); // address of the argument bytes
        if (cls == IRAGG_VALUE8) {
            int w = szb <= 4 ? 4 : 8;
            x86_mov_rm(S, w, R_S1, x86_mem(ar, 0));
            if (prg[k] >= 0) {
                x86_mov_mr(S, 8, x86_mem(X86_RSP, 8 * (stbase + stage)), R_S1);
                stage++;
            } else {
                x86_mov_mr(S, w, x86_mem(X86_RSP, 8 * pslot[k]), R_S1);
            }
        } else { // VALUE16 (2 slots) or MEMORY ((szb+7)/8 slots); the tail slot
            // loads only the remaining bytes (chunk <= 4 ? 4 : 8), like the AST
            // codegen, so we never read past the source object.
            int pn = (szb + 7) / 8;
            int base = prg[k] >= 0 ? stbase + stage : pslot[k];
            if (prg[k] >= 0) stage += 2;
            for (int j = 0; j < pn; j++) {
                int chunk = szb - 8 * j;
                int ld = chunk <= 4 ? 4 : 8;
                x86_mov_rm(S, ld, R_S1, x86_mem(ar, 8 * j));
                x86_mov_mr(S, 8, x86_mem(X86_RSP, 8 * (base + j)), R_S1);
            }
        }
    }
    // The indirect target goes through r10: not an argument register and never
    // a pm temporary.
    if (!c->sym) mv[nm++] = (Mv){pl_reg(RC_G, X86_R10), pl_of(c->a), IRT_I64};
    pm_run(mv, nm);
    // sret: the result temp address is the hidden first argument. rdi is never
    // a pm destination here (the walks above start GP counting at 1) and no
    // interval is live in rdi across the call (caller-saved).
    if (c->ret_agg < 0) x86_lea(S, 8, X86_RDI, off_mem(be.ctemp[c->id]));
    // Register-placed aggregates were staged in the outgoing area; load them
    // into their ABI registers now that pm_run has freed them.
    stage = 0;
    for (int k = 0; k < c->nargs; k++) {
        int cls = c->arg_class ? c->arg_class[k] & 7 : 0;
        if (cls == IRAGG_VALUE8 && prg[k] >= 0) {
            int e = c->arg_class[k];
            int w = (e >> 3) <= 4 ? 4 : 8;
            x86_mov_rm(S, w, (X86Reg)prg[k], x86_mem(X86_RSP, 8 * (stbase + stage)));
            stage++;
        } else if (cls == IRAGG_VALUE16 && prg[k] >= 0) {
            x86_mov_rm(S, 8, (X86Reg)prg[k], x86_mem(X86_RSP, 8 * (stbase + stage)));
            x86_mov_rm(S, 8, (X86Reg)prg2[k], x86_mem(X86_RSP, 8 * (stbase + stage + 1)));
            stage += 2;
        }
    }
    if (c->variadic) x86_mov_ri(S, 4, R_S0, nx);
    if (c->sym) cg_ir_call(c->sym);
    else
        x86_call_r(S, X86_R10);
    if (c->ret_agg > 0 && LOC(c->id)->kind != LK_NONE) {
        // VALUE16-class return: RAX[:RDX] hold the bytes; spill to the temp.
        int tmp = be.ctemp[c->id];
        x86_mov_mr(S, 8, off_mem(tmp), X86_RAX);
        if (c->ret_agg > 8) x86_mov_mr(S, 8, off_mem(tmp - 8), X86_RDX);
        X86Reg rd = def_g(c->id);
        x86_lea(S, 8, rd, off_mem(tmp));
        fin_g(c->id, rd);
    } else if (c->ret_agg < 0 && LOC(c->id)->kind != LK_NONE) {
        // sret: the callee returns the same address, but materialize ours.
        X86Reg rd = def_g(c->id);
        x86_lea(S, 8, rd, off_mem(be.ctemp[c->id]));
        fin_g(c->id, rd);
    } else if (c->type != IRT_VOID && LOC(c->id)->kind != LK_NONE) {
        if (tflt(c->type)) fin_x(c->id, X86_XMM0);
        else
            fin_g(c->id, R_S0);
    }
    free(mv);
    free(prg);
    free(prg2);
    free(pslot);
    free(pnsl);
}

static void emit_epilogue(void) {
    if (be.frameless) {
        x86_ret(S);
        return;
    }
    for (int k = 0; k < be.ncs; k++) x86_mov_rm(S, 8, be.cs_reg[k], off_mem(be.cs_base + 8 * (k + 1)));
    x86_leave(S);
    x86_ret(S);
}

static void jmp_to(IRBlock *t, IRBlock *nxt) {
    if (t != nxt) cg_ir_jmp(blabel(t));
}

static void emit_br(IRInstr *br, IRBlock *nxt) {
    IRBlock *T = resolve(br->t), *F = resolve(br->f);
    int cc;
    int pf = 0;
    if (br->mw == 1) {
        cc = emit_icmp(br->a, br->b, br->cc);
    } else if (br->mw == 2) {
        FCond fc = emit_fcmp(br->a, br->b, br->cc);
        cc = fc.cc;
        pf = fc.pf;
    } else {
        int64_t imm;
        if (const_of(br->a, &imm)) {
            jmp_to(imm ? T : F, nxt);
            return;
        }
        if (LOC(br->a)->kind == LK_SPILL) {
            x86_cmp_mi(S, 4, slot_mem(br->a), 0);
        } else {
            X86Reg r = use_g(br->a, R_S0);
            x86_test_rr(S, 4, r, r);
        }
        cc = X86_NE;
    }
    if (T == F) {
        jmp_to(T, nxt);
    } else if (pf == 0) {
        if (F == nxt) {
            cg_ir_jcc(cc, blabel(T));
        } else if (T == nxt) {
            cg_ir_jcc(cc ^ 1, blabel(F));
        } else if (be.oidx[F->id] < be.oidx[T->id]) { // the backward edge gets the conditional jump
            cg_ir_jcc(cc ^ 1, blabel(F));
            cg_ir_jmp(blabel(T));
        } else {
            cg_ir_jcc(cc, blabel(T));
            cg_ir_jmp(blabel(F));
        }
    } else if (pf == 1) { // true iff ZF && !PF
        cg_ir_jcc(X86_P, blabel(F));
        if (T == nxt) {
            cg_ir_jcc(X86_NE, blabel(F));
        } else {
            cg_ir_jcc(X86_E, blabel(T));
            jmp_to(F, nxt);
        }
    } else { // true iff !ZF || PF
        cg_ir_jcc(X86_P, blabel(T));
        if (F == nxt) {
            cg_ir_jcc(X86_NE, blabel(T));
        } else {
            cg_ir_jcc(X86_E, blabel(F));
            jmp_to(T, nxt);
        }
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
            if (be.f->ret_agg > 0 && !be.f->ret_sret) {
                // VALUE16-class aggregate return: RAX = [a], RDX = [a+8].
                X86Reg r = use_g(i->a, R_S1);
                x86_mov_rm(S, 8, R_S0, x86_mem(r, 0));
                if (be.f->ret_agg > 8) x86_mov_rm(S, 8, X86_RDX, x86_mem(r, 8));
            } else if (tflt(V(i->a)->type)) {
                load_x(X86_XMM0, i->a);
            } else {
                // scalars, and sret (a IS the hidden return pointer value)
                load_g(R_S0, i->a);
            }
        }
        emit_epilogue();
        break;
    case IR_UNREACHABLE:
        x86_ud2(S);
        break;
    default:
        abort();
    }
}

static void frame_layout(void) {
    IRFunc *f = be.f;
    // Aggregate params (VALUE8 / VALUE16 / MEMORY) are materialized into frame
    // temps; their PARAM value rematerializes as the temp address.
    IRBlock *entry = f->blocks[0];
    for (int k = 0; k < entry->nins; k++) {
        IRInstr *p = entry->ins[k];
        if (p->op != IR_PARAM || p->imm >= f->nparams) continue;
        if (be.pcls[p->imm] == IRAGG_VALUE8 || be.pcls[p->imm] == IRAGG_VALUE16 ||
            be.pcls[p->imm] == IRAGG_MEMORY)
            LOC(p->id)->kind = LK_REMAT;
    }
    uint32_t used = 0;
    for (int v = 0; v < be.nv; v++)
        if (LOC(v)->kind == LK_REG && rc_of(v) == RC_G) used |= 1u << LOC(v)->reg;
    for (size_t k = 0; k < sizeof gpr_callee_order; k++)
        if (used >> gpr_callee_order[k] & 1) be.cs_reg[be.ncs++] = gpr_callee_order[k];
    // Aggregate temp area below the address-taken locals: materialized
    // aggregate params first, then one temp per aggregate-returning call.
    // An offset o addresses bytes [rbp-o, rbp-o+sz): o is the distance from
    // rbp at which the storage ENDS, matching the FRAMEADDR/LVar convention.
    // Locals are sized from the FRAMEADDRs that survived inlining and the
    // optimizer (AST stack_size also counts slots SSA construction promoted
    // to registers, which would reserve dead stack on every call).
    int lbytes = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; b && k < b->nins; k++)
            if (b->ins[k]->op == IR_FRAMEADDR && b->ins[k]->imm > lbytes) lbytes = (int)b->ins[k]->imm;
    }
    int aggbase = (lbytes + 7) & ~7;
    int aggtotal = 0;
    for (int k = 0; k < f->nparams; k++) {
        if (be.pcls[k] != IRAGG_VALUE8 && be.pcls[k] != IRAGG_VALUE16 && be.pcls[k] != IRAGG_MEMORY)
            continue;
        int szb = ((f->param_class[k] >> 3) + 7) & ~7;
        aggtotal += szb;
        be.paggoff[k] = aggbase + aggtotal;
    }
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_CALL || be.vi[i->id].dead || !i->ret_agg) continue;
            int szb = ((i->ret_agg < 0 ? -i->ret_agg : i->ret_agg) + 7) & ~7;
            aggtotal += szb;
            be.ctemp[i->id] = aggbase + aggtotal;
        }
    }
    be.locals = aggbase + aggtotal;
    be.cs_base = be.locals;
    be.spill_base = be.cs_base + 8 * be.ncs;
    int nslots = be.nspill[RC_G] + be.nspill[RC_X];
    for (int v = 0; v < be.nv; v++) {
        VInfo *x = &be.vi[v];
        if (LOC(v)->kind != LK_SPILL || x->slotidx < 0) continue;
        int idx = x->slotidx + (rc_of(v) == RC_X ? be.nspill[RC_G] : 0);
        LOC(v)->slot = be.spill_base + 8 * (idx + 1);
    }
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_CALL || be.vi[i->id].dead) continue;
            int ng = i->ret_agg < 0 ? 1 : 0, nx = 0, nst = 0, stage = 0;
            for (int a = 0; a < i->nargs; a++) {
                int rg, rg2, slot0, nsl;
                stage += call_arg_slot(i, a, &ng, &nx, &nst, &rg, &rg2, &slot0, &nsl);
            }
            if (8 * (nst + stage) > be.outgoing) be.outgoing = 8 * (nst + stage);
        }
    }
    int total = be.spill_base + 8 * nslots + be.outgoing;
    be.frame = (total + 15) & ~15;
    // A leaf that touches neither the frame nor callee-saved registers needs no
    // frame pointer at all.
    be.frameless = be.ncall == 0 && be.ncs == 0 && nslots == 0;
    for (int k = 0; k < f->nparams; k++)
        if (be.pstk[k] || be.pcls[k] == IRAGG_VALUE8 || be.pcls[k] == IRAGG_VALUE16 ||
            be.pcls[k] == IRAGG_MEMORY)
            be.frameless = false;
    for (int v = 0; v < be.nv; v++)
        if (LOC(v)->kind == LK_REMAT && V(v)->op == IR_FRAMEADDR) be.frameless = false;
    if (aggtotal) be.frameless = false;
    for (int bi = 0; bi < be.nord; bi++) {
        IRBlock *b = be.ord[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if ((i->op == IR_LOAD || i->op == IR_STORE) && i->aux == 1 && emitted(i)) be.frameless = false;
        }
    }
}

static void emit_prologue(void) {
    IRFunc *f = be.f;
    if (!be.frameless) {
        x86_push(S, X86_RBP);
        x86_mov_rr(S, 8, X86_RBP, X86_RSP);
        if (be.frame) x86_sub_ri(S, 8, X86_RSP, be.frame);
        for (int k = 0; k < be.ncs; k++) x86_mov_mr(S, 8, off_mem(be.cs_base + 8 * (k + 1)), be.cs_reg[k]);
    }
    IRBlock *entry = f->blocks[0];
    // Materialize aggregate params (VALUE8 / VALUE16 / MEMORY) into their
    // frame temps; the PARAM value rematerializes as the temp address.
    for (int k = 0; k < entry->nins; k++) {
        IRInstr *p = entry->ins[k];
        if (p->op != IR_PARAM || p->imm >= f->nparams) continue;
        int idx = (int)p->imm;
        int cls = be.pcls[idx];
        if (cls != IRAGG_VALUE8 && cls != IRAGG_VALUE16 && cls != IRAGG_MEMORY) continue;
        int szb = f->param_class[idx] >> 3;
        int toff = be.paggoff[idx];
        if (cls == IRAGG_VALUE8) {
            int w = szb <= 4 ? 4 : 8;
            if (be.pstk[idx]) {
                x86_mov_rm(S, w, R_S1, x86_mem(X86_RBP, be.pstk[idx]));
                x86_mov_mr(S, w, off_mem(toff), R_S1);
            } else {
                x86_mov_mr(S, w, off_mem(toff), (X86Reg)be.pabi[idx]);
            }
        } else {
            int pn = (szb + 7) / 8;
            for (int j = 0; j < pn; j++) {
                if (be.pstk[idx]) {
                    x86_mov_rm(S, 8, R_S1, x86_mem(X86_RBP, be.pstk[idx] + 8 * j));
                } else if (j == 0) {
                    x86_mov_rr(S, 8, R_S1, (X86Reg)be.pabi[idx]);
                } else {
                    x86_mov_rr(S, 8, R_S1, (X86Reg)be.p2nd[idx]);
                }
                x86_mov_mr(S, 8, off_mem(toff - 8 * j), R_S1);
            }
        }
    }
    Mv *mv = NEW(Mv, entry->nins + 1);
    int nm = 0;
    for (int k = 0; k < entry->nins; k++) {
        IRInstr *p = entry->ins[k];
        if (p->op != IR_PARAM || !needs_loc(p->id) || LOC(p->id)->kind == LK_NONE) continue;
        int idx = (int)p->imm;
        if (be.pcls[idx] == IRAGG_VALUE8 || be.pcls[idx] == IRAGG_VALUE16 ||
            be.pcls[idx] == IRAGG_MEMORY)
            continue; // rematerialized as its frame temp address
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

void ir_emit_x86(Program *prog, IRFunc *f, bool emitting_local_copy) {
    static int serial;
    const char *sym;
    size_t start = cg_ir_begin_fn(prog, f->fn, emitting_local_copy, &sym);
    memset(&be, 0, sizeof be);
    be.f = f;
    be.sym = sym;
    be.serial = serial++;

    cfg_prep(f);
    ir_analyze_loops(f);
    layout(f);
    be.nv = f->nvals;
    be.vi = NEW(VInfo, be.nv + 1);
    be.loc = NEW(Loc, be.nv + 1);
    be.lbl = NEW(const char *, f->nblocks);
    be.tramp = NEW(uint8_t, f->nblocks);
    be.ctemp = NEW(int, be.nv + 1);
    prep_ir();
    build_intervals();
    regalloc();
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
    free(be.bslot);
    free(be.bterm);
    free(be.tramp);
    free(be.lbl);
    free(be.callpos);
    free(be.blk[0]);
    free(be.blk[1]);
    free(be.pabi);
    free(be.pstk);
    free(be.p2nd);
    free(be.pcls);
    free(be.paggoff);
    free(be.ctemp);
}
