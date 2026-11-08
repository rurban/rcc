// ir_core.c -- IR construction helpers, CFG analyses, dump and verifier.
#include "ir.h"

#define GROW(arr, n, cap) \
    do { \
        if ((n) >= (cap)) { \
            (cap) = (cap) ? (cap) * 2 : 8; \
            (arr) = xrealloc((arr), sizeof(*(arr)) * (size_t)(cap)); \
        } \
    } while (0)

IRFunc *ir_func_new(Function *fn) {
    IRFunc *f = xcalloc(1, sizeof *f);
    f->fn = fn;
    return f;
}

void ir_func_free(IRFunc *f) {
    if (!f) return;
    for (int i = 0; i < f->nvals; i++) {
        free(f->vals[i]->args);
        free(f->vals[i]->arg_class);
        free(f->vals[i]);
    }
    free(f->vals);
    for (int i = 0; i < f->nblocks; i++) {
        IRBlock *b = f->blocks[i];
        free(b->phis);
        free(b->ins);
        free(b->preds);
        free(b);
    }
    free(f->blocks);
    free(f->param_types);
    free(f->param_class);
    free(f);
}

IRBlock *ir_block_new(IRFunc *f) {
    IRBlock *b = xcalloc(1, sizeof *b);
    b->id = f->nblocks;
    GROW(f->blocks, f->nblocks, f->capblocks);
    f->blocks[f->nblocks++] = b;
    return b;
}

static const char **dbg_files;
static int n_dbg_files, cap_dbg_files;

int ir_dbg_file_idx(const char *filename) {
    for (int i = 0; i < n_dbg_files; i++)
        if (dbg_files[i] == filename) return i + 1;
    if (n_dbg_files >= 2047) return 0; // IR_LOC keeps 11 bits of file index
    if (n_dbg_files == cap_dbg_files) {
        cap_dbg_files = cap_dbg_files ? cap_dbg_files * 2 : 16;
        dbg_files = xrealloc(dbg_files, (size_t)cap_dbg_files * sizeof *dbg_files);
    }
    dbg_files[n_dbg_files++] = filename;
    return n_dbg_files;
}

const char *ir_dbg_file(int idx) { return idx >= 1 && idx <= n_dbg_files ? dbg_files[idx - 1] : NULL; }

IRInstr *ir_instr_new(IRFunc *f, IROp op, IRType type) {
    IRInstr *i = xcalloc(1, sizeof *i);
    i->op = (uint8_t)op;
    i->type = (uint8_t)type;
    i->a = i->b = i->c = IR_NONE;
    i->scale = 1;
    i->id = f->nvals;
    GROW(f->vals, f->nvals, f->capvals);
    f->vals[f->nvals++] = i;
    return i;
}

void ir_block_append(IRBlock *b, IRInstr *i) {
    GROW(b->ins, b->nins, b->capins);
    b->ins[b->nins++] = i;
    i->blk = b;
}

void ir_block_add_phi(IRBlock *b, IRInstr *phi) {
    GROW(b->phis, b->nphis, b->capphis);
    b->phis[b->nphis++] = phi;
    phi->blk = b;
}

void ir_block_prepend(IRBlock *b, IRInstr *i) {
    GROW(b->ins, b->nins, b->capins);
    memmove(b->ins + 1, b->ins, sizeof(*b->ins) * (size_t)b->nins);
    b->ins[0] = i;
    b->nins++;
    i->blk = b;
}

void ir_block_remove(IRBlock *b, IRInstr *i) {
    for (int k = 0; k < b->nphis; k++)
        if (b->phis[k] == i) {
            memmove(b->phis + k, b->phis + k + 1, sizeof(*b->phis) * (size_t)(b->nphis - k - 1));
            b->nphis--;
            goto done;
        }
    for (int k = 0; k < b->nins; k++)
        if (b->ins[k] == i) {
            memmove(b->ins + k, b->ins + k + 1, sizeof(*b->ins) * (size_t)(b->nins - k - 1));
            b->nins--;
            goto done;
        }
done:
    i->op = IR_NOP;
    i->type = IRT_VOID;
    i->a = i->b = i->c = IR_NONE;
    i->blk = NULL;
}

