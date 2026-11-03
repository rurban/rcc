// ir_opt.c -- IR-to-IR optimization pipeline run between ir_build() and the backend.
//
//   cfg_cleanup   fold constant/identical-target branches, drop unreachable blocks,
//                 thread jumps through empty blocks, merge straight-line blocks
//   simplify      constant folding, algebraic rewrites, trivial-phi/copy propagation
//   dce           mark/sweep dead code (handles dead phi cycles)
//   gvn           dominator-tree value numbering of pure instrs
//   memopt        load forwarding / redundant-load elimination, dead-store elimination
//   sroa          scalar replacement of aggregates: non-escaping frame slots whose
//                 accesses all decompose to constant displacements become SSA cells
//   licm          hoist invariant pure instrs into (created) loop preheaders
//
// Semantics preserved exactly: I32 values stay sign-extended in imm, shifts only fold
// with an in-range count, signed div/rem only fold when the hardware would not trap,
// float folds use the host's IEEE arithmetic at the instr's own precision and never
// produce NaN constants, trapping divides are never removed, hoisted or merged, and
// no memory access is moved across an access or call that may alias it.
//
// Value forwarding: a pass that replaces instr X by value Y records X->aux = Y
// (fwd_set) and applies all forwards to every operand once at the end (fwd_apply),
// instead of rescanning the function per replacement. res() chases the chain.
#include "ir.h"
#include <math.h>

#define OPT_ROUNDS 8

// ------------------------------------------------------------------ utilities

static void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n ? n : 1, sz);
    if (!p) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    return p;
}

#define V(f, id) ((f)->vals[id])

static unsigned width_of(IRType t) { return t == IRT_I32 ? 32 : 64; }
static uint64_t mask_of(IRType t) { return t == IRT_I32 ? 0xFFFFFFFFull : ~0ull; }
// Normalize a raw 64-bit result to the canonical imm form of type t.
static int64_t wrap(IRType t, uint64_t x) { return t == IRT_I32 ? (int64_t)(int32_t)(uint32_t)x : (int64_t)x; }
static bool is_pow2(uint64_t u) { return u && !(u & (u - 1)); }

static void kill(IRInstr *i) {
    i->op = IR_NOP;
    i->type = IRT_VOID;
    i->a = i->b = i->c = IR_NONE;
    i->nargs = 0;
    i->t = i->f = NULL;
    i->blk = NULL;
}

// Drops killed (IR_NOP) entries from the phi and instr lists in one sweep.
static void compact_block(IRBlock *b) {
    int w = 0;
    for (int k = 0; k < b->nphis; k++)
        if (b->phis[k]->op != IR_NOP) b->phis[w++] = b->phis[k];
    b->nphis = w;
    w = 0;
    for (int k = 0; k < b->nins; k++)
        if (b->ins[k]->op != IR_NOP) b->ins[w++] = b->ins[k];
    b->nins = w;
}

static void fwd_reset(IRFunc *f) {
    for (int i = 0; i < f->nvals; i++) f->vals[i]->aux = IR_NONE;
}

static int res(IRFunc *f, int id) {
    if (id == IR_NONE) return id;
    int r = id;
    while (V(f, r)->aux != IR_NONE) r = V(f, r)->aux;
    while (id != r) {
        int n = V(f, id)->aux;
        V(f, id)->aux = r;
        id = n;
    }
    return r;
}

static void fwd_set(IRFunc *f, int from, int to) {
    to = res(f, to);
    if (to != from) V(f, from)->aux = to;
}

static void fwd_apply(IRFunc *f) {
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            i->a = res(f, i->a);
            i->b = res(f, i->b);
            i->c = res(f, i->c);
            for (int j = 0; j < i->nargs; j++) i->args[j] = res(f, i->args[j]);
        }
    }
}

static bool has_pred(const IRBlock *b, const IRBlock *p) {
    for (int k = 0; k < b->npreds; k++)
        if (b->preds[k] == p) return true;
    return false;
}

static int pred_index(const IRBlock *b, const IRBlock *p) {
    for (int k = 0; k < b->npreds; k++)
        if (b->preds[k] == p) return k;
    return -1;
}

static int pred_count(const IRBlock *b, const IRBlock *p) {
    int n = 0;
    for (int k = 0; k < b->npreds; k++)
        if (b->preds[k] == p) n++;
    return n;
}

static void phi_add_arg(IRInstr *p, int v) {
    int32_t *n = realloc(p->args, sizeof(*n) * (size_t)(p->nargs + 1));
    if (!n) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    p->args = n;
    p->args[p->nargs++] = v;
}

// Divides that provably cannot trap: constant, nonzero divisor, and for signed
// division not -1 (INT_MIN / -1 faults on x86).
static bool div_safe(IRFunc *f, const IRInstr *i) {
    IRInstr *d = V(f, i->b);
    if (d->op != IR_ICONST || d->imm == 0) return false;
    return i->op == IR_UDIV || i->op == IR_UREM || d->imm != -1;
}

// True for instrs whose only effect is their result: removable when unused.
// Loads count as removable (an unused load is not an observable effect here);
// PARAM is kept so the backend always sees the full ABI argument list.
static bool is_removable(IRFunc *f, const IRInstr *i) {
    switch (i->op) {
    case IR_STORE:
    case IR_CALL:
    case IR_PARAM:
    case IR_JMP:
    case IR_BR:
    case IR_RET:
    case IR_UNREACHABLE:
        return false;
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
        return div_safe(f, i);
    default:
        return true;
    }
}

static bool is_leaf(const IRInstr *i) {
    return i->op == IR_ICONST || i->op == IR_FCONST || i->op == IR_FRAMEADDR || i->op == IR_GADDR ||
        i->op == IR_UNDEF;
}

static bool is_commutative(IROp op) {
    return op == IR_ADD || op == IR_MUL || op == IR_AND || op == IR_OR || op == IR_XOR ||
        op == IR_FADD || op == IR_FMUL;
}

static IRCC swap_cc(IRCC cc) {
    switch (cc) {
    case IRCC_SLT: return IRCC_SGT;
    case IRCC_SGT: return IRCC_SLT;
    case IRCC_SLE: return IRCC_SGE;
    case IRCC_SGE: return IRCC_SLE;
    case IRCC_ULT: return IRCC_UGT;
    case IRCC_UGT: return IRCC_ULT;
    case IRCC_ULE: return IRCC_UGE;
    case IRCC_UGE: return IRCC_ULE;
    case IRCC_FLT: return IRCC_FGT;
    case IRCC_FGT: return IRCC_FLT;
    case IRCC_FLE: return IRCC_FGE;
    case IRCC_FGE: return IRCC_FLE;
    default: return cc;
    }
}

// Logical negation of a predicate. Fails for FLT/FLE/FGT/FGE: !(a<b) != (a>=b) with NaN.
static bool invert_cc(IRCC cc, IRCC *out) {
    switch (cc) {
    case IRCC_EQ: *out = IRCC_NE; return true;
    case IRCC_NE: *out = IRCC_EQ; return true;
    case IRCC_SLT: *out = IRCC_SGE; return true;
    case IRCC_SGE: *out = IRCC_SLT; return true;
    case IRCC_SLE: *out = IRCC_SGT; return true;
    case IRCC_SGT: *out = IRCC_SLE; return true;
    case IRCC_ULT: *out = IRCC_UGE; return true;
    case IRCC_UGE: *out = IRCC_ULT; return true;
    case IRCC_ULE: *out = IRCC_UGT; return true;
    case IRCC_UGT: *out = IRCC_ULE; return true;
    case IRCC_FEQ: *out = IRCC_FNE; return true;
    case IRCC_FNE: *out = IRCC_FEQ; return true;
    default: return false;
    }
}

// ----------------------------------------------------------- constant folding

static bool eval_icmp(IRCC cc, IRType t, int64_t x, int64_t y) {
    uint64_t ux = (uint64_t)x & mask_of(t), uy = (uint64_t)y & mask_of(t);
    switch (cc) {
    case IRCC_EQ: return x == y;
    case IRCC_NE: return x != y;
    case IRCC_SLT: return x < y;
    case IRCC_SLE: return x <= y;
    case IRCC_SGT: return x > y;
    case IRCC_SGE: return x >= y;
    case IRCC_ULT: return ux < uy;
    case IRCC_ULE: return ux <= uy;
    case IRCC_UGT: return ux > uy;
    case IRCC_UGE: return ux >= uy;
    default: return false;
    }
}

static bool eval_fcmp(IRCC cc, double x, double y) {
    switch (cc) {
    case IRCC_FEQ: return x == y;
    case IRCC_FNE: return x != y;
    case IRCC_FLT: return x < y;
    case IRCC_FLE: return x <= y;
    case IRCC_FGT: return x > y;
    case IRCC_FGE: return x >= y;
    default: return false;
    }
}

// Folds x (op) y at type t; false when the operation is not foldable (trapping
// divide, or a shift count outside [0, width) which is C UB).
static bool fold_int_bin(IROp op, IRType t, int64_t x, int64_t y, int64_t *out) {
    uint64_t m = mask_of(t), ux = (uint64_t)x & m, uy = (uint64_t)y & m, r;
    int64_t imin = t == IRT_I32 ? INT32_MIN : INT64_MIN;
    switch (op) {
    case IR_ADD: r = ux + uy; break;
    case IR_SUB: r = ux - uy; break;
    case IR_MUL: r = ux * uy; break;
    case IR_SDIV:
    case IR_SREM:
        if (y == 0 || (y == -1 && x == imin)) return false;
        r = (uint64_t)(op == IR_SDIV ? x / y : x % y);
        break;
    case IR_UDIV:
        if (uy == 0) return false;
        r = ux / uy;
        break;
    case IR_UREM:
        if (uy == 0) return false;
        r = ux % uy;
        break;
    case IR_AND: r = ux & uy; break;
    case IR_OR: r = ux | uy; break;
    case IR_XOR: r = ux ^ uy; break;
    case IR_SHL:
        if (y < 0 || y >= (int64_t)width_of(t)) return false;
        r = ux << y;
        break;
    case IR_SHR:
        if (y < 0 || y >= (int64_t)width_of(t)) return false;
        r = ux >> y;
        break;
    case IR_SAR:
        if (y < 0 || y >= (int64_t)width_of(t)) return false;
        r = (uint64_t)(x >> y);
        break;
    default:
        return false;
    }
    *out = wrap(t, r);
    return true;
}

// Float folding at the instr's own precision. NaN results are left to run time
// (their sign/payload is target-defined).
static bool fold_float_bin(IROp op, IRType t, double x, double y, double *out) {
    double r;
    if (t == IRT_F32) {
        float a = (float)x, b = (float)y, q;
        switch (op) {
        case IR_FADD: q = a + b; break;
        case IR_FSUB: q = a - b; break;
        case IR_FMUL: q = a * b; break;
        case IR_FDIV: q = a / b; break;
        default: return false;
        }
        r = q;
    } else {
        switch (op) {
        case IR_FADD: r = x + y; break;
        case IR_FSUB: r = x - y; break;
        case IR_FMUL: r = x * y; break;
        case IR_FDIV: r = x / y; break;
        default: return false;
        }
    }
    if (isnan(r)) return false;
    *out = r;
    return true;
}

// ----------------------------------------------------------------- simplifier

typedef enum { S_NONE,
               S_CHANGED,
               S_REPLACED } SRes;

typedef struct {
    IRFunc *f;
    IRBlock *b; // block being processed
    int pos; // index in b->ins of the current instr
    bool changed;
} Simp;

static SRes replace_with(Simp *s, IRInstr *i, int v) {
    fwd_set(s->f, i->id, v);
    kill(i);
    return S_REPLACED;
}

static SRes to_iconst(IRInstr *i, int64_t v) {
    i->op = IR_ICONST;
    i->imm = wrap((IRType)i->type, (uint64_t)v);
    i->a = i->b = i->c = IR_NONE;
    i->cc = 0;
    return S_CHANGED;
}

static SRes to_fconst(IRInstr *i, double v) {
    i->op = IR_FCONST;
    i->fimm = v;
    i->a = i->b = i->c = IR_NONE;
    i->cc = 0;
    return S_CHANGED;
}

// Inserts a fresh instr before the current one (which keeps being s->pos).
static IRInstr *insert_before(Simp *s, IROp op, IRType t) {
    IRBlock *b = s->b;
    IRInstr *n = ir_instr_new(s->f, op, t);
    n->aux = IR_NONE;
    ir_block_append(b, n);
    memmove(b->ins + s->pos + 1, b->ins + s->pos, sizeof(*b->ins) * (size_t)(b->nins - 1 - s->pos));
    b->ins[s->pos] = n;
    s->pos++;
    return n;
}

static int mk_iconst(Simp *s, IRType t, int64_t v) {
    IRInstr *n = insert_before(s, IR_ICONST, t);
    n->imm = wrap(t, (uint64_t)v);
    return n->id;
}

static int mk_fconst(Simp *s, IRType t, double v) {
    IRInstr *n = insert_before(s, IR_FCONST, t);
    n->fimm = v;
    return n->id;
}

// Width/signedness of values that are already normalized (in place) to a narrow
// range: SEXT/ZEXT results, narrow loads and comparison results (0/1).
static bool ext_info(const IRInstr *y, unsigned *w, bool *sgn) {
    switch (y->op) {
    case IR_SEXT8:
        *w = 8;
        *sgn = true;
        return true;
    case IR_SEXT16:
        *w = 16;
        *sgn = true;
        return true;
    case IR_ZEXT8:
        *w = 8;
        *sgn = false;
        return true;
    case IR_ZEXT16:
        *w = 16;
        *sgn = false;
        return true;
    case IR_LOAD:
        if (y->type != IRT_I32 || (y->mw != IRM_I8 && y->mw != IRM_I16)) return false;
        *w = y->mw == IRM_I8 ? 8 : 16;
        *sgn = y->msign != 0;
        return true;
    case IR_CMP:
    case IR_FCMP:
        *w = 1;
        *sgn = false;
        return true;
    default: return false;
    }
}

static bool is_ext_op(IROp op) { return op == IR_SEXT8 || op == IR_SEXT16 || op == IR_ZEXT8 || op == IR_ZEXT16; }

static SRes simp_ext(Simp *s, IRInstr *i, IRInstr *A) {
    if (A->op == IR_ICONST) {
        int64_t v = A->imm;
        switch (i->op) {
        case IR_SEXT8: v = (int8_t)v; break;
        case IR_SEXT16: v = (int16_t)v; break;
        case IR_ZEXT8: v = (uint8_t)v; break;
        default: v = (uint16_t)v; break;
        }
        return to_iconst(i, v);
    }
    unsigned ew = (i->op == IR_SEXT8 || i->op == IR_ZEXT8) ? 8 : 16, yw;
    bool es = i->op == IR_SEXT8 || i->op == IR_SEXT16, ys;
    if (!ext_info(A, &yw, &ys)) return S_NONE;
    // A already lies in the range this extension produces.
    if ((ew > yw && (!ys || es)) || (ew == yw && es == ys)) return replace_with(s, i, A->id);
    // Only the low ew bits of the inner extension matter.
    if (ew <= yw && is_ext_op((IROp)A->op)) {
        i->a = A->a;
        return S_CHANGED;
    }
    return S_NONE;
}

// Moves a leading constant to the right operand of a commutative op.
static SRes canon_const_right(IRInstr *i, IRInstr *A, IRInstr *B) {
    if (A->op == IR_ICONST && B->op != IR_ICONST && is_commutative((IROp)i->op)) {
        int t = i->a;
        i->a = i->b;
        i->b = t;
        return S_CHANGED;
    }
    return S_NONE;
}

