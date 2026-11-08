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

// Escape check for callee frame slots, same rule as ir_opt.c's memopt: a slot
// escapes when a pointer derived from its FRAMEADDR (through ADD only) is used
// for anything but the address operand of a LOAD/STORE (call arg, stored value,
// phi arg, compare, return, ...). A callee with an escaping slot is not
// inlinable: its slot addresses would have to keep working in the caller's
// frame, and a leaked address could outlive the inlined region.
// Only the reachable blocks (rpo) are rooted, mirroring memopt; uses in any
// block are examined, unreachable ones just have no root to match.
static bool frame_slots_escape(const IRFunc *c, IRBlock *const *rpo, int nr) {
    int n = c->nvals;
    int *root = xmalloc(sizeof(int) * (size_t)n); // 0 or slot index+1
    memset(root, 0, sizeof(int) * (size_t)n);
    int32_t *imm = xmalloc(sizeof(int32_t) * (size_t)n);
    int nslots = 0;
    for (int bi = 0; bi < nr; bi++)
        for (int k = 0; k < rpo[bi]->nins; k++) {
            IRInstr *i = rpo[bi]->ins[k];
            if (i->op == IR_FRAMEADDR) {
                int s = 0;
                while (s < nslots && imm[s] != i->imm) s++;
                if (s == nslots) imm[nslots++] = i->imm;
                root[i->id] = s + 1;
            } else if (i->op == IR_ADD) {
                int ra = i->a != IR_NONE ? root[i->a] : 0, rb = i->b != IR_NONE ? root[i->b] : 0;
                if (ra && !rb) root[i->id] = ra;
                else if (!ra && rb)
                    root[i->id] = rb;
            }
        }
    bool esc = false;
    for (int bi = 0; bi < c->nblocks && !esc; bi++) {
        const IRBlock *b = c->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins && !esc; k++) {
            const IRInstr *u = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            for (int p = 0; p < 3 && !esc; p++) {
                int v = p == 0 ? u->a : p == 1 ? u->b
                                               : u->c;
                if (v == IR_NONE || !root[v]) continue;
                if ((u->op == IR_LOAD || u->op == IR_STORE) && p == 0) continue;
                if (u->op == IR_ADD && root[u->id] == root[v]) continue; // further derivation, not a use
                esc = true;
            }
            for (int j = 0; j < u->nargs && !esc; j++) {
                int v = u->args[j];
                if (!root[v]) continue;
                esc = true; // phi arg / call arg: the address leaves the slot's loads/stores
            }
        }
    }
    free(root);
    free(imm);
    return esc;
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
    // Frame slots are inlinable only when no slot address escapes: see splice.
    if (frame_slots_escape(callee, rpo, n)) return false;
    *nr = n;
    *added = total;
    return true;
}

// Frame-slot rebasing. A FRAMEADDR imm o addresses bytes [rbp-o, rbp-o+sz):
// parser.c assigns LVar.offset = align_to(prev_offset + size, align), so an
// offset is the distance below rbp at which the object's storage ENDS, objects
// are disjoint (each starts no higher than the previous one's end), and an
// object's size is <= its offset. frame_layout() in ir_x86.c sizes the locals
// area from the max FRAMEADDR imm (lbytes) and places aggregate temps (paggoff,
// ctemp) at >= align8(lbytes), so any imm below the post-splice max stays
// disjoint from the aggregate-temp and spill ranges, which sit below.
//
// Rebase rule: fresh = base + callee_imm with base = align8(max FRAMEADDR imm
// already present in the caller, 0 if none). The uniform shift preserves the
// callee's internal layout (relative offsets and disjointness are unchanged),
// and fresh >= base + size keeps the slot's whole storage strictly below
// rbp-base <= rbp-caller_max, i.e. below every existing caller slot (whose
// deepest byte is rbp-caller_max). Since the rebased imms are plain FRAMEADDR
// imms of the caller, frame_layout automatically reserves the space and keeps
// the agg-temp area above it; splicing later callees re-scans the caller, so
// every inline site gets its own disjoint copies. Distinct callee imms map to
// distinct fresh imms; one callee imm always maps to one fresh imm (same slot
// in the callee -> same slot in this clone).
//
// This is sound because the callee's slot storage exists in the caller's frame
// during the inlined execution exactly as it would in the callee's own frame;
// frame addresses of locals are not observably distinct across the two
// compilations for valid C (comparing addresses of objects from different
// frames is unspecified), and a non-escaping address never outlives the
// inlined region. Loads/stores keep their displacements relative to the
// rebased address, so nothing else in the clone changes.
static int32_t frame_rebase(int32_t *map_imm, int32_t *map_fresh, int *n, int32_t base, int32_t imm) {
    for (int k = 0; k < *n; k++)
        if (map_imm[k] == imm) return map_fresh[k];
    map_imm[*n] = imm;
    map_fresh[*n] = base + imm;
    return map_fresh[(*n)++];
}