void ir_add_pred(IRBlock *b, IRBlock *pred) {
    GROW(b->preds, b->npreds, b->cappreds);
    b->preds[b->npreds++] = pred;
}

static inline void repl(int32_t *x, int from, int to) {
    if (*x == from) *x = to;
}

void ir_replace_uses(IRFunc *f, int from, int to) {
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            repl(&i->a, from, to);
            repl(&i->b, from, to);
            repl(&i->c, from, to);
            for (int j = 0; j < i->nargs; j++) repl(&i->args[j], from, to);
        }
    }
}

bool ir_is_terminator(IROp op) {
    return op == IR_JMP || op == IR_BR || op == IR_RET || op == IR_UNREACHABLE;
}

bool ir_has_side_effects(const IRInstr *i) {
    switch (i->op) {
    case IR_STORE:
    case IR_CALL:
    case IR_SDIV:
    case IR_UDIV:
    case IR_SREM:
    case IR_UREM:
        return true;
    case IR_LOAD:
        return i->vol != 0; // a volatile read is observable: never removable, never a sink/fold bystander
    default:
        return ir_is_terminator((IROp)i->op);
    }
}

int ir_succs(IRBlock *b, IRBlock *out[2]) {
    if (b->nins == 0) return 0;
    IRInstr *t = b->ins[b->nins - 1];
    switch (t->op) {
    case IR_JMP:
        out[0] = t->t;
        return 1;
    case IR_BR:
        out[0] = t->t;
        out[1] = t->f;
        return 2;
    default:
        return 0;
    }
}

void ir_remove_pred_edge(IRBlock *b, IRBlock *pred) {
    int idx = -1;
    for (int k = 0; k < b->npreds; k++)
        if (b->preds[k] == pred) {
            idx = k;
            break;
        }
    if (idx < 0) return;
    memmove(b->preds + idx, b->preds + idx + 1, sizeof(*b->preds) * (size_t)(b->npreds - idx - 1));
    b->npreds--;
    for (int k = 0; k < b->nphis; k++) {
        IRInstr *p = b->phis[k];
        memmove(p->args + idx, p->args + idx + 1, sizeof(*p->args) * (size_t)(p->nargs - idx - 1));
        p->nargs--;
    }
}

// ------------------------------------------------------------------ analyses

int ir_rpo(IRFunc *f, IRBlock **out) {
    int n = f->nblocks;
    if (n == 0) return 0;
    unsigned char *seen = xcalloc((size_t)n, 1);
    IRBlock **stack = xmalloc(sizeof(*stack) * (size_t)n);
    int *next = xcalloc((size_t)n, sizeof(int));
    int sp = 0, cnt = 0;
    IRBlock **post = xmalloc(sizeof(*post) * (size_t)n);
    stack[sp++] = f->blocks[0];
    seen[0] = 1;
    while (sp) {
        IRBlock *b = stack[sp - 1];
        IRBlock *s[2];
        int ns = ir_succs(b, s);
        if (next[b->id] < ns) {
            IRBlock *c = s[next[b->id]++];
            if (!seen[c->id]) {
                seen[c->id] = 1;
                stack[sp++] = c;
            }
        } else {
            post[cnt++] = b;
            sp--;
        }
    }
    for (int i = 0; i < cnt; i++) out[i] = post[cnt - 1 - i];
    free(seen);
    free(stack);
    free(next);
    free(post);
    return cnt;
}

