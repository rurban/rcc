// ir_inline.c -- IR-level inlining of small same-TU callees.
//
// Runs once per caller, right after ir_build() and before ir_opt() (which
// folds the spliced constants/params, merges the split blocks and removes
// trivial phis). Only the caller's ORIGINAL direct call sites are considered:
// a spliced body is never searched for further inlining, so calls it contains
// stay calls and the pass terminates without a worklist or recursion guard.
//
// The callee IRFunc is the shared as-built template: it is read (never
// written, not even IRBlock/IRInstr scratch fields) and cloned.
//
// Splicing a call C in block B (all instrs before C stay in B):
//     B:  ...pre; jmp E'            E' = clone of the callee entry
//     X': cloned callee blocks, each `ret v` becomes `jmp K`
//     K:  [phi of the ret values]; post...; <B's old terminator>
// B's successors then see K as their predecessor (same pred slot, so their
// phi argument order is untouched). IR_PARAM(i) is replaced by C's i-th
// argument, and C's value by the single ret operand or by K's phi.
#include "ir.h"

#define MAX_CALLEE_BLOCKS 12 // reachable blocks of an inlinable callee
#define MAX_CALLEE_COST 60 // non-constant instrs, call site inside a loop
#define MAX_CALLEE_COST_COLD 25 // same, call site at loop depth 0
#define MAX_CALLER_SIZE 3000 // stop inlining into a caller beyond this many instrs

typedef struct {
    IRInstr *call;
    int loop_depth;
} Site;

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    return p;
}

static int32_t *dup_i32(const int32_t *src, int n) {
    int32_t *p = xmalloc(sizeof(*p) * (size_t)n);
    memcpy(p, src, sizeof(*p) * (size_t)n);
    return p;
}

static bool all_scalar(const int32_t *cls, int n) {
    if (!cls) return true;
    for (int i = 0; i < n; i++)
        if (cls[i]) return false;
    return true;
}

// Instrs that cost nothing to duplicate: the backend rematerializes them.
static bool is_free_op(int op) {
    return op == IR_PARAM || op == IR_ICONST || op == IR_FCONST || op == IR_UNDEF || op == IR_NOP;
}

// Decides whether `call` may be replaced by a copy of `callee`. On success
// returns true, stores the callee's reachable blocks (entry first) in rpo[]
// (caller provides callee->nblocks slots) and their count in *nr, and the
// number of instrs the clone adds in *added.
static bool inlinable(const IRFunc *caller, const IRInstr *call, IRFunc *callee, int loop_depth, IRBlock **rpo, int *nr,
                      int *added) {
    if (callee->fn == caller->fn) return false;
    if (call->variadic || call->ret_agg != 0 || !all_scalar(call->arg_class, call->nargs)) return false;
    if (callee->ret_agg != 0 || callee->ret_sret != 0) return false;
    if (callee->nparams != call->nargs || callee->ret_type != (IRType)call->type) return false;
    if (!all_scalar(callee->param_class, callee->nparams)) return false;
    for (int i = 0; i < call->nargs; i++)
        if (caller->vals[call->args[i]]->type != callee->param_types[i]) return false;

    int n = ir_rpo(callee, rpo);
    if (n > MAX_CALLEE_BLOCKS) return false;
    int limit = loop_depth > 0 ? MAX_CALLEE_COST : MAX_CALLEE_COST_COLD;
    int cost = 0, total = 0, nrets = 0;
    for (int bi = 0; bi < n; bi++) {
        IRBlock *b = rpo[bi];
        cost += b->nphis;
        total += b->nphis;
        for (int k = 0; k < b->nins; k++) {
            const IRInstr *o = b->ins[k];
            // Frame slots would need rebasing into the caller's frame.
            if (o->op == IR_FRAMEADDR) return false;
            if (o->op == IR_NOP) continue;
            total++;
            if (o->op == IR_PARAM && (o->imm < 0 || o->imm >= callee->nparams || o->type != (uint8_t)callee->param_types[o->imm]))
                return false;
            if (o->op == IR_RET) {
                // A value-less ret in a non-void callee would leave the call result undefined.
                if ((o->a == IR_NONE) != (callee->ret_type == IRT_VOID)) return false;
                if (o->a != IR_NONE && callee->vals[o->a]->type != (uint8_t)callee->ret_type) return false;
                nrets++;
            }
            if (!is_free_op(o->op)) cost++;
        }
        if (cost > limit) return false;
    }
    // A callee that never returns would leave the continuation without a result.
    if (nrets == 0) return false;
    *nr = n;
    *added = total;
    return true;
}