static SRes simp_intbin(Simp *s, IRInstr *i, IRInstr *A, IRInstr *B) {
    IRFunc *f = s->f;
    IRType t = (IRType)i->type;
    IROp op = (IROp)i->op;
    bool ca = A->op == IR_ICONST, cb = B->op == IR_ICONST;
    int64_t x = A->imm, y = B->imm;
    if (ca && cb) {
        int64_t r;
        return fold_int_bin(op, t, x, y, &r) ? to_iconst(i, r) : S_NONE;
    }
    if (canon_const_right(i, A, B) == S_CHANGED) return S_CHANGED;
    unsigned W = width_of(t);
    uint64_t uy = (uint64_t)y & mask_of(t);
    bool shift = op == IR_SHL || op == IR_SHR || op == IR_SAR;

    if (!shift && i->a == i->b) {
        switch (op) {
        case IR_SUB:
        case IR_XOR: return to_iconst(i, 0);
        case IR_AND:
        case IR_OR: return replace_with(s, i, i->a);
        default: break;
        }
    }
    if (ca && op == IR_SUB && x == 0) { // 0 - b
        i->op = IR_NEG;
        i->a = i->b;
        i->b = IR_NONE;
        return S_CHANGED;
    }
    if (ca && shift) {
        if (x == 0 || (op == IR_SAR && x == -1)) return replace_with(s, i, i->a);
        return S_NONE;
    }

    if (!cb) {
        // a + (-b) -> a - b, a - (-b) -> a + b
        if ((op == IR_ADD || op == IR_SUB) && B->op == IR_NEG) {
            i->op = op == IR_ADD ? IR_SUB : IR_ADD;
            i->b = B->a;
            return S_CHANGED;
        }
        return S_NONE;
    }

    switch (op) {
    case IR_ADD:
        if (y == 0) return replace_with(s, i, i->a);
        if (A->op == IR_ADD && V(f, A->b)->op == IR_ICONST) { // (x + c1) + c2
            int64_t c = V(f, A->b)->imm;
            i->a = A->a;
            i->b = mk_iconst(s, t, (uint64_t)c + (uint64_t)y);
            return S_CHANGED;
        }
        return S_NONE;
    case IR_SUB: // x - c -> x + (-c)
        i->op = IR_ADD;
        i->b = mk_iconst(s, t, 0 - (uint64_t)y);
        return S_CHANGED;
    case IR_MUL:
        if (y == 0) return to_iconst(i, 0);
        if (y == 1) return replace_with(s, i, i->a);
        if (y == -1) {
            i->op = IR_NEG;
            i->b = IR_NONE;
            return S_CHANGED;
        }
        if (is_pow2(uy)) {
            i->op = IR_SHL;
            i->b = mk_iconst(s, IRT_I32, __builtin_ctzll(uy));
            return S_CHANGED;
        }
        if (A->op == IR_MUL && V(f, A->b)->op == IR_ICONST) { // (x * c1) * c2
            int64_t c = V(f, A->b)->imm;
            i->a = A->a;
            i->b = mk_iconst(s, t, (uint64_t)c * (uint64_t)y);
            return S_CHANGED;
        }
        return S_NONE;
    case IR_SDIV:
        return y == 1 ? replace_with(s, i, i->a) : S_NONE;
    case IR_SREM:
        return y == 1 ? to_iconst(i, 0) : S_NONE;
    case IR_UDIV:
        if (uy == 1) return replace_with(s, i, i->a);
        if (is_pow2(uy)) {
            i->op = IR_SHR;
            i->b = mk_iconst(s, IRT_I32, __builtin_ctzll(uy));
            return S_CHANGED;
        }
        return S_NONE;
    case IR_UREM:
        if (uy == 1) return to_iconst(i, 0);
        if (is_pow2(uy)) {
            i->op = IR_AND;
            i->b = mk_iconst(s, t, (uint64_t)(uy - 1));
            return S_CHANGED;
        }
        return S_NONE;
    case IR_AND:
    case IR_OR:
    case IR_XOR:
        if (op == IR_AND && y == 0) return to_iconst(i, 0);
        if (op == IR_AND && y == -1) return replace_with(s, i, i->a);
        if (op == IR_OR && y == -1) return to_iconst(i, -1);
        if (op != IR_AND && y == 0) return replace_with(s, i, i->a);
        if (op == IR_XOR && y == -1) {
            i->op = IR_NOT;
            i->b = IR_NONE;
            return S_CHANGED;
        }
        if (A->op == op && V(f, A->b)->op == IR_ICONST) { // (x op c1) op c2
            int64_t c = V(f, A->b)->imm, r;
            fold_int_bin(op, t, c, y, &r);
            i->a = A->a;
            i->b = mk_iconst(s, t, (uint64_t)r);
            return S_CHANGED;
        }
        return S_NONE;
    case IR_SHL:
    case IR_SHR:
    case IR_SAR: {
        if (y < 0 || y >= (int64_t)W) return S_NONE; // C UB: leave the instr alone
        if (y == 0) return replace_with(s, i, i->a);
        if (A->op == op && V(f, A->b)->op == IR_ICONST) { // (x sh c1) sh c2
            int64_t c1 = V(f, A->b)->imm;
            if (c1 >= 0 && c1 < (int64_t)W) {
                int64_t sum = c1 + y;
                if (sum < (int64_t)W) {
                    i->a = A->a;
                    i->b = mk_iconst(s, IRT_I32, sum);
                    return S_CHANGED;
                }
                if (op == IR_SAR) {
                    i->a = A->a;
                    i->b = mk_iconst(s, IRT_I32, (int64_t)W - 1);
                    return S_CHANGED;
                }
                return to_iconst(i, 0);
            }
        }
        // (x << k) >> k on I32: the sign/zero extension of the low 32-k bits
        if (t == IRT_I32 && op != IR_SHL && (y == 24 || y == 16) && A->op == IR_SHL &&
            V(f, A->b)->op == IR_ICONST && V(f, A->b)->imm == y) {
            i->op = op == IR_SAR ? (y == 24 ? IR_SEXT8 : IR_SEXT16) : (y == 24 ? IR_ZEXT8 : IR_ZEXT16);
            i->a = A->a;
            i->b = IR_NONE;
            return S_CHANGED;
        }
        return S_NONE;
    }
    default:
        return S_NONE;
    }
}

static SRes simp_floatbin(Simp *s, IRInstr *i, IRInstr *A, IRInstr *B) {
    IROp op = (IROp)i->op;
    IRType t = (IRType)i->type;
    bool ca = A->op == IR_FCONST, cb = B->op == IR_FCONST;
    if (ca && cb) {
        double r;
        return fold_float_bin(op, t, A->fimm, B->fimm, &r) ? to_fconst(i, r) : S_NONE;
    }
    if (ca && (op == IR_FADD || op == IR_FMUL)) {
        int tmp = i->a;
        i->a = i->b;
        i->b = tmp;
        return S_CHANGED;
    }
    if (!cb) return S_NONE;
    double y = B->fimm;
    switch (op) {
    case IR_FMUL:
    case IR_FDIV:
        if (y == 1.0) return replace_with(s, i, i->a);
        if (op == IR_FDIV && isfinite(y) && y != 0.0) {
            // x / 2^k == x * 2^-k exactly when the reciprocal is a normal number
            int e;
            double m = frexp(fabs(y), &e);
            if (m != 0.5) return S_NONE;
            if (t == IRT_F32) {
                float r = 1.0f / (float)y;
                if (!isnormal(r)) return S_NONE;
                i->op = IR_FMUL;
                i->b = mk_fconst(s, t, (double)r);
                return S_CHANGED;
            }
            double r = 1.0 / y;
            if (!isnormal(r)) return S_NONE;
            i->op = IR_FMUL;
            i->b = mk_fconst(s, t, r);
            return S_CHANGED;
        }
        return S_NONE;
    case IR_FSUB:
        return (y == 0.0 && !signbit(y)) ? replace_with(s, i, i->a) : S_NONE;
    case IR_FADD:
        return (y == 0.0 && signbit(y)) ? replace_with(s, i, i->a) : S_NONE;
    default:
        return S_NONE;
    }
}

// Range-edge identities for `x cc y` with a constant y: 0/1 = constant result,
// 2 = becomes EQ, 3 = becomes NE, -1 = no simplification.
static int cmp_edge(IRCC cc, IRType t, int64_t y) {
    uint64_t umax = mask_of(t), uy = (uint64_t)y & umax;
    int64_t imin = t == IRT_I32 ? INT32_MIN : INT64_MIN, imax = t == IRT_I32 ? INT32_MAX : INT64_MAX;
    switch (cc) {
    case IRCC_ULT: return uy == 0 ? 0 : uy == umax ? 3
                                                   : -1;
    case IRCC_UGE: return uy == 0 ? 1 : uy == umax ? 2
                                                   : -1;
    case IRCC_ULE: return uy == 0 ? 2 : uy == umax ? 1
                                                   : -1;
    case IRCC_UGT: return uy == 0 ? 3 : uy == umax ? 0
                                                   : -1;
    case IRCC_SLT: return y == imin ? 0 : y == imax ? 3
                                                    : -1;
    case IRCC_SGE: return y == imin ? 1 : y == imax ? 2
                                                    : -1;
    case IRCC_SLE: return y == imin ? 2 : y == imax ? 1
                                                    : -1;
    case IRCC_SGT: return y == imin ? 3 : y == imax ? 0
                                                    : -1;
    default: return -1;
    }
}

static bool cc_is_signed(IRCC cc) { return cc >= IRCC_SLT && cc <= IRCC_SGE; }
static bool cc_is_unsigned(IRCC cc) { return cc >= IRCC_ULT && cc <= IRCC_UGE; }

// I64 comparison of widened I32 values -> I32 comparison of the originals.
static SRes narrow_cmp(Simp *s, IRInstr *i, IRInstr *A, IRInstr *B) {
    IRCC cc = (IRCC)i->cc;
    bool eq = cc == IRCC_EQ || cc == IRCC_NE;
    if (A->op == IR_SEXT32 && (eq || cc_is_signed(cc))) {
        if (B->op == IR_SEXT32) {
            i->a = A->a;
            i->b = B->a;
            return S_CHANGED;
        }
        if (B->op == IR_ICONST && B->imm >= INT32_MIN && B->imm <= INT32_MAX) {
            i->a = A->a;
            i->b = mk_iconst(s, IRT_I32, B->imm);
            return S_CHANGED;
        }
    }
    if (A->op == IR_ZEXT32 && (eq || cc_is_unsigned(cc))) {
        if (B->op == IR_ZEXT32) {
            i->a = A->a;
            i->b = B->a;
            return S_CHANGED;
        }
        if (B->op == IR_ICONST && B->imm >= 0 && B->imm <= (int64_t)0xFFFFFFFF) {
            i->a = A->a;
            i->b = mk_iconst(s, IRT_I32, B->imm);
            return S_CHANGED;
        }
    }
    return S_NONE;
}

static SRes simp_cmp(Simp *s, IRInstr *i, IRInstr *A, IRInstr *B) {
    IRCC cc = (IRCC)i->cc;
    IRType ot = (IRType)A->type;
    if (i->a == i->b) {
        switch (cc) {
        case IRCC_EQ:
        case IRCC_SLE:
        case IRCC_SGE:
        case IRCC_ULE:
        case IRCC_UGE:
            return to_iconst(i, 1);
        default:
            return to_iconst(i, 0);
        }
    }
    bool ca = A->op == IR_ICONST, cb = B->op == IR_ICONST;
    if (ca && cb) return to_iconst(i, eval_icmp(cc, ot, A->imm, B->imm));
    if (ca) {
        int t = i->a;
        i->a = i->b;
        i->b = t;
        i->cc = (uint8_t)swap_cc(cc);
        return S_CHANGED;
    }
    if (cb) {
        int64_t y = B->imm;
        bool eq = cc == IRCC_EQ || cc == IRCC_NE;
        if ((A->op == IR_CMP || A->op == IR_FCMP) && eq) { // A is 0/1
            if (y != 0 && y != 1) return to_iconst(i, cc == IRCC_NE);
            if ((cc == IRCC_NE) == (y == 0)) return replace_with(s, i, A->id);
            IRCC inv;
            if (invert_cc((IRCC)A->cc, &inv)) {
                i->op = A->op;
                i->cc = (uint8_t)inv;
                i->a = A->a;
                i->b = A->b;
                return S_CHANGED;
            }
            return S_NONE;
        }
        if (eq && y == 0 && (A->op == IR_SUB || A->op == IR_XOR)) { // a-b==0 <=> a==b
            i->a = A->a;
            i->b = A->b;
            return S_CHANGED;
        }
        int e = cmp_edge(cc, ot, y);
        if (e == 0 || e == 1) return to_iconst(i, e);
        if (e >= 2) {
            i->cc = e == 2 ? IRCC_EQ : IRCC_NE;
            return S_CHANGED;
        }
    }
    if (ot == IRT_I64) return narrow_cmp(s, i, A, B);
    return S_NONE;
}

static bool fits_i32(double d) { return d > -2147483649.0 && d < 2147483648.0; }

// Conversions and unary ops.
static SRes simp_unary(Simp *s, IRInstr *i, IRInstr *A) {
    IRType t = (IRType)i->type;
    switch (i->op) {
    case IR_NEG:
        if (A->op == IR_ICONST) return to_iconst(i, (int64_t)(0 - (uint64_t)A->imm));
        if (A->op == IR_NEG) return replace_with(s, i, A->a);
        if (A->op == IR_SUB) { // -(a - b) = b - a
            i->op = IR_SUB;
            i->b = A->a;
            i->a = A->b;
            return S_CHANGED;
        }
        return S_NONE;
    case IR_NOT:
        if (A->op == IR_ICONST) return to_iconst(i, (int64_t)~(uint64_t)A->imm);
        if (A->op == IR_NOT) return replace_with(s, i, A->a);
        return S_NONE;
    case IR_FNEG:
        if (A->op == IR_FCONST) return to_fconst(i, -A->fimm);
        if (A->op == IR_FNEG) return replace_with(s, i, A->a);
        return S_NONE;
    case IR_SEXT8:
    case IR_SEXT16:
    case IR_ZEXT8:
    case IR_ZEXT16:
        return simp_ext(s, i, A);
    case IR_SEXT32:
        return A->op == IR_ICONST ? to_iconst(i, A->imm) : S_NONE;
    case IR_ZEXT32:
        return A->op == IR_ICONST ? to_iconst(i, (int64_t)(uint32_t)A->imm) : S_NONE;
    case IR_TRUNC:
        if (A->op == IR_ICONST) return to_iconst(i, A->imm);
        if (A->op == IR_SEXT32 || A->op == IR_ZEXT32) return replace_with(s, i, A->a);
        return S_NONE;
    case IR_SITOF:
        if (A->op == IR_ICONST) {
            double d = t == IRT_F32 ? (double)(float)A->imm : (double)A->imm;
            return to_fconst(i, d);
        }
        if (A->op == IR_SEXT32) { // converting the widened value == converting the original
            i->a = A->a;
            return S_CHANGED;
        }
        return S_NONE;
    case IR_UITOF:
        if (A->op == IR_ICONST) {
            double d = t == IRT_F32 ? (double)(float)(uint64_t)A->imm : (double)(uint64_t)A->imm;
            return to_fconst(i, d);
        }
        return S_NONE;
    case IR_FTOSI:
        if (A->op == IR_FCONST && !isnan(A->fimm)) {
            double d = A->fimm;
            if (t == IRT_I32 ? fits_i32(d) : (d >= -9223372036854775808.0 && d < 9223372036854775808.0))
                return to_iconst(i, (int64_t)d);
        }
        return S_NONE;
    case IR_FTOU64:
        if (A->op == IR_FCONST && A->fimm >= 0.0 && A->fimm < 18446744073709551616.0)
            return to_iconst(i, (int64_t)(uint64_t)A->fimm);
        return S_NONE;
    case IR_FEXT:
        return A->op == IR_FCONST ? to_fconst(i, A->fimm) : S_NONE;
    case IR_FTRUNC:
        if (A->op == IR_FCONST && !isnan(A->fimm)) return to_fconst(i, (double)(float)A->fimm);
        if (A->op == IR_FEXT) return replace_with(s, i, A->a);
        return S_NONE;
    default:
        return S_NONE;
    }
}

static SRes simp_br(Simp *s, IRInstr *i, IRInstr *A) {
    // br (cmp ne x, 0) -> br x ; br (cmp eq x, 0) t,f -> br x f,t   (x is I32)
    if (A->op != IR_CMP) return S_NONE;
    IRInstr *X = V(s->f, A->a), *Y = V(s->f, A->b);
    if (X->type != IRT_I32 || Y->op != IR_ICONST || Y->imm != 0) return S_NONE;
    if (A->cc == IRCC_NE) {
        i->a = A->a;
        return S_CHANGED;
    }
    if (A->cc == IRCC_EQ) {
        IRBlock *t = i->t;
        i->t = i->f;
        i->f = t;
        i->a = A->a;
        return S_CHANGED;
    }
    return S_NONE;
}