void ir_analyze_loops(IRFunc *f) {
    int n = f->nblocks;
    for (int i = 0; i < n; i++) f->blocks[i]->loop_depth = 0;
    if (n == 0) return;
    // DFS colors to find back edges (edge to a block still on the DFS stack).
    unsigned char *color = xcalloc((size_t)n, 1); // 0 white, 1 gray, 2 black
    IRBlock **stack = xmalloc(sizeof(*stack) * (size_t)n);
    int *next = xcalloc((size_t)n, sizeof(int));
    // back_src[h] = list of latches for header h
    int **latch = xcalloc((size_t)n, sizeof(int *));
    int *nlatch = xcalloc((size_t)n, sizeof(int));
    int sp = 0;
    stack[sp++] = f->blocks[0];
    color[0] = 1;
    while (sp) {
        IRBlock *b = stack[sp - 1];
        IRBlock *s[2];
        int ns = ir_succs(b, s);
        if (next[b->id] < ns) {
            IRBlock *c = s[next[b->id]++];
            if (color[c->id] == 0) {
                color[c->id] = 1;
                stack[sp++] = c;
            } else if (color[c->id] == 1) {
                latch[c->id] = xrealloc(latch[c->id], sizeof(int) * (size_t)(nlatch[c->id] + 1));
                latch[c->id][nlatch[c->id]++] = b->id;
            }
        } else {
            color[b->id] = 2;
            sp--;
        }
    }
    unsigned char *in = xmalloc((size_t)n);
    int *work = xmalloc(sizeof(int) * (size_t)n);
    for (int h = 0; h < n; h++) {
        if (!nlatch[h]) continue;
        memset(in, 0, (size_t)n);
        int wp = 0;
        in[h] = 1;
        for (int k = 0; k < nlatch[h]; k++) {
            int l = latch[h][k];
            if (!in[l]) {
                in[l] = 1;
                work[wp++] = l;
            }
        }
        while (wp) {
            IRBlock *x = f->blocks[work[--wp]];
            for (int k = 0; k < x->npreds; k++) {
                int p = x->preds[k]->id;
                if (!in[p]) {
                    in[p] = 1;
                    work[wp++] = p;
                }
            }
        }
        for (int i = 0; i < n; i++)
            if (in[i]) f->blocks[i]->loop_depth++;
    }
    for (int h = 0; h < n; h++) free(latch[h]);
    free(latch);
    free(nlatch);
    free(color);
    free(stack);
    free(next);
    free(in);
    free(work);
}

// ---------------------------------------------------------------------- dump

static const char *const op_names[IR_NUM_OPS] = {
    [IR_ICONST] = "iconst",
    [IR_FCONST] = "fconst",
    [IR_PARAM] = "param",
    [IR_FRAMEADDR] = "frameaddr",
    [IR_GADDR] = "gaddr",
    [IR_UNDEF] = "undef",
    [IR_ADD] = "add",
    [IR_SUB] = "sub",
    [IR_MUL] = "mul",
    [IR_SDIV] = "sdiv",
    [IR_UDIV] = "udiv",
    [IR_SREM] = "srem",
    [IR_UREM] = "urem",
    [IR_AND] = "and",
    [IR_OR] = "or",
    [IR_XOR] = "xor",
    [IR_SHL] = "shl",
    [IR_SHR] = "shr",
    [IR_SAR] = "sar",
    [IR_NEG] = "neg",
    [IR_NOT] = "not",
    [IR_FADD] = "fadd",
    [IR_FSUB] = "fsub",
    [IR_FMUL] = "fmul",
    [IR_FDIV] = "fdiv",
    [IR_FNEG] = "fneg",
    [IR_CMP] = "cmp",
    [IR_FCMP] = "fcmp",
    [IR_SEXT8] = "sext8",
    [IR_SEXT16] = "sext16",
    [IR_ZEXT8] = "zext8",
    [IR_ZEXT16] = "zext16",
    [IR_SEXT32] = "sext32",
    [IR_ZEXT32] = "zext32",
    [IR_TRUNC] = "trunc",
    [IR_SITOF] = "sitof",
    [IR_UITOF] = "uitof",
    [IR_FTOSI] = "ftosi",
    [IR_FTOU64] = "ftou64",
    [IR_FEXT] = "fext",
    [IR_FTRUNC] = "ftrunc",
    [IR_LOAD] = "load",
    [IR_STORE] = "store",
    [IR_CALL] = "call",
    [IR_PHI] = "phi",
    [IR_JMP] = "jmp",
    [IR_BR] = "br",
    [IR_RET] = "ret",
    [IR_UNREACHABLE] = "unreachable",
    [IR_NOP] = "nop",
};
static const char *const type_names[] = {"void", "i32", "i64", "f32", "f64"};
static const char *const cc_names[] = {"eq", "ne", "slt", "sle", "sgt", "sge", "ult", "ule", "ugt",
                                       "uge", "feq", "fne", "flt", "fle", "fgt", "fge"};
