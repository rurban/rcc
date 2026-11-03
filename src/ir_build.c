// ir_build.c -- AST -> SSA IR lowering.
//
// SSA is built on the fly (Braun et al., "Simple and Efficient Construction of
// Static Single Assignment Form"): scalar locals whose address is never taken
// are SSA variables; everything else (arrays, structs, address-taken scalars)
// lives in its frame slot and is accessed through IR_FRAMEADDR + LOAD/STORE.
//
// ir_build() is all-or-nothing: any construct it does not model exactly makes
// it return NULL and the function is compiled by the AST codegen instead.
//
// Invariant: an IR_I32 value of C type char/short/_Bool is always normalized
// (sign/zero-extended within its 32 bits per its C signedness).
#include "ir.h"
#include "cg_ir.h"

typedef struct {
    IRType t; // IRT_I32 / I64 / F32 / F64
    int size; // C size in bytes (1, 2, 4, 8)
    bool uns; // unsigned integer (pointers count as unsigned)
    bool flt;
    bool ptr;
    bool boolean;
} Sc;

// A (variable, phi) pair waiting for its block to be sealed.
typedef struct {
    int n, cap;
    int *var, *phi;
} Pending;

typedef struct {
    LVar *var;
    int ssa; // SSA variable index, or -1 for a memory/global variable
    Sc sc; // valid when ssa >= 0
} LvInfo;

typedef struct {
    int fwd; // value this trivial phi was replaced by, or -1
    int n, cap;
    int *users; // phis that use this phi as an operand
} PhiAux;

typedef struct {
    Program *prog;
    Function *fn;
    IRFunc *f;
    bool fail;
    const char *why;
    // LVar -> LvInfo (open addressing)
    LvInfo *tab;
    int tabcap, tabn;
    LVar **marked;
    int nmarked, capmarked;
    // SSA variables
    IRType *ssa_type;
    int nssa, capssa;
    int *undef_val; // per SSA variable cached UNDEF value or -1
    int **cur; // cur[block id][ssa var] = value id + 1 (0 = unset)
    int *curn; // allocated length of cur[block id]
    int curcap;
    PhiAux *aux; // indexed by value id
    int auxcap;
    IRBlock *entry;
    IRBlock *cur_blk; // insertion block, NULL after a terminator
    IRBlock **brk, **cont; // loop target stacks
    int depth, capdepth;
} B;

// ------------------------------------------------------------------ helpers

static void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n ? n : 1, sz ? sz : 1);
    if (!p) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    return p;
}
static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    return q;
}

static int fail(B *b, const char *why) {
    if (!b->fail) {
        b->fail = true;
        b->why = why;
    }
    return IR_NONE;
}

// fail() with the AST node kind in the message (diagnostics for -fdump-ir).
static int fail_kind(B *b, const char *why, NodeKind kind) {
    static char buf[96];
    if (!b->fail) snprintf(buf, sizeof buf, "%s: %s", why, cg_ir_node_kind_name((int)kind));
    return fail(b, buf);
}

static bool sc_of(Type *ty, Sc *s) {
    if (!ty) return false;
    memset(s, 0, sizeof *s);
    switch (ty->kind) {
    case TY_BOOL:
        *s = (Sc){IRT_I32, 1, true, false, false, true};
        return true;
    case TY_CHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_LONG:
    case TY_LLONG: {
        int sz = (int)ty->size;
        if (sz != 1 && sz != 2 && sz != 4 && sz != 8) return false;
        *s = (Sc){sz == 8 ? IRT_I64 : IRT_I32, sz, ty->is_unsigned, false, false, false};
        return true;
    }
    case TY_PTR:
    case TY_ARRAY:
    case TY_FUNC:
        *s = (Sc){IRT_I64, 8, true, false, true, false};
        return true;
    case TY_FLOAT:
        *s = (Sc){IRT_F32, 4, false, true, false, false};
        return true;
    case TY_DOUBLE:
        *s = (Sc){IRT_F64, 8, false, true, false, false};
        return true;
    default:
        return false;
    }
}

// -------------------------------------------------------- per-id side tables

static PhiAux *aux_of(B *b, int id) {
    if (id >= b->auxcap) {
        int nc = b->auxcap ? b->auxcap : 64;
        while (nc <= id) nc *= 2;
        b->aux = xrealloc(b->aux, sizeof(PhiAux) * (size_t)nc);
        for (int i = b->auxcap; i < nc; i++) b->aux[i] = (PhiAux){-1, 0, 0, NULL};
        b->auxcap = nc;
    }
    return &b->aux[id];
}

static int resolve(B *b, int id) {
    if (id < 0) return id;
    int r = id;
    while (r < b->auxcap && b->aux[r].fwd >= 0) r = b->aux[r].fwd;
    // path compression
    while (id < b->auxcap && b->aux[id].fwd >= 0 && b->aux[id].fwd != r) {
        int nx = b->aux[id].fwd;
        b->aux[id].fwd = r;
        id = nx;
    }
    return r;
}

// ----------------------------------------------------------- IR emission

static IRBlock *new_block(B *b) {
    IRBlock *blk = ir_block_new(b->f);
    if (blk->id > 30000) fail(b, "too many blocks");
    return blk;
}

static void start_block(B *b, IRBlock *blk) { b->cur_blk = blk; }

// The block new instructions go into. After a terminator (dead code) a fresh
// sealed, predecessor-less block is created so lowering stays uniform; ir_opt
// removes it.
static IRBlock *cur_block(B *b) {
    if (!b->cur_blk) {
        IRBlock *d = new_block(b);
        d->sealed = true;
        b->cur_blk = d;
    }
    return b->cur_blk;
}

static IRInstr *emit(B *b, IROp op, IRType t) {
    IRInstr *i = ir_instr_new(b->f, op, t);
    ir_block_append(cur_block(b), i);
    return i;
}

static int op0(B *b, IROp op, IRType t, int64_t imm) {
    IRInstr *i = emit(b, op, t);
    i->imm = imm;
    return i->id;
}
static int op1(B *b, IROp op, IRType t, int a) {
    IRInstr *i = emit(b, op, t);
    i->a = a;
    return i->id;
}
static int op2(B *b, IROp op, IRType t, int a, int c) {
    IRInstr *i = emit(b, op, t);
    i->a = a;
    i->b = c;
    return i->id;
}

static int iconst(B *b, IRType t, int64_t v) {
    if (t == IRT_I32) v = (int32_t)v;
    return op0(b, IR_ICONST, t, v);
}
static int fconst(B *b, IRType t, double v) {
    IRInstr *i = emit(b, IR_FCONST, t);
    i->fimm = t == IRT_F32 ? (double)(float)v : v;
    return i->id;
}
static int cmp_i(B *b, IRCC cc, int x, int y) {
    IRInstr *i = emit(b, IR_CMP, IRT_I32);
    i->a = x;
    i->b = y;
    i->cc = (uint8_t)cc;
    return i->id;
}
static int cmp_f(B *b, IRCC cc, int x, int y) {
    IRInstr *i = emit(b, IR_FCMP, IRT_I32);
    i->a = x;
    i->b = y;
    i->cc = (uint8_t)cc;
    return i->id;
}

static int zero_of(B *b, IRType t) {
    if (t == IRT_F32 || t == IRT_F64) return fconst(b, t, 0.0);
    return iconst(b, t, 0);
}

static int emit_load(B *b, IRMem mw, bool sign, IRType t, int addr) {
    IRInstr *i = emit(b, IR_LOAD, t);
    i->a = addr;
    i->mw = (uint8_t)mw;
    i->msign = sign;
    return i->id;
}
static void emit_store(B *b, IRMem mw, int addr, int val) {
    IRInstr *i = emit(b, IR_STORE, IRT_VOID);
    i->a = addr;
    i->c = val;
    i->mw = (uint8_t)mw;
}