// Clones the callee body into `caller` in place of `call`; see the file comment.
static void splice(IRFunc *caller, IRInstr *call, const IRFunc *callee, IRBlock **rpo, int nr) {
    IRBlock *head = call->blk;
    const IRType rt = (IRType)call->type;
    const int call_id = call->id;
    const int line = call->line;

    // Split: everything after the call (including the terminator) moves to `cont`.
    int pos = 0;
    while (head->ins[pos] != call) pos++;
    IRBlock *cont = ir_block_new(caller);
    cont->loop_depth = head->loop_depth;
    for (int k = pos + 1; k < head->nins; k++) ir_block_append(cont, head->ins[k]);
    head->nins = pos + 1;
    ir_block_remove(head, call); // also drops the call from head; its id stays reserved
    IRBlock *succ[2];
    int ns = ir_succs(cont, succ);
    for (int k = 0; k < ns; k++)
        for (int j = 0; j < succ[k]->npreds; j++)
            if (succ[k]->preds[j] == head) succ[k]->preds[j] = cont; // same slot: phi args stay aligned

    // Value and block maps from callee ids to caller ids / blocks.
    int *vmap = xmalloc(sizeof(*vmap) * (size_t)callee->nvals);
    for (int i = 0; i < callee->nvals; i++) vmap[i] = IR_NONE;
    IRBlock **bmap = xmalloc(sizeof(*bmap) * (size_t)callee->nblocks);
    for (int i = 0; i < callee->nblocks; i++) bmap[i] = NULL;
    for (int bi = 0; bi < nr; bi++) {
        IRBlock *nb = ir_block_new(caller);
        nb->loop_depth = head->loop_depth;
        bmap[rpo[bi]->id] = nb;
    }
    // Predecessors that are unreachable in the callee have no clone: their edge (and phi slot) is dropped.
    for (int bi = 0; bi < nr; bi++) {
        const IRBlock *ob = rpo[bi];
        IRBlock *nb = bmap[ob->id];
        for (int j = 0; j < ob->npreds; j++)
            if (bmap[ob->preds[j]->id]) ir_add_pred(nb, bmap[ob->preds[j]->id]);
    }
    ir_add_pred(bmap[rpo[0]->id], head);
    IRInstr *jmp = ir_instr_new(caller, IR_JMP, IRT_VOID);
    jmp->t = bmap[rpo[0]->id];
    jmp->line = line;
    ir_block_append(head, jmp);

    // Pass 1: create the clones (operands are still callee ids).
    int nrets = 0;
    int32_t *ret_vals = xmalloc(sizeof(*ret_vals) * (size_t)callee->nblocks); // callee ids; one ret per block
    for (int bi = 0; bi < nr; bi++) {
        const IRBlock *ob = rpo[bi];
        IRBlock *nb = bmap[ob->id];
        for (int k = 0; k < ob->nphis + ob->nins; k++) {
            const IRInstr *o = k < ob->nphis ? ob->phis[k] : ob->ins[k - ob->nphis];
            if (o->op == IR_NOP) continue;
            if (o->op == IR_PARAM) {
                vmap[o->id] = call->args[o->imm];
                continue;
            }
            IRInstr *ni;
            if (o->op == IR_RET) {
                ret_vals[nrets++] = o->a;
                ni = ir_instr_new(caller, IR_JMP, IRT_VOID);
                ni->t = cont;
                ir_add_pred(cont, nb);
            } else {
                ni = ir_instr_new(caller, (IROp)o->op, (IRType)o->type);
                int id = ni->id;
                *ni = *o;
                ni->id = id;
                ni->aux = 0;
                ni->args = NULL;
                ni->arg_class = NULL;
                ni->nargs = 0;
                if (o->op != IR_PHI && o->nargs) {
                    ni->args = dup_i32(o->args, o->nargs);
                    ni->nargs = o->nargs;
                    if (o->arg_class) ni->arg_class = dup_i32(o->arg_class, o->nargs);
                }
            }
            ni->line = line;
            vmap[o->id] = ni->id;
            if (o->op == IR_PHI) ir_block_add_phi(nb, ni);
            else
                ir_block_append(nb, ni);
        }
    }

    // Pass 2: remap operands, branch targets and phi arguments.
    for (int bi = 0; bi < nr; bi++) {
        const IRBlock *ob = rpo[bi];
        for (int k = 0; k < ob->nphis + ob->nins; k++) {
            const IRInstr *o = k < ob->nphis ? ob->phis[k] : ob->ins[k - ob->nphis];
            if (o->op == IR_NOP || o->op == IR_PARAM || o->op == IR_RET) continue;
            IRInstr *ni = caller->vals[vmap[o->id]];
            if (o->a != IR_NONE) ni->a = vmap[o->a];
            if (o->b != IR_NONE) ni->b = vmap[o->b];
            if (o->c != IR_NONE) ni->c = vmap[o->c];
            if (o->t) ni->t = bmap[o->t->id];
            if (o->f) ni->f = bmap[o->f->id];
            if (o->op == IR_PHI) {
                IRBlock *nb = ni->blk;
                ni->args = xmalloc(sizeof(*ni->args) * (size_t)(nb->npreds ? nb->npreds : 1));
                for (int j = 0; j < ob->npreds; j++)
                    if (bmap[ob->preds[j]->id]) ni->args[ni->nargs++] = vmap[o->args[j]];
            } else {
                for (int j = 0; j < ni->nargs; j++) ni->args[j] = vmap[ni->args[j]];
            }
        }
    }

    // The call's value: the lone ret operand, or a phi over all of them in `cont`.
    if (rt != IRT_VOID) {
        int res;
        if (nrets == 1) {
            res = vmap[ret_vals[0]];
        } else {
            IRInstr *phi = ir_instr_new(caller, IR_PHI, rt);
            phi->args = xmalloc(sizeof(*phi->args) * (size_t)nrets);
            for (int k = 0; k < nrets; k++) phi->args[k] = vmap[ret_vals[k]];
            phi->nargs = nrets;
            phi->line = line;
            ir_block_add_phi(cont, phi);
            res = phi->id;
        }
        ir_replace_uses(caller, call_id, res);
    }
    free(vmap);
    free(bmap);
    free(ret_vals);
}