static const char *const mem_names[] = {"i8", "i16", "i32", "i64", "f32", "f64"};

static void dump_instr(IRInstr *i, FILE *o) {
    if (i->type != IRT_VOID) fprintf(o, "  v%d = ", i->id);
    else
        fprintf(o, "  ");
    fprintf(o, "%s", op_names[i->op] ? op_names[i->op] : "?");
    if (i->type != IRT_VOID) fprintf(o, ".%s", type_names[i->type]);
    switch (i->op) {
    case IR_CMP:
    case IR_FCMP:
        fprintf(o, " %s", cc_names[i->cc]);
        break;
    case IR_LOAD:
    case IR_STORE:
        fprintf(o, " %s%s%s", i->vol ? "volatile " : "", mem_names[i->mw],
                (i->op == IR_LOAD && i->mw < IRM_I32) ? (i->msign ? "s" : "u") : "");
        break;
    default:
        break;
    }
    if (i->a != IR_NONE) fprintf(o, " v%d", i->a);
    if (i->b != IR_NONE) fprintf(o, ", v%d", i->b);
    if (i->c != IR_NONE) fprintf(o, ", v%d", i->c);
    if (i->op == IR_ICONST || i->op == IR_PARAM || i->op == IR_FRAMEADDR) fprintf(o, " %lld", (long long)i->imm);
    if (i->op == IR_FCONST) fprintf(o, " %g", i->fimm);
    if (i->op == IR_GADDR) fprintf(o, " %s%+lld", i->sym, (long long)i->imm);
    if (i->op == IR_LOAD || i->op == IR_STORE) {
        if (i->b != IR_NONE) fprintf(o, " *%d", i->scale);
        if (i->imm) fprintf(o, " %+lld", (long long)i->imm);
    }
    if (i->op == IR_CALL) {
        if (i->sym) fprintf(o, " %s", i->sym);
        fprintf(o, " (");
        for (int k = 0; k < i->nargs; k++) {
            if (i->arg_class && i->arg_class[k])
                fprintf(o, "%sagg%d:v%d", k ? ", " : "", i->arg_class[k], i->args[k]);
            else
                fprintf(o, "%sv%d", k ? ", " : "", i->args[k]);
        }
        fprintf(o, ")%s%s", i->variadic ? " variadic" : "", i->ret_agg ? " aggret" : "");
    }
    if (i->op == IR_PHI) {
        fprintf(o, " [");
        for (int k = 0; k < i->nargs; k++) {
            IRBlock *p = i->blk && k < i->blk->npreds ? i->blk->preds[k] : NULL;
            fprintf(o, "%sb%d:v%d", k ? ", " : "", p ? p->id : -1, i->args[k]);
        }
        fprintf(o, "]");
    }
    if (i->op == IR_JMP) fprintf(o, " b%d", i->t ? i->t->id : -1);
    if (i->op == IR_BR) fprintf(o, " ? b%d : b%d", i->t ? i->t->id : -1, i->f ? i->f->id : -1);
    fprintf(o, "\n");
}

void ir_dump(IRFunc *f, FILE *o) {
    fprintf(o, "function %s (", f->fn ? f->fn->name : "?");
    for (int k = 0; k < f->nparams; k++) fprintf(o, "%s%s", k ? ", " : "", type_names[f->param_types[k]]);
    fprintf(o, ") -> %s\n", type_names[f->ret_type]);
    for (int bi = 0; bi < f->nblocks; bi++) {
        IRBlock *b = f->blocks[bi];
        fprintf(o, "b%d:  ; preds:", b->id);
        for (int k = 0; k < b->npreds; k++) fprintf(o, " b%d", b->preds[k]->id);
        fprintf(o, "  depth=%d\n", b->loop_depth);
        for (int k = 0; k < b->nphis; k++) dump_instr(b->phis[k], o);
        for (int k = 0; k < b->nins; k++) dump_instr(b->ins[k], o);
    }
}