// ------------------------------------------------------------- control flow

static void edge(IRBlock *from, IRBlock *to) { ir_add_pred(to, from); }

static void jump(B *b, IRBlock *to) {
    if (!b->cur_blk) return;
    IRInstr *i = emit(b, IR_JMP, IRT_VOID);
    i->t = to;
    edge(b->cur_blk, to);
    b->cur_blk = NULL;
}

static void branch(B *b, int cond, IRBlock *t, IRBlock *f) {
    if (!b->cur_blk) return;
    if (t == f) {
        jump(b, t);
        return;
    }
    IRInstr *i = emit(b, IR_BR, IRT_VOID);
    i->a = cond;
    i->t = t;
    i->f = f;
    edge(b->cur_blk, t);
    edge(b->cur_blk, f);
    b->cur_blk = NULL;
}

// ----------------------------------------------------------------- Braun SSA

static int *blk_cur(B *b, IRBlock *blk) {
    if (blk->id >= b->curcap) {
        int nc = b->curcap ? b->curcap : 32;
        while (nc <= blk->id) nc *= 2;
        b->cur = xrealloc(b->cur, sizeof(int *) * (size_t)nc);
        b->curn = xrealloc(b->curn, sizeof(int) * (size_t)nc);
        for (int i = b->curcap; i < nc; i++) {
            b->cur[i] = NULL;
            b->curn[i] = 0;
        }
        b->curcap = nc;
    }
    int id = blk->id;
    if (b->curn[id] < b->capssa) { // first use, or SSA variables were registered since
        b->cur[id] = xrealloc(b->cur[id], sizeof(int) * (size_t)b->capssa);
        memset(b->cur[id] + b->curn[id], 0, sizeof(int) * (size_t)(b->capssa - b->curn[id]));
        b->curn[id] = b->capssa;
    }
    return b->cur[id];
}

static void write_var(B *b, int var, IRBlock *blk, int val) { blk_cur(b, blk)[var] = val + 1; }

static int read_var(B *b, int var, IRBlock *blk);

static int undef_for(B *b, int var) {
    if (b->undef_val[var] < 0) {
        IRInstr *i = ir_instr_new(b->f, IR_UNDEF, b->ssa_type[var]);
        ir_block_prepend(b->entry, i);
        b->undef_val[var] = i->id;
    }
    return b->undef_val[var];
}

static void add_user(B *b, int phi, int user) {
    PhiAux *a = aux_of(b, phi);
    if (a->n == a->cap) {
        a->cap = a->cap ? a->cap * 2 : 4;
        a->users = xrealloc(a->users, sizeof(int) * (size_t)a->cap);
    }
    a->users[a->n++] = user;
}

// A phi is trivial when all its operands are the phi itself or one single other
// value; it is then replaced (lazily, via the forwarding table) by that value.
// Returns the value the phi stands for (the phi itself if it is a real merge).
static int try_remove_trivial(B *b, IRInstr *phi) {
    int same = IR_NONE;
    for (int k = 0; k < phi->nargs; k++) {
        if (phi->args[k] == IR_NONE) return phi->id; // operands still being gathered
        int a = resolve(b, phi->args[k]);
        if (a == phi->id || a == same) continue;
        if (same != IR_NONE) return phi->id; // merges two values: a real phi
        same = a;
    }
    if (same == IR_NONE) same = undef_for(b, phi->aux); // unreachable or uninitialized
    aux_of(b, phi->id)->fwd = same;
    // Phis that used this one may have become trivial. The users array and the
    // aux table can be reallocated by the recursion: re-fetch every iteration.
    for (int k = 0; k < aux_of(b, phi->id)->n; k++) {
        int u0 = aux_of(b, phi->id)->users[k];
        if (u0 == phi->id) continue; // compare before resolving: it now forwards elsewhere
        int u = resolve(b, u0);
        IRInstr *ui = b->f->vals[u];
        if (ui->op == IR_PHI && ui->blk && ui->aux >= 0 && ui->args && aux_of(b, u)->fwd < 0)
            try_remove_trivial(b, ui);
    }
    return same;
}

static int add_phi_operands(B *b, int var, IRInstr *phi) {
    IRBlock *blk = phi->blk;
    phi->nargs = blk->npreds;
    phi->args = xcalloc((size_t)blk->npreds, sizeof(int32_t));
    for (int k = 0; k < blk->npreds; k++) phi->args[k] = IR_NONE; // not yet filled
    for (int k = 0; k < blk->npreds; k++) {
        int v = read_var(b, var, blk->preds[k]);
        phi->args[k] = v;
        if (b->f->vals[v]->op == IR_PHI) add_user(b, v, phi->id);
    }
    return try_remove_trivial(b, phi);
}

static IRInstr *new_var_phi(B *b, int var, IRBlock *blk) {
    IRInstr *p = ir_instr_new(b->f, IR_PHI, b->ssa_type[var]);
    p->aux = var;
    ir_block_add_phi(blk, p);
    return p;
}

static void pending_add(IRBlock *blk, int var, int phi) {
    Pending *p = blk->auxp;
    if (!p) p = blk->auxp = xcalloc(1, sizeof *p);
    if (p->n == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 4;
        p->var = xrealloc(p->var, sizeof(int) * (size_t)p->cap);
        p->phi = xrealloc(p->phi, sizeof(int) * (size_t)p->cap);
    }
    p->var[p->n] = var;
    p->phi[p->n++] = phi;
}

static int read_var_recursive(B *b, int var, IRBlock *blk) {
    int val;
    if (!blk->sealed) {
        IRInstr *p = new_var_phi(b, var, blk);
        pending_add(blk, var, p->id);
        val = p->id;
    } else if (blk->npreds == 1) {
        val = read_var(b, var, blk->preds[0]);
    } else if (blk->npreds == 0) {
        val = undef_for(b, var);
    } else {
        IRInstr *p = new_var_phi(b, var, blk);
        write_var(b, var, blk, p->id); // break cycles
        val = add_phi_operands(b, var, p);
    }
    write_var(b, var, blk, val);
    return val;
}

static int read_var(B *b, int var, IRBlock *blk) {
    int *c = blk_cur(b, blk);
    if (c[var]) {
        int r = resolve(b, c[var] - 1);
        c[var] = r + 1;
        return r;
    }
    return resolve(b, read_var_recursive(b, var, blk));
}

static void seal_block(B *b, IRBlock *blk) {
    if (blk->sealed) return;
    Pending *p = blk->auxp;
    blk->sealed = true;
    if (p) {
        for (int k = 0; k < p->n; k++) add_phi_operands(b, p->var[k], b->f->vals[p->phi[k]]);
        free(p->var);
        free(p->phi);
        free(p);
        blk->auxp = NULL;
    }
}

// Manual phi for ?: and &&/|| values: arguments are appended in the order the
// incoming edges were created.
static void phi_add(B *b, IRInstr *phi, int val) {
    (void)b;
    phi->args = xrealloc(phi->args, sizeof(int32_t) * (size_t)(phi->nargs + 1));
    phi->args[phi->nargs++] = val;
}

// ------------------------------------------------------------------ LVars

static bool is_marked(B *b, LVar *v) {
    for (int i = 0; i < b->nmarked; i++)
        if (b->marked[i] == v) return true;
    return false;
}
static void mark_addr_taken(B *b, LVar *v) {
    if (is_marked(b, v)) return;
    if (b->nmarked == b->capmarked) {
        b->capmarked = b->capmarked ? b->capmarked * 2 : 16;
        b->marked = xrealloc(b->marked, sizeof(LVar *) * (size_t)b->capmarked);
    }
    b->marked[b->nmarked++] = v;
}