// Clones the callee body into `caller` in place of `call`; see the file comment.
static void splice(IRFunc *caller, IRInstr *call, const IRFunc *callee, IRBlock **rpo, int nr) {
    IRBlock *head = call->blk;
    const IRType rt = (IRType)call->type;
    const int call_id = call->id;
    const int line = call->line;

    // Rebase callee frame slots above the caller's own (see frame_rebase).
    int caller_max = 0;
    for (int bi = 0; bi < caller->nblocks; bi++)
        for (int k = 0; k < caller->blocks[bi]->nins; k++) {
            IRInstr *i = caller->blocks[bi]->ins[k];
            if (i->op == IR_FRAMEADDR && i->imm > caller_max) caller_max = i->imm;
        }
    const int32_t frame_base = (int32_t)((caller_max + 7) & ~7);
    int32_t fmap_imm[128], fmap_fresh[128]; // <= ~60 FRAMEADDRs fit the cost limit; 128 is generous headroom
    int nfmap = 0;

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
                if (o->op == IR_FRAMEADDR) // rebase the slot into the caller's frame
                    ni->imm = frame_rebase(fmap_imm, fmap_fresh, &nfmap, frame_base, o->imm);
                if (o->op != IR_PHI && o->nargs) {
                    ni->args = dup_i32(o->args, o->nargs);
                    ni->nargs = o->nargs;
                    if (o->arg_class) ni->arg_class = dup_i32(o->arg_class, o->nargs);
                }
            }
            ni->line = line;
            vmap[o->id] = ni->id;
            if (o->op == IR_DBGVAL) continue; // callee locals are gone after inlining
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

// ---------------------------------------------------------------- ir_tailrec
//
// Self-recursive tail calls become a loop. Runs once on a freshly built (and
// inlined) function, before ir_opt().
//
// A tail call is a direct, scalar, non-variadic IR_CALL of f's own symbol that
// is the last instr before a `ret` of exactly its result (void: a value-less
// `ret`), possibly through a chain of empty `jmp` blocks, possibly ending in a
// merge block that holds only `phi; ret phi` where the phi's incoming value on
// this edge is the call. The result must have no other use, and the function
// must contain no IR_FRAMEADDR: no frame slot can have been handed to the
// callee, so reusing this frame for the next iteration is invisible.
//
// Rewrite (E = old entry, keeps only the IR_PARAMs so it stays blocks[0]):
//     E: params; jmp H
//     H: one phi per CHANGED param [E: the IR_PARAM, tail block: the call arg];
//        everything that used to follow the params in E
//     tail block: call removed, `ret` / jmp chain replaced by `jmp H`
// Every use of a changed param is rewritten to its phi. A param that all tail
// calls pass through unchanged gets no phi, so no trivial phi is left behind.
// The phis are evaluated in parallel (f(a,b) -> f(b,a) swaps), which is the
// semantics of the call's simultaneous argument passing.
// The edge a tail block had into the jmp chain / merge block is dropped; blocks
// that lose their last predecessor become UNREACHABLE for ir_opt to delete.
typedef struct {
    IRInstr *call;
    IRBlock *first; // first block after the call block (the chain's head), NULL if `ret` follows directly
} Tail;

