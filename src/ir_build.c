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

// Vector values passed or returned by value are not modelled by the AArch64 emitter (the
// IRAggClass classification covers structs and unions only); such functions stay on the AST
// backend there.
#if defined(ARCH_ARM64) || defined(_WIN32)
#define IR_NO_AGG_ABI(ty) ((ty)->is_vector)
#else
#define IR_NO_AGG_ABI(ty) 0
#endif

// A call to a function without a prototype (implicit declaration) is typed `int`, but the AST
// backend leaves the full register alone when such a result is converted to a pointer (legacy
// code calls pointer-returning libc functions that way, and on AArch64 and Win64 heap addresses do not fit
// 32 bits). The IR mirrors that: the call is an I64 whose TRUNC is the int value, and an
// int -> pointer conversion of that value uses the untruncated register (convert()).
#if defined(ARCH_ARM64) || defined(_WIN32)
#define IR_WIDE_IMPLICIT_INT 1
#else
#define IR_WIDE_IMPLICIT_INT 0
#endif

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

// Innermost-first stack of enclosing switch statements: maps ND_CASE nodes
// (which can nest inside loops — Duff's device) to their pre-created blocks.
typedef struct {
    Node *node; // the ND_SWITCH
    IRBlock **cblk; // per case_next entry
    int nc;
    IRBlock *dflt; // NULL when the switch has no default
} SwCtx;

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
    int retbuf_off; // frame offset of the parser's __retbuf local (sret functions)
    IRBlock *cur_blk; // insertion block, NULL after a terminator
    IRBlock **brk, **cont; // loop target stacks
    int depth, capdepth;
    SwCtx *sw;
    int swdepth, capsw;
    // goto targets: label name (interned) -> block, created lazily
    char **label_name;
    IRBlock **label_blk;
    int nlabels, caplabels;
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