void ir_inline(IRFunc *caller, IRFunc *(*lookup)(const char *sym, void *ud), void *ud) {
    ir_analyze_loops(caller);
    int size = 0, nsites = 0;
    for (int bi = 0; bi < caller->nblocks; bi++) {
        const IRBlock *b = caller->blocks[bi];
        size += b->nphis + b->nins;
        for (int k = 0; k < b->nins; k++)
            if (b->ins[k]->op == IR_CALL && b->ins[k]->sym) nsites++;
    }
    if (nsites == 0) return;
    // Snapshot the original call sites: splicing splits blocks and reorders
    // ids, but each IRInstr stays valid and keeps its (updated) ->blk.
    Site *sites = xmalloc(sizeof(*sites) * (size_t)nsites);
    int n = 0;
    for (int bi = 0; bi < caller->nblocks; bi++) {
        IRBlock *b = caller->blocks[bi];
        for (int k = 0; k < b->nins; k++)
            if (b->ins[k]->op == IR_CALL && b->ins[k]->sym) sites[n++] = (Site){b->ins[k], b->loop_depth};
    }

    IRBlock **rpo = NULL;
    int rpo_cap = 0;
    for (int s = 0; s < n && size <= MAX_CALLER_SIZE; s++) {
        IRInstr *call = sites[s].call;
        IRFunc *callee = lookup(call->sym, ud);
        if (!callee) continue;
        if (callee->nblocks > rpo_cap) {
            free(rpo);
            rpo_cap = callee->nblocks;
            rpo = xmalloc(sizeof(*rpo) * (size_t)rpo_cap);
        }
        int nr, added;
        if (!inlinable(caller, call, callee, sites[s].loop_depth, rpo, &nr, &added)) continue;
        splice(caller, call, callee, rpo, nr);
        size += added;
    }
    free(rpo);
    free(sites);

    caller->has_calls = false;
    for (int bi = 0; bi < caller->nblocks && !caller->has_calls; bi++)
        for (int k = 0; k < caller->blocks[bi]->nins; k++)
            if (caller->blocks[bi]->ins[k]->op == IR_CALL) {
                caller->has_calls = true;
                break;
            }
}