static int count_uses(const IRFunc *f, int id) {
    int n = 0;
    for (int bi = 0; bi < f->nblocks; bi++) {
        const IRBlock *b = f->blocks[bi];
        for (int k = 0; k < b->nphis + b->nins; k++) {
            const IRInstr *i = k < b->nphis ? b->phis[k] : b->ins[k - b->nphis];
            n += (i->a == id) + (i->b == id) + (i->c == id);
            for (int j = 0; j < i->nargs; j++) n += i->args[j] == id;
        }
    }
    return n;
}

// True when `call` (an IR_CALL in some block of f) is a self tail call.
static bool tail_site(const IRFunc *f, IRInstr *call, Tail *t) {
    if (call->op != IR_CALL || !call->sym || call->a != IR_NONE || call->variadic || call->ret_agg != 0) return false;
    if (strcmp(call->sym, f->fn->name) != 0 || call->type != f->ret_type || call->nargs != f->nparams) return false;
    if (!all_scalar(call->arg_class, call->nargs)) return false;
    for (int j = 0; j < call->nargs; j++)
        if (f->vals[call->args[j]]->type != f->param_types[j]) return false;

    IRBlock *b = call->blk;
    int k = b->nins - 2; // the call must be the last real instr before the terminator
    while (k >= 0 && b->ins[k]->op == IR_NOP) k--;
    if (k < 0 || b->ins[k] != call) return false;
    const IRInstr *term = b->ins[b->nins - 1];
    const bool is_void = f->ret_type == IRT_VOID;

    // Follow empty jmp blocks. A value flows through them only if the call
    // dominates nothing but this path: every such block has this edge alone.
    const IRBlock *last = b, *r = NULL;
    t->first = NULL;
    if (term->op == IR_JMP) {
        r = term->t;
        t->first = term->t;
        for (int steps = 0; steps < f->nblocks && r->nphis == 0 && r->nins == 1 && r->ins[0]->op == IR_JMP; steps++) {
            if (!is_void && r->npreds != 1) return false;
            last = r;
            r = r->ins[0]->t;
        }
        if (r->nins != 1 || r->ins[0]->op != IR_RET) return false;
        term = r->ins[0];
    } else if (term->op != IR_RET) {
        return false;
    }

    if (is_void) return term->a == IR_NONE && (!r || r->nphis == 0);
    if (count_uses(f, call->id) != 1) return false;
    if (!r) return term->a == call->id;
    if (r->nphis == 0) return term->a == call->id && r->npreds == 1;
    // merge block `phi; ret phi`: this edge must carry the call
    if (r->nphis != 1 || term->a != r->phis[0]->id) return false;
    for (int j = 0; j < r->npreds; j++)
        if (r->preds[j] == last) return r->phis[0]->args[j] == call->id;
    return false;
}

// Removes the edge from->to; a block that loses its last predecessor dies
// (UNREACHABLE terminator) and so on down the jmp chain.
static void drop_edge(IRBlock *from, IRBlock *to) {
    ir_remove_pred_edge(to, from);
    while (to->npreds == 0) {
        IRInstr *t = to->ins[to->nins - 1];
        IRBlock *s[2];
        int ns = ir_succs(to, s);
        t->op = IR_UNREACHABLE;
        t->a = IR_NONE;
        t->t = t->f = NULL;
        if (ns != 1) break; // chain blocks have exactly one successor; a dead `ret` ends the walk
        ir_remove_pred_edge(s[0], to);
        to = s[0];
    }
}