static LvInfo *lv_find(B *b, LVar *v, bool create) {
    if (b->tabn * 2 >= b->tabcap) {
        int nc = b->tabcap ? b->tabcap * 2 : 64;
        LvInfo *nt = xcalloc((size_t)nc, sizeof(LvInfo));
        for (int i = 0; i < b->tabcap; i++)
            if (b->tab[i].var) {
                unsigned h = (unsigned)(((uintptr_t)b->tab[i].var >> 4) * 2654435761u) & (unsigned)(nc - 1);
                while (nt[h].var) h = (h + 1) & (unsigned)(nc - 1);
                nt[h] = b->tab[i];
            }
        free(b->tab);
        b->tab = nt;
        b->tabcap = nc;
    }
    unsigned h = (unsigned)(((uintptr_t)v >> 4) * 2654435761u) & (unsigned)(b->tabcap - 1);
    while (b->tab[h].var) {
        if (b->tab[h].var == v) return &b->tab[h];
        h = (h + 1) & (unsigned)(b->tabcap - 1);
    }
    if (!create) return NULL;
    b->tabn++;
    b->tab[h].var = v;
    b->tab[h].ssa = -1;
    return &b->tab[h];
}

static bool lv_is_global(LVar *v) { return !v->is_local || v->is_static || v->is_extern || v->is_function; }

// Classify a local once: SSA variable or memory slot.
static LvInfo *lv_info(B *b, LVar *v) {
    LvInfo *e = lv_find(b, v, false);
    if (e) return e;
    e = lv_find(b, v, true);
    if (lv_is_global(v)) return e;
    Sc sc;
    if (!v->addr_taken && !is_marked(b, v) && !ty_volatile(v->ty) && !v->is_global_reg && !v->asm_name &&
        !v->section_name && !v->alias_target && sc_of(v->ty, &sc) && v->ty->kind != TY_ARRAY && v->ty->kind != TY_FUNC) {
        if (b->nssa == b->capssa) { // grow the variable tables; per-block arrays follow lazily
            int nc = b->capssa * 2;
            b->ssa_type = xrealloc(b->ssa_type, sizeof(IRType) * (size_t)nc);
            b->undef_val = xrealloc(b->undef_val, sizeof(int) * (size_t)nc);
            b->capssa = nc;
        }
        e->ssa = b->nssa;
        e->sc = sc;
        b->ssa_type[b->nssa] = sc.t;
        b->undef_val[b->nssa] = IR_NONE;
        b->nssa++;
    }
    return e;
}

// ---------------------------------------------------------------- prescan

static void scan(B *b, Node *n);
static void scan_chain(B *b, Node *n) {
    for (; n; n = n->next) scan(b, n);
}

// `x` is evaluated for its address: locals on the address spine live in memory.
static void mark_spine(B *b, Node *x) {
    for (; x;) {
        switch (x->kind) {
        case ND_LVAR:
            if (x->var) mark_addr_taken(b, x->var);
            return;
        case ND_MEMBER:
        case ND_CAST:
            x = x->lhs;
            break;
        case ND_COMMA:
            x = x->rhs;
            break;
        case ND_COND:
            mark_spine(b, x->then);
            x = x->els;
            break;
        default:
            return;
        }
    }
}

static void scan(B *b, Node *n) {
    if (!n || b->fail) return;
    switch (n->kind) {
    case ND_NUM:
    case ND_FNUM:
    case ND_STR:
    case ND_NULL:
    case ND_BREAK:
    case ND_CONTINUE:
        return;
    case ND_LVAR:
        if (n->chain_depth) fail(b, "nested-function variable");
        return;
    case ND_ADDR:
        mark_spine(b, n->lhs);
        scan(b, n->lhs);
        return;
    case ND_MEMBER:
        if (n->member && (n->member->bit_width || n->member->offset_expr)) {
            fail(b, "bitfield/variable member");
            return;
        }
        scan(b, n->lhs);
        return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_SHL:
    case ND_SHR:
    case ND_BITAND:
    case ND_BITXOR:
    case ND_BITOR:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_ASSIGN:
    case ND_LOGAND:
    case ND_LOGOR:
    case ND_COMMA:
        scan(b, n->lhs);
        scan(b, n->rhs);
        return;
    case ND_POST_INC:
    case ND_POST_DEC:
    case ND_PRE_INC:
    case ND_PRE_DEC:
    case ND_DEREF:
    case ND_CAST:
    case ND_BITNOT:
    case ND_NEG:
    case ND_NOT:
    case ND_EXPR_STMT:
        scan(b, n->lhs);
        return;
    case ND_FUNCALL:
        scan(b, n->lhs);
        scan_chain(b, n->args);
        return;
    case ND_ZERO_INIT:
        scan(b, n->lhs);
        return;
    case ND_RETURN:
        scan(b, n->lhs);
        return;
    case ND_COND:
        scan(b, n->cond);
        scan(b, n->then);
        scan(b, n->els);
        return;
    case ND_IF:
        scan_chain(b, n->init);
        scan(b, n->cond);
        scan(b, n->then);
        scan(b, n->els);
        return;
    case ND_FOR:
        if (n->target_loop || n->label_name) fail(b, "labeled loop");
        scan_chain(b, n->init);
        scan(b, n->cond);
        scan(b, n->then);
        scan(b, n->inc);
        return;
    case ND_DO:
        if (n->label_name) fail(b, "labeled loop");
        scan(b, n->then);
        scan(b, n->cond);
        return;
    case ND_BLOCK:
        scan_chain(b, n->body);
        return;
    default:
        fail_kind(b, "unsupported node", n->kind);
        return;
    }
}

// --------------------------------------------------------------- conversions

static int norm(B *b, int v, Sc s) {
    if (s.t != IRT_I32 || s.size >= 4) return v;
    IROp op = s.size == 1 ? (s.uns ? IR_ZEXT8 : IR_SEXT8) : (s.uns ? IR_ZEXT16 : IR_SEXT16);
    return op1(b, op, IRT_I32, v);
}

// 0/1 truth value of `v` (type from `s`) as I32.
static int to_bool01(B *b, int v, Sc s) {
    IRInstr *d = b->f->vals[v];
    if (d->op == IR_CMP || d->op == IR_FCMP) return v;
    if (s.flt) return cmp_f(b, IRCC_FNE, v, fconst(b, s.t, 0.0));
    return cmp_i(b, IRCC_NE, v, iconst(b, s.t, 0));
}

static int convert(B *b, int v, Sc from, Sc to) {
    if (v == IR_NONE) return v;
    if (to.boolean) return from.boolean ? v : to_bool01(b, v, from);
    if (from.flt && to.flt) {
        if (from.t == to.t) return v;
        return op1(b, from.t == IRT_F32 ? IR_FEXT : IR_FTRUNC, to.t, v);
    }
    if (from.flt) { // float -> integer/pointer
        if (to.size == 8) return op1(b, to.uns ? IR_FTOU64 : IR_FTOSI, IRT_I64, v);
        if (to.size == 4 && to.uns) return op1(b, IR_TRUNC, IRT_I32, op1(b, IR_FTOSI, IRT_I64, v));
        return norm(b, op1(b, IR_FTOSI, IRT_I32, v), to);
    }
    if (to.flt) { // integer -> float
        if (from.t == IRT_I64) return op1(b, from.uns ? IR_UITOF : IR_SITOF, to.t, v);
        if (from.uns && from.size == 4) v = op1(b, IR_ZEXT32, IRT_I64, v);
        return op1(b, IR_SITOF, to.t, v);
    }
    // integer/pointer -> integer/pointer
    if (to.size == 8) {
        if (from.t == IRT_I64) return v;
        return op1(b, from.uns ? IR_ZEXT32 : IR_SEXT32, IRT_I64, v);
    }
    if (from.t == IRT_I64) v = op1(b, IR_TRUNC, IRT_I32, v);
    if (to.size == 4) return v;
    // narrow target: skip the normalization only when it is value-preserving
    bool same = from.t == IRT_I32 &&
        (from.size < to.size ? (from.uns || !to.uns) : (from.size == to.size && from.uns == to.uns));
    return same ? v : norm(b, v, to);
}