static SRes simplify_instr(Simp *s, IRInstr *i) {
    IRFunc *f = s->f;
    i->a = res(f, i->a);
    i->b = res(f, i->b);
    i->c = res(f, i->c);
    IRInstr *A = i->a != IR_NONE ? V(f, i->a) : NULL;
    IRInstr *B = i->b != IR_NONE ? V(f, i->b) : NULL;
    switch (i->op) {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
    case IR_SHL:
    case IR_SHR:
    case IR_SAR:
        return simp_intbin(s, i, A, B);
    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
        return simp_floatbin(s, i, A, B);
    case IR_CMP:
        return simp_cmp(s, i, A, B);
    case IR_FCMP:
        if (A->op == IR_FCONST && B->op == IR_FCONST) return to_iconst(i, eval_fcmp((IRCC)i->cc, A->fimm, B->fimm));
        if (A->op == IR_FCONST) {
            int t = i->a;
            i->a = i->b;
            i->b = t;
            i->cc = (uint8_t)swap_cc((IRCC)i->cc);
            return S_CHANGED;
        }
        return S_NONE;
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
    case IR_SITOF:
    case IR_UITOF:
    case IR_FTOSI:
    case IR_FTOU64:
    case IR_FEXT:
    case IR_FTRUNC:
        return simp_unary(s, i, A);
    case IR_BR:
        return simp_br(s, i, A);
    default:
        return S_NONE;
    }
}

// Replaces phis whose incoming values are all one value v (ignoring the phi itself).
static void simplify_phis(Simp *s, IRBlock *b) {
    IRFunc *f = s->f;
    for (int k = 0; k < b->nphis; k++) {
        IRInstr *p = b->phis[k];
        int v = IR_NONE;
        bool trivial = true;
        for (int j = 0; j < p->nargs && trivial; j++) {
            int a = res(f, p->args[j]);
            p->args[j] = a;
            if (a == p->id) continue;
            if (v == IR_NONE) v = a;
            else if (v != a)
                trivial = false;
        }
        if (trivial && v != IR_NONE) {
            replace_with(s, p, v);
            s->changed = true;
        }
    }
    compact_block(b);
}

static bool simplify(IRFunc *f) {
    fwd_reset(f);
    IRBlock **rpo = xcalloc((size_t)f->nblocks, sizeof(*rpo));
    int n = ir_rpo(f, rpo);
    Simp s = {f, NULL, 0, false};
    for (int bi = 0; bi < n; bi++) {
        IRBlock *b = s.b = rpo[bi];
        simplify_phis(&s, b);
        for (s.pos = 0; s.pos < b->nins; s.pos++) {
            IRInstr *i = b->ins[s.pos];
            for (int iter = 0; iter < 8; iter++) {
                SRes r = simplify_instr(&s, i);
                if (r != S_NONE) s.changed = true;
                if (r != S_CHANGED) break;
            }
        }
        compact_block(b);
    }
    fwd_apply(f);
    free(rpo);
    return s.changed;
}

// ------------------------------------------------------------------------ DCE

static bool dce(IRFunc *f) {
    unsigned char *live = xcalloc((size_t)f->nvals, 1);
    int *stack = xcalloc((size_t)f->nvals, sizeof(int));
    int sp = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (!is_removable(f, i) && !live[i->id]) {
                live[i->id] = 1;
                stack[sp++] = i->id;
            }
        }
    }
#define MARK(x) \
    do { \
        int _x = (x); \
        if (_x != IR_NONE && !live[_x]) { \
            live[_x] = 1; \
            stack[sp++] = _x; \
        } \
    } while (0)
    while (sp) {
        IRInstr *i = V(f, stack[--sp]);
        MARK(i->a);
        MARK(i->b);
        MARK(i->c);
        for (int j = 0; j < i->nargs; j++) MARK(i->args[j]);
    }
#undef MARK
    bool changed = false;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        bool dirty = false;
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (!live[i->id]) {
                kill(i);
                dirty = true;
            }
        }
        if (dirty) {
            compact_block(b);
            changed = true;
        }
    }
    free(live);
    free(stack);
    return changed;
}

// ---------------------------------------------------------------- CFG cleanup

static void free_block(IRBlock *b) {
    for (int k = 0; k < b->nphis; k++) kill(b->phis[k]);
    for (int k = 0; k < b->nins; k++) kill(b->ins[k]);
    free(b->phis);
    free(b->ins);
    free(b->preds);
    free(b);
}

// Drops blocks flagged in dead[] (indexed by current id) and renumbers the rest.
static void compact_blocks(IRFunc *f, const unsigned char *dead) {
    int w = 0;
    for (int i = 0; i < f->nblocks; i++) {
        IRBlock *b = f->blocks[i];
        if (dead[i]) {
            free_block(b);
        } else {
            f->blocks[w] = b;
            b->id = w++;
        }
    }
    f->nblocks = w;
}

static void make_jmp(IRInstr *t, IRBlock *to) {
    t->op = IR_JMP;
    t->t = to;
    t->f = NULL;
    t->a = IR_NONE;
}

static bool fold_branches(IRFunc *f) {
    bool changed = false;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        IRInstr *t = b->ins[b->nins - 1];
        if (t->op != IR_BR) continue;
        if (t->t == t->f) {
            // Both edges go to one block; they must carry the same phi values.
            IRBlock *s = t->t;
            int i1 = pred_index(s, b), i2 = -1;
            for (int k = i1 + 1; k < s->npreds; k++)
                if (s->preds[k] == b) {
                    i2 = k;
                    break;
                }
            bool same = i2 >= 0;
            for (int k = 0; same && k < s->nphis; k++)
                if (s->phis[k]->args[i1] != s->phis[k]->args[i2]) same = false;
            if (!same) continue;
            ir_remove_pred_edge(s, b);
            make_jmp(t, s);
            changed = true;
        } else if (V(f, t->a)->op == IR_ICONST) {
            IRBlock *taken = V(f, t->a)->imm != 0 ? t->t : t->f;
            IRBlock *other = taken == t->t ? t->f : t->t;
            ir_remove_pred_edge(other, b);
            make_jmp(t, taken);
            changed = true;
        }
    }
    return changed;
}

static bool remove_unreachable(IRFunc *f) {
    int n = f->nblocks;
    unsigned char *seen = xcalloc((size_t)n, 1);
    IRBlock **stack = xcalloc((size_t)n, sizeof(*stack));
    int sp = 0;
    seen[0] = 1;
    stack[sp++] = f->blocks[0];
    while (sp) {
        IRBlock *b = stack[--sp];
        IRBlock *s[2];
        int ns = ir_succs(b, s);
        for (int k = 0; k < ns; k++)
            if (!seen[s[k]->id]) {
                seen[s[k]->id] = 1;
                stack[sp++] = s[k];
            }
    }
    bool any = false;
    unsigned char *dead = xcalloc((size_t)n, 1);
    for (int i = 0; i < n; i++)
        if (!seen[i]) dead[i] = any = 1;
    if (any) {
        for (int i = 0; i < n; i++) {
            if (!dead[i]) continue;
            IRBlock *b = f->blocks[i], *s[2];
            int ns = ir_succs(b, s);
            for (int k = 0; k < ns; k++)
                while (has_pred(s[k], b)) ir_remove_pred_edge(s[k], b);
        }
        compact_blocks(f, dead);
    }
    free(seen);
    free(stack);
    free(dead);
    return any;
}

static bool is_forwarder(const IRBlock *e) { return e->nphis == 0 && e->nins == 1 && e->ins[0]->op == IR_JMP; }

// Redirects predecessors of an empty block E (only `jmp T`) straight to T.
static bool thread_jumps(IRFunc *f) {
    bool changed = false;
    for (int bi = 1; bi < f->nblocks; bi++) {
        IRBlock *e = f->blocks[bi];
        if (!is_forwarder(e)) continue;
        IRBlock *t = e->ins[0]->t;
        if (t == e) continue;
        // A cycle of forwarders back to E would let threading ping-pong forever.
        IRBlock *c = t;
        for (int steps = 0; steps < f->nblocks && is_forwarder(c) && c != e; steps++) c = c->ins[0]->t;
        if (c == e) continue;
        for (int k = e->npreds - 1; k >= 0; k--) {
            if (k >= e->npreds) continue;
            IRBlock *x = e->preds[k];
            if (x == e || pred_count(e, x) != 1) continue;
            if (t->nphis > 0 && has_pred(t, x)) continue; // would give x two edges with distinct phi values
            IRInstr *term = x->ins[x->nins - 1];
            if (term->t == e) term->t = t;
            else
                term->f = t;
            int ei = pred_index(t, e);
            ir_add_pred(t, x);
            for (int j = 0; j < t->nphis; j++) phi_add_arg(t->phis[j], t->phis[j]->args[ei]);
            ir_remove_pred_edge(e, x);
            changed = true;
        }
    }
    return changed;
}

// Absorbs B into A when A ends in `jmp B` and B has no other predecessor.
static bool merge_blocks(IRFunc *f) {
    bool changed = false;
    unsigned char *dead = xcalloc((size_t)f->nblocks, 1);
    fwd_reset(f);
    for (int ai = 0; ai < f->nblocks; ai++) {
        IRBlock *a = f->blocks[ai];
        if (dead[ai]) continue;
        for (;;) {
            IRInstr *term = a->ins[a->nins - 1];
            if (term->op != IR_JMP) break;
            IRBlock *b = term->t;
            if (b == a || b == f->blocks[0] || b->npreds != 1) break;
            for (int k = 0; k < b->nphis; k++) {
                fwd_set(f, b->phis[k]->id, b->phis[k]->args[0]);
                kill(b->phis[k]);
            }
            b->nphis = 0;
            a->nins--;
            kill(term);
            for (int k = 0; k < b->nins; k++) ir_block_append(a, b->ins[k]);
            b->nins = 0;
            dead[b->id] = 1;
            IRBlock *s[2];
            int ns = ir_succs(a, s);
            for (int k = 0; k < ns; k++)
                for (int j = 0; j < s[k]->npreds; j++)
                    if (s[k]->preds[j] == b) s[k]->preds[j] = a;
            changed = true;
        }
    }
    if (changed) {
        fwd_apply(f);
        compact_blocks(f, dead);
    }
    free(dead);
    return changed;
}

static bool cfg_cleanup(IRFunc *f) {
    bool any = false;
    for (int r = 0; r < 32; r++) {
        bool ch = fold_branches(f);
        ch |= remove_unreachable(f);
        ch |= thread_jumps(f);
        ch |= remove_unreachable(f);
        ch |= merge_blocks(f);
        if (!ch) break;
        any = true;
    }
    return any;
}

// ----------------------------------------------------------------- dominators

typedef struct {
    IRBlock **rpo;
    int n; // reachable blocks
    int *num; // block id -> rpo index, -1 if unreachable
    int *idom; // block id -> idom block id (entry: itself), -1 if unreachable
} Dom;

static void dom_build(IRFunc *f, Dom *d) {
    int nb = f->nblocks;
    d->rpo = xcalloc((size_t)nb, sizeof(*d->rpo));
    d->num = xcalloc((size_t)nb, sizeof(int));
    d->idom = xcalloc((size_t)nb, sizeof(int));
    d->n = ir_rpo(f, d->rpo);
    for (int i = 0; i < nb; i++) d->num[i] = d->idom[i] = -1;
    for (int i = 0; i < d->n; i++) d->num[d->rpo[i]->id] = i;
    d->idom[d->rpo[0]->id] = d->rpo[0]->id;
    for (bool changed = true; changed;) {
        changed = false;
        for (int i = 1; i < d->n; i++) {
            IRBlock *b = d->rpo[i];
            int nd = -1;
            for (int k = 0; k < b->npreds; k++) {
                int p = b->preds[k]->id;
                if (d->num[p] < 0 || d->idom[p] < 0) continue;
                if (nd < 0) {
                    nd = p;
                    continue;
                }
                int x = p, y = nd;
                while (x != y) {
                    while (d->num[x] > d->num[y]) x = d->idom[x];
                    while (d->num[y] > d->num[x]) y = d->idom[y];
                }
                nd = x;
            }
            if (nd >= 0 && d->idom[b->id] != nd) {
                d->idom[b->id] = nd;
                changed = true;
            }
        }
    }
}

static void dom_free(Dom *d) {
    free(d->rpo);
    free(d->num);
    free(d->idom);
}

// Does block a dominate block b (ids)? b may be unreachable (then false).
static bool dominates(const Dom *d, int a, int b) {
    if (d->idom[b] < 0) return false;
    for (;;) {
        if (b == a) return true;
        if (d->idom[b] == b) return false;
        b = d->idom[b];
    }
}

// ------------------------------------------------------------------------ GVN

static bool gvn_eligible(IRFunc *f, const IRInstr *i) {
    switch (i->op) {
    case IR_ICONST:
    case IR_FCONST:
    case IR_FRAMEADDR:
    case IR_GADDR:
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
    case IR_SHL:
    case IR_SHR:
    case IR_SAR:
    case IR_NEG:
    case IR_NOT:
    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
    case IR_FNEG:
    case IR_CMP:
    case IR_FCMP:
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
        return true;
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
        return div_safe(f, i);
    default:
        return false;
    }
}

// Operands/predicate in canonical order so commutative forms and swapped
// comparisons hash and compare equal.
static void gvn_canon(const IRInstr *i, int *a, int *b, int *cc) {
    *a = i->a;
    *b = i->b;
    *cc = i->cc;
    if ((is_commutative((IROp)i->op) || i->op == IR_CMP || i->op == IR_FCMP) && *a > *b) {
        int t = *a;
        *a = *b;
        *b = t;
        if (i->op == IR_CMP || i->op == IR_FCMP) *cc = swap_cc((IRCC)i->cc);
    }
}

static bool gvn_same(const IRInstr *x, const IRInstr *y) {
    if (x->op != y->op || x->type != y->type) return false;
    switch (x->op) {
    case IR_ICONST:
    case IR_FRAMEADDR:
        return x->imm == y->imm;
    case IR_FCONST:
        return memcmp(&x->fimm, &y->fimm, sizeof(double)) == 0;
    case IR_GADDR:
        return x->imm == y->imm && strcmp(x->sym, y->sym) == 0;
    default: {
        int xa, xb, xc, ya, yb, yc;
        gvn_canon(x, &xa, &xb, &xc);
        gvn_canon(y, &ya, &yb, &yc);
        return xa == ya && xb == yb && xc == yc;
    }
    }
}

static uint64_t hash_mix(uint64_t h, uint64_t v) {
    h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    return h;
}

static uint64_t gvn_hash(const IRInstr *i) {
    uint64_t h = hash_mix(i->op, i->type);
    switch (i->op) {
    case IR_ICONST:
    case IR_FRAMEADDR:
        return hash_mix(h, (uint64_t)i->imm);
    case IR_FCONST: {
        uint64_t bits;
        memcpy(&bits, &i->fimm, sizeof bits);
        return hash_mix(h, bits);
    }
    case IR_GADDR:
        h = hash_mix(h, (uint64_t)i->imm);
        for (const char *p = i->sym; *p; p++) h = hash_mix(h, (unsigned char)*p);
        return h;
    default: {
        int a, b, cc;
        gvn_canon(i, &a, &b, &cc);
        return hash_mix(hash_mix(hash_mix(h, (uint64_t)(unsigned)a), (uint64_t)(unsigned)b), (uint64_t)cc);
    }
    }
}