void ir_tailrec(IRFunc *f) {
    if (!f->fn || !f->fn->name || f->fn->is_variadic || f->ret_agg || f->ret_sret || !f->has_calls) return;
    if (!all_scalar(f->param_class, f->nparams)) return;
    int ncalls = 0;
    for (int bi = 0; bi < f->nblocks; bi++)
        for (int k = 0; k < f->blocks[bi]->nins; k++) {
            const IRInstr *i = f->blocks[bi]->ins[k];
            if (i->op == IR_FRAMEADDR) return;
            ncalls += i->op == IR_CALL;
        }
    if (ncalls == 0) return;

    IRBlock *entry = f->blocks[0];
    int *pv = xmalloc(sizeof(*pv) * (size_t)(f->nparams ? f->nparams : 1)); // param index -> IR_PARAM value
    for (int j = 0; j < f->nparams; j++) pv[j] = IR_NONE;
    for (int k = 0; k < entry->nins; k++) {
        const IRInstr *i = entry->ins[k];
        if (i->op != IR_PARAM) continue;
        if (i->imm < 0 || i->imm >= f->nparams || pv[i->imm] != IR_NONE) {
            free(pv);
            return;
        }
        pv[i->imm] = i->id;
    }

    Tail *tails = xmalloc(sizeof(*tails) * (size_t)ncalls);
    int nt = 0;
    for (int bi = 0; bi < f->nblocks; bi++)
        for (int k = 0; k < f->blocks[bi]->nins; k++)
            if (tail_site(f, f->blocks[bi]->ins[k], &tails[nt])) {
                tails[nt].call = f->blocks[bi]->ins[k];
                nt++;
            }
    if (nt == 0) {
        free(tails);
        free(pv);
        return;
    }

    // Split: the params stay in the entry, the rest moves to the loop header H.
    IRBlock *h = ir_block_new(f);
    ir_add_pred(h, entry); // preds[0] = entry edge, then one per tail block in order
    int keep = 0;
    for (int k = 0; k < entry->nins; k++) {
        IRInstr *i = entry->ins[k];
        if (i->op == IR_PARAM) entry->ins[keep++] = i;
        else
            ir_block_append(h, i);
    }
    entry->nins = keep;
    IRBlock *succ[2];
    int ns = ir_succs(h, succ);
    for (int k = 0; k < ns; k++)
        for (int j = 0; j < succ[k]->npreds; j++)
            if (succ[k]->preds[j] == entry) succ[k]->preds[j] = h; // same slot: phi args stay aligned
    IRInstr *jmp = ir_instr_new(f, IR_JMP, IRT_VOID);
    jmp->t = h;
    jmp->line = h->ins[h->nins - 1]->line;
    ir_block_append(entry, jmp);
    h->loop_depth = 0;

    // One phi per param some tail call changes; uses are rewritten before the
    // phi operands are filled in (so the entry operand keeps the raw IR_PARAM).
    int *phi = xmalloc(sizeof(*phi) * (size_t)(f->nparams ? f->nparams : 1));
    for (int j = 0; j < f->nparams; j++) {
        phi[j] = IR_NONE;
        if (pv[j] == IR_NONE) continue;
        bool changed = false;
        for (int t = 0; t < nt; t++) changed |= tails[t].call->args[j] != pv[j];
        if (!changed) continue;
        IRInstr *p = ir_instr_new(f, IR_PHI, (IRType)f->vals[pv[j]]->type);
        p->args = xmalloc(sizeof(*p->args) * (size_t)(nt + 1));
        p->line = jmp->line;
        ir_block_add_phi(h, p);
        phi[j] = p->id;
    }
    for (int j = 0; j < f->nparams; j++)
        if (phi[j] != IR_NONE) ir_replace_uses(f, pv[j], phi[j]);
    for (int j = 0; j < f->nparams; j++) {
        if (phi[j] == IR_NONE) continue;
        IRInstr *p = f->vals[phi[j]];
        p->args[p->nargs++] = pv[j];
        for (int t = 0; t < nt; t++) p->args[p->nargs++] = tails[t].call->args[j];
    }

    for (int t = 0; t < nt; t++) {
        IRInstr *call = tails[t].call;
        IRBlock *b = call->blk;
        IRInstr *term = b->ins[b->nins - 1];
        if (tails[t].first) drop_edge(b, tails[t].first);
        ir_block_remove(b, call);
        term->op = IR_JMP;
        term->a = IR_NONE;
        term->t = h;
        term->f = NULL;
        ir_add_pred(h, b);
    }

    f->has_calls = false;
    for (int bi = 0; bi < f->nblocks && !f->has_calls; bi++)
        for (int k = 0; k < f->blocks[bi]->nins; k++)
            if (f->blocks[bi]->ins[k]->op == IR_CALL) {
                f->has_calls = true;
                break;
            }
    ir_analyze_loops(f); // H is a new loop header: loop_depth feeds LICM and the allocator's spill weights
    free(phi);
    free(tails);
    free(pv);
}