// ---------------------------------------------------------------- expressions

static int gen_expr(B *b, Node *n);
static int address_of(B *b, Node *n);
static void cond_br(B *b, Node *n, IRBlock *t, IRBlock *f);
static void gen_zero_init(B *b, Node *n);

static int gen_as(B *b, Node *n, Sc to) {
    Sc from;
    if (!sc_of(n->ty, &from)) return fail(b, "operand type");
    int v = gen_expr(b, n);
    if (b->fail) return IR_NONE;
    return convert(b, v, from, to);
}

static IRMem mem_of(Sc s) {
    if (s.flt) return s.t == IRT_F32 ? IRM_F32 : IRM_F64;
    switch (s.size) {
    case 1: return IRM_I8;
    case 2: return IRM_I16;
    case 4: return IRM_I32;
    default: return IRM_I64;
    }
}

static int load_scalar(B *b, int addr, Type *ty) {
    Sc s;
    if (!sc_of(ty, &s)) return fail(b, "load type");
    if (ty_volatile(ty)) return fail(b, "volatile access"); // ir_opt may drop or merge plain loads
    return emit_load(b, mem_of(s), !s.uns && !s.flt, s.t, addr);
}

static bool is_aggregate_value(Type *ty) {
    return ty->kind == TY_ARRAY || ty->kind == TY_FUNC;
}

static int gaddr(B *b, const char *sym, int64_t add) {
    IRInstr *i = emit(b, IR_GADDR, IRT_I64);
    i->sym = sym;
    i->imm = add;
    return i->id;
}

static int var_address(B *b, LVar *v) {
    if (lv_is_global(v)) {
        if (!v->name && !v->asm_name) return fail(b, "anonymous global");
        int mode = cg_ir_global_access(v);
        if (mode < 0) return fail(b, "register/TLS/alias global");
        int a = gaddr(b, cg_ir_var_sym(v), 0);
        if (mode == 1) b->f->vals[a]->msign = 1; // address comes from the GOT
        return a;
    }
    return op0(b, IR_FRAMEADDR, IRT_I64, v->offset);
}

static int address_of(B *b, Node *n) {
    switch (n->kind) {
    case ND_LVAR:
        if (!n->var) return fail(b, "no var");
        if (n->var->is_local && !lv_is_global(n->var) && lv_info(b, n->var)->ssa >= 0) return fail(b, "address of ssa var");
        return var_address(b, n->var);
    case ND_DEREF: {
        Sc pc = {IRT_I64, 8, true, false, true, false};
        return gen_as(b, n->lhs, pc);
    }
    case ND_MEMBER: {
        int base = address_of(b, n->lhs);
        if (b->fail) return IR_NONE;
        if (n->member->offset == 0) return base;
        return op2(b, IR_ADD, IRT_I64, base, iconst(b, IRT_I64, n->member->offset));
    }
    case ND_COMMA:
        gen_expr(b, n->lhs);
        return address_of(b, n->rhs);
    case ND_STR:
        return gaddr(b, format(".LC%d", n->str_id), 0);
    default:
        return fail_kind(b, "unsupported lvalue", n->kind);
    }
}

// Value of an lvalue-typed node (array/function designators decay to addresses).
static int value_from_lvalue(B *b, Node *n) {
    if (n->ty && is_aggregate_value(n->ty)) return address_of(b, n);
    int addr = address_of(b, n);
    if (b->fail) return IR_NONE;
    return load_scalar(b, addr, n->ty);
}

static bool is_ptr_type(Type *ty) { return ty && ty->base && (ty->kind == TY_PTR || ty->kind == TY_ARRAY); }

// ptr +/- (prescaled integer): the parser emits `(long)(idx * elem_size)` with
// the multiply in the index's own width. Recognize it and use the exact 64-bit
// form sext(idx) * size; otherwise lower the expression as written.
static int gen_offset(B *b, Node *r) {
    Sc l64 = {IRT_I64, 8, false, false, false, false};
    Node *m = r;
    Sc cs = {0};
    bool have_cast = false;
    if (m->kind == ND_CAST && m->lhs && m->lhs->kind == ND_MUL) {
        have_cast = sc_of(m->ty, &cs) && cs.t == IRT_I64 && !cs.flt;
        m = m->lhs;
    }
    if (m->kind == ND_MUL && m->rhs && m->rhs->kind == ND_NUM && m->lhs && m->lhs->ty) {
        Sc is;
        if (sc_of(m->lhs->ty, &is) && !is.flt && !is.ptr && (is.size < 8 || have_cast || r == m)) {
            int idx = gen_expr(b, m->lhs);
            if (b->fail) return IR_NONE;
            idx = convert(b, idx, is, l64);
            if (m->rhs->val == 1) return idx;
            return op2(b, IR_MUL, IRT_I64, idx, iconst(b, IRT_I64, m->rhs->val));
        }
    }
    return gen_as(b, r, l64);
}

static IRCC cc_for(NodeKind k, bool uns) {
    switch (k) {
    case ND_EQ: return IRCC_EQ;
    case ND_NE: return IRCC_NE;
    case ND_LT: return uns ? IRCC_ULT : IRCC_SLT;
    default: return uns ? IRCC_ULE : IRCC_SLE;
    }
}
static IRCC fcc_for(NodeKind k) {
    switch (k) {
    case ND_EQ: return IRCC_FEQ;
    case ND_NE: return IRCC_FNE;
    case ND_LT: return IRCC_FLT;
    default: return IRCC_FLE;
    }
}

static int gen_compare(B *b, Node *n) {
    Sc ls, rs;
    if (!sc_of(n->lhs->ty, &ls) || !sc_of(n->rhs->ty, &rs)) return fail(b, "compare type");
    int x = gen_expr(b, n->lhs);
    int y = gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    if (ls.flt || rs.flt) {
        Sc ct = (ls.flt && ls.t == IRT_F64) || (rs.flt && rs.t == IRT_F64) ? (Sc){IRT_F64, 8, false, true, false, false}
                                                                           : (Sc){IRT_F32, 4, false, true, false, false};
        x = convert(b, x, ls, ct);
        y = convert(b, y, rs, ct);
        return cmp_f(b, fcc_for(n->kind), x, y);
    }
    // Unsigned iff either promoted operand is unsigned (or a pointer): same
    // rule as the AST codegen's use_unsigned_cmp().
    bool uns = (ls.size >= 4 && ls.uns) || (rs.size >= 4 && rs.uns) || ls.ptr || rs.ptr;
    Sc ct = ls.t == IRT_I64 || rs.t == IRT_I64 ? (Sc){IRT_I64, 8, uns, false, false, false}
                                               : (Sc){IRT_I32, 4, uns, false, false, false};
    x = convert(b, x, ls, ct);
    y = convert(b, y, rs, ct);
    return cmp_i(b, cc_for(n->kind, uns), x, y);
}

static int gen_arith(B *b, Node *n) {
    Sc s;
    if (!sc_of(n->ty, &s) || s.boolean) return fail(b, "arith type");
    if (s.flt) {
        IROp op;
        switch (n->kind) {
        case ND_ADD: op = IR_FADD; break;
        case ND_SUB: op = IR_FSUB; break;
        case ND_MUL: op = IR_FMUL; break;
        case ND_DIV: op = IR_FDIV; break;
        default: return fail(b, "float op");
        }
        int x = gen_as(b, n->lhs, s);
        int y = gen_as(b, n->rhs, s);
        if (b->fail) return IR_NONE;
        return op2(b, op, s.t, x, y);
    }
    if (s.size < 4) return fail(b, "narrow arith result");
    IROp op;
    switch (n->kind) {
    case ND_ADD: op = IR_ADD; break;
    case ND_SUB: op = IR_SUB; break;
    case ND_MUL: op = IR_MUL; break;
    case ND_DIV: op = s.uns ? IR_UDIV : IR_SDIV; break;
    case ND_MOD: op = s.uns ? IR_UREM : IR_SREM; break;
    case ND_BITAND: op = IR_AND; break;
    case ND_BITOR: op = IR_OR; break;
    default: op = IR_XOR; break;
    }
    int x = gen_as(b, n->lhs, s);
    int y = gen_as(b, n->rhs, s);
    if (b->fail) return IR_NONE;
    return op2(b, op, s.t, x, y);
}