static bool gvn(IRFunc *f) {
    fwd_reset(f);
    Dom d;
    dom_build(f, &d);
    int nb = f->nblocks;
    // dominator-tree children lists
    int *first = xcalloc((size_t)nb, sizeof(int)), *next = xcalloc((size_t)nb, sizeof(int));
    for (int i = 0; i < nb; i++) first[i] = next[i] = -1;
    for (int i = d.n - 1; i >= 1; i--) {
        int id = d.rpo[i]->id, p = d.idom[id];
        next[id] = first[p];
        first[p] = id;
    }
    size_t hsize = 16;
    while (hsize < (size_t)f->nvals * 2) hsize <<= 1;
    int *head = xcalloc(hsize, sizeof(int));
    for (size_t i = 0; i < hsize; i++) head[i] = -1;
    int *eid = xcalloc((size_t)f->nvals, sizeof(int)), *enext = xcalloc((size_t)f->nvals, sizeof(int));
    size_t *ebucket = xcalloc((size_t)f->nvals, sizeof(size_t));
    int nent = 0;
    int *stk = xcalloc((size_t)nb, sizeof(int)), *slog = xcalloc((size_t)nb, sizeof(int));
    int *scur = xcalloc((size_t)nb, sizeof(int));
    bool changed = false;
    stk[0] = d.rpo[0]->id;
    slog[0] = -1; // -1: block not yet entered
    int sp = 1;
    while (sp) {
        int top = sp - 1;
        IRBlock *b = f->blocks[stk[top]];
        if (slog[top] < 0) {
            slog[top] = nent;
            scur[top] = first[b->id];
            for (int k = 0; k < b->nins; k++) {
                IRInstr *i = b->ins[k];
                i->a = res(f, i->a);
                i->b = res(f, i->b);
                i->c = res(f, i->c);
                if (!gvn_eligible(f, i)) continue;
                size_t bk = (size_t)(gvn_hash(i) & (hsize - 1));
                int hit = IR_NONE;
                for (int e = head[bk]; e >= 0; e = enext[e])
                    if (gvn_same(V(f, eid[e]), i)) {
                        hit = eid[e];
                        break;
                    }
                if (hit != IR_NONE) {
                    fwd_set(f, i->id, hit);
                    kill(i);
                    changed = true;
                } else {
                    eid[nent] = i->id;
                    enext[nent] = head[bk];
                    ebucket[nent] = bk;
                    head[bk] = nent++;
                }
            }
            compact_block(b);
        }
        if (scur[top] >= 0) {
            int c = scur[top];
            scur[top] = next[c];
            stk[sp] = c;
            slog[sp] = -1;
            sp++;
        } else {
            while (nent > slog[top]) {
                nent--;
                head[ebucket[nent]] = enext[nent];
            }
            sp--;
        }
    }
    if (changed) fwd_apply(f);
    free(first);
    free(next);
    free(head);
    free(eid);
    free(enext);
    free(ebucket);
    free(stk);
    free(slog);
    free(scur);
    dom_free(&d);
    return changed;
}

// ------------------------------------------------------------ known branches
//
// A branch on a value that a dominating conditional edge already decided is folded.
// Rotation leaves one guard per nested loop, all testing the same `0 < n`. The edge
// T->D is the only way into D (D has the single predecessor T), so every path to a
// block D dominates crosses it, and since the tested value's definition dominates T
// the value seen at the later branch is the one the edge decided.

static bool fold_known_branches(IRFunc *f) {
    Dom d;
    dom_build(f, &d);
    bool changed = false;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        if (d.idom[bi] < 0) continue;
        IRInstr *t = b->ins[b->nins - 1];
        if (t->op != IR_BR || t->t == t->f) continue;
        int known = -1;
        for (int c = bi; known < 0; c = d.idom[c]) {
            IRBlock *D = f->blocks[c];
            if (D->npreds == 1) {
                IRBlock *T = D->preds[0];
                IRInstr *tt = T->ins[T->nins - 1];
                if (tt->op == IR_BR && tt->t != tt->f && tt->a == t->a) known = tt->t == D;
            }
            if (d.idom[c] == c) break;
        }
        if (known < 0) continue;
        IRBlock *taken = known ? t->t : t->f, *other = known ? t->f : t->t;
        ir_remove_pred_edge(other, b);
        make_jmp(t, taken);
        changed = true;
    }
    dom_free(&d);
    return changed;
}

// ----------------------------------------------------------------------- LICM

typedef struct {
    IRBlock *header;
    IRBlock *pre; // preheader: the unique outside predecessor, ends in jmp header
    IRBlock **latches;
    int nlatches;
    int size; // blocks in the natural loop
    unsigned char *in; // membership by block id
    bool created; // pre is a new block (otherwise reused)
} Loop;

// Creates a block ph with `jmp h`, moves the outside edges (indices out[]) of h
// to it, and keeps phis consistent (new phis in ph where outside values differ).
static IRBlock *make_preheader(IRFunc *f, IRBlock *h, const int *out, int nout) {
    IRBlock *ph = ir_block_new(f);
    IRInstr *j = ir_instr_new(f, IR_JMP, IRT_VOID);
    j->t = h;
    ir_block_append(ph, j);
    for (int k = 0; k < nout; k++) {
        IRBlock *x = h->preds[out[k]];
        IRInstr *term = x->ins[x->nins - 1];
        if (term->t == h) term->t = ph;
        else
            term->f = ph;
        ir_add_pred(ph, x);
    }
    unsigned char *is_out = xcalloc((size_t)h->npreds, 1);
    for (int k = 0; k < nout; k++) is_out[out[k]] = 1;
    for (int pi = 0; pi < h->nphis; pi++) {
        IRInstr *p = h->phis[pi];
        bool same = true;
        for (int k = 1; k < nout; k++)
            if (p->args[out[k]] != p->args[out[0]]) same = false;
        int pv = p->args[out[0]];
        if (!same) {
            IRInstr *q = ir_instr_new(f, IR_PHI, (IRType)p->type);
            q->aux = IR_NONE;
            q->nargs = nout;
            q->args = xcalloc((size_t)nout, sizeof(int32_t));
            for (int k = 0; k < nout; k++) q->args[k] = p->args[out[k]];
            ir_block_add_phi(ph, q);
            pv = q->id;
        }
        int w = 0;
        int32_t *na = xcalloc((size_t)h->npreds, sizeof(int32_t));
        na[w++] = pv;
        for (int k = 0; k < h->npreds; k++)
            if (!is_out[k]) na[w++] = p->args[k];
        memcpy(p->args, na, sizeof(int32_t) * (size_t)w);
        p->nargs = w;
        free(na);
    }
    int w = 0;
    IRBlock **np = xcalloc((size_t)h->npreds, sizeof(*np));
    np[w++] = ph;
    for (int k = 0; k < h->npreds; k++)
        if (!is_out[k]) np[w++] = h->preds[k];
    memcpy(h->preds, np, sizeof(*np) * (size_t)w);
    h->npreds = w;
    free(np);
    free(is_out);
    return ph;
}

static bool hoistable(IRFunc *f, const IRInstr *i) {
    switch (i->op) {
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
    case IR_SHL:
    case IR_SHR:
    case IR_SAR:
    case IR_NEG:
    case IR_NOT:
    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
    case IR_FNEG:
    case IR_CMP:
    case IR_FCMP:
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
        return true;
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
        return div_safe(f, i);
    default:
        return false;
    }
}

enum { USE_ADDR = 1,
       USE_OTHER = 2,
       USE_BR = 4 };

static void classify_uses(IRFunc *f, unsigned char *fl) {
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (i->a != IR_NONE) fl[i->a] |= (i->op == IR_LOAD || i->op == IR_STORE) ? USE_ADDR : i->op == IR_BR ? USE_BR
                                                                                                                 : USE_OTHER;
            if (i->b != IR_NONE) fl[i->b] |= (i->op == IR_LOAD || i->op == IR_STORE) ? USE_ADDR : USE_OTHER;
            if (i->c != IR_NONE) fl[i->c] |= USE_OTHER;
            for (int j = 0; j < i->nargs; j++) fl[i->args[j]] |= USE_OTHER;
        }
    }
}

// Instrs the backend folds into an addressing mode / fused branch for free:
// hoisting them would only lengthen a live range.
static bool free_at_use(IRFunc *f, const IRInstr *i, unsigned char fl) {
    switch (i->op) {
    case IR_ADD:
        return fl == USE_ADDR;
    case IR_SHL:
        return fl == USE_ADDR && V(f, i->b)->op == IR_ICONST && V(f, i->b)->imm >= 1 && V(f, i->b)->imm <= 3;
    case IR_MUL: {
        IRInstr *c = V(f, i->b);
        return fl == USE_ADDR && c->op == IR_ICONST && (c->imm == 2 || c->imm == 4 || c->imm == 8);
    }
    case IR_CMP:
    case IR_FCMP:
        return fl == USE_BR;
    default:
        return false;
    }
}

static int loop_cmp(const void *x, const void *y) {
    const Loop *a = x, *b = y;
    return a->size - b->size;
}

// Fills L->in/size: the header plus every block reaching a latch without passing it.
static void loop_members(Loop *L, int nb, IRBlock **work) {
    free(L->in);
    L->in = xcalloc((size_t)nb, 1);
    int wp = 0;
    L->in[L->header->id] = 1;
    L->size = 1;
    for (int j = 0; j < L->nlatches; j++)
        if (!L->in[L->latches[j]->id]) {
            L->in[L->latches[j]->id] = 1;
            L->size++;
            work[wp++] = L->latches[j];
        }
    while (wp) {
        IRBlock *x = work[--wp];
        for (int k = 0; k < x->npreds; k++) {
            IRBlock *p = x->preds[k];
            if (!L->in[p->id]) {
                L->in[p->id] = 1;
                L->size++;
                work[wp++] = p;
            }
        }
    }
}

// Natural loops from dominance back edges, one per header, inner loops first. A loop
// whose header does not dominate every member (irreducible control flow) is skipped.
// Membership arrays are sized for the CFG as it is now: any pass that adds blocks
// must rediscover (or loop_members()) before consulting them again.
static Loop *find_loops(IRFunc *f, int *nl) {
    Dom d;
    dom_build(f, &d);
    IRBlock **work = xcalloc((size_t)f->nblocks, sizeof(*work));
    Loop *loops = xcalloc((size_t)f->nblocks, sizeof(*loops));
    int n = 0;
    for (int i = 0; i < d.n; i++) {
        IRBlock *h = d.rpo[i];
        Loop L = {0};
        L.header = h;
        L.latches = xcalloc((size_t)h->npreds, sizeof(IRBlock *));
        for (int k = 0; k < h->npreds; k++)
            if (dominates(&d, h->id, h->preds[k]->id)) L.latches[L.nlatches++] = h->preds[k];
        bool ok = L.nlatches > 0;
        if (ok) {
            loop_members(&L, f->nblocks, work);
            for (int b = 0; b < f->nblocks && ok; b++)
                if (L.in[b] && !dominates(&d, h->id, b)) ok = false;
        }
        if (ok) {
            loops[n++] = L;
        } else {
            free(L.latches);
            free(L.in);
        }
    }
    dom_free(&d);
    free(work);
    qsort(loops, (size_t)n, sizeof(*loops), loop_cmp);
    *nl = n;
    return loops;
}

static void free_loops(Loop *loops, int n) {
    for (int li = 0; li < n; li++) {
        free(loops[li].latches);
        free(loops[li].in);
    }
    free(loops);
}

// The preheader of L (the unique outside predecessor of the header, ending in
// `jmp header`), created when the loop has none. *created reports a new block.
static IRBlock *loop_pre(IRFunc *f, Loop *L, bool *created) {
    IRBlock *h = L->header;
    int *out = xcalloc((size_t)h->npreds, sizeof(int));
    int nout = 0;
    for (int k = 0; k < h->npreds; k++) {
        bool latch = false;
        for (int j = 0; j < L->nlatches; j++)
            if (L->latches[j] == h->preds[k]) latch = true;
        if (!latch) out[nout++] = k;
    }
    IRBlock *x = h->preds[out[0]], *pre = x;
    *created = !(nout == 1 && x->ins[x->nins - 1]->op == IR_JMP);
    if (*created) pre = make_preheader(f, h, out, nout);
    free(out);
    return pre;
}

// Gives every loop a preheader (L->pre), creating blocks where needed, and refreshes
// the membership arrays for the grown CFG. Returns the number of blocks created.
static int ensure_preheaders(IRFunc *f, Loop *loops, int nloops) {
    int created = 0;
    for (int li = 0; li < nloops; li++) {
        Loop *L = &loops[li];
        L->pre = loop_pre(f, L, &L->created);
        created += L->created;
    }
    if (created) {
        // Keep the block layout natural: place each new preheader right before its header.
        IRBlock **nbk = xcalloc((size_t)f->nblocks, sizeof(*nbk));
        int w = 0, oldn = f->nblocks - created;
        for (int i = 0; i < oldn; i++) {
            IRBlock *b = f->blocks[i];
            for (int li = 0; li < nloops; li++)
                if (loops[li].header == b && loops[li].created) nbk[w++] = loops[li].pre;
            nbk[w++] = b;
        }
        memcpy(f->blocks, nbk, sizeof(*nbk) * (size_t)w);
        for (int i = 0; i < w; i++) f->blocks[i]->id = i;
        free(nbk);
    }
    IRBlock **work = xcalloc((size_t)f->nblocks, sizeof(*work));
    for (int li = 0; li < nloops; li++) loop_members(&loops[li], f->nblocks, work);
    free(work);
    qsort(loops, (size_t)nloops, sizeof(*loops), loop_cmp); // inner loops first
    return created;
}

static bool licm(IRFunc *f) {
    int nloops;
    Loop *loops = find_loops(f, &nloops);
    if (!nloops) {
        free_loops(loops, 0);
        return false;
    }
    int created = ensure_preheaders(f, loops, nloops);
    int nb = f->nblocks;

    unsigned char *fl = xcalloc((size_t)f->nvals, 1);
    unsigned char *hoist = xcalloc((size_t)f->nvals, 1);
    int *sel = xcalloc((size_t)f->nvals, sizeof(int));
    classify_uses(f, fl);
    IRBlock **rpo = xcalloc((size_t)nb, sizeof(*rpo));
    int nrpo = ir_rpo(f, rpo);
    bool changed = false;
    for (int li = 0; li < nloops; li++) {
        Loop *L = &loops[li];
        int nsel = 0;
        // Select in RPO / program order: defs precede uses, so one sweep reaches the fixpoint.
        for (int bi = 0; bi < nrpo; bi++) {
            IRBlock *b = rpo[bi];
            if (!L->in[b->id]) continue;
            for (int k = 0; k < b->nins; k++) {
                IRInstr *i = b->ins[k];
                if (i->op == IR_GADDR) {
                    // Leaf address: the backend rematerializes it (a lea) at every use unless
                    // its def sits outside the loop of the uses, where it earns a register.
                    if (!hoist[i->id]) {
                        hoist[i->id] = 1;
                        sel[nsel++] = i->id;
                    }
                    continue;
                }
                if (!hoistable(f, i) || free_at_use(f, i, fl[i->id])) continue;
                int32_t ops[3] = {i->a, i->b, i->c};
                bool inv = true;
                for (int j = 0; j < 3 && inv; j++) {
                    if (ops[j] == IR_NONE) continue;
                    IRInstr *o = V(f, ops[j]);
                    inv = !L->in[o->blk->id] || hoist[o->id] || is_leaf(o);
                }
                if (!inv) continue;
                for (int j = 0; j < 3; j++) {
                    if (ops[j] == IR_NONE) continue;
                    IRInstr *o = V(f, ops[j]);
                    if (L->in[o->blk->id] && !hoist[o->id]) { // loop-local leaf constant moves along
                        hoist[o->id] = 1;
                        sel[nsel++] = o->id;
                    }
                }
                hoist[i->id] = 1;
                sel[nsel++] = i->id;
            }
        }
        if (!nsel) continue;
        IRBlock *pre = L->pre;
        for (int bi = 0; bi < nrpo; bi++) {
            IRBlock *b = rpo[bi];
            if (!L->in[b->id]) continue;
            int w = 0;
            for (int k = 0; k < b->nins; k++) {
                IRInstr *i = b->ins[k];
                if (hoist[i->id]) {
                    IRInstr *t = pre->ins[pre->nins - 1];
                    pre->nins--;
                    ir_block_append(pre, i);
                    ir_block_append(pre, t);
                } else {
                    b->ins[w++] = i;
                }
            }
            b->nins = w;
        }
        // Flags are per loop: an instr hoisted out of an inner loop still varies in the outer one.
        for (int k = 0; k < nsel; k++) hoist[sel[k]] = 0;
        changed = true;
    }
    free(rpo);
    free(fl);
    free(hoist);
    free(sel);
    free_loops(loops, nloops);
    return changed || created;
}

// ------------------------------------------------------------ memory optimization
//
// Every LOAD/STORE in the IR is a plain access (the builder rejects volatile, atomics
// and asm), so loads can be forwarded/merged and stores dropped subject to aliasing.
// An access is an address root (frame slot / global symbol / unknown pointer) plus
// either a constant offset from the root (cst) or (base, idx, scale, disp).
// Alias facts used (all valid C):
//  - constant-offset accesses to frame slots/globals compare by byte interval (the
//    frame address of slot imm is rbp - imm);
//  - different global symbols, and frame vs global, are distinct objects;
//  - a frame slot whose address never escapes is distinct from every pointer not
//    derived from it, and calls cannot touch it. A slot escapes when a pointer derived
//    from its FRAMEADDR (through ADD) is used other than as the address operand of a
//    LOAD/STORE (call arg, stored value, phi, compare, return, index operand, ...).
// Everything not proven distinct may alias.