// -------------------------------------------------------------------- verify

#define VFAIL(...) \
    do { \
        fprintf(stderr, "ir_verify(%s): ", f->fn ? f->fn->name : "?"); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        ir_dump(f, stderr); \
        abort(); \
    } while (0)

static IRInstr *vdef(IRFunc *f, IRInstr *user, int32_t id) {
    if (id < 0 || id >= f->nvals) VFAIL("v%d uses out-of-range operand %d", user->id, id);
    IRInstr *d = f->vals[id];
    if (!d->blk) VFAIL("v%d uses dead/unlinked v%d", user->id, id);
    if (d->type == IRT_VOID) VFAIL("v%d uses void v%d", user->id, id);
    return d;
}

static int is_int(int t) { return t == IRT_I32 || t == IRT_I64; }
static int is_flt(int t) { return t == IRT_F32 || t == IRT_F64; }

void ir_verify(IRFunc *f) {
    int n = f->nblocks;
    if (n <= 0) VFAIL("no blocks");
    if (f->blocks[0]->npreds != 0) VFAIL("entry block has predecessors");
    // structure
    for (int bi = 0; bi < n; bi++) {
        IRBlock *b = f->blocks[bi];
        if (b->nins == 0) VFAIL("b%d is empty", b->id);
        if (!ir_is_terminator((IROp)b->ins[b->nins - 1]->op)) VFAIL("b%d lacks a terminator", b->id);
        for (int k = 0; k < b->nins - 1; k++)
            if (ir_is_terminator((IROp)b->ins[k]->op)) VFAIL("b%d has a terminator mid-block", b->id);
        for (int k = 0; k < b->nphis; k++) {
            IRInstr *p = b->phis[k];
            if (p->op != IR_PHI) VFAIL("b%d phi list holds non-phi v%d", b->id, p->id);
            if (p->nargs != b->npreds) VFAIL("v%d phi arity %d != preds %d", p->id, p->nargs, b->npreds);
        }
        IRBlock *s[2];
        int ns = ir_succs(b, s);
        for (int k = 0; k < ns; k++) {
            int found = 0;
            for (int j = 0; j < s[k]->npreds; j++)
                if (s[k]->preds[j] == b) found++;
            if (!found) VFAIL("edge b%d->b%d missing from preds", b->id, s[k]->id);
        }
        for (int k = 0; k < b->npreds; k++) {
            IRBlock *s2[2];
            int n2 = ir_succs(b->preds[k], s2), ok = 0;
            for (int j = 0; j < n2; j++)
                if (s2[j] == b) ok = 1;
            if (!ok) VFAIL("pred b%d of b%d has no such edge", b->preds[k]->id, b->id);
        }
    }
    // dominators (Cooper/Harvey/Kennedy) over reachable blocks
    IRBlock **rpo = xmalloc(sizeof(*rpo) * (size_t)n);
    int cnt = ir_rpo(f, rpo);
    int *num = xmalloc(sizeof(int) * (size_t)n);
    int *idom = xmalloc(sizeof(int) * (size_t)n);
    for (int i = 0; i < n; i++) num[i] = idom[i] = -1;
    for (int i = 0; i < cnt; i++) num[rpo[i]->id] = i;
    idom[rpo[0]->id] = rpo[0]->id;
    for (int changed = 1; changed;) {
        changed = 0;
        for (int i = 1; i < cnt; i++) {
            IRBlock *b = rpo[i];
            int nd = -1;
            for (int k = 0; k < b->npreds; k++) {
                int p = b->preds[k]->id;
                if (num[p] < 0 || idom[p] < 0) continue;
                if (nd < 0) {
                    nd = p;
                    continue;
                }
                int x = p, y = nd;
                while (x != y) {
                    while (num[x] > num[y]) x = idom[x];
                    while (num[y] > num[x]) y = idom[y];
                }
                nd = x;
            }
            if (nd >= 0 && idom[b->id] != nd) {
                idom[b->id] = nd;
                changed = 1;
            }
        }
    }
#define DOMINATES(da, db) /* block ids */ ({ \
        int _x = (db), _r = 0; \
        while (1) { \
            if (_x == (da)) { _r = 1; break; } \
            if (idom[_x] == _x) break; \
            _x = idom[_x]; \
        } \
        _r; })
    // operand checks
    for (int bi = 0; bi < n; bi++) {
        IRBlock *b = f->blocks[bi];
        if (num[b->id] < 0) continue; // unreachable blocks are tolerated until cleanup
        for (int k = 0; k < b->nphis + b->nins; k++) {
            IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            int32_t ops[3] = {i->a, i->b, i->c};
            if (i->vol && i->op != IR_LOAD && i->op != IR_STORE) VFAIL("v%d volatile flag on a non-memory op", i->id);
            if (i->op == IR_PHI) {
                for (int j = 0; j < i->nargs; j++) {
                    IRInstr *d = vdef(f, i, i->args[j]);
                    if (d->type != i->type) VFAIL("v%d phi arg v%d type mismatch", i->id, d->id);
                    int pb = b->preds[j]->id;
                    if (num[pb] >= 0 && !DOMINATES(d->blk->id, pb))
                        VFAIL("v%d phi arg v%d does not dominate pred b%d", i->id, d->id, pb);
                }
                continue;
            }
            for (int j = 0; j < 3; j++) {
                if (ops[j] == IR_NONE) continue;
                IRInstr *d = vdef(f, i, ops[j]);
                if (d->blk == b) {
                    // same block: def must come earlier (phis count as earliest)
                    int dpos = -1, upos = -1;
                    for (int m = 0; m < b->nins; m++) {
                        if (b->ins[m] == d) dpos = m;
                        if (b->ins[m] == i) upos = m;
                    }
                    if (d->op != IR_PHI && dpos >= upos) VFAIL("v%d uses v%d before its definition", i->id, d->id);
                } else if (!DOMINATES(d->blk->id, b->id)) {
                    VFAIL("def v%d (b%d) does not dominate use v%d (b%d)", d->id, d->blk->id, i->id, b->id);
                }
            }
            for (int j = 0; j < i->nargs; j++) {
                IRInstr *d = vdef(f, i, i->args[j]);
                if (d->blk != b && !DOMINATES(d->blk->id, b->id))
                    VFAIL("call arg v%d does not dominate v%d", d->id, i->id);
            }
            // light type checks
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
                if (!is_int(i->type) || f->vals[i->a]->type != i->type || f->vals[i->b]->type != i->type)
                    VFAIL("v%d int binop type mismatch", i->id);
                break;
            case IR_SHL:
            case IR_SHR:
            case IR_SAR:
                if (!is_int(i->type) || f->vals[i->a]->type != i->type || f->vals[i->b]->type != IRT_I32)
                    VFAIL("v%d shift type mismatch", i->id);
                break;
            case IR_FADD:
            case IR_FSUB:
            case IR_FMUL:
            case IR_FDIV:
                if (!is_flt(i->type) || f->vals[i->a]->type != i->type || f->vals[i->b]->type != i->type)
                    VFAIL("v%d float binop type mismatch", i->id);
                break;
            case IR_CMP:
                if (i->type != IRT_I32 || !is_int(f->vals[i->a]->type) || f->vals[i->a]->type != f->vals[i->b]->type)
                    VFAIL("v%d cmp type mismatch", i->id);
                break;
            case IR_FCMP:
                if (i->type != IRT_I32 || !is_flt(f->vals[i->a]->type) || f->vals[i->a]->type != f->vals[i->b]->type)
                    VFAIL("v%d fcmp type mismatch", i->id);
                break;
            case IR_BR:
                if (f->vals[i->a]->type != IRT_I32) VFAIL("v%d br condition not i32", i->id);
                break;
            case IR_LOAD:
                if (f->vals[i->a]->type != IRT_I64) VFAIL("v%d load base not i64", i->id);
                if (i->b != IR_NONE && f->vals[i->b]->type != IRT_I64) VFAIL("v%d load index not i64", i->id);
                break;
            case IR_STORE:
                if (f->vals[i->a]->type != IRT_I64) VFAIL("v%d store base not i64", i->id);
                if (i->b != IR_NONE && f->vals[i->b]->type != IRT_I64) VFAIL("v%d store index not i64", i->id);
                if (i->c == IR_NONE) VFAIL("v%d store lacks a value", i->id);
                break;
            default:
                break;
            }
        }
    }