static int emit_load(B *b, IRMem mw, bool sign, IRType t, int addr, bool vol) {
    IRInstr *i = emit(b, IR_LOAD, t);
    i->a = addr;
    i->mw = (uint8_t)mw;
    i->msign = sign;
    i->vol = vol;
    return i->id;
}
static void emit_store(B *b, IRMem mw, int addr, int val, bool vol) {
    IRInstr *i = emit(b, IR_STORE, IRT_VOID);
    i->a = addr;
    i->c = val;
    i->mw = (uint8_t)mw;
    i->vol = vol;
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

// -------------------------------------------------------------- bitfields
//
// A bitfield lives in a storage unit at member->offset: usz bytes (the AST
// codegen's access width: Member.bf_load_size for dense-packed fields that
// cross their declared type, else the declared type's size), bit_offset bits
// up from the unit's least-significant bit. Reads load the unit zero-extended
// and extract; writes are load/clear/or/store of the same unit, so struct
// layout is whatever the parser decided.
//
// Unlike the AST codegen (which treats _Bool fields as plain unsigned bits
// and every enum as unsigned) this follows C: a store to a _Bool field
// normalizes to 0/1, and an enum field is signed iff its underlying type is.
typedef struct {
    int usz; // unit access width in bytes: 1, 2, 4, 8
    int disp; // byte displacement of the unit from member->offset (<= 0)
    int bo, bw; // field position / width in bits within the unit
    IRType wt; // register type holding the unit: I32 (usz <= 4) / I64
    bool uns; // extract zero-extended (else sign-extended from bit bw-1)
    bool boolean; // _Bool field
    bool vol; // volatile-qualified access: the unit load/store below are volatile
    Sc ns; // C type of the member expression (the promoted field type)
} Bf;

// Plan the access for ND_MEMBER `n` naming a bitfield. NULL on success, else
// the reason this access cannot be lowered exactly.
static const char *bf_plan(Node *n, Bf *p) {
    Member *m = n->member;
    Type *mt = m->ty, *st = n->lhs ? n->lhs->ty : NULL;
    if (!mt || !n->ty || !st) return "bitfield: untyped";
    // Volatile qualifiers can sit on the member, the member expression or the containing
    // object. The access is then the unit load (+ merge) and unit store with IRInstr.vol
    // set: one read of the whole unit and one write of it, at the unit's width.
    p->vol = ty_volatile(mt) || ty_volatile(n->ty) || ty_volatile(st);
    switch (mt->kind) { // enums are TY_INT; _BitInt and __int128 are not modelled
    case TY_BOOL:
    case TY_CHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_LONG:
    case TY_LLONG:
        break;
    default:
        return "bitfield: member type";
    }
    int usz = m->bf_load_size ? m->bf_load_size : (int)mt->size;
    int bw = m->bit_width, bo = m->bit_offset;
    // bf_load_size > 8 is a >64-bit span: the AST codegen needs a second access.
    if (usz != 1 && usz != 2 && usz != 4 && usz != 8) return "bitfield: unit size";
    if (bw < 1 || bo < 0 || bo + bw > usz * 8) return "bitfield: straddles unit";
    if ((st->kind != TY_STRUCT && st->kind != TY_UNION) || st->size <= 0 || m->offset < 0)
        return "bitfield: container";
    // Packed layouts can put the declared-type-wide unit past the end of the
    // struct (`#pragma pack(1) struct { char a:3; int b:5; }` is 1 byte). Never
    // touch bytes outside the object: use the smallest power-of-two unit that
    // covers the field, and when even that sticks out (a 3-byte field region
    // at the very end needs a 4-byte unit) slide it back to end at the struct's
    // end, moving the field's bit offset up by the displacement.
    int disp = 0;
    if (m->offset + usz > st->size) {
        int need = (bo + bw + 7) / 8, q = 1;
        while (q < need) q *= 2;
        if (m->offset + need > st->size || q > st->size) return "bitfield: unit beyond struct";
        usz = q;
        if (m->offset + q > st->size) {
            disp = (int)(st->size - q - m->offset); // < 0
            bo -= 8 * disp;
        }
    }
    Sc ns;
    if (!sc_of(n->ty, &ns) || ns.flt || ns.ptr || ns.boolean) return "bitfield: expression type";
    p->usz = usz;
    p->disp = disp;
    p->bo = bo;
    p->bw = bw;
    p->wt = usz == 8 ? IRT_I64 : IRT_I32;
    // The member expression's type (ns.t) may be narrower than the unit
    // register (an int field in a dense-packed 8-byte unit); bf_retype()
    // reconciles them. A 64-bit field type is promoted to int below 33 bits,
    // so an I64 expression always sits in an 8-byte unit.
    if (ns.t == IRT_I64 && p->wt == IRT_I32) return "bitfield: expression type";
    p->boolean = mt->kind == TY_BOOL;
    p->uns = p->boolean || mt->is_unsigned;
    p->ns = ns;
    return NULL;
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

// Does the parser-recorded cleanup range [begin, end) contain any variable
// that actually needs a cleanup run? The range is non-empty for every nested
// scope exit (locals grew); only cleanup_func/defer_stmt vars matter.
static bool range_has_cleanup(LVar *begin, LVar *end) {
    for (LVar *v = begin; v && v != end; v = v->next)
        if (v->cleanup_func || v->defer_stmt) return true;
    return false;
}

static void scan(B *b, Node *n) {
    if (!n || b->fail) return;
    switch (n->kind) {
    case ND_NUM:
    case ND_FNUM:
    case ND_STR:
    case ND_NULL:
        return;
    case ND_BREAK:
    case ND_CONTINUE:
        // Non-empty cleanup range with actual cleanups: exiting here must run
        // them (see ND_RETURN).
        if (range_has_cleanup(n->cleanup_begin, n->cleanup_end)) fail(b, "cleanup on control flow");
        return;
    case ND_LVAR:
        if (n->chain_depth) fail(b, "nested-function variable");
        // Block-scoped cleanup/defer/VLA locals are gone from fn->locals by the
        // time fn_eligible walks it, so every use must be rejected here.
        if (n->var && (n->var->cleanup_func || n->var->defer_stmt)) fail(b, "cleanup local");
        if (n->var && n->var->ty && (n->var->ty->kind == TY_VLA || n->var->ty->cleanup_func))
            fail(b, "VLA/cleanup local");
        return;
    case ND_ADDR:
        mark_spine(b, n->lhs);
        scan(b, n->lhs);
        return;
    case ND_MEMBER:
        if (n->member && n->member->offset_expr) {
            fail(b, "variable member");
            return;
        }
        if (n->member && n->member->bit_width) {
            Bf p;
            const char *why = bf_plan(n, &p);
            if (why) {
                fail(b, why);
                return;
            }
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
        // A cleanup range holding actual cleanup vars means reaching this
        // return must run them first -- the IR lowering does not emit them.
        // (defer_retspill is reserved for ANY non-empty range, so it is not
        // a usable predicate by itself.)
        if (range_has_cleanup(n->cleanup_begin, n->cleanup_end)) fail(b, "cleanup on return");
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
    case ND_SWITCH:
        scan_chain(b, n->init);
        scan(b, n->cond);
        scan_chain(b, n->then); // case markers with their bodies
        return;
    case ND_CASE:
        scan(b, n->lhs);
        return;
    case ND_LABEL:
        scan(b, n->lhs);
        return;
    case ND_GOTO:
        if (range_has_cleanup(n->cleanup_begin, n->cleanup_end)) fail(b, "cleanup on control flow");
        return;
    case ND_STMT_EXPR:
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
    if (IR_WIDE_IMPLICIT_INT && to.ptr && !to.boolean && !from.flt && from.t == IRT_I32) {
        IRInstr *d = b->f->vals[v];
        if (d->op == IR_TRUNC && d->a != IR_NONE && b->f->vals[d->a]->op == IR_CALL && b->f->vals[d->a]->msign)
            return d->a;
    }
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
static void gen_stmt(B *b, Node *n);
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
    return emit_load(b, mem_of(s), !s.uns && !s.flt, s.t, addr, ty_volatile(ty));
}

static bool is_aggregate_value(Type *ty) {
    return ty->kind == TY_ARRAY || ty->kind == TY_FUNC;
}

// A struct/union/array whose bytes would be read or written wholesale (copy, by-value argument,
// return) and that is volatile itself or holds a volatile member/element: those accesses must be
// volatile too, per member and at the declared widths, which the exact-width copy loop does not
// model. Rejected (old codegen).
static bool is_volatile_aggregate(Type *ty) {
    if (!ty) return false;
    if (ty_volatile(ty)) return true;
    if (ty->kind == TY_ARRAY) return is_volatile_aggregate(ty->base);
    if (ty->kind == TY_STRUCT || ty->kind == TY_UNION)
        for (Member *m = ty->members; m; m = m->next)
            if (is_volatile_aggregate(m->ty)) return true;
    return false;
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
    case ND_CAST: // a cast does not change the address: &((T)lv) == &lv
        return address_of(b, n->lhs);
    case ND_STR:
        return gaddr(b, format(".LC%d", n->str_id), 0);
    case ND_FUNCALL: // struct/union call result: gen_call's value IS the address
        return gen_expr(b, n);
    case ND_ASSIGN: // struct assignment: gen_assign's value IS the address (side effects once)
        return gen_expr(b, n);
    default:
        return fail_kind(b, "unsupported lvalue", n->kind);
    }
}

// Address of the unit holding the bitfield named by ND_MEMBER `n`.
static int bf_addr(B *b, Node *n, const Bf *p) {
    int a = address_of(b, n);
    if (b->fail || !p->disp) return a;
    return op2(b, IR_ADD, IRT_I64, a, iconst(b, IRT_I64, p->disp));
}

static IRMem bf_mem(int usz) {
    switch (usz) {
    case 1: return IRM_I8;
    case 2: return IRM_I16;
    case 4: return IRM_I32;
    default: return IRM_I64;
    }
}

// Move an integer between the unit register type and the expression type.
// Widening sign-extends: callers only look at the low field bits then.
static int bf_retype(B *b, int v, IRType from, IRType to) {
    if (from == to) return v;
    if (from == IRT_I64) return op1(b, IR_TRUNC, IRT_I32, v);
    return op1(b, IR_SEXT32, IRT_I64, v);
}

// The field's value, in the unit register type, from the unit `u` (loaded
// zero-extended to that width).
static int bf_field(B *b, const Bf *p, int u) {
    int W = p->wt == IRT_I64 ? 64 : 32, v = u;
    if (p->uns) {
        if (p->bo) v = op2(b, IR_SHR, p->wt, v, iconst(b, IRT_I32, p->bo));
        // the unit is zero-extended, so a field ending at the unit's top needs no mask
        if (p->bo + p->bw < p->usz * 8) v = op2(b, IR_AND, p->wt, v, iconst(b, p->wt, (int64_t)((1ULL << p->bw) - 1)));
        return v;
    }
    int l = W - p->bo - p->bw, r = W - p->bw; // shl+sar sign-extends from bit bw-1
    if (l) v = op2(b, IR_SHL, p->wt, v, iconst(b, IRT_I32, l));
    if (r) v = op2(b, IR_SAR, p->wt, v, iconst(b, IRT_I32, r));
    return v;
}

// Read a bitfield from the unit at `addr`; *raw (optional) receives the unit.
static int bf_load(B *b, int addr, const Bf *p, int *raw) {
    int u = emit_load(b, bf_mem(p->usz), false, p->wt, addr, p->vol);
    if (raw) *raw = u;
    return bf_retype(b, bf_field(b, p, u), p->wt, p->ns.t);
}

// Write `v` (already of the field's C type, 0/1 for _Bool) into the bitfield
// whose unit is at `addr`: load, clear the field bits, or in (v & mask) << bo,
// store. `raw` is the unit if the caller just loaded it (IR_NONE: load here).
// Returns the field's value after truncation to the field width.
static int bf_store(B *b, int addr, const Bf *p, int v, int raw) {
    int W = p->wt == IRT_I64 ? 64 : 32;
    int vw = bf_retype(b, v, p->ns.t, p->wt); // only the low bw bits matter
    uint64_t fm = p->bw >= 64 ? ~0ULL : (1ULL << p->bw) - 1;
    int ins = vw;
    if (p->bw < W) ins = op2(b, IR_AND, p->wt, vw, iconst(b, p->wt, (int64_t)fm));
    int nu = p->bo ? op2(b, IR_SHL, p->wt, ins, iconst(b, IRT_I32, p->bo)) : ins;
    if (p->bw < p->usz * 8) { // the unit holds other bits: merge
        int old = raw != IR_NONE ? raw : emit_load(b, bf_mem(p->usz), false, p->wt, addr, p->vol);
        int keep = op2(b, IR_AND, p->wt, old, iconst(b, p->wt, (int64_t)~(fm << p->bo)));
        nu = op2(b, IR_OR, p->wt, keep, nu);
    }
    emit_store(b, bf_mem(p->usz), addr, nu, p->vol);
    int res = ins;
    if (!p->uns && p->bw < W) {
        res = op2(b, IR_SHL, p->wt, vw, iconst(b, IRT_I32, W - p->bw));
        res = op2(b, IR_SAR, p->wt, res, iconst(b, IRT_I32, W - p->bw));
    } else if (!p->uns) {
        res = vw;
    }
    return bf_retype(b, res, p->wt, p->ns.t);
}

// The C type an assigned value must have before storing into the field: _Bool
// fields normalize to 0/1, everything else converts to the promoted type.
static Sc bf_target(const Bf *p) {
    if (p->boolean) return (Sc){IRT_I32, 1, true, false, false, true};
    return p->ns;
}

// `lhs = rhs` / compound assignment (the parser lowers `a.f op= x` to
// ASSIGN(a.f, a.f op x)) with a bitfield lhs. Value: the field after truncation.
static int gen_bf_assign(B *b, Node *n) {
    Bf p;
    const char *why = bf_plan(n->lhs, &p);
    if (why) return fail(b, why);
    Sc rs;
    if (!sc_of(n->rhs->ty, &rs)) return fail(b, "assign type");
    int v = gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    v = convert(b, v, rs, bf_target(&p));
    int addr = bf_addr(b, n->lhs, &p);
    if (b->fail) return IR_NONE;
    return bf_store(b, addr, &p, v, IR_NONE);
}

static int gen_bf_incdec(B *b, Node *n) {
    Bf p;
    const char *why = bf_plan(n->lhs, &p);
    if (why) return fail(b, why);
    bool inc = n->kind == ND_POST_INC || n->kind == ND_PRE_INC;
    bool post = n->kind == ND_POST_INC || n->kind == ND_POST_DEC;
    int addr = bf_addr(b, n->lhs, &p);
    if (b->fail) return IR_NONE;
    int raw, old = bf_load(b, addr, &p, &raw);
    int nv;
    if (p.boolean) { // C: b++ stores 1; b-- stores !b (0-1 is nonzero)
        Sc i4 = {IRT_I32, 4, false, false, false, false};
        nv = convert(b, op2(b, inc ? IR_ADD : IR_SUB, IRT_I32, old, iconst(b, IRT_I32, 1)), i4, bf_target(&p));
    } else {
        nv = op2(b, inc ? IR_ADD : IR_SUB, p.ns.t, old, iconst(b, p.ns.t, 1));
    }
    int res = bf_store(b, addr, &p, nv, raw);
    return post ? old : res;
}

// Value of an lvalue-typed node (array/function designators decay to addresses).
static int value_from_lvalue(B *b, Node *n) {
    if (n->ty && is_aggregate_value(n->ty)) return address_of(b, n);
    if (n->kind == ND_MEMBER && n->member && n->member->bit_width) {
        Bf p;
        const char *why = bf_plan(n, &p);
        if (why) return fail(b, why);
        int addr = bf_addr(b, n, &p);
        if (b->fail) return IR_NONE;
        return bf_load(b, addr, &p, NULL);
    }
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

// GCC computes on a 33..63-bit `unsigned long long x : N` in N-bit precision,
// so +, -, * and << involving a direct read of one wrap at N bits. The AST
// codegen implements this for exactly these operand shapes (direct member
// reads of an unsigned 8-byte-typed field); mirror it. 0 = no reduction.
static int bf_wide_width(Node *op) {
    if (op && op->kind == ND_MEMBER && op->member && op->member->bit_width > 32 && op->member->bit_width < 64 &&
        op->member->ty && op->member->ty->size >= 8 && op->member->ty->is_unsigned)
        return op->member->bit_width;
    return 0;
}
static int bf_reduce(B *b, int v, int bw) {
    if (!bw) return v;
    return op2(b, IR_AND, IRT_I64, v, iconst(b, IRT_I64, (int64_t)((1ULL << bw) - 1)));
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
    int r = op2(b, op, s.t, x, y);
    if (s.t == IRT_I64 && (n->kind == ND_ADD || n->kind == ND_SUB || n->kind == ND_MUL)) {
        int wl = bf_wide_width(n->lhs), wr = bf_wide_width(n->rhs);
        r = bf_reduce(b, r, wl > wr ? wl : wr);
    }
    return r;
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
    // The AST optimizer's strength reduction (x * 2^k -> x << k) builds the count as an untyped
    // ND_NUM; it is an int constant.
    bool untyped_count = !n->rhs->ty && n->rhs->kind == ND_NUM;
    if (untyped_count) cs = (Sc){IRT_I32, 4, false, false, false, false};
    else if (!sc_of(n->rhs->ty, &cs) || cs.flt)
        return fail(b, "shift count type");
    int x = gen_as(b, n->lhs, s);
    int c = untyped_count ? iconst(b, IRT_I32, n->rhs->val) : gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    if (cs.t == IRT_I64) c = op1(b, IR_TRUNC, IRT_I32, c);
    IROp op = n->kind == ND_SHL ? IR_SHL : (s.uns ? IR_SHR : IR_SAR);
    int r = norm(b, op2(b, op, s.t, x, c), s);
    if (n->kind == ND_SHL && s.t == IRT_I64) r = bf_reduce(b, r, bf_wide_width(n->lhs));
    return r;
}

// Store `val` (already of the lvalue's C type) through the lvalue `lhs`; for SSA
// variables write the variable. Returns the stored (normalized) value.
static int assign_to(B *b, Node *lhs, int val) {
    Sc s;
    if (!sc_of(lhs->ty, &s)) return fail(b, "assign type");
    if (lhs->kind == ND_LVAR && lhs->var && !lv_is_global(lhs->var)) {
        LvInfo *e = lv_info(b, lhs->var);
        if (e->ssa >= 0) {
            write_var(b, e->ssa, cur_block(b), val);
            return val;
        }
    }
    int addr = address_of(b, lhs);
    if (b->fail) return IR_NONE;
    emit_store(b, mem_of(s), addr, val, ty_volatile(lhs->ty));
    return val;
}

// Copy `size` bytes from src to dst (both I64 addresses): unrolled
// exact-width loads/stores, or a memcpy call for large copies. Struct copies
// may not overlap in defined programs, so a forward copy is fine.
static void gen_copy_bytes(B *b, int dst, int src, int64_t size) {
    if (size <= 0 || b->fail) return;
    if (size > 256) {
        IRInstr *c = emit(b, IR_CALL, IRT_VOID);
        c->sym = "memcpy";
        c->nargs = 3;
        c->args = xcalloc(3, sizeof(int32_t));
        c->args[0] = dst;
        c->args[1] = src;
        c->args[2] = iconst(b, IRT_I64, size);
        b->f->has_calls = true;
        return;
    }
    static const struct {
        int bytes;
        IRMem mw;
    } w[] = {{8, IRM_I64}, {4, IRM_I32}, {2, IRM_I16}, {1, IRM_I8}};
    int64_t off = 0;
    for (int k = 0; k < 4 && !b->fail; k++) {
        while (size - off >= w[k].bytes) {
            IRInstr *li = emit(b, IR_LOAD, w[k].bytes == 8 ? IRT_I64 : IRT_I32);
            li->a = src;
            li->mw = (uint8_t)w[k].mw;
            li->imm = off;
            IRInstr *s = emit(b, IR_STORE, IRT_VOID);
            s->a = dst;
            s->c = li->id;
            s->mw = (uint8_t)w[k].mw;
            s->imm = off;
            off += w[k].bytes;
        }
    }
}

static int gen_assign(B *b, Node *n) {
    Type *lty = n->lhs->ty;
    if (n->lhs->kind == ND_MEMBER && n->lhs->member && n->lhs->member->bit_width) return gen_bf_assign(b, n);
    // struct/union copy, or whole-array assignment (GNU extension rcc allows);
    // the value is the address of lhs.
    if (lty && (lty->kind == TY_STRUCT || lty->kind == TY_UNION || lty->kind == TY_ARRAY)) {
        if (is_volatile_aggregate(lty) || is_volatile_aggregate(n->rhs->ty)) return fail(b, "volatile aggregate copy");
        int dst = address_of(b, n->lhs);
        int src = address_of(b, n->rhs);
        if (b->fail) return IR_NONE;
        gen_copy_bytes(b, dst, src, lty->size);
        return dst;
    }
    Sc ls, rs;
    if (lty && lty->kind == TY_FUNC)
        return fail(b, "function assignment"); // sc_of() models arrays as pointers; assigning copies bytes
    if (!sc_of(lty, &ls) || !sc_of(n->rhs->ty, &rs)) return fail(b, "assign type");
    int v = gen_expr(b, n->rhs);
    if (b->fail) return IR_NONE;
    v = convert(b, v, rs, ls);
    return assign_to(b, n->lhs, v);
}

static int gen_incdec(B *b, Node *n) {
    Node *lhs = n->lhs;
    if (lhs->kind == ND_MEMBER && lhs->member && lhs->member->bit_width) return gen_bf_incdec(b, n);
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
        emit_store(b, mem_of(s), addr, nv, ty_volatile(lhs->ty));
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

// abs/labs/llabs: the AST codegen expands them as builtins (never a real call),
// using the branch-free (x ^ (x >> w)) - (x >> w); do the same, in the width of
// the C prototype (int for abs, long for labs/llabs). INT_MIN wraps like the
// hardware sequence does.
static int gen_abs(B *b, Node *n, bool wide) {
    Sc as, to = wide ? (Sc){IRT_I64, 8, false, false, false, false} : (Sc){IRT_I32, 4, false, false, false, false};
    if (!n->args || n->args->next || !sc_of(n->args->ty, &as) || as.flt || as.ptr)
        return fail(b, "abs argument");
    int x = gen_as(b, n->args, to);
    if (b->fail) return IR_NONE;
    int m = op2(b, IR_SAR, to.t, x, iconst(b, IRT_I32, wide ? 63 : 31));
    return op2(b, IR_SUB, to.t, op2(b, IR_XOR, to.t, x, m), m);
}

static int gen_call(B *b, Node *n) {
    Node *callee = n->lhs;
    const char *sym = NULL;
    int target = IR_NONE;
    Type *fty = NULL;
    if (callee && callee->kind == ND_LVAR && callee->var && callee->var->is_function && !callee->var->asm_name) {
        const char *nm = callee->var->name;
        if (nm && !strcmp(nm, "abs")) return gen_abs(b, n, false);
        if (nm && (!strcmp(nm, "labs") || !strcmp(nm, "llabs"))) return gen_abs(b, n, true);
    }
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
    int32_t *arg_class = xcalloc((size_t)nargs, sizeof(int32_t));
    bool any_agg = false;
    bool unproto = !proto;
    int k = 0;
    int nnamed = 0;
    if (fty && fty->is_variadic && !fty->is_oldstyle)
        for (Type *t = fty->param_types; t; t = t->param_next) nnamed++;
    for (Node *a = n->args; a; a = a->next, k++) {
        int cls = ir_agg_class(a->ty);
        if (cls != IRAGG_NONE && IR_NO_AGG_ABI(a->ty)) { // by-value vector: ABI not modelled by this backend
            fail(b, "aggregate argument (target ABI)");
            goto fail_call;
        }
#ifdef _WIN32 // Win64 slot = argument position (hidden return pointer first); see the callee guard in ir_build()
        if (cls == IRAGG_PTR && k + ((n->ty && (n->ty->kind == TY_STRUCT || n->ty->kind == TY_UNION) && n->ty->size > 8) ? 1 : 0) >= 4) {
            fail(b, "aggregate argument on the stack (target ABI)");
            goto fail_call;
        }
#endif
        if (cls != IRAGG_NONE) { // aggregate: the argument is the address of the bytes
            if (unproto) {
                fail(b, "aggregate argument to unprototyped/variadic call");
                goto fail_call;
            }
            if (is_volatile_aggregate(a->ty)) {
                fail(b, "volatile aggregate argument");
                goto fail_call;
            }
            int v = address_of(b, a);
            if (b->fail) goto fail_call;
            args[k] = v;
            arg_class[k] = ((int32_t)a->ty->size << 3) | cls;
            any_agg = true;
            pt = pt ? pt->param_next : NULL;
            continue;
        }
        Sc as;
        if (!sc_of(a->ty, &as)) {
            fail(b, "argument type");
            goto fail_call;
        }
        int v = gen_expr(b, a);
        if (b->fail) goto fail_call;
        if (pt) { // prototyped parameter: convert to its type
            Sc ps;
            if (!sc_of(pt, &ps)) {
                fail(b, "parameter type");
                goto fail_call;
            }
            v = convert(b, v, as, ps);
            pt = pt->param_next;
        } else if (as.t == IRT_F32) { // default argument promotion
            Sc d = {IRT_F64, 8, false, true, false, false};
            v = convert(b, v, as, d);
        }
        args[k] = v;
    }

    bool agg_ret = n->ty && (n->ty->kind == TY_STRUCT || n->ty->kind == TY_UNION || n->ty->is_vector) && n->ty->size > 0;
    if (agg_ret && IR_NO_AGG_ABI(n->ty)) {
        fail(b, "aggregate return (target ABI)");
        goto fail_call;
    }
    bool is_void = !n->ty || n->ty->kind == TY_VOID;
    Sc rs = {0};
    if (!is_void && !agg_ret && !sc_of(n->ty, &rs)) {
        fail(b, "return type");
        goto fail_call;
    }
    // msign marks the I64-typed result of an unprototyped call returning int (see IR_WIDE_IMPLICIT_INT)
    bool wide_int = IR_WIDE_IMPLICIT_INT && !proto && !is_void && !agg_ret && rs.t == IRT_I32 && rs.size == 4 && !rs.boolean;
    IRInstr *c = emit(b, IR_CALL, agg_ret ? IRT_I64 : (is_void ? IRT_VOID : wide_int ? IRT_I64
                                                                                     : rs.t));
    c->msign = wide_int;
    c->sym = sym;
    c->a = target;
    c->args = args;
    c->nargs = nargs;
    c->arg_class = any_agg ? arg_class : NULL;
    if (!any_agg) free(arg_class);
    if (agg_ret) // > 0: two-reg RAX[:RDX] return; < 0: hidden return pointer
        c->ret_agg = ir_ret_two_reg(n->ty) ? (int32_t)n->ty->size : -(int32_t)n->ty->size;
    c->variadic = !fty || fty->is_variadic || fty->is_oldstyle;
    if (nnamed || (fty && fty->is_variadic && !fty->is_oldstyle)) c->var_from = nnamed + 1;
    if (!c->var_from && sym) {
        // The AST backend treats the printf family as variadic even when no (variadic)
        // prototype is visible (gen_funcall: bi_s_printf & co); Darwin passes the unnamed
        // arguments of such calls on the stack.
        int nn = !strcmp(sym, "printf") ? 1 : (!strcmp(sym, "sprintf") || !strcmp(sym, "fprintf")) ? 2
            : !strcmp(sym, "snprintf")                                                             ? 3
                                                                                                   : 0;
        if (nn) c->var_from = nn + 1;
    }
    c->noret = callee && callee->kind == ND_LVAR && callee->var && callee->var->is_function && callee->var->is_noreturn;
    b->f->has_calls = true;
    if (is_void) return IR_NONE;
    if (agg_ret) return c->id; // address of the returned bytes
    if (wide_int) return op1(b, IR_TRUNC, IRT_I32, c->id);
    return norm(b, c->id, rs); // callers may not rely on the callee normalizing narrow results
fail_call:
    free(args);
    free(arg_class);
    return IR_NONE;
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
    case ND_STMT_EXPR: { // GNU ({ ...; expr; }): the value is the last expression
        Node *last = NULL;
        for (Node *s = n->body; s; s = s->next) last = s;
        for (Node *s = n->body; s && s != last; s = s->next) gen_stmt(b, s);
        if (!last || last->kind != ND_EXPR_STMT || !last->lhs) return fail(b, "statement expression value");
        return gen_expr(b, last->lhs);
    }
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

// The block a label names; created lazily so forward gotos can target it.
// Label blocks stay unsealed until the driver seals everything at the end.
static IRBlock *label_block(B *b, const char *name, bool create) {
    for (int i = 0; i < b->nlabels; i++)
        if (!strcmp(b->label_name[i], name)) return b->label_blk[i];
    if (!create) return NULL;
    if (b->nlabels == b->caplabels) {
        b->caplabels = b->caplabels ? b->caplabels * 2 : 8;
        b->label_name = xrealloc(b->label_name, sizeof(char *) * (size_t)b->caplabels);
        b->label_blk = xrealloc(b->label_blk, sizeof(IRBlock *) * (size_t)b->caplabels);
    }
    IRBlock *blk = new_block(b);
    b->label_name[b->nlabels] = (char *)name;
    b->label_blk[b->nlabels] = blk;
    b->nlabels++;
    return blk;
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
        if (b->f->ret_agg > 0) { // aggregate return: the result is the address of the bytes
            if (n->lhs) {
                if (is_volatile_aggregate(n->lhs->ty)) {
                    fail(b, "volatile aggregate return");
                    return;
                }
                int addr = address_of(b, n->lhs); // struct rvalue = address
                if (b->fail) return;
                if (b->f->ret_sret) {
                    int rb = emit_load(b, IRM_I64, false, IRT_I64, op0(b, IR_FRAMEADDR, IRT_I64, b->retbuf_off), false);
                    gen_copy_bytes(b, rb, addr, b->f->ret_agg);
                    v = rb;
                } else {
                    v = addr; // the backend loads RAX[:RDX] from it
                }
            } else if (b->f->ret_sret) {
                v = emit_load(b, IRM_I64, false, IRT_I64, op0(b, IR_FRAMEADDR, IRT_I64, b->retbuf_off), false);
            } else {
                IRInstr *u = emit(b, IR_UNDEF, IRT_I64); // falling off: unspecified value
                v = u->id;
            }
        } else if (n->lhs) {
            Sc from, to;
            if (b->f->ret_type == IRT_VOID) {
                // Callers ignore the value, but it still lands in rax -- the
                // parser's implicit `return 0` in main defines the exit code.
                v = gen_expr(b, n->lhs);
                if (b->fail) return;
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
    case ND_SWITCH: {
        if (n->target_loop || n->label_name) {
            fail(b, "labeled switch");
            return;
        }
        gen_stmts(b, n->init);
        Sc cs;
        if (!sc_of(n->cond->ty, &cs) || cs.flt || cs.boolean) {
            fail(b, "switch type");
            return;
        }
        int v = gen_expr(b, n->cond);
        if (b->fail) return;
        int nc = 0;
        for (Node *c = n->case_next; c; c = c->case_next) nc++;
        IRBlock **cblk = xcalloc((size_t)(nc ? nc : 1), sizeof(IRBlock *));
        {
            int i = 0;
            for (Node *c = n->case_next; c; c = c->case_next, i++) cblk[i] = new_block(b);
        }
        IRBlock *dflt = n->default_case ? new_block(b) : NULL;
        IRBlock *end = new_block(b);
        if (nc == 0) { // switch with only a default
            jump(b, dflt ? dflt : end);
        } else {
            // Linear compare chain (jump tables/binary search would need
            // backend support; keep the semantics, revisit hot switches later).
            int i = 0;
            for (Node *c = n->case_next; c; c = c->case_next, i++) {
                IRBlock *nxt = (i + 1 < nc) ? new_block(b) : (dflt ? dflt : end);
                int take;
                if (c->is_case_range) { // case lo ... hi
                    IRCC ge = cs.uns ? IRCC_UGE : IRCC_SGE;
                    IRCC le = cs.uns ? IRCC_ULE : IRCC_SLE;
                    int g = cmp_i(b, ge, v, iconst(b, cs.t, c->case_val));
                    int l = cmp_i(b, le, v, iconst(b, cs.t, c->case_end));
                    take = op2(b, IR_AND, IRT_I32, g, l);
                } else {
                    take = cmp_i(b, IRCC_EQ, v, iconst(b, cs.t, c->case_val));
                }
                branch(b, take, cblk[i], nxt);
                if (i + 1 < nc) {
                    seal_block(b, nxt); // exactly one predecessor: this compare
                    start_block(b, nxt);
                }
            }
            b->cur_blk = NULL; // the chain's last block already jumped away
        }
        // Generate the bodies; ND_CASE statements (wherever they nest) start
        // their blocks, wiring fallthrough from the previous body.
        if (b->swdepth == b->capsw) {
            b->capsw = b->capsw ? b->capsw * 2 : 4;
            b->sw = xrealloc(b->sw, sizeof(SwCtx) * (size_t)b->capsw);
        }
        SwCtx *sx = &b->sw[b->swdepth++];
        sx->node = n;
        sx->cblk = cblk;
        sx->nc = nc;
        sx->dflt = dflt;
        IRBlock *outer_cont = b->depth ? b->cont[b->depth - 1] : NULL;
        push_loop(b, end, outer_cont); // continue inside a switch targets the enclosing loop
        gen_stmts(b, n->then);
        b->depth--;
        b->swdepth--;
        free(cblk);
        if (b->cur_blk) jump(b, end);
        if (dflt) seal_block(b, dflt);
        seal_block(b, end);
        start_block(b, end);
        return;
    }
    case ND_CASE: {
        SwCtx *sx = b->swdepth ? &b->sw[b->swdepth - 1] : NULL;
        IRBlock *blk = NULL;
        if (sx) {
            if (n == sx->node->default_case) {
                blk = sx->dflt;
            } else {
                int i = 0;
                for (Node *c = sx->node->case_next; c; c = c->case_next, i++)
                    if (c == n) {
                        blk = sx->cblk[i];
                        break;
                    }
            }
        }
        if (!blk) {
            fail(b, "case outside switch");
            return;
        }
        if (b->cur_blk) jump(b, blk); // fallthrough from the previous body
        seal_block(b, blk);
        start_block(b, blk);
        gen_stmt(b, n->lhs);
        return;
    }
    case ND_LABEL: {
        // NOT sealed here: forward gotos may still add predecessors; the
        // driver seals every block (and resolves the resulting phis) at the end.
        IRBlock *blk = label_block(b, n->label_name, true);
        if (b->cur_blk) jump(b, blk);
        start_block(b, blk);
        if (n->lhs) gen_stmt(b, n->lhs);
        return;
    }
    case ND_GOTO:
        if (!n->label_name) {
            fail(b, "computed goto");
            return;
        }
        jump(b, label_block(b, n->label_name, true));
        return;
    case ND_BREAK:
        if (n->target_loop || !b->depth) {
            fail(b, "labeled break");
            return;
        }
        jump(b, b->brk[b->depth - 1]);
        return;
    case ND_CONTINUE:
        if (n->target_loop || !b->depth || !b->cont[b->depth - 1]) {
            fail(b, "stray or labeled continue");
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
    free(b->sw);
    free(b->label_name);
    free(b->label_blk);
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
        else if ((rt->kind == TY_STRUCT || rt->kind == TY_UNION || rt->is_vector) && rt->size > 0 && IR_NO_AGG_ABI(rt)) {
            fail(b, "aggregate return (target ABI)");
            ok = false;
        } else if ((rt->kind == TY_STRUCT || rt->kind == TY_UNION || rt->is_vector) && rt->size > 0) {
            f->ret_agg = (int32_t)rt->size;
            f->ret_sret = !ir_ret_two_reg(rt); // two-reg RAX[:RDX] vs hidden return pointer
            if (f->ret_sret) { // the parser created a __retbuf local holding the hidden pointer
                for (LVar *v = fn->locals; v; v = v->next)
                    if (v->name && !strcmp(v->name, "__retbuf")) {
                        b->retbuf_off = v->offset;
                        break;
                    }
                if (!b->retbuf_off) {
                    fail(b, "missing __retbuf");
                    ok = false;
                }
            }
        } else {
            fail(b, "return type");
            ok = false;
        }
    }
    // Parameters: scalars and aggregates per ir_agg_class().
    int nparams = 0;
    if (ok)
        for (LVar *p = fn->params; p; p = p->param_next) {
            if (ir_agg_class(p->ty) != IRAGG_NONE && IR_NO_AGG_ABI(p->ty)) {
                fail(b, "aggregate parameter (target ABI)");
                ok = false;
                break;
            }
            if (ir_agg_class(p->ty) != IRAGG_NONE) {
#ifdef _WIN32 // Win64 slot = argument position; the AST callee reads a by-pointer aggregate that overflows into the stack as raw bytes
                if (ir_agg_class(p->ty) == IRAGG_PTR && nparams + (f->ret_sret ? 1 : 0) >= 4) {
                    fail(b, "aggregate parameter on the stack (target ABI)");
                    ok = false;
                    break;
                }
#endif
                nparams++;
                continue;
            }
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
    int total_params = nparams + (f->ret_sret ? 1 : 0);
    f->nparams = total_params;
    f->param_types = xcalloc((size_t)(total_params ? total_params : 1), sizeof(IRType));
    f->param_class = xcalloc((size_t)(total_params ? total_params : 1), sizeof(int32_t));
    int idx = 0;
    if (f->ret_sret) { // hidden return buffer pointer, stored into __retbuf like the AST prologue
        f->param_types[0] = IRT_I64;
        int pv = op0(b, IR_PARAM, IRT_I64, 0);
        emit_store(b, IRM_I64, op0(b, IR_FRAMEADDR, IRT_I64, b->retbuf_off), pv, false);
        idx = 1;
    }
    for (LVar *p = fn->params; p; p = p->param_next, idx++) {
        int cls = ir_agg_class(p->ty);
        if (cls != IRAGG_NONE) { // the param value is the address of the incoming bytes
            int pv = op0(b, IR_PARAM, IRT_I64, idx);
            f->param_types[idx] = IRT_I64;
            f->param_class[idx] = ((int32_t)p->ty->size << 3) | cls;
            // Give the local its own storage (the incoming bytes may live in the
            // caller's outgoing stack area, which the callee must not modify).
            gen_copy_bytes(b, op0(b, IR_FRAMEADDR, IRT_I64, p->offset), pv, p->ty->size);
            continue;
        }
        Sc ps;
        sc_of(p->ty, &ps);
        f->param_types[idx] = ps.t;
        int pv = op0(b, IR_PARAM, ps.t, idx);
        pv = norm(b, pv, ps); // callers may leave the high bits of narrow args undefined
        LvInfo *e = lv_info(b, p);
        if (e->ssa >= 0) write_var(b, e->ssa, entry, pv);
        else
            emit_store(b, mem_of(ps), op0(b, IR_FRAMEADDR, IRT_I64, p->offset), pv, false);
    }

    gen_stmts(b, fn->body);

    if (!b->fail && b->cur_blk) { // fell off the end
        // Falling off the end: main returns 0 (C99 5.1.2.2.3, applied to void
        // main too -- the exit status would otherwise be whatever rax happened
        // to hold). Everywhere else the value is unspecified.
        bool is_main = fn->name && !strcmp(fn->name, "main");
        int v;
        if (f->ret_agg > 0) {
            if (f->ret_sret)
                v = emit_load(b, IRM_I64, false, IRT_I64, op0(b, IR_FRAMEADDR, IRT_I64, b->retbuf_off), false);
            else {
                IRInstr *u = emit(b, IR_UNDEF, IRT_I64);
                v = u->id;
            }
        } else if (f->ret_type == IRT_VOID) {
            v = is_main ? iconst(b, IRT_I32, 0) : IR_NONE;
        } else {
            v = zero_of(b, f->ret_type);
        }
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