enum { R_NONE,
       R_FRAME,
       R_GLOBAL };
#define MEM_CAP 48 // availability-table entries per path; the oldest is forgotten when full
#define PEND_CAP 32

typedef struct {
    bool cst; // root + constant offset (no base value / index)
    int rk; // R_*
    int slot; // R_FRAME: slot index
    const char *sym; // R_GLOBAL
    int base, idx, scale; // !cst: base value, index value or IR_NONE, scale
    int64_t disp; // cst frame: rbp-relative byte offset; cst global: offset from sym; else displacement
    int size;
} Acc;

typedef struct {
    IRFunc *f;
    int *root; // per value: slot index+1 (frame-derived), -(GADDR id+1) (global-derived), 0 none
    int64_t *slot_imm;
    bool *slot_esc;
    int nslots;
} Mem;

static int mem_size(int mw) {
    static const int sz[] = {1, 2, 4, 8, 4, 8};
    return sz[mw];
}

// Result type of a LOAD of width mw.
static IRType mem_type(int mw) {
    switch (mw) {
    case IRM_I64: return IRT_I64;
    case IRM_F32: return IRT_F32;
    case IRM_F64: return IRT_F64;
    default: return IRT_I32;
    }
}

static int slot_index(Mem *m, int64_t imm) {
    for (int k = 0; k < m->nslots; k++)
        if (m->slot_imm[k] == imm) return k;
    m->slot_imm[m->nslots] = imm;
    return m->nslots++;
}

static void esc_use(Mem *m, const IRInstr *user, int pos, int v) {
    int r = m->root[v];
    if (r <= 0) return;
    if ((user->op == IR_LOAD || user->op == IR_STORE) && pos == 0) return;
    if (user->op == IR_ADD && m->root[user->id] == r) return;
    m->slot_esc[r - 1] = true;
}

static void mem_init(Mem *m, IRFunc *f) {
    m->f = f;
    m->root = xcalloc((size_t)f->nvals, sizeof(int));
    m->slot_imm = xcalloc((size_t)f->nvals, sizeof(int64_t));
    m->slot_esc = xcalloc((size_t)f->nvals, 1);
    m->nslots = 0;
    IRBlock **rpo = xcalloc((size_t)f->nblocks, sizeof(*rpo));
    int n = ir_rpo(f, rpo);
    for (int bi = 0; bi < n; bi++) {
        IRBlock *b = rpo[bi];
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_FRAMEADDR) {
                m->root[i->id] = slot_index(m, i->imm) + 1;
            } else if (i->op == IR_GADDR) {
                m->root[i->id] = -(i->id + 1);
            } else if (i->op == IR_ADD) {
                int ra = m->root[i->a], rb = m->root[i->b];
                if (ra && !rb) m->root[i->id] = ra;
                else if (!ra && rb)
                    m->root[i->id] = rb;
            }
        }
    }
    free(rpo);
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (i->a != IR_NONE) esc_use(m, i, 0, i->a);
            if (i->b != IR_NONE) esc_use(m, i, 1, i->b);
            if (i->c != IR_NONE) esc_use(m, i, 2, i->c);
            for (int j = 0; j < i->nargs; j++) esc_use(m, i, 3, i->args[j]);
        }
    }
}

static void mem_free(Mem *m) {
    free(m->root);
    free(m->slot_imm);
    free(m->slot_esc);
}

// Describes the address of LOAD/STORE i (operands already resolved).
static void acc_make(Mem *m, const IRInstr *i, Acc *o) {
    IRFunc *f = m->f;
    int a = i->a;
    int64_t disp = i->imm;
    for (;;) { // peel constant additions off the base
        IRInstr *d = V(f, a);
        if (d->op != IR_ADD) break;
        int x = res(f, d->a), y = res(f, d->b);
        if (V(f, y)->op == IR_ICONST) {
            disp += V(f, y)->imm;
            a = x;
        } else if (V(f, x)->op == IR_ICONST) {
            disp += V(f, x)->imm;
            a = y;
        } else {
            break;
        }
    }
    int idx = i->b, scale = i->scale;
    if (idx != IR_NONE && V(f, idx)->op == IR_ICONST) {
        disp += V(f, idx)->imm * scale;
        idx = IR_NONE;
    }
    if (idx == IR_NONE) scale = 1;
    memset(o, 0, sizeof *o);
    o->size = mem_size(i->mw);
    o->idx = idx;
    o->scale = scale;
    o->base = a;
    IRInstr *d = V(f, a);
    int r = m->root[a];
    if (d->op == IR_FRAMEADDR) {
        o->rk = R_FRAME;
        o->slot = r - 1;
        if (idx == IR_NONE) {
            o->cst = true;
            disp -= d->imm;
        }
    } else if (d->op == IR_GADDR) {
        o->rk = R_GLOBAL;
        o->sym = d->sym;
        if (idx == IR_NONE) {
            o->cst = true;
            disp += d->imm;
        }
    } else if (r > 0) {
        o->rk = R_FRAME;
        o->slot = r - 1;
    } else if (r < 0) {
        o->rk = R_GLOBAL;
        o->sym = V(f, -r - 1)->sym;
    }
    o->disp = disp;
}

static bool overlap(int64_t a, int sa, int64_t b, int sb) { return a < b + sb && b < a + sa; }

// Same start address, provably.
static bool acc_equal(Mem *m, const Acc *x, const Acc *y) {
    if (x->cst != y->cst || x->rk != y->rk || x->disp != y->disp) return false;
    if (x->cst) return x->rk == R_FRAME || strcmp(x->sym, y->sym) == 0;
    return x->scale == y->scale && res(m->f, x->base) == res(m->f, y->base) &&
        res(m->f, x->idx) == res(m->f, y->idx);
}

static bool acc_alias(Mem *m, const Acc *x, const Acc *y) {
    if (x->cst && y->cst) {
        if (x->rk != y->rk) return false;
        if (x->rk == R_GLOBAL && strcmp(x->sym, y->sym) != 0) return false;
        return overlap(x->disp, x->size, y->disp, y->size);
    }
    if (!x->cst && !y->cst && x->scale == y->scale && res(m->f, x->base) == res(m->f, y->base) &&
        res(m->f, x->idx) == res(m->f, y->idx))
        return overlap(x->disp, x->size, y->disp, y->size);
    // different bases: only provably distinct roots separate them
    if ((x->rk == R_FRAME && y->rk == R_GLOBAL) || (x->rk == R_GLOBAL && y->rk == R_FRAME)) return false;
    if (x->rk == R_GLOBAL && y->rk == R_GLOBAL) return strcmp(x->sym, y->sym) == 0;
    if (x->rk == R_FRAME && y->rk == R_NONE) return m->slot_esc[x->slot];
    if (x->rk == R_NONE && y->rk == R_FRAME) return m->slot_esc[y->slot];
    return true;
}

// A call cannot touch a frame slot whose address never escapes.
static bool acc_call_safe(const Mem *m, const Acc *a) { return a->rk == R_FRAME && !m->slot_esc[a->slot]; }

typedef struct {
    Acc acc;
    int val; // loaded value (LOAD entry) or stored value (STORE entry)
    uint8_t mw, msign;
    bool is_store;
} Ent;

static void tab_add(Ent *e, int *n, const Ent *x) {
    if (*n == MEM_CAP) {
        memmove(e, e + 1, sizeof(*e) * (MEM_CAP - 1));
        (*n)--;
    }
    e[(*n)++] = *x;
}

// How a LOAD of value type/width L can be produced from stored value v of the same
// width: 1 = v itself, 2 = TRUNC(v), 3 = *op(v) (narrow sign/zero extension), 0 = no.
static int fwd_from_store(const IRInstr *L, const IRInstr *v, IROp *op) {
    if (L->type != mem_type(L->mw)) return 0;
    switch (L->mw) {
    case IRM_I64: return v->type == IRT_I64;
    case IRM_I32: return v->type == IRT_I32 ? 1 : v->type == IRT_I64 ? 2
                                                                     : 0;
    case IRM_I16:
    case IRM_I8:
        if (v->type != IRT_I32) return 0;
        *op = L->mw == IRM_I8 ? (L->msign ? IR_SEXT8 : IR_ZEXT8) : (L->msign ? IR_SEXT16 : IR_ZEXT16);
        return 3;
    case IRM_F32: return v->type == IRT_F32;
    default: return v->type == IRT_F64;
    }
}

typedef struct {
    Ent *e;
    int n, refs;
} Saved;

// Redundant-load elimination and store-to-load forwarding along single-predecessor
// chains (a block with several predecessors starts with an empty table).
static bool forward_loads(Mem *m, IRBlock **rpo, int nrpo) {
    IRFunc *f = m->f;
    Saved **sv = xcalloc((size_t)f->nblocks, sizeof(*sv));
    Ent *tab = xcalloc(MEM_CAP, sizeof(*tab));
    bool changed = false;
    for (int bi = 0; bi < nrpo; bi++) {
        IRBlock *b = rpo[bi];
        int n = 0;
        if (b->npreds == 1 && sv[b->preds[0]->id]) {
            Saved *s = sv[b->preds[0]->id];
            memcpy(tab, s->e, sizeof(*tab) * (size_t)s->n);
            n = s->n;
            if (--s->refs == 0) {
                free(s->e);
                free(s);
                sv[b->preds[0]->id] = NULL;
            }
        }
        bool dirty = false;
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op == IR_LOAD) {
                i->a = res(f, i->a);
                i->b = res(f, i->b);
                Acc a;
                acc_make(m, i, &a);
                int how = 0;
                IROp xop = IR_NOP;
                IRInstr *hv = NULL;
                for (int e = n - 1; e >= 0 && !how; e--) {
                    Ent *E = &tab[e];
                    if (E->mw != i->mw || !acc_equal(m, &E->acc, &a)) continue;
                    IRInstr *v = V(f, res(f, E->val));
                    if (!E->is_store) {
                        if (v->type == i->type && (i->mw >= IRM_I32 || E->msign == i->msign)) how = 1;
                    } else {
                        how = fwd_from_store(i, v, &xop);
                    }
                    hv = v;
                }
                if (how == 1) {
                    fwd_set(f, i->id, hv->id);
                    kill(i);
                } else if (how >= 2) { // exact in-place rewrite of the load
                    i->op = how == 2 ? IR_TRUNC : xop;
                    i->a = hv->id;
                    i->b = IR_NONE;
                    i->imm = 0;
                    i->mw = i->msign = 0;
                    i->scale = 1;
                } else {
                    Ent x = {a, i->id, i->mw, i->msign, false};
                    tab_add(tab, &n, &x);
                    continue;
                }
                changed = dirty = true;
            } else if (i->op == IR_STORE) {
                i->a = res(f, i->a);
                i->b = res(f, i->b);
                i->c = res(f, i->c);
                Acc a;
                acc_make(m, i, &a);
                int w = 0;
                for (int e = 0; e < n; e++)
                    if (!acc_alias(m, &tab[e].acc, &a)) tab[w++] = tab[e];
                n = w;
                Ent x = {a, i->c, i->mw, 0, true};
                tab_add(tab, &n, &x);
            } else if (i->op == IR_CALL) {
                int w = 0;
                for (int e = 0; e < n; e++)
                    if (acc_call_safe(m, &tab[e].acc)) tab[w++] = tab[e];
                n = w;
            }
        }
        if (dirty) compact_block(b);
        IRBlock *s[2];
        int ns = ir_succs(b, s), refs = 0;
        for (int k = 0; k < ns; k++)
            if (s[k]->npreds == 1) refs++;
        if (refs && n) {
            Saved *sa = xcalloc(1, sizeof *sa);
            sa->e = xcalloc((size_t)n, sizeof(*tab));
            memcpy(sa->e, tab, sizeof(*tab) * (size_t)n);
            sa->n = n;
            sa->refs = refs;
            sv[b->id] = sa;
        }
    }
    for (int i = 0; i < f->nblocks; i++)
        if (sv[i]) {
            free(sv[i]->e);
            free(sv[i]);
        }
    free(sv);
    free(tab);
    return changed;
}

typedef struct {
    IRInstr *st;
    Acc acc;
} Pend;

// A STORE overwritten by a later same-address STORE of equal or greater width, with no
// possibly-aliasing load, call or trapping divide in between, is dead. (A trapping
// divide ends the program, whose memory state is observable, so it counts as a reader.)
static bool dse_block(Mem *m, IRBlock *b) {
    Pend pend[PEND_CAP];
    int np = 0;
    bool changed = false;
    for (int k = 0; k < b->nins; k++) {
        IRInstr *i = b->ins[k];
        switch (i->op) {
        case IR_STORE: {
            Acc a;
            acc_make(m, i, &a);
            int w = 0;
            for (int j = 0; j < np; j++) {
                if (acc_equal(m, &pend[j].acc, &a) && a.size >= pend[j].acc.size) {
                    kill(pend[j].st);
                    changed = true;
                } else {
                    pend[w++] = pend[j];
                }
            }
            np = w;
            if (np == PEND_CAP) {
                memmove(pend, pend + 1, sizeof(*pend) * (PEND_CAP - 1));
                np--;
            }
            pend[np].st = i;
            pend[np++].acc = a;
            break;
        }
        case IR_LOAD: {
            Acc a;
            acc_make(m, i, &a);
            int w = 0;
            for (int j = 0; j < np; j++)
                if (!acc_alias(m, &pend[j].acc, &a)) pend[w++] = pend[j];
            np = w;
            break;
        }
        case IR_CALL: {
            int w = 0;
            for (int j = 0; j < np; j++)
                if (acc_call_safe(m, &pend[j].acc)) pend[w++] = pend[j];
            np = w;
            break;
        }
        case IR_SDIV:
        case IR_UDIV:
        case IR_SREM:
        case IR_UREM:
            if (!div_safe(m->f, i)) np = 0;
            break;
        default:
            break;
        }
    }
    if (changed) compact_block(b);
    return changed;
}

// Stores to a non-escaping frame slot that no load in the function can read.
static bool dead_local_stores(Mem *m) {
    IRFunc *f = m->f;
    int nloads = 0, nstores = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nins; k++) {
            if (b->ins[k]->op == IR_LOAD) nloads++;
            else if (b->ins[k]->op == IR_STORE)
                nstores++;
        }
    }
    if (!nstores || (int64_t)nloads * nstores > 4000000) return false;
    Acc *loads = xcalloc((size_t)nloads, sizeof(*loads));
    int nl = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nins; k++)
            if (b->ins[k]->op == IR_LOAD) {
                acc_make(m, b->ins[k], &loads[nl]);
                if (loads[nl].rk == R_FRAME) nl++;
            }
    }
    bool changed = false;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        bool dirty = false;
        for (int k = 0; k < b->nins; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_STORE) continue;
            Acc a;
            acc_make(m, i, &a);
            if (a.rk != R_FRAME || m->slot_esc[a.slot]) continue;
            bool read = false;
            for (int j = 0; j < nl && !read; j++) read = acc_alias(m, &loads[j], &a);
            if (!read) {
                kill(i);
                dirty = true;
            }
        }
        if (dirty) {
            compact_block(b);
            changed = true;
        }
    }
    free(loads);
    return changed;
}

static bool memopt(IRFunc *f) {
    bool any = false;
    for (int bi = 0; bi < f->nblocks && !any; bi++)
        for (int k = 0; k < f->blocks[bi]->nins && !any; k++) {
            IROp op = (IROp)f->blocks[bi]->ins[k]->op;
            any = op == IR_LOAD || op == IR_STORE;
        }
    if (!any) return false;
    fwd_reset(f);
    Mem m;
    mem_init(&m, f);
    IRBlock **rpo = xcalloc((size_t)f->nblocks, sizeof(*rpo));
    int n = ir_rpo(f, rpo);
    bool changed = forward_loads(&m, rpo, n);
    if (changed) fwd_apply(f);
    for (int bi = 0; bi < f->nblocks; bi++) changed |= dse_block(&m, f->blocks[bi]);
    changed |= dead_local_stores(&m);
    free(rpo);
    mem_free(&m);
    return changed;
}