static int gen_addsub(B *b, Node *n) {
    Type *lty = n->lhs->ty, *rty = n->rhs->ty;
    if (!lty || !rty) return fail(b, "untyped add");
    if (is_number(lty) && is_number(rty)) return gen_arith(b, n);
    Sc l64 = {IRT_I64, 8, false, false, false, false};
    if (is_ptr_type(lty) && is_integer(rty)) { // ptr +/- prescaled integer
        if (lty->base->kind == TY_VLA) return fail(b, "vla pointer arith");
        int p = gen_as(b, n->lhs, l64);
        int off = gen_offset(b, n->rhs);
        if (b->fail) return IR_NONE;
        return op2(b, n->kind == ND_ADD ? IR_ADD : IR_SUB, IRT_I64, p, off);
    }
    if (is_ptr_type(lty) && is_ptr_type(rty) && n->kind == ND_SUB) {
        int64_t sz = lty->base->size;
        if (sz <= 0 || lty->base->kind == TY_VLA) return fail(b, "ptrdiff elem size");
        int p = gen_as(b, n->lhs, l64);
        int q = gen_as(b, n->rhs, l64);
        if (b->fail) return IR_NONE;
        int d = op2(b, IR_SUB, IRT_I64, p, q);
        if (sz == 1) return d;
        if ((sz & (sz - 1)) == 0) {
            int sh = 0;
            while ((1LL << sh) < sz) sh++;
            return op2(b, IR_SAR, IRT_I64, d, iconst(b, IRT_I32, sh)); // exact division
        }
        return op2(b, IR_SDIV, IRT_I64, d, iconst(b, IRT_I64, sz));
    }
    return fail(b, "pointer arithmetic form");
}

static int gen_shift(B *b, Node *n) {
    Sc s;
    if (!sc_of(n->ty, &s) || s.flt || s.boolean) return fail(b, "shift type");
    Sc cs;
    if (!sc_of(n->rhs->ty, &cs) || cs.flt) return fail(b, "shift count type");
    int x = gen_as(b, n->lhs, s);
    int c = gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    if (cs.t == IRT_I64) c = op1(b, IR_TRUNC, IRT_I32, c);
    IROp op = n->kind == ND_SHL ? IR_SHL : (s.uns ? IR_SHR : IR_SAR);
    return norm(b, op2(b, op, s.t, x, c), s);
}

// Store `val` (already of the lvalue's C type) through the lvalue `lhs`; for SSA
// variables write the variable. Returns the stored (normalized) value.
static int assign_to(B *b, Node *lhs, int val) {
    Sc s;
    if (!sc_of(lhs->ty, &s)) return fail(b, "assign type");
    if (ty_volatile(lhs->ty)) return fail(b, "volatile access");
    if (lhs->kind == ND_LVAR && lhs->var && !lv_is_global(lhs->var)) {
        LvInfo *e = lv_info(b, lhs->var);
        if (e->ssa >= 0) {
            write_var(b, e->ssa, cur_block(b), val);
            return val;
        }
    }
    int addr = address_of(b, lhs);
    if (b->fail) return IR_NONE;
    emit_store(b, mem_of(s), addr, val);
    return val;
}

static int gen_assign(B *b, Node *n) {
    Sc ls, rs;
    if (n->lhs->ty && (n->lhs->ty->kind == TY_ARRAY || n->lhs->ty->kind == TY_FUNC))
        return fail(b, "array assignment"); // sc_of() models arrays as pointers; assigning copies bytes
    if (!sc_of(n->lhs->ty, &ls) || !sc_of(n->rhs->ty, &rs)) return fail(b, "assign type");
    int v = gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    v = convert(b, v, rs, ls);
    return assign_to(b, n->lhs, v);
}

static int gen_incdec(B *b, Node *n) {
    Node *lhs = n->lhs;
    Sc s;
    if (!sc_of(lhs->ty, &s)) return fail(b, "incdec type");
    bool inc = n->kind == ND_POST_INC || n->kind == ND_PRE_INC;
    bool post = n->kind == ND_POST_INC || n->kind == ND_POST_DEC;
    // Read the old value, remembering the address for memory lvalues.
    int addr = IR_NONE, old;
    LvInfo *e = lhs->kind == ND_LVAR && lhs->var && !lv_is_global(lhs->var) ? lv_info(b, lhs->var) : NULL;
    bool ssa = e && e->ssa >= 0;
    if (ssa) {
        old = read_var(b, e->ssa, cur_block(b));
    } else {
        addr = address_of(b, lhs);
        if (b->fail) return IR_NONE;
        old = load_scalar(b, addr, lhs->ty);
    }
    int nv;
    if (s.flt) {
        nv = op2(b, inc ? IR_FADD : IR_FSUB, s.t, old, fconst(b, s.t, 1.0));
    } else if (s.ptr) {
        int64_t sz = lhs->ty->base ? lhs->ty->base->size : 1;
        if (sz <= 0 || lhs->ty->base->kind == TY_VLA) return fail(b, "incdec pointer elem size");
        nv = op2(b, inc ? IR_ADD : IR_SUB, IRT_I64, old, iconst(b, IRT_I64, sz));
    } else if (s.boolean) {
        Sc i4 = {IRT_I32, 4, false, false, false, false};
        nv = convert(b, op2(b, inc ? IR_ADD : IR_SUB, IRT_I32, old, iconst(b, IRT_I32, 1)), i4, s);
    } else {
        nv = norm(b, op2(b, inc ? IR_ADD : IR_SUB, s.t, old, iconst(b, s.t, 1)), s);
    }
    if (ssa) write_var(b, e->ssa, b->cur_blk, nv);
    else
        emit_store(b, mem_of(s), addr, nv);
    return post ? old : nv;
}

static int gen_cond_value(B *b, Node *n) {
    Sc rs = {0};
    bool is_void = !n->ty || n->ty->kind == TY_VOID;
    if (!is_void && !sc_of(n->ty, &rs)) return fail(b, "?: type");
    IRBlock *tb = new_block(b), *eb = new_block(b), *join = new_block(b);
    // GNU `a ?: b`: `then` is the condition node itself or a cast of it. The
    // condition must be evaluated once; the true arm reuses its value.
    bool omitted = n->then == n->cond || (n->then->kind == ND_CAST && n->then->lhs == n->cond);
    int cv = IR_NONE;
    Sc cs = {0};
    if (omitted) {
        if (is_void || !sc_of(n->cond->ty, &cs)) return fail(b, "?: omitted operand type");
        cv = gen_expr(b, n->cond);
        if (b->fail) return IR_NONE;
        int t = cv;
        if (cs.flt) t = cmp_f(b, IRCC_FNE, cv, fconst(b, cs.t, 0.0));
        else if (cs.t == IRT_I64)
            t = cmp_i(b, IRCC_NE, cv, iconst(b, IRT_I64, 0));
        branch(b, t, tb, eb);
    } else {
        cond_br(b, n->cond, tb, eb);
    }
    seal_block(b, tb);
    seal_block(b, eb);
    IRInstr *phi = NULL;
    if (!is_void) {
        phi = ir_instr_new(b->f, IR_PHI, rs.t);
        phi->aux = -1;
        ir_block_add_phi(join, phi);
    }
    for (int side = 0; side < 2; side++) {
        start_block(b, side ? eb : tb);
        Node *arm = side ? n->els : n->then;
        int v = IR_NONE;
        if (is_void) {
            gen_expr(b, arm);
        } else if (omitted && side == 0) {
            v = convert(b, cv, cs, rs);
        } else {
            v = gen_as(b, arm, rs);
            if (b->fail) return IR_NONE;
        }
        if (b->cur_blk) {
            jump(b, join);
            if (phi) phi_add(b, phi, v);
        }
    }
    seal_block(b, join);
    start_block(b, join);
    return phi ? phi->id : IR_NONE;
}