#undef DOMINATES
    free(rpo);
    free(num);
    free(idom);
}

// ------------------------------------------------------------------ ABI

// True when no float/double/long-double/complex leaf occurs anywhere in the
// (recursive) layout. Mirrors codegen.c's type_is_all_integer() EXACTLY,
// including the union merge rule: a union is all-integer when ANY member is
// (INTEGER dominates SSE in the eightbyte merge).
#ifndef _WIN32 // Win64 classes ignore the member kinds
static bool ir_type_all_integer(Type *ty) {
    if (!ty) return true;
    switch (ty->kind) {
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
    case TY_COMPLEX:
        return false;
    case TY_ARRAY:
        return ir_type_all_integer(ty->base);
    case TY_STRUCT:
        for (Member *m = ty->members; m; m = m->next)
            if (!ir_type_all_integer(m->ty)) return false;
        return true;
    case TY_UNION:
        for (Member *m = ty->members; m; m = m->next)
            if (ir_type_all_integer(m->ty)) return true;
        return false;
    default:
        return true;
    }
}
#endif

#if defined(ARCH_ARM64) || defined(_WIN32)
// AArch64 (mirrors the AST backend's simplified AAPCS64, gen_funcall / the prologue): a
// homogeneous float aggregate (struct of 1-4 floats or doubles, nested structs allowed) goes in
// V registers; any other struct/union <= 8 bytes is a raw value in one GP register; everything
// larger is passed by pointer (the callee copies the pointee). Vectors are not modelled.
// Win64: no HFAs; struct/union <= 8 bytes is a raw value in one slot, larger ones are passed
// by pointer, exactly as the AST backend's gen_funcall / prologue do.
int ir_agg_class(Type *ty) {
    if (!ty) return IRAGG_NONE;
    int kind = ty->kind;
    bool aggregate = kind == TY_STRUCT || kind == TY_UNION || ty->is_vector;
    if (!aggregate) return IRAGG_NONE;
    int64_t size = ty->size;
    if (size <= 0) return IRAGG_PTR; // incomplete: cannot happen for real args
#ifdef ARCH_ARM64
    int elem = 0;
    if (!ty->is_vector && cg_ir_arm64_hfa(ty, &elem) > 0) return elem == 4 ? IRAGG_HFA4 : IRAGG_HFA8;
#endif
    if (size <= 8) return IRAGG_VALUE8;
    return IRAGG_PTR;
}
#else
int ir_agg_class(Type *ty) {
    if (!ty) return IRAGG_NONE;
    int kind = ty->kind;
    bool aggregate = kind == TY_STRUCT || kind == TY_UNION || ty->is_vector;
    if (!aggregate) return IRAGG_NONE;
    int64_t size = ty->size;
    if (size <= 0) return IRAGG_PTR; // incomplete: cannot happen for real args
    if (size <= 8) return IRAGG_VALUE8;
    if (size <= 16) {
        if (!ty->is_vector && ir_type_all_integer(ty)) return IRAGG_VALUE16;
        return IRAGG_PTR;
    }
    if (!ty->is_vector) return IRAGG_MEMORY;
    return IRAGG_PTR;
}
#endif

// True when `ty` (struct/union) is returned in RAX[:RDX] per the AST codegen's
// simplification (all-integer leaves, <= 16 bytes, not a vector) -- mirrors
// codegen.c's struct_returns_in_gp_regs().
bool ir_ret_two_reg(Type *ty) {
#ifdef _WIN32 // Win64: a struct/union of up to 8 bytes comes back in RAX, anything larger via the hidden pointer
    return ty && (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && !ty->is_vector && ty->size > 0 && ty->size <= 8;
#else
    return ty && (ty->kind == TY_STRUCT || ty->kind == TY_UNION) && !ty->is_vector &&
        ty->size > 0 && ty->size <= 16 && ir_type_all_integer(ty);
#endif
}