// ---------------------------------------------------- scalar replacement of aggregates
//
// sroa promotes a non-escaping frame slot into independent SSA "cells" (byte intervals
// addressed at constant displacements from the slot's FRAMEADDR) when that is sound:
//  - the slot's address must not escape (mem_init's analysis: a pointer derived from its
//    FRAMEADDR through ADD counts only as a LOAD/STORE address operand);
//  - every LOAD/STORE touching the slot must decompose (acc_make) to (this slot, a
//    constant displacement); a variable-index or otherwise undecomposable access bails
//    the slot -- no partial promotion;
//  - two cells whose byte intervals overlap at all bail the slot -- that covers
//    differing widths over the same bytes (union-style punning) AND the subtler
//    equal-width case of intervals straddling each other (4B at +0 vs 4B at +2), which
//    no single SSA value per displacement can reproduce. Per the rule, each cell then
//    has exactly one width.
// This is sound because for a non-escaping slot those accesses are the ONLY ones that
// can ever observe or mutate it: unknown pointers cannot alias it (acc_alias) and calls
// cannot reach it (acc_call_safe). Each cell's value is tracked as an SSA value along
// single-predecessor chains; multi-predecessor blocks merge cell values available from
// every processed predecessor with phis (back-edge predecessors not yet processed in
// RPO simply contribute nothing, so loop-carried cells are not promoted). A load whose
// cell has no available value on the reaching path would read unspecified bytes, so it
// bails the whole slot: promotion is all-or-nothing. A promoted load rewrites to the
// cell value -- directly when types match, else via the exact TRUNC / narrow-extension
// conversions memopt's fwd_from_store uses; a promoted store vanishes. The FRAMEADDR and
// its ADD chain lose their uses and DCE removes them, so the backend stops allocating
// the slot (it sizes locals from surviving FRAMEADDRs).

typedef struct {
    int64_t disp;
    int width;
    IRInstr *i; // the LOAD/STORE
    bool is_store;
} SAcc;

typedef struct {
    int64_t disp;
    int val; // SSA value id the cell currently holds
} SEnt;

typedef struct {
    SEnt *e;
    int n, cap;
} SMap; // disp -> value, small (cells per slot)

typedef struct {
    IRInstr *i; // the LOAD to rewrite
    int v; // cell value
    int how; // 1 = replace by v, 2 = TRUNC(v), 3 = narrow extension xop
    IROp xop;
} SLd;

static int smap_find(const SMap *s, int64_t disp) {
    for (int k = 0; k < s->n; k++)
        if (s->e[k].disp == disp) return k;
    return -1;
}

static void smap_put(SMap *s, int64_t disp, int val) {
    int k = smap_find(s, disp);
    if (k >= 0) {
        s->e[k].val = val;
        return;
    }
    if (s->n == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 8;
        s->e = realloc(s->e, sizeof(*s->e) * (size_t)s->cap);
        if (!s->e) {
            fprintf(stderr, "rcc: out of memory\n");
            exit(1);
        }
    }
    s->e[s->n].disp = disp;
    s->e[s->n].val = val;
    s->n++;
}

static bool sroa_slot(IRFunc *f, Mem *m, IRBlock **rpo, const int *pos, int nrpo, int slot) {
    if (m->slot_esc[slot]) return false;
    // Phase 1: collect this slot's accesses in reachable blocks; any access that does
    // not decompose to (slot, constant disp) bails the slot.
    SAcc *accs = NULL;
    int nacc = 0, acccap = 0;
    bool bail = false;
    for (int bi = 0; bi < nrpo && !bail; bi++) {
        IRBlock *b = rpo[bi];
        for (int k = 0; k < b->nins && !bail; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_LOAD && i->op != IR_STORE) continue;
            i->a = res(f, i->a);
            i->b = res(f, i->b);
            if (i->op == IR_STORE) i->c = res(f, i->c);
            Acc a;
            acc_make(m, i, &a);
            if (a.rk != R_FRAME || a.slot != slot) continue;
            if (!a.cst) { // variable index or undecomposable address
                bail = true;
                break;
            }
            if (nacc == acccap) {
                if (acccap >= 512) { // keep the pass cheap on pathological functions
                    bail = true;
                    break;
                }
                acccap = acccap ? acccap * 2 : 16;
                accs = realloc(accs, sizeof(*accs) * (size_t)acccap);
                if (!accs) {
                    fprintf(stderr, "rcc: out of memory\n");
                    exit(1);
                }
            }
            accs[nacc].disp = a.disp;
            accs[nacc].width = a.size;
            accs[nacc].i = i;
            accs[nacc].is_store = i->op == IR_STORE;
            nacc++;
        }
    }
    if (bail || !nacc) {
        free(accs);
        return false;
    }
    // Cells must be pairwise disjoint byte intervals: two accesses whose intervals
    // overlap at all (same displacement with differing widths, or -- the easy trap --
    // equal-width intervals straddling each other, e.g. a 4B access at +0 and another
    // at +2) describe memory no single SSA value per displacement can reproduce, so
    // the slot stays in memory. Same displacement + same width is one cell: fine.
    for (int x = 0; x < nacc && !bail; x++)
        for (int y = x + 1; y < nacc; y++) {
            if (accs[x].disp == accs[y].disp && accs[x].width == accs[y].width) continue; // one cell
            if (overlap(accs[x].disp, accs[x].width, accs[y].disp, accs[y].width)) {
                bail = true;
                break;
            }
        }
    free(accs);
    if (bail) return false;
    // Phase 2: dataflow walk recording the rewrites; bail discards everything.
    SMap *st = xcalloc((size_t)f->nblocks, sizeof(*st)); // exit state per block
    SLd *lds = NULL;
    int nld = 0, ldcap = 0;
    IRInstr **kills = NULL; // stores to drop
    int nkill = 0, killcap = 0;
    IRInstr **newphis = NULL; // phis created by the walk (removed if the slot bails)
    int nphi = 0, phicap = 0;
    IRBlock **dirty = NULL;
    int ndirty = 0, dirtycap = 0;
    for (int bi = 0; bi < nrpo && !bail; bi++) {
        IRBlock *b = rpo[bi];
        SMap in = {0};
        if (b->npreds == 1) {
            IRBlock *p = b->preds[0];
            if (pos[p->id] && pos[p->id] - 1 < bi && st[p->id].n) {
                in.e = xcalloc((size_t)st[p->id].n, sizeof(*in.e));
                memcpy(in.e, st[p->id].e, sizeof(*in.e) * (size_t)st[p->id].n);
                in.n = in.cap = st[p->id].n;
            }
        } else if (b->npreds > 1) {
            // Merge: a cell present in every processed predecessor becomes a phi.
            // Predecessors processed later in RPO (back edges, irreducible loops)
            // block the merge by contributing nothing.
            IRBlock *p0 = b->preds[0];
            if (pos[p0->id] && pos[p0->id] - 1 < bi) {
                SMap *s0 = &st[p0->id];
                for (int e = 0; e < s0->n && !bail; e++) {
                    int64_t disp = s0->e[e].disp;
                    int args[64];
                    bool all = b->npreds <= (int)(sizeof args / sizeof *args);
                    if (!all) break;
                    args[0] = s0->e[e].val;
                    for (int k = 1; k < b->npreds && all; k++) {
                        IRBlock *p = b->preds[k];
                        if (!pos[p->id] || pos[p->id] - 1 >= bi) {
                            all = false;
                            break;
                        }
                        int j = smap_find(&st[p->id], disp);
                        if (j < 0) {
                            all = false;
                            break;
                        }
                        args[k] = st[p->id].e[j].val;
                    }
                    if (!all) continue;
                    IRType t = V(f, args[0])->type;
                    for (int k = 1; k < b->npreds; k++)
                        if (V(f, args[k])->type != t) {
                            bail = true;
                            break;
                        }
                    if (bail) break;
                    IRInstr *ph = ir_instr_new(f, IR_PHI, t);
                    ph->aux = IR_NONE; // created after fwd_reset: keep res() chains off it
                    ph->nargs = b->npreds;
                    ph->args = xcalloc((size_t)b->npreds, sizeof(int32_t));
                    for (int k = 0; k < b->npreds; k++) ph->args[k] = args[k];
                    ir_block_add_phi(b, ph);
                    if (nphi == phicap) {
                        phicap = phicap ? phicap * 2 : 8;
                        newphis = realloc(newphis, sizeof(*newphis) * (size_t)phicap);
                        if (!newphis) {
                            fprintf(stderr, "rcc: out of memory\n");
                            exit(1);
                        }
                    }
                    newphis[nphi++] = ph;
                    smap_put(&in, disp, ph->id);
                }
            }
        }
        for (int k = 0; k < b->nins && !bail; k++) {
            IRInstr *i = b->ins[k];
            if (i->op != IR_LOAD && i->op != IR_STORE) continue;
            Acc a;
            acc_make(m, i, &a);
            if (a.rk != R_FRAME || a.slot != slot) continue;
            int j = smap_find(&in, a.disp);
            if (i->op == IR_STORE) {
                smap_put(&in, a.disp, i->c);
                if (nkill == killcap) {
                    killcap = killcap ? killcap * 2 : 16;
                    kills = realloc(kills, sizeof(*kills) * (size_t)killcap);
                    if (!kills) {
                        fprintf(stderr, "rcc: out of memory\n");
                        exit(1);
                    }
                }
                kills[nkill++] = i;
            } else {
                if (j < 0) { // would read bytes no store produced: keep the slot in memory
                    bail = true;
                    break;
                }
                IROp xop = IR_NOP;
                int how = fwd_from_store(i, V(f, in.e[j].val), &xop);
                if (!how) {
                    bail = true;
                    break;
                }
                if (nld == ldcap) {
                    ldcap = ldcap ? ldcap * 2 : 16;
                    lds = realloc(lds, sizeof(*lds) * (size_t)ldcap);
                    if (!lds) {
                        fprintf(stderr, "rcc: out of memory\n");
                        exit(1);
                    }
                }
                lds[nld].i = i;
                lds[nld].v = in.e[j].val;
                lds[nld].how = how;
                lds[nld].xop = xop;
                nld++;
            }
        }
        st[b->id] = in; // exit state for single-pred successors
        if (ndirty == dirtycap) { // remember blocks with recorded rewrites
            dirtycap = dirtycap ? dirtycap * 2 : 8;
            dirty = realloc(dirty, sizeof(*dirty) * (size_t)dirtycap);
            if (!dirty) {
                fprintf(stderr, "rcc: out of memory\n");
                exit(1);
            }
        }
        dirty[ndirty++] = b;
    }
    if (bail) {
        for (int k = 0; k < nphi; k++) ir_block_remove(newphis[k]->blk, newphis[k]);
        for (int k = 0; k < f->nblocks; k++) free(st[k].e);
        free(st);
        free(lds);
        free(kills);
        free(newphis);
        free(dirty);
        return false;
    }
    // Apply: loads become the cell value (or its exact conversion), stores vanish.
    for (int k = 0; k < nld; k++) {
        IRInstr *i = lds[k].i;
        if (lds[k].how == 1) {
            fwd_set(f, i->id, lds[k].v);
            kill(i);
        } else { // in-place rewrite; the load's id and result type stay
            i->op = lds[k].how == 2 ? IR_TRUNC : lds[k].xop;
            i->a = lds[k].v;
            i->b = IR_NONE;
            i->c = IR_NONE;
            i->imm = 0;
            i->mw = i->msign = 0;
            i->scale = 1;
        }
    }
    for (int k = 0; k < nkill; k++) kill(kills[k]);
    fwd_apply(f);
    for (int k = 0; k < ndirty; k++) compact_block(dirty[k]);
    for (int k = 0; k < f->nblocks; k++) free(st[k].e);
    free(st);
    free(lds);
    free(kills);
    free(newphis);
    free(dirty);
    return true;
}

static bool sroa(IRFunc *f) {
    bool any = false;
    for (int bi = 0; bi < f->nblocks && !any; bi++)
        for (int k = 0; k < f->blocks[bi]->nins && !any; k++) {
            IROp op = (IROp)f->blocks[bi]->ins[k]->op;
            any = op == IR_LOAD || op == IR_STORE;
        }
    if (!any) return false;
    fwd_reset(f);
    Mem m;
    mem_init(&m, f);
    IRBlock **rpo = xcalloc((size_t)f->nblocks, sizeof(*rpo));
    int n = ir_rpo(f, rpo);
    int *pos = xcalloc((size_t)f->nblocks, sizeof(int));
    for (int k = 0; k < n; k++) pos[rpo[k]->id] = k + 1; // 0 = unreachable in RPO
    bool changed = false;
    for (int slot = 0; slot < m.nslots; slot++) changed |= sroa_slot(f, &m, rpo, pos, n, slot);
    free(pos);
    free(rpo);
    mem_free(&m);
    return changed;
}

// ------------------------------------------------------------------ loop passes
//
//   loop_rotate   guard + do-while: the header's exit test is copied into the
//                 preheader (the guard) and re-evaluated at the bottom of the loop
//   iv_widen      i32 induction variables that index memory become i64 phis
//   hoist_loads   loop-invariant loads that no store in the loop can overwrite
//
// They run after the scalar fixpoint, on loops from find_loops(); each pass that
// changes the CFG rediscovers loops itself, so none relies on stale membership.

typedef struct {
    IRInstr *u;
    int slot; // 0..2: a, b, c; 3+j: args[j] (phi incoming value / call argument j)
} Use;

static int *use_slot(IRInstr *u, int slot) {
    return slot == 0 ? &u->a : slot == 1 ? &u->b
        : slot == 2                      ? &u->c
                                         : &u->args[slot - 3];
}

static void use_add(Use **us, int *n, int *cap, IRInstr *u, int slot) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 16;
        *us = realloc(*us, sizeof(**us) * (size_t)*cap);
        if (!*us) {
            fprintf(stderr, "rcc: out of memory\n");
            exit(1);
        }
    }
    (*us)[(*n)++] = (Use){u, slot};
}

// Appends the not yet linked instr n just before the terminator of b.
static void put_before_term(IRBlock *b, IRInstr *n) {
    IRInstr *t = b->ins[b->nins - 1];
    b->nins--;
    ir_block_append(b, n);
    ir_block_append(b, t);
}

// Links the new instr n into b right before the existing non-phi instr `at`.
static void put_before(IRBlock *b, const IRInstr *at, IRInstr *n) {
    ir_block_append(b, n);
    int pos = 0;
    while (b->ins[pos] != at) pos++;
    memmove(b->ins + pos + 1, b->ins + pos, sizeof(*b->ins) * (size_t)(b->nins - 1 - pos));
    b->ins[pos] = n;
}

static IRInstr *new_instr(IRFunc *f, IROp op, IRType t, int a, int b) {
    IRInstr *n = ir_instr_new(f, op, t);
    n->aux = IR_NONE;
    n->a = a;
    n->b = b;
    return n;
}

static IRInstr *new_iconst(IRFunc *f, IRType t, int64_t v) {
    IRInstr *n = new_instr(f, IR_ICONST, t, IR_NONE, IR_NONE);
    n->imm = v;
    return n;
}

// ------------------------------------------------------------ value ranges
//
// A tiny, sound range analysis for I32 values, used to prove that an increment
// cannot wrap. Facts come from (1) constants, (2) conditional edges whose target has
// that edge as its only entry and dominates the point of interest: the branch
// condition is a signed compare of the value (an SSA value keeps its value for the
// whole iteration in which it is defined, and every path from the definition to
// the dominated point crosses the edge), and (3) phis: the hull of what flows in
// along each incoming edge (facts of the edge's source block plus the edge itself).

typedef struct {
    int64_t lo, hi;
} Rng;

static Rng rng_full(void) { return (Rng){INT32_MIN, INT32_MAX}; }

// Intersection. An empty result means the point is unreachable, so any answer is sound.
static Rng rng_meet(Rng a, Rng b) {
    Rng r = {a.lo > b.lo ? a.lo : b.lo, a.hi < b.hi ? a.hi : b.hi};
    return r.lo <= r.hi ? r : a;
}

static Rng rng_hull(Rng a, Rng b) {
    return (Rng){a.lo < b.lo ? a.lo : b.lo, a.hi > b.hi ? a.hi : b.hi};
}