static int gen_logical_value(B *b, Node *n) {
    bool is_and = n->kind == ND_LOGAND;
    IRBlock *rhs = new_block(b), *decided = new_block(b), *join = new_block(b);
    IRInstr *phi = ir_instr_new(b->f, IR_PHI, IRT_I32);
    phi->aux = -1; // not a variable phi
    ir_block_add_phi(join, phi);
    // `a && b`: a false decides 0; `a || b`: a true decides 1. The deciding
    // edges (however many cond_br makes) all land in `decided`.
    cond_br(b, n->lhs, is_and ? rhs : decided, is_and ? decided : rhs);
    seal_block(b, rhs);
    seal_block(b, decided);
    start_block(b, decided);
    int dv = iconst(b, IRT_I32, is_and ? 0 : 1);
    jump(b, join);
    phi_add(b, phi, dv);
    start_block(b, rhs);
    int r = gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    Sc rs;
    if (!sc_of(n->rhs->ty, &rs)) return fail(b, "logical operand type");
    r = to_bool01(b, r, rs);
    jump(b, join);
    phi_add(b, phi, r);
    seal_block(b, join);
    start_block(b, join);
    return phi->id;
}

static int gen_cast(B *b, Node *n) {
    if (n->ty && n->ty->kind == TY_VOID) {
        gen_expr(b, n->lhs);
        return IR_NONE;
    }
    Sc to, from;
    if (!sc_of(n->ty, &to) || !sc_of(n->lhs->ty, &from)) return fail(b, "cast type");
    int v = gen_expr(b, n->lhs);
    if (b->fail) return IR_NONE;
    return convert(b, v, from, to);
}

// Callees the AST codegen expands inline or treats specially (compiler
// builtins incl. the abs family, alloca, functions that return twice): never
// lowered as plain calls. gen_builtin_call() assumes the standard semantics of
// abs/labs/llabs even when the TU defines its own (gcc does too).
static bool callee_is_special(const char *nm) {
    if (!nm) return true;
    if (!strncmp(nm, "__builtin_", 10) || !strcmp(nm, "alloca")) return true;
    if (!strcmp(nm, "abs") || !strcmp(nm, "labs") || !strcmp(nm, "llabs")) return true;
    return strstr(nm, "setjmp") || strstr(nm, "vfork") || strstr(nm, "getcontext") ||
        strstr(nm, "swapcontext") || strstr(nm, "savectx");
}

// The TY_FUNC behind a callee expression's type (function or pointer to
// function); NULL when unknown (implicit declaration: treated as unprototyped).
static Type *func_type_of(Type *t) {
    if (!t) return NULL;
    if (t->kind == TY_FUNC) return t;
    if (t->kind == TY_PTR && t->base && t->base->kind == TY_FUNC) return t->base;
    return NULL;
}

static int gen_call(B *b, Node *n) {
    Node *callee = n->lhs;
    const char *sym = NULL;
    int target = IR_NONE;
    Type *fty = NULL;
    if (!callee) {
        if (!n->funcname || callee_is_special(n->funcname)) return fail(b, "call without callee");
        sym = n->funcname; // call by bare name: unprototyped (fty stays NULL)
    } else if (callee->kind == ND_LVAR && callee->var && callee->var->is_function) {
        LVar *fv = callee->var;
        if (fv->asm_name || callee_is_special(fv->name))
            return fail(b, "special callee");
        sym = fv->name;
        fty = func_type_of(callee->ty);
        if (!fty) fty = func_type_of(fv->ty);
    } else if (callee->ty && callee->ty->kind == TY_PTR && callee->ty->base && callee->ty->base->kind == TY_FUNC) {
        Sc pc = {IRT_I64, 8, true, false, true, false};
        target = gen_as(b, callee, pc);
        if (b->fail) return IR_NONE;
        fty = callee->ty->base;
    } else {
        return fail(b, "call target form");
    }
    // fty == NULL: unprototyped call (default promotions, %al set).
    bool proto = fty && !fty->is_oldstyle;
    Type *pt = proto ? fty->param_types : NULL;

    int nargs = 0;
    for (Node *a = n->args; a; a = a->next) nargs++;
    int32_t *args = xcalloc((size_t)nargs, sizeof(int32_t));
    int k = 0;
    for (Node *a = n->args; a; a = a->next, k++) {
        Sc as;
        if (!sc_of(a->ty, &as)) {
            free(args);
            return fail(b, "argument type");
        }
        int v = gen_expr(b, a);
        if (b->fail) {
            free(args);
            return IR_NONE;
        }
        if (pt) { // prototyped parameter: convert to its type
            Sc ps;
            if (!sc_of(pt, &ps)) {
                free(args);
                return fail(b, "parameter type");
            }
            v = convert(b, v, as, ps);
            pt = pt->param_next;
        } else if (as.t == IRT_F32) { // default argument promotion
            Sc d = {IRT_F64, 8, false, true, false, false};
            v = convert(b, v, as, d);
        }
        args[k] = v;
    }

    bool is_void = !n->ty || n->ty->kind == TY_VOID;
    Sc rs = {0};
    if (!is_void && !sc_of(n->ty, &rs)) {
        free(args);
        return fail(b, "return type");
    }
    IRInstr *c = emit(b, IR_CALL, is_void ? IRT_VOID : rs.t);
    c->sym = sym;
    c->a = target;
    c->args = args;
    c->nargs = nargs;
    c->variadic = !fty || fty->is_variadic || fty->is_oldstyle;
    b->f->has_calls = true;
    if (is_void) return IR_NONE;
    return norm(b, c->id, rs); // callers may not rely on the callee normalizing narrow results
}