// Narrows *r by what taking (truth) or not taking the branch on `cmp` says about v.
static void rng_refine(IRFunc *f, Rng *r, int v, const IRInstr *cmp, bool truth) {
    if (cmp->op != IR_CMP || V(f, cmp->a)->type != IRT_I32) return;
    IRCC cc = (IRCC)cmp->cc;
    int x;
    if (cmp->a == v) {
        x = cmp->b;
    } else if (cmp->b == v) {
        x = cmp->a;
        cc = swap_cc(cc);
    } else {
        return;
    }
    if (x == v) return;
    if (!truth && !invert_cc(cc, &cc)) return;
    Rng xr = rng_full();
    if (V(f, x)->op == IR_ICONST) xr.lo = xr.hi = V(f, x)->imm;
    switch (cc) {
    case IRCC_SLT:
        if (xr.hi - 1 < r->hi) r->hi = xr.hi - 1;
        break;
    case IRCC_SLE:
        if (xr.hi < r->hi) r->hi = xr.hi;
        break;
    case IRCC_SGT:
        if (xr.lo + 1 > r->lo) r->lo = xr.lo + 1;
        break;
    case IRCC_SGE:
        if (xr.lo > r->lo) r->lo = xr.lo;
        break;
    case IRCC_EQ: *r = rng_meet(*r, xr); break;
    default: break;
    }
}

// What the conditional edges dominating the start of block bid say about v.
static Rng rng_dom(IRFunc *f, const Dom *d, int v, int bid) {
    Rng r = rng_full();
    if (d->idom[bid] < 0) return r;
    for (int c = bid;; c = d->idom[c]) {
        IRBlock *B = f->blocks[c];
        if (B->npreds == 1) {
            IRBlock *T = B->preds[0];
            IRInstr *t = T->ins[T->nins - 1];
            if (t->op == IR_BR && t->t != t->f) rng_refine(f, &r, v, V(f, t->a), t->t == B);
        }
        if (d->idom[c] == c) break;
    }
    return r;
}

static Rng rng_val(IRFunc *f, const Dom *d, int v, int bid, int depth);

// Range of v as it flows along the edge x -> h.
static Rng rng_edge(IRFunc *f, const Dom *d, int v, IRBlock *x, IRBlock *h, int depth) {
    Rng r = rng_val(f, d, v, x->id, depth);
    IRInstr *t = x->ins[x->nins - 1];
    if (t->op == IR_BR && t->t != t->f) rng_refine(f, &r, v, V(f, t->a), t->t == h);
    return r;
}

// Range of the I32 value v at any point of block bid that it dominates.
static Rng rng_val(IRFunc *f, const Dom *d, int v, int bid, int depth) {
    IRInstr *i = V(f, v);
    if (i->op == IR_ICONST) return (Rng){i->imm, i->imm};
    Rng r = rng_dom(f, d, v, bid);
    if (i->op == IR_PHI && depth > 0) {
        Rng u = {INT64_MAX, INT64_MIN};
        for (int k = 0; k < i->nargs; k++)
            u = rng_hull(u, rng_edge(f, d, i->args[k], i->blk->preds[k], i->blk, depth - 1));
        r = rng_meet(r, u);
    }
    return r;
}

// ------------------------------------------------------- induction variable widening
//
// p = phi(init, q), q = p + K (K constant) is an I32 value; code often uses it only as
// sext32(p) for addressing, which costs a movsxd per use. When q provably never wraps,
// sext32(p) == w for the I64 phi w = phi(sext32(init), w + K), so those sign
// extensions disappear, and p itself, being dead afterwards, with them. Wrap is
// observable (rcc compiles signed overflow as 32-bit wrap), hence the proof:
// range(p) at the increment must leave room for K. The typical proofs are the loop
// test `p < n` (K = 1: p <= INT_MAX-1 where it is incremented) or `p >= 0` (K = -1).
//
// Uses of p/q: sext32 -> w/w'; signed compare against a loop-invariant -> an I64 compare
// against sext32(invariant) (equal ordering, since both sides are exact); add/sub of a
// constant that cannot wrap, feeding sext32 -> w + c; anything else gets trunc(w).
// The rewrite is only done when it removes more in-loop instrs than it adds.

enum { WU_SEXT,
       WU_CMP,
       WU_OTHER,
       WU_GONE };

typedef struct {
    IRInstr *u;
    int slot, kind;
    int src; // narrow value named by the operand: p, q or a derived add of p
    int64_t off; // WU_SEXT of a derived add: the constant it adds
    bool der;
} WUse;

static bool cc_signed_or_eq(int cc) {
    return cc == IRCC_EQ || cc == IRCC_NE || cc == IRCC_SLT || cc == IRCC_SLE || cc == IRCC_SGT || cc == IRCC_SGE;
}

// Block where the use of an operand happens (phi arguments are used at the end of the predecessor).
static IRBlock *use_block(const IRInstr *u, int slot) {
    return u->op == IR_PHI ? u->blk->preds[slot - 3] : u->blk;
}

// When the sign extensions of the IV are used only as `mul s, C` with one constant C that
// addressing modes cannot absorb (not 1, 2, 4, 8: struct strides such as 12, 24, 40), the
// widened IV can carry the product itself: it steps by C*K, which replaces a multiply per
// iteration (and a register) by an add. The compares move to the scaled domain too; as
// the narrow IV never wraps, sext32 values are int32 and C * value cannot overflow I64
// (C is limited to 2^20). Returns C (0: not applicable) and the products in *mu, with the
// narrow value (src) each one scales.
static int64_t sext_scale(IRFunc *f, const WUse *us, int nus, WUse **mu, int *nmu) {
    unsigned char *is_s = xcalloc((size_t)f->nvals, 1);
    int *src_of = xcalloc((size_t)f->nvals, sizeof(int));
    int ns = 0, cm = 0;
    for (int k = 0; k < nus; k++)
        if (us[k].kind == WU_SEXT && !us[k].der) {
            is_s[us[k].u->id] = 1;
            src_of[us[k].u->id] = us[k].src;
            ns++;
        }
    int64_t C = 0;
    bool ok = ns > 0;
    *mu = NULL;
    *nmu = 0;
    for (int bi = 0; bi < f->nblocks && ok; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins && ok; k++) {
            IRInstr *u = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            int nsl = 3 + u->nargs;
            for (int slot = 0; slot < nsl && ok; slot++) {
                int v = *use_slot(u, slot);
                if (v == IR_NONE || !is_s[v]) continue;
                if (u->op != IR_MUL || u->type != IRT_I64 || u->a == u->b) {
                    ok = false;
                    break;
                }
                IRInstr *oi = V(f, u->a == v ? u->b : u->a);
                if (oi->op != IR_ICONST || oi->imm < 3 || oi->imm > (1 << 20) || is_pow2((uint64_t)oi->imm) ||
                    (C && C != oi->imm)) {
                    ok = false;
                    break;
                }
                C = oi->imm;
                if (*nmu == cm) {
                    cm = cm ? cm * 2 : 8;
                    *mu = realloc(*mu, sizeof(**mu) * (size_t)cm);
                }
                (*mu)[(*nmu)++] = (WUse){u, 0, 0, src_of[v], 0, false};
            }
        }
    }
    free(is_s);
    free(src_of);
    if (!ok || !*nmu) {
        free(*mu);
        *mu = NULL;
        *nmu = 0;
        return 0;
    }
    return C;
}

static bool widen_iv(IRFunc *f, const Dom *d, const Loop *L, IRInstr *p, unsigned char *touched) {
    IRBlock *H = L->header, *P = L->pre;
    int pi = pred_index(H, P);
    if (pi < 0 || p->type != IRT_I32) return false;
    int init = p->args[pi], q = IR_NONE;
    int64_t K = 0;
    for (int k = 0; k < H->npreds; k++) {
        if (k == pi) continue;
        IRInstr *s = V(f, p->args[k]);
        if ((s->op != IR_ADD && s->op != IR_SUB) || s->a != p->id || V(f, s->b)->op != IR_ICONST) return false;
        if (!L->in[s->blk->id]) return false;
        int64_t c = s->op == IR_ADD ? V(f, s->b)->imm : -V(f, s->b)->imm;
        if (q == IR_NONE) {
            q = s->id;
            K = c;
        } else if (q != s->id) {
            return false;
        }
    }
    if (q == IR_NONE || K == 0) return false;
    IRInstr *qi = V(f, q);
    Rng pr = rng_val(f, d, p->id, qi->blk->id, 2);
    if (K > 0 ? pr.hi + K > INT32_MAX : pr.lo + K < INT32_MIN) return false;

    WUse *us = NULL;
    int nus = 0, cus = 0;
    int *der = NULL, nder = 0;
    int64_t *doff = NULL;
    bool ok = true;
    // Direct uses of p and q.
    for (int bi = 0; bi < f->nblocks && ok; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins && ok; k++) {
            IRInstr *u = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (u == p || u == qi) continue;
            int nsl = 3 + u->nargs;
            for (int slot = 0; slot < nsl; slot++) {
                int v = *use_slot(u, slot);
                if (v != p->id && v != q) continue;
                WUse w = {u, slot, WU_OTHER, v, 0, false};
                if (u->op == IR_SEXT32) {
                    w.kind = WU_SEXT;
                } else if (u->op == IR_CMP && slot <= 1 && cc_signed_or_eq(u->cc)) {
                    int o = slot == 0 ? u->b : u->a;
                    IRInstr *oi = V(f, o);
                    if (o != p->id && o != q && (oi->op == IR_ICONST || !L->in[oi->blk->id])) w.kind = WU_CMP;
                } else if ((u->op == IR_ADD || u->op == IR_SUB) && v == p->id && slot == 0 &&
                           V(f, u->b)->op == IR_ICONST) {
                    int64_t c = u->op == IR_ADD ? V(f, u->b)->imm : -V(f, u->b)->imm;
                    Rng r = rng_val(f, d, p->id, u->blk->id, 2);
                    if (r.lo + c >= INT32_MIN && r.hi + c <= INT32_MAX) {
                        der = realloc(der, sizeof(*der) * (size_t)(nder + 1));
                        doff = realloc(doff, sizeof(*doff) * (size_t)(nder + 1));
                        der[nder] = u->id;
                        doff[nder++] = c;
                    }
                }
                if (nus == cus) {
                    cus = cus ? cus * 2 : 16;
                    us = realloc(us, sizeof(*us) * (size_t)cus);
                }
                us[nus++] = w;
                if (nus > 256) ok = false; // not worth the bookkeeping
            }
        }
    }
    // Users of the derived adds: sext32 of them become w + c, any other user keeps the add alive.
    int *drem = xcalloc((size_t)nder + 1, sizeof(int));
    for (int bi = 0; bi < f->nblocks && ok && nder; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins && ok; k++) {
            IRInstr *u = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            int nsl = 3 + u->nargs;
            for (int slot = 0; slot < nsl; slot++) {
                int v = *use_slot(u, slot);
                for (int e = 0; e < nder; e++) {
                    if (v != der[e]) continue;
                    if (u->op == IR_SEXT32) {
                        if (nus == cus) {
                            cus = cus ? cus * 2 : 16;
                            us = realloc(us, sizeof(*us) * (size_t)cus);
                        }
                        us[nus++] = (WUse){u, slot, WU_SEXT, der[e], doff[e], true};
                        if (nus > 256) ok = false;
                    } else {
                        drem[e]++;
                    }
                }
            }
        }
    }
    // A derived add without other users disappears, taking its use of p along.
    for (int e = 0; e < nder; e++)
        if (!drem[e])
            for (int k = 0; k < nus; k++)
                if (us[k].u->id == der[e]) us[k].kind = WU_GONE;
    // Profit: sign extensions removed vs truncations added, counted on in-loop code.
    int nsext = 0, nother = 0, nother_all = 0;
    for (int k = 0; k < nus && ok; k++) {
        if (us[k].kind == WU_GONE) continue;
        nother_all += us[k].kind == WU_OTHER;
        IRBlock *ub = use_block(us[k].u, us[k].slot);
        if (!L->in[ub->id]) continue;
        if (us[k].kind == WU_SEXT) nsext++;
        else if (us[k].kind == WU_OTHER)
            nother++;
    }
    if (!ok || nother >= nsext) {
        free(us);
        free(der);
        free(doff);
        free(drem);
        return false;
    }
    // Strength reduction: a pure index IV whose sign extensions only feed `mul C`.
    WUse *mu = NULL;
    int nmu = 0;
    int64_t C = 0;
    if (nother_all == 0 && nder == 0) C = sext_scale(f, us, nus, &mu, &nmu);

    // Commit. New values: sext32(init), K, the I64 phi w and its increment w2 (before q).
    // With a scale C (strength reduction) w stands for C * sext32(p) instead: it starts at
    // C * init, steps by C * K, and is compared against C * bound.
    int64_t sc = C ? C : 1;
    IRInstr *ii = V(f, init), *w0;
    if (ii->op == IR_ICONST) {
        w0 = new_iconst(f, IRT_I64, ii->imm * sc);
    } else {
        w0 = new_instr(f, IR_SEXT32, IRT_I64, init, IR_NONE);
        if (sc != 1) {
            put_before_term(P, w0);
            IRInstr *c64 = new_iconst(f, IRT_I64, sc);
            put_before_term(P, c64);
            w0 = new_instr(f, IR_MUL, IRT_I64, w0->id, c64->id);
        }
    }
    put_before_term(P, w0);
    IRInstr *k64 = new_iconst(f, IRT_I64, K * sc);
    put_before_term(P, k64);
    IRInstr *w = ir_instr_new(f, IR_PHI, IRT_I64);
    w->aux = IR_NONE;
    w->nargs = H->npreds;
    w->args = xcalloc((size_t)H->npreds, sizeof(int32_t));
    IRInstr *w2 = new_instr(f, IR_ADD, IRT_I64, w->id, k64->id);
    put_before(qi->blk, qi, w2);
    for (int k = 0; k < H->npreds; k++) w->args[k] = k == pi ? w0->id : w2->id;
    ir_block_add_phi(H, w);
    // Sign-extended (scaled) copies of loop-invariant compare operands, one per distinct operand.
    int *yv = NULL, *y64 = NULL, ny = 0;
    for (int k = 0; k < nus; k++) {
        WUse *x = &us[k];
        IRInstr *u = x->u;
        IRBlock *ublk = u->blk; // kill() unlinks u
        int wide = x->src == p->id ? w->id : w2->id;
        switch (x->kind) {
        case WU_SEXT:
            if (x->der) { // sext32(p + c) -> w + c
                IRInstr *c64 = new_iconst(f, IRT_I64, x->off);
                put_before_term(P, c64);
                u->op = IR_ADD;
                u->a = w->id;
                u->b = c64->id;
            } else if (!C) {
                fwd_set(f, u->id, wide);
                kill(u);
            }
            break;
        case WU_CMP: {
            int oslot = x->slot == 0 ? 1 : 0, o = *use_slot(u, oslot), m = -1;
            for (int j = 0; j < ny; j++)
                if (yv[j] == o) m = y64[j];
            if (m < 0) {
                IRInstr *oi = V(f, o), *n;
                if (oi->op == IR_ICONST) {
                    n = new_iconst(f, IRT_I64, oi->imm * sc);
                    put_before_term(P, n);
                } else {
                    n = new_instr(f, IR_SEXT32, IRT_I64, o, IR_NONE);
                    put_before_term(P, n);
                    if (sc != 1) {
                        IRInstr *c64 = new_iconst(f, IRT_I64, sc);
                        put_before_term(P, c64);
                        n = new_instr(f, IR_MUL, IRT_I64, n->id, c64->id);
                        put_before_term(P, n);
                    }
                }
                yv = realloc(yv, sizeof(*yv) * (size_t)(ny + 1));
                y64 = realloc(y64, sizeof(*y64) * (size_t)(ny + 1));
                yv[ny] = o;
                y64[ny++] = m = n->id;
            }
            *use_slot(u, x->slot) = wide;
            *use_slot(u, oslot) = m;
            break;
        }
        case WU_OTHER: {
            IRInstr *t = new_instr(f, IR_TRUNC, IRT_I32, wide, IR_NONE);
            IRBlock *ub = use_block(u, x->slot);
            if (u->op == IR_PHI) put_before_term(ub, t);
            else
                put_before(u->blk, u, t);
            *use_slot(u, x->slot) = t->id;
            break;
        }
        default:
            break;
        }
        if (x->kind == WU_SEXT && !x->der && !C) touched[ublk->id] = 1;
    }
    for (int k = 0; k < nmu; k++) { // the C * sext32 products are the scaled IV itself
        IRBlock *mb = mu[k].u->blk;
        fwd_set(f, mu[k].u->id, mu[k].src == p->id ? w->id : w2->id);
        kill(mu[k].u);
        touched[mb->id] = 1;
    }
    free(mu);
    free(yv);
    free(y64);
    free(us);
    free(der);
    free(doff);
    free(drem);
    return true;
}