static int gen_expr(B *b, Node *n) {
    if (b->fail || !n) return IR_NONE;
    Sc s;
    switch (n->kind) {
    case ND_NUM: {
        if (!sc_of(n->ty, &s) || s.flt) return fail(b, "num type");
        int64_t v = n->val;
        if (s.boolean) v = v != 0;
        else if (s.size == 1)
            v = s.uns ? (uint8_t)v : (int8_t)v;
        else if (s.size == 2)
            v = s.uns ? (uint16_t)v : (int16_t)v;
        return iconst(b, s.t, v);
    }
    case ND_FNUM:
        if (!sc_of(n->ty, &s) || !s.flt) return fail(b, "fnum type");
        return fconst(b, s.t, n->fval);
    case ND_STR:
        return gaddr(b, format(".LC%d", n->str_id), 0);
    case ND_LVAR: {
        if (!n->var) return fail(b, "no var");
        if (n->var->is_function) return var_address(b, n->var); // designator: the address is the value
        if (!lv_is_global(n->var)) {
            LvInfo *e = lv_info(b, n->var);
            if (e->ssa >= 0) return read_var(b, e->ssa, cur_block(b));
        }
        return value_from_lvalue(b, n);
    }
    case ND_DEREF:
    case ND_MEMBER:
        return value_from_lvalue(b, n);
    case ND_ADDR: {
        int a = address_of(b, n->lhs);
        return a;
    }
    case ND_ADD:
    case ND_SUB:
        return gen_addsub(b, n);
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITXOR:
    case ND_BITOR:
        return gen_arith(b, n);
    case ND_SHL:
    case ND_SHR:
        return gen_shift(b, n);
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
        return gen_compare(b, n);
    case ND_NEG: {
        if (!sc_of(n->ty, &s) || s.boolean) return fail(b, "neg type");
        int x = gen_as(b, n->lhs, s);
        if (b->fail) return IR_NONE;
        return norm(b, op1(b, s.flt ? IR_FNEG : IR_NEG, s.t, x), s);
    }
    case ND_BITNOT: {
        if (!sc_of(n->ty, &s) || s.flt || s.boolean) return fail(b, "bitnot type");
        int x = gen_as(b, n->lhs, s);
        if (b->fail) return IR_NONE;
        return norm(b, op1(b, IR_NOT, s.t, x), s);
    }
    case ND_NOT: {
        Sc os;
        if (!sc_of(n->lhs->ty, &os)) return fail(b, "not type");
        int x = gen_expr(b, n->lhs);
        if (b->fail) return IR_NONE;
        if (os.flt) return cmp_f(b, IRCC_FEQ, x, fconst(b, os.t, 0.0));
        return cmp_i(b, IRCC_EQ, x, iconst(b, os.t, 0));
    }
    case ND_LOGAND:
    case ND_LOGOR:
        return gen_logical_value(b, n);
    case ND_COND:
        return gen_cond_value(b, n);
    case ND_COMMA:
        gen_expr(b, n->lhs);
        return gen_expr(b, n->rhs);
    case ND_ASSIGN:
        return gen_assign(b, n);
    case ND_POST_INC:
    case ND_POST_DEC:
    case ND_PRE_INC:
    case ND_PRE_DEC:
        return gen_incdec(b, n);
    case ND_CAST:
        return gen_cast(b, n);
    case ND_FUNCALL:
        return gen_call(b, n);
    case ND_ZERO_INIT:
        gen_zero_init(b, n);
        return IR_NONE;
    default:
        return fail_kind(b, "unsupported expression", n->kind);
    }
}

// Branch to `t` if `n` is true, else to `f`; ends the current block.
static void cond_br(B *b, Node *n, IRBlock *t, IRBlock *f) {
    if (b->fail) return;
    switch (n->kind) {
    case ND_LOGAND: {
        IRBlock *mid = new_block(b);
        cond_br(b, n->lhs, mid, f);
        seal_block(b, mid);
        start_block(b, mid);
        cond_br(b, n->rhs, t, f);
        return;
    }
    case ND_LOGOR: {
        IRBlock *mid = new_block(b);
        cond_br(b, n->lhs, t, mid);
        seal_block(b, mid);
        start_block(b, mid);
        cond_br(b, n->rhs, t, f);
        return;
    }
    case ND_NOT:
        cond_br(b, n->lhs, f, t);
        return;
    default:
        break;
    }
    Sc s;
    if (!sc_of(n->ty, &s)) {
        fail(b, "condition type");
        return;
    }
    int v = gen_expr(b, n);
    if (b->fail) return;
    if (s.t != IRT_I32 || s.boolean) {
        if (s.flt) v = cmp_f(b, IRCC_FNE, v, fconst(b, s.t, 0.0));
        else if (s.t == IRT_I64)
            v = cmp_i(b, IRCC_NE, v, iconst(b, IRT_I64, 0));
    }
    branch(b, v, t, f);
}

// ----------------------------------------------------------------- statements

static void gen_stmt(B *b, Node *n);

static void gen_stmts(B *b, Node *n) {
    for (; n && !b->fail; n = n->next) gen_stmt(b, n);
}

static void push_loop(B *b, IRBlock *brk, IRBlock *cont) {
    if (b->depth == b->capdepth) {
        b->capdepth = b->capdepth ? b->capdepth * 2 : 8;
        b->brk = xrealloc(b->brk, sizeof(IRBlock *) * (size_t)b->capdepth);
        b->cont = xrealloc(b->cont, sizeof(IRBlock *) * (size_t)b->capdepth);
    }
    b->brk[b->depth] = brk;
    b->cont[b->depth] = cont;
    b->depth++;
}

// ND_ZERO_INIT: zero-fill a local variable (`= {0}` / `= {}`): a scalar SSA
// variable gets a zero constant, small memory variables get zero stores, large
// ones a memset call.
static void gen_zero_init(B *b, Node *n) {
    LVar *v = n->lhs ? n->lhs->var : NULL;
    if (!v || !v->is_local || !v->ty || v->ty->size <= 0) return; // the AST codegen ignores these too
    if (!lv_is_global(v)) {
        LvInfo *e = lv_info(b, v);
        if (e->ssa >= 0) {
            write_var(b, e->ssa, cur_block(b), zero_of(b, e->sc.t));
            return;
        }
    }
    int64_t size = v->ty->size;
    int base = op0(b, IR_FRAMEADDR, IRT_I64, v->offset);
    if (size > 256) {
        int zero = iconst(b, IRT_I32, 0), len = iconst(b, IRT_I64, size);
        IRInstr *c = emit(b, IR_CALL, IRT_VOID);
        c->sym = "memset";
        c->nargs = 3;
        c->args = xcalloc(3, sizeof(int32_t));
        c->args[0] = base;
        c->args[1] = zero;
        c->args[2] = len;
        b->f->has_calls = true;
        return;
    }
    int z64 = iconst(b, IRT_I64, 0), z32 = iconst(b, IRT_I32, 0);
    static const struct {
        int bytes;
        IRMem mw;
    } widths[] = {{8, IRM_I64}, {4, IRM_I32}, {2, IRM_I16}, {1, IRM_I8}};
    int64_t off = 0;
    for (int w = 0; w < 4; w++) {
        while (size - off >= widths[w].bytes) {
            IRInstr *s = emit(b, IR_STORE, IRT_VOID);
            s->a = base;
            s->c = w == 0 ? z64 : z32;
            s->mw = (uint8_t)widths[w].mw;
            s->imm = off;
            off += widths[w].bytes;
        }
    }
}

static void gen_stmt(B *b, Node *n) {
    if (b->fail || !n) return;
    switch (n->kind) {
    case ND_NULL:
        return;
    case ND_ZERO_INIT:
        gen_zero_init(b, n);
        return;
    case ND_EXPR_STMT:
        gen_expr(b, n->lhs);
        return;
    case ND_BLOCK:
        gen_stmts(b, n->body);
        return;
    case ND_RETURN: {
        int v = IR_NONE;
        if (n->lhs) {
            Sc from, to;
            if (b->f->ret_type == IRT_VOID) {
                gen_expr(b, n->lhs);
            } else {
                if (!sc_of(n->lhs->ty, &from) || !sc_of(b->fn->ty->return_ty, &to)) {
                    fail(b, "return type");
                    return;
                }
                v = gen_expr(b, n->lhs);
                if (b->fail) return;
                v = convert(b, v, from, to);
            }
        } else if (b->f->ret_type != IRT_VOID) {
            v = zero_of(b, b->f->ret_type); // `return;` in a non-void function: unspecified
        }
        IRInstr *r = emit(b, IR_RET, IRT_VOID);
        r->a = v;
        b->cur_blk = NULL;
        return;
    }
    case ND_IF: {
        gen_stmts(b, n->init);
        IRBlock *tb = new_block(b), *join = new_block(b);
        IRBlock *eb = n->els ? new_block(b) : join;
        cond_br(b, n->cond, tb, eb);
        seal_block(b, tb);
        if (n->els) seal_block(b, eb);
        start_block(b, tb);
        gen_stmt(b, n->then);
        jump(b, join);
        if (n->els) {
            start_block(b, eb);
            gen_stmt(b, n->els);
            jump(b, join);
        }
        seal_block(b, join);
        start_block(b, join);
        return;
    }
    case ND_FOR: {
        gen_stmts(b, n->init);
        IRBlock *header = new_block(b), *body = new_block(b), *inc = new_block(b), *exit = new_block(b);
        jump(b, header);
        start_block(b, header);
        if (n->cond) {
            cond_br(b, n->cond, body, exit);
        } else {
            jump(b, body);
        }
        seal_block(b, body);
        push_loop(b, exit, inc);
        start_block(b, body);
        gen_stmt(b, n->then);
        jump(b, inc);
        b->depth--;
        seal_block(b, inc);
        start_block(b, inc);
        if (n->inc) gen_expr(b, n->inc);
        jump(b, header);
        seal_block(b, header);
        seal_block(b, exit);
        start_block(b, exit);
        return;
    }
    case ND_DO: {
        IRBlock *body = new_block(b), *cont = new_block(b), *exit = new_block(b);
        jump(b, body);
        push_loop(b, exit, cont);
        start_block(b, body);
        gen_stmt(b, n->then);
        jump(b, cont);
        b->depth--;
        seal_block(b, cont);
        start_block(b, cont);
        cond_br(b, n->cond, body, exit);
        seal_block(b, body);
        seal_block(b, exit);
        start_block(b, exit);
        return;
    }
    case ND_BREAK:
        if (n->target_loop || !b->depth) {
            fail(b, "labeled break");
            return;
        }
        jump(b, b->brk[b->depth - 1]);
        return;
    case ND_CONTINUE:
        if (n->target_loop || !b->depth) {
            fail(b, "labeled continue");
            return;
        }
        jump(b, b->cont[b->depth - 1]);
        return;
    default:
        gen_expr(b, n); // expressions (e.g. initializer assignments) can appear without an ND_EXPR_STMT wrapper
        return;
    }
}

// ------------------------------------------------------------------- driver

static void free_builder(B *b) {
    free(b->tab);
    free(b->marked);
    free(b->ssa_type);
    free(b->undef_val);
    for (int i = 0; i < b->curcap; i++) free(b->cur[i]);
    free(b->cur);
    free(b->curn);
    for (int i = 0; i < b->auxcap; i++) free(b->aux[i].users);
    free(b->aux);
    free(b->brk);
    free(b->cont);
}

static bool fn_eligible(B *b) {
    Function *fn = b->fn;
    if (fn->is_variadic || fn->is_nested || fn->alias_target || fn->n_target_clones || fn->target_attr ||
        fn->dealloc_vla || fn->needs_stack_chk) {
        fail(b, "function attribute");
        return false;
    }
    if (opt_g || opt_ubsan || opt_asan || opt_float_cast_overflow || opt_sanitize_list || opt_stack_protector) {
        fail(b, "debug/sanitizer/stack-protector");
        return false;
    }
    for (LVar *v = fn->locals; v; v = v->next) {
        if (v->defer_stmt || v->cleanup_func || (v->ty && v->ty->cleanup_func) || v->ty->kind == TY_VLA) {
            fail(b, "cleanup/VLA local");
            return false;
        }
    }
    return true;
}

IRFunc *ir_build(Program *prog, Function *fn) {
    B bs;
    memset(&bs, 0, sizeof bs);
    B *b = &bs;
    b->prog = prog;
    b->fn = fn;
    b->f = ir_func_new(fn);
    IRFunc *f = b->f;
    f->frame_size = fn->stack_size;

    bool ok = fn_eligible(b);
    Type *rt = fn->ty ? fn->ty->return_ty : NULL;
    Sc rsc;
    if (ok) {
        if (!rt || rt->kind == TY_VOID) f->ret_type = IRT_VOID;
        else if (sc_of(rt, &rsc))
            f->ret_type = rsc.t;
        else {
            fail(b, "return type");
            ok = false;
        }
    }
    // Parameters: scalar only.
    int nparams = 0;
    if (ok)
        for (LVar *p = fn->params; p; p = p->param_next) {
            Sc ps;
            if (!sc_of(p->ty, &ps) || p->ty->kind == TY_ARRAY || p->ty->kind == TY_FUNC) {
                fail(b, "parameter type");
                ok = false;
                break;
            }
            nparams++;
        }
    if (ok) {
        scan_chain(b, fn->body);
        ok = !b->fail;
    }
    if (!ok) {
        if (opt_fdump_ir) fprintf(stderr, "ir: %s: not eligible (%s)\n", fn->name, b->why ? b->why : "?");
        free_builder(b);
        ir_func_free(f);
        return NULL;
    }

    // Register every scalar local/param as an SSA variable up front (the
    // per-block current-definition arrays are sized from this count).
    int nloc = 0;
    for (LVar *v = fn->locals; v; v = v->next) nloc++;
    for (LVar *p = fn->params; p; p = p->param_next) nloc++;
    b->capssa = nloc + 1;
    b->ssa_type = xcalloc((size_t)b->capssa, sizeof(IRType));
    b->undef_val = xcalloc((size_t)b->capssa, sizeof(int));
    for (LVar *p = fn->params; p; p = p->param_next) lv_info(b, p);
    for (LVar *v = fn->locals; v; v = v->next)
        if (!lv_is_global(v)) lv_info(b, v);

    IRBlock *entry = new_block(b);
    entry->sealed = true;
    b->entry = entry;
    start_block(b, entry);

    // Incoming parameters.
    f->nparams = nparams;
    f->param_types = xcalloc((size_t)(nparams ? nparams : 1), sizeof(IRType));
    int idx = 0;
    for (LVar *p = fn->params; p; p = p->param_next, idx++) {
        Sc ps;
        sc_of(p->ty, &ps);
        f->param_types[idx] = ps.t;
        int pv = op0(b, IR_PARAM, ps.t, idx);
        pv = norm(b, pv, ps); // callers may leave the high bits of narrow args undefined
        LvInfo *e = lv_info(b, p);
        if (e->ssa >= 0) write_var(b, e->ssa, entry, pv);
        else
            emit_store(b, mem_of(ps), op0(b, IR_FRAMEADDR, IRT_I64, p->offset), pv);
    }

    gen_stmts(b, fn->body);

    if (!b->fail && b->cur_blk) { // fell off the end
        // Falling off the end: main returns 0 (C99 5.1.2.2.3); elsewhere unspecified.
        int v = f->ret_type == IRT_VOID ? IR_NONE : zero_of(b, f->ret_type);
        IRInstr *r = emit(b, IR_RET, IRT_VOID);
        r->a = v;
        b->cur_blk = NULL;
    }
    if (b->fail) {
        if (opt_fdump_ir) fprintf(stderr, "ir: %s: not eligible (%s)\n", fn->name, b->why ? b->why : "?");
        // Unsealed blocks may still own pending lists.
        for (int i = 0; i < f->nblocks; i++) {
            Pending *p = f->blocks[i]->auxp;
            if (p) {
                free(p->var);
                free(p->phi);
                free(p);
                f->blocks[i]->auxp = NULL;
            }
        }
        free_builder(b);
        ir_func_free(f);
        return NULL;
    }
    // Seal anything left open (unreachable constructs).
    for (int i = 0; i < f->nblocks; i++) seal_block(b, f->blocks[i]);

    // Resolve forwarded (trivial) phis everywhere and drop them.
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *blk = f->blocks[bi];
        for (int k = 0; k < blk->nphis + blk->nins; k++) {
            IRInstr *i = k < blk->nphis ? blk->phis[k] : blk->ins[k - blk->nphis];
            if (i->a != IR_NONE) i->a = resolve(b, i->a);
            if (i->b != IR_NONE) i->b = resolve(b, i->b);
            if (i->c != IR_NONE) i->c = resolve(b, i->c);
            for (int j = 0; j < i->nargs; j++) i->args[j] = resolve(b, i->args[j]);
        }
    }
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *blk = f->blocks[bi];
        for (int k = 0; k < blk->nphis;) {
            IRInstr *p = blk->phis[k];
            if (p->id < b->auxcap && b->aux[p->id].fwd >= 0) ir_block_remove(blk, p);
            else
                k++;
        }
    }
    free_builder(b);
    ir_verify(f);
    return f;
}