static bool iv_widen(IRFunc *f) {
    int nl;
    Loop *loops = find_loops(f, &nl);
    bool any = false;
    for (int li = 0; li < nl && !any; li++)
        for (int k = 0; k < loops[li].header->nphis && !any; k++) any = loops[li].header->phis[k]->type == IRT_I32;
    if (!any) {
        free_loops(loops, nl);
        return false;
    }
    ensure_preheaders(f, loops, nl);
    Dom d;
    dom_build(f, &d);
    fwd_reset(f);
    unsigned char *touched = xcalloc((size_t)f->nblocks, 1);
    bool changed = false;
    for (int li = 0; li < nl; li++) {
        Loop *L = &loops[li];
        int np = L->header->nphis;
        for (int k = 0; k < np; k++) {
            if (!widen_iv(f, &d, L, L->header->phis[k], touched)) continue;
            // Later candidates must not see operands that name the sign extensions just removed.
            fwd_apply(f);
            for (int bi = 0; bi < f->nblocks; bi++)
                if (touched[bi]) compact_block(f->blocks[bi]);
            memset(touched, 0, (size_t)f->nblocks);
            changed = true;
        }
    }
    free(touched);
    dom_free(&d);
    free_loops(loops, nl);
    return changed;
}

// ------------------------------------------------------------------ loop rotation
//
// `for (i = 0; i < n; i++) body` is built as header{phis; test} -> body -> latch ->
// header, so every iteration runs the header test plus a jump. Rotation copies the
// header's non-phi instrs into the preheader (header phis replaced by their incoming
// value from it), which then guards the loop (br test' ? body : exit), and leaves
// the old header as the loop's bottom test: the body block becomes the loop header.
// A value defined in the old header and used elsewhere now has two definitions, the
// guard's copy and the bottom test's; SSA is restored with phis placed by on-demand
// backward reads (Braun et al.): at the body entry (guard edge vs bottom edge) and
// wherever else the two paths meet. The result is also what lets hoist_loads() move
// loads into the guarded preheader. A loop whose latch already exits is considered
// rotated, which makes the pass idempotent.

#define ROT_MAX_INSTRS 8
#define ROT_MAX_LOOPS 64

static bool rot_cloneable(int op) {
    switch (op) {
    case IR_ICONST:
    case IR_FCONST:
    case IR_FRAMEADDR:
    case IR_GADDR:
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_AND:
    case IR_OR:
    case IR_XOR:
    case IR_SHL:
    case IR_SHR:
    case IR_SAR:
    case IR_NEG:
    case IR_NOT:
    case IR_FADD:
    case IR_FSUB:
    case IR_FMUL:
    case IR_FDIV:
    case IR_FNEG:
    case IR_CMP:
    case IR_FCMP:
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
    case IR_LOAD:
        return true;
    default:
        return false;
    }
}

typedef struct {
    IRFunc *f;
    IRBlock *H, *P;
    int *map; // old header value -> its definition at the end of the guard block P
} Rot;

static int rot_end(Rot *r, int v, int *memo, IRBlock *y);

// Definition of the old-header value v that reaches the start of block u.
static int rot_start(Rot *r, int v, int *memo, IRBlock *u) {
    if (memo[u->id] != -2) return memo[u->id];
    if (u->npreds < 2) { // a cycle needs a join, which memoizes its phi before recursing
        int x = u->npreds ? rot_end(r, v, memo, u->preds[0]) : v;
        memo[u->id] = x;
        return x;
    }
    IRFunc *f = r->f;
    IRInstr *phi = ir_instr_new(f, IR_PHI, (IRType)V(f, v)->type);
    phi->aux = IR_NONE;
    phi->nargs = u->npreds;
    phi->args = xcalloc((size_t)u->npreds, sizeof(int32_t));
    ir_block_add_phi(u, phi);
    memo[u->id] = phi->id;
    for (int k = 0; k < u->npreds; k++) phi->args[k] = rot_end(r, v, memo, u->preds[k]);
    return phi->id;
}

static int rot_end(Rot *r, int v, int *memo, IRBlock *y) {
    if (y == r->H) return v;
    if (y == r->P) return r->map[v];
    return rot_start(r, v, memo, y);
}

// Rotates L when it has the header-tested shape; true if the CFG changed (including
// the creation of a preheader), after which the caller must rediscover loops.
static bool rotate_loop(IRFunc *f, Loop *L) {
    IRBlock *H = L->header;
    IRInstr *term = H->ins[H->nins - 1];
    if (term->op != IR_BR || term->t == term->f) return false;
    bool tin = L->in[term->t->id], fin = L->in[term->f->id];
    if (tin == fin) return false;
    IRBlock *B = tin ? term->t : term->f, *X = tin ? term->f : term->t;
    if (B == H || B->npreds != 1) return false;
    for (int j = 0; j < L->nlatches; j++) { // a latch that exits: already bottom-tested
        IRBlock *s[2];
        int ns = ir_succs(L->latches[j], s);
        for (int k = 0; k < ns; k++)
            if (!L->in[s[k]->id]) return false;
    }
    if (H->nins - 1 > ROT_MAX_INSTRS || H->nphis > 24) return false;
    for (int k = 0; k < H->nins - 1; k++)
        if (!rot_cloneable(H->ins[k]->op)) return false;
    bool created;
    IRBlock *P = loop_pre(f, L, &created);
    if (X == P) return created;
    int nb = f->nblocks, nv0 = f->nvals;
    int pi = pred_index(H, P);
    Rot r = {f, H, P, xcalloc((size_t)nv0, sizeof(int))};
    for (int i = 0; i < nv0; i++) r.map[i] = i;

    // Uses of old-header values elsewhere (including the latch edges of the header phis).
    Use *us = NULL;
    int nus = 0, cus = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *u = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            if (b == H && u->op != IR_PHI) continue;
            int nsl = 3 + u->nargs;
            for (int slot = 0; slot < nsl; slot++) {
                int v = *use_slot(u, slot);
                if (v == IR_NONE || V(f, v)->blk != H) continue;
                if (b == H && slot - 3 == pi) continue;
                use_add(&us, &nus, &cus, u, slot);
            }
        }
    }

    // The guard: a copy of the header's instrs in P, ending in the header's branch.
    for (int k = 0; k < H->nphis; k++) r.map[H->phis[k]->id] = H->phis[k]->args[pi];
    for (int k = 0; k < H->nins - 1; k++) {
        IRInstr *o = H->ins[k];
        IRInstr *n = ir_instr_new(f, (IROp)o->op, (IRType)o->type);
        n->aux = IR_NONE;
        n->cc = o->cc;
        n->mw = o->mw;
        n->msign = o->msign;
        n->scale = o->scale;
        n->imm = o->imm;
        n->fimm = o->fimm;
        n->sym = o->sym;
        n->line = o->line;
        n->a = o->a == IR_NONE ? IR_NONE : r.map[o->a];
        n->b = o->b == IR_NONE ? IR_NONE : r.map[o->b];
        n->c = o->c == IR_NONE ? IR_NONE : r.map[o->c];
        r.map[o->id] = n->id;
        put_before_term(P, n);
    }
    IRInstr *g = P->ins[P->nins - 1];
    g->op = IR_BR;
    g->a = r.map[term->a];
    g->t = term->t;
    g->f = term->f;
    int bi0 = pred_index(B, H), xi0 = pred_index(X, H);
    ir_add_pred(B, P);
    for (int k = 0; k < B->nphis; k++) phi_add_arg(B->phis[k], r.map[B->phis[k]->args[bi0]]);
    ir_add_pred(X, P);
    for (int k = 0; k < X->nphis; k++) phi_add_arg(X->phis[k], r.map[X->phis[k]->args[xi0]]);

    // Re-establish SSA for the recorded uses.
    int *vl = NULL, **memo = NULL, nvl = 0;
    for (int k = 0; k < nus; k++) {
        IRInstr *u = us[k].u;
        int v = *use_slot(u, us[k].slot), j = 0;
        while (j < nvl && vl[j] != v) j++;
        if (j == nvl) {
            vl = realloc(vl, sizeof(*vl) * (size_t)(nvl + 1));
            memo = realloc(memo, sizeof(*memo) * (size_t)(nvl + 1));
            memo[nvl] = xcalloc((size_t)nb, sizeof(int));
            for (int i = 0; i < nb; i++) memo[nvl][i] = -2;
            vl[nvl++] = v;
        }
        *use_slot(u, us[k].slot) = u->op == IR_PHI ? rot_end(&r, v, memo[j], u->blk->preds[us[k].slot - 3])
                                                   : rot_start(&r, v, memo[j], u->blk);
    }
    ir_remove_pred_edge(H, P);
    for (int j = 0; j < nvl; j++) free(memo[j]);
    free(memo);
    free(vl);
    free(us);
    free(r.map);
    return true;
}

static bool loop_rotate(IRFunc *f) {
    bool any = false;
    for (int n = 0; n < ROT_MAX_LOOPS; n++) {
        int nl;
        Loop *loops = find_loops(f, &nl);
        bool done = false;
        for (int li = 0; li < nl && !done; li++) done = rotate_loop(f, &loops[li]);
        free_loops(loops, nl);
        if (!done) break;
        any = true;
    }
    return any;
}

// ------------------------------------------------------------------ load hoisting
//
// A load whose address is loop-invariant is hoisted into the preheader when the value
// it reads cannot change while the loop runs and the hoisted load cannot fault where
// the original would not have:
//  - the loop has no call (a callee may write anything, or never return) and no
//    divide that can trap (a fault the load would then precede);
//  - no store in the loop may alias it (acc_alias, the facts memopt uses);
//  - the load's block dominates every exit of the loop, so some iteration executes it
//    before control leaves, and the preheader runs only when the loop is entered.
// Address computations the load needs and that the loop only contains (pure,
// invariant operands) move along; they are what licm() leaves behind as free_at_use.

static bool inv_ok(IRFunc *f, const Loop *L, const unsigned char *hoist, int v, int depth) {
    IRInstr *o = V(f, v);
    if (!L->in[o->blk->id] || hoist[v] || is_leaf(o)) return true;
    if (depth == 0 || !hoistable(f, o)) return false;
    int32_t ops[3] = {o->a, o->b, o->c};
    for (int j = 0; j < 3; j++)
        if (ops[j] != IR_NONE && !inv_ok(f, L, hoist, ops[j], depth - 1)) return false;
    return true;
}

static void inv_mark(IRFunc *f, const Loop *L, unsigned char *hoist, int *sel, int *nsel, int v) {
    IRInstr *o = V(f, v);
    if (!L->in[o->blk->id] || hoist[v]) return;
    int32_t ops[3] = {o->a, o->b, o->c};
    for (int j = 0; j < 3; j++)
        if (ops[j] != IR_NONE) inv_mark(f, L, hoist, sel, nsel, ops[j]);
    hoist[v] = 1;
    sel[(*nsel)++] = v;
}

#define HOIST_MAX_STORES 64

static bool hoist_loads(IRFunc *f) {
    bool anyload = false;
    for (int bi = 0; bi < f->nblocks && !anyload; bi++)
        for (int k = 0; k < f->blocks[bi]->nins && !anyload; k++) anyload = f->blocks[bi]->ins[k]->op == IR_LOAD;
    if (!anyload) return false;
    int nl;
    Loop *loops = find_loops(f, &nl);
    if (!nl) {
        free_loops(loops, 0);
        return false;
    }
    ensure_preheaders(f, loops, nl);
    fwd_reset(f);
    Mem m;
    mem_init(&m, f);
    Dom d;
    dom_build(f, &d);
    int nb = f->nblocks;
    IRBlock **rpo = xcalloc((size_t)nb, sizeof(*rpo)), **ex = xcalloc((size_t)nb, sizeof(*ex));
    int nrpo = ir_rpo(f, rpo);
    unsigned char *hoist = xcalloc((size_t)f->nvals, 1);
    int *sel = xcalloc((size_t)f->nvals, sizeof(int));
    Acc st[HOIST_MAX_STORES];
    bool changed = false;
    for (int li = 0; li < nl; li++) {
        Loop *L = &loops[li];
        int nst = 0, nex = 0;
        bool bad = false;
        for (int bi = 0; bi < nrpo && !bad; bi++) {
            IRBlock *b = rpo[bi];
            if (!L->in[b->id]) continue;
            for (int k = 0; k < b->nins && !bad; k++) {
                IRInstr *i = b->ins[k];
                if (i->op == IR_CALL || ((i->op == IR_SDIV || i->op == IR_UDIV || i->op == IR_SREM || i->op == IR_UREM) && !div_safe(f, i)))
                    bad = true;
                else if (i->op == IR_STORE) {
                    if (nst == HOIST_MAX_STORES) bad = true;
                    else
                        acc_make(&m, i, &st[nst++]);
                }
            }
            IRBlock *s[2];
            int ns = ir_succs(b, s);
            bool exits = ns == 0;
            for (int k = 0; k < ns; k++)
                if (!L->in[s[k]->id]) exits = true;
            if (exits) ex[nex++] = b;
        }
        if (bad || !nex) continue;
        int nsel = 0;
        for (int bi = 0; bi < nrpo; bi++) {
            IRBlock *b = rpo[bi];
            if (!L->in[b->id]) continue;
            bool all = true;
            for (int e = 0; e < nex && all; e++) all = dominates(&d, b->id, ex[e]->id);
            if (!all) continue;
            for (int k = 0; k < b->nins; k++) {
                IRInstr *i = b->ins[k];
                if (i->op != IR_LOAD || hoist[i->id]) continue;
                if (!inv_ok(f, L, hoist, i->a, 3) || (i->b != IR_NONE && !inv_ok(f, L, hoist, i->b, 3))) continue;
                Acc a;
                acc_make(&m, i, &a);
                bool alias = false;
                for (int s = 0; s < nst && !alias; s++) alias = acc_alias(&m, &a, &st[s]);
                if (alias) continue;
                inv_mark(f, L, hoist, sel, &nsel, i->a);
                if (i->b != IR_NONE) inv_mark(f, L, hoist, sel, &nsel, i->b);
                hoist[i->id] = 1;
                sel[nsel++] = i->id;
            }
        }
        if (!nsel) continue;
        IRBlock *pre = L->pre;
        for (int bi = 0; bi < nrpo; bi++) {
            IRBlock *b = rpo[bi];
            if (!L->in[b->id]) continue;
            int w = 0;
            for (int k = 0; k < b->nins; k++) {
                IRInstr *i = b->ins[k];
                if (hoist[i->id]) put_before_term(pre, i);
                else
                    b->ins[w++] = i;
            }
            b->nins = w;
        }
        for (int k = 0; k < nsel; k++) hoist[sel[k]] = 0;
        changed = true;
    }
    free(rpo);
    free(ex);
    free(hoist);
    free(sel);
    mem_free(&m);
    dom_free(&d);
    free_loops(loops, nl);
    return changed;
}

// ---------------------------------------------------------------------- driver

// The scalar fixpoint: simplify / CFG cleanup / DCE / GVN / memopt until nothing changes.
static void scalar_opts(IRFunc *f) {
    for (int round = 0; round < OPT_ROUNDS; round++) {
        bool ch = simplify(f);
        ch |= cfg_cleanup(f);
        ch |= dce(f);
        ch |= gvn(f);
        ch |= fold_known_branches(f);
        ch |= memopt(f);
        ch |= sroa(f);
        if (!ch) break;
    }
    dce(f);
}

void ir_opt(IRFunc *f) {
    if (f->nblocks == 0) return;
    cfg_cleanup(f);
    scalar_opts(f);
    // Loop shape. Rotation first: its guard is a range fact for iv_widen, and its
    // result (trivial phis, a mergeable bottom test) must be cleaned up before the
    // induction variables can be recognized. The passes below create preheaders that
    // the CFG cleanup would dissolve again, so they run last.
    if (loop_rotate(f)) scalar_opts(f);
    if (iv_widen(f)) scalar_opts(f);
    // Hoisting can land equal expressions from sibling loops in one preheader.
    bool hoisted = licm(f);
    if (hoist_loads(f)) { // loads in the preheaders make more pure instrs invariant
        licm(f);
        hoisted = true;
    }
    if (hoisted) gvn(f);
    ir_analyze_loops(f);
}
