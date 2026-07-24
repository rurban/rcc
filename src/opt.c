// SPDX-License-Identifier: LGPL-2.1-or-later
#include "rcc.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <ctype.h>

#define CF_NEXT 0
#define CF_RETURN 1

// Known pure/const library functions that can be folded at compile time.
// "const" = result depends only on args, no side effects, no global state.
// "pure"  = no side effects (may read global state, e.g. errno).
typedef enum { PF_CONST,
               PF_PURE } PureKind;

typedef struct PureFn PureFn;
struct PureFn {
    const char *name;
    PureKind kind;
    // Evaluator: returns the folded value, or LONG_MIN if can't fold.
    // args[0..nargs-1] are the integer argument values.
    // str_args[0..nargs-1] are the string argument pointers (NULL if not a string).
    long (*eval)(int *args, char **str_args, int nargs);
};

static long eval_abs(int *args, char **str_args, int nargs) {
    (void)str_args;
    return nargs >= 1 ? (args[0] < 0 ? -(long)args[0] : (long)args[0]) : LONG_MIN;
}

static long eval_strlen(int *args, char **str_args, int nargs) {
    (void)args;
    if (nargs >= 1 && str_args && str_args[0])
        return (long)strlen(str_args[0]);
    return LONG_MIN;
}

static long eval_strcmp(int *args, char **str_args, int nargs) {
    (void)args;
    if (nargs >= 2 && str_args && str_args[0] && str_args[1])
        return (long)strcmp(str_args[0], str_args[1]);
    return LONG_MIN;
}

static long eval_isdigit(int *args, char **str_args, int nargs) {
    (void)str_args;
    return nargs >= 1 ? (long)!!isdigit(args[0] & 0xff) : LONG_MIN;
}

static long eval_toupper(int *args, char **str_args, int nargs) {
    (void)str_args;
    return nargs >= 1 ? (long)toupper(args[0] & 0xff) : LONG_MIN;
}

static const PureFn pure_fns[] = {
    {"abs", PF_CONST, eval_abs},
    {"labs", PF_CONST, eval_abs},
    {"llabs", PF_CONST, eval_abs},
    {"strlen", PF_PURE, eval_strlen},
    {"strcmp", PF_PURE, eval_strcmp},
    {"isdigit", PF_PURE, eval_isdigit},
    {"toupper", PF_PURE, eval_toupper},
    {NULL, 0, NULL},
};

// Check if a function call is pure/const. First checks C23
// [[unsequenced]] / [[reproducible]] attributes on the global
// symbol, then falls back to the known-pure table.
static PureKind fn_purity(const char *name) {
    LVar *g = find_global_name((char *)name);
    if (g && g->is_function) {
        if (g->is_unsequenced) return PF_CONST;
        if (g->is_reproducible) return PF_PURE;
    }
    for (const PureFn *p = pure_fns; p->name; p++)
        if (strcmp(p->name, name) == 0)
            return p->kind;
    return -1; // not pure
}

// Try to fold a call to a known pure function with constant args.
// Returns true if folded, storing result in *result.
static bool try_fold_pure(const char *name, Node *args, long *result) {
    int iargs[10];
    char *strs[10];
    int nargs = 0;
    bool all_const = true;
    for (Node *arg = args; arg; arg = arg->next) {
        if (nargs >= 10) return false;
        strs[nargs] = NULL;
        if (arg->kind == ND_NUM) {
            iargs[nargs] = (int)arg->val;
        } else if (arg->kind == ND_NEG && arg->lhs && arg->lhs->kind == ND_NUM) {
            iargs[nargs] = (int)-(arg->lhs->val);
        } else if (arg->kind == ND_STR && arg->str) {
            iargs[nargs] = 0;
            strs[nargs] = arg->str;
        } else {
            all_const = false;
            break;
        }
        nargs++;
    }
    if (!all_const) return false;

    // Check known-pure table and C23 purity attributes
    if (fn_purity(name) >= 0) {
        for (const PureFn *p = pure_fns; p->name; p++) {
            if (strcmp(p->name, name) == 0) {
                long v = p->eval(iargs, strs, nargs);
                if (v != LONG_MIN) {
                    *result = v;
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

static int eval_ast(Program *prog, Function *fn, Node *node, int *env, int *cf, bool *success) {
    if (!node || !*success) return 0;

    switch (node->kind) {
    case ND_NUM:
        return node->val;
    case ND_LVAR:
        return env[node->var->offset / 8];
    case ND_ASSIGN: {
        if (node->lhs->kind != ND_LVAR) {
            *success = false;
            return 0;
        }
        int dummy_cf = CF_NEXT;
        int val = eval_ast(prog, fn, node->rhs, env, &dummy_cf, success);
        env[node->lhs->var->offset / 8] = val;
        return val;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            int ret = eval_ast(prog, fn, n, env, cf, success);
            if (!*success || *cf == CF_RETURN) return ret;
        }
        return 0;
    case ND_RETURN: {
        int dummy_cf = CF_NEXT;
        int ret = eval_ast(prog, fn, node->lhs, env, &dummy_cf, success);
        *cf = CF_RETURN;
        return ret;
    }
    case ND_IF: {
        int dummy_cf = CF_NEXT;
        int cond = eval_ast(prog, fn, node->cond, env, &dummy_cf, success);
        if (!*success) return 0;
        if (cond) {
            return eval_ast(prog, fn, node->then, env, cf, success);
        } else if (node->els) {
            return eval_ast(prog, fn, node->els, env, cf, success);
        }
        return 0;
    }
    case ND_EXPR_STMT: {
        int dummy_cf = CF_NEXT;
        return eval_ast(prog, fn, node->lhs, env, &dummy_cf, success);
    }
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        int dummy_cf = CF_NEXT;
        int l = eval_ast(prog, fn, node->lhs, env, &dummy_cf, success);
        int r = eval_ast(prog, fn, node->rhs, env, &dummy_cf, success);
        if (!*success) return 0;
        if (node->kind == ND_ADD) return l + r;
        if (node->kind == ND_SUB) return l - r;
        if (node->kind == ND_MUL) return l * r;
        if (node->kind == ND_DIV) {
            if (r == 0) {
                *success = false;
                return 0;
            }
            return l / r;
        }
        if (node->kind == ND_MOD) {
            if (r == 0) {
                *success = false;
                return 0;
            }
            return l % r;
        }
        if (node->kind == ND_EQ) return l == r;
        if (node->kind == ND_NE) return l != r;
        if (node->kind == ND_LT) return l < r;
        if (node->kind == ND_LE) return l <= r;
        *success = false;
        return 0;
    }
    case ND_FUNCALL: {
        int args[10];
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            int dummy_cf = CF_NEXT;
            args[nargs++] = eval_ast(prog, fn, arg, env, &dummy_cf, success);
            if (!*success || nargs >= 10) {
                *success = false;
                return 0;
            }
        }
        Function *target = NULL;
        for (TLItem *item = prog->items; item; item = item->next) {
            if (item->kind == TL_FUNC && item->fn->name == node->funcname) {
                target = item->fn;
                break;
            }
        }
        if (target && target->body && target->name != bi_s_printf) {
            int new_env[256] = {0};
            LVar *param = target->params;
            for (int i = 0; i < nargs; i++) {
                if (param) {
                    new_env[param->offset / 8] = args[i];
                    param = param->param_next;
                }
            }
            int cf_ret = CF_NEXT;
            int ret = 0;
            for (Node *stmt = target->body; stmt; stmt = stmt->next) {
                ret = eval_ast(prog, target, stmt, new_env, &cf_ret, success);
                if (!*success || cf_ret == CF_RETURN) break;
            }
            return ret;
        }
        *success = false;
        return 0;
    }
    default:
        *success = false;
        return 0;
    }
}

static bool has_cleanup_local(Function *fn) {
    for (LVar *var = fn->locals; var; var = var->next) {
        if (var->cleanup_func)
            return true;
    }
    return false;
}

static bool has_addr_arg(Node *node) {
    for (Node *arg = node->args; arg; arg = arg->next) {
        if (arg->kind == ND_ADDR)
            return true;
    }
    return false;
}

// ---- -finline: inline tiny leaf functions --------------------------------
//
// A deliberately small, fast inliner in the spirit of gcc's
// -finline-small-functions. Only functions whose whole body is a single
// "return EXPR;" are candidates, and only when every argument is a
// side-effect-free "simple" expression. Under those constraints the call
// can be replaced in place by EXPR with each parameter reference rewritten
// to its argument, without introducing any new locals, labels or control
// flow - so no stack-offset surgery is needed and the transform stays cheap.

#define MAX_INLINE_PARAMS 16
// gcc-style budgets (max-inline-insns-{auto,single}), but in units of our
// crude expr_cost() rather than GIMPLE instructions.
#define INLINE_COST_AUTO 20 // functions not declared `inline`
#define INLINE_COST_INLINE 60 // functions the programmer marked `inline`

// gcc-style cost estimate: roughly the number of "instructions" in an
// expression. Leaves (constants, variable reads) are free; each operator
// counts one; division/modulo and nested calls are pricier. This mirrors
// the intent of estimate_num_insns() without its precision.
static int expr_cost(Node *n) {
    if (!n) return 0;
    int c;
    switch (n->kind) {
    case ND_NUM:
    case ND_FNUM:
    case ND_STR:
    case ND_LVAR:
        c = 0;
        break;
    case ND_FUNCALL:
        c = 3;
        break;
    case ND_DIV:
    case ND_MOD:
        c = 2;
        break;
    default:
        c = 1;
        break;
    }
    c += expr_cost(n->lhs) + expr_cost(n->rhs) + expr_cost(n->cond) +
        expr_cost(n->then) + expr_cost(n->els);
    for (Node *a = n->args; a; a = a->next)
        c += expr_cost(a);
    return c;
}

// Deep-clone an expression subtree. Sub-lists (body/args) are cloned as
// fresh lists; the root's `next` is cleared so the caller can splice it in.
static Node *clone_expr(Node *n) {
    if (!n) return NULL;
    Node *c = arena_alloc(sizeof(Node));
    *c = *n;
    c->next = NULL;
    c->lhs = clone_expr(n->lhs);
    c->rhs = clone_expr(n->rhs);
    c->cond = clone_expr(n->cond);
    c->then = clone_expr(n->then);
    c->els = clone_expr(n->els);
    c->init = clone_expr(n->init);
    c->inc = clone_expr(n->inc);
    Node bhead = {0}, *bt = &bhead;
    Node *stmt_result_clone = NULL;
    for (Node *b = n->body; b; b = b->next) {
        bt->next = clone_expr(b);
        bt = bt->next;
        // stmt_expr_result aliases the lhs of the last ND_EXPR_STMT in body
        // (see parser.c's `(...)`/inline-pack ND_STMT_EXPR construction),
        // and codegen locates the value-producing statement via pointer
        // equality against that alias. Preserve the aliasing in the clone
        // instead of cloning stmt_expr_result independently below, which
        // would produce a distinct object that never compares equal.
        if (b->kind == ND_EXPR_STMT && b->lhs == n->stmt_expr_result)
            stmt_result_clone = bt->lhs;
    }
    c->body = bhead.next;
    c->stmt_expr_result = stmt_result_clone ? stmt_result_clone : clone_expr(n->stmt_expr_result);
    Node ahead = {0}, *at = &ahead;
    for (Node *a = n->args; a; a = a->next) {
        at->next = clone_expr(a);
        at = at->next;
    }
    c->args = ahead.next;
    return c;
}

// True if evaluating `n` has no side effects and duplicating or dropping
// the evaluation is observationally safe, so it can replace a parameter
// used any number of times (including zero).
static bool arg_is_simple(Node *n) {
    if (!n) return false;
    switch (n->kind) {
    case ND_NUM:
    case ND_FNUM:
    case ND_STR:
    case ND_LVAR:
        return true;
    case ND_NEG:
    case ND_BITNOT:
    case ND_NOT:
    case ND_ADDR:
    case ND_CAST:
    case ND_MEMBER:
        return arg_is_simple(n->lhs);
    default:
        return false;
    }
}

// Does an expression call `name` (interned pointer)? Used to skip directly
// recursive functions, which would otherwise expand without bound.
static bool calls_name(Node *n, char *name) {
    if (!n) return false;
    if (n->kind == ND_FUNCALL) {
        if (n->funcname == name)
            return true;
        if (n->lhs && n->lhs->kind == ND_LVAR && n->lhs->var && n->lhs->var->name == name)
            return true;
    }
    if (calls_name(n->lhs, name) || calls_name(n->rhs, name) ||
        calls_name(n->cond, name) || calls_name(n->then, name) ||
        calls_name(n->els, name) || calls_name(n->init, name) ||
        calls_name(n->inc, name) || calls_name(n->stmt_expr_result, name))
        return true;
    for (Node *b = n->body; b; b = b->next)
        if (calls_name(b, name)) return true;
    for (Node *a = n->args; a; a = a->next)
        if (calls_name(a, name)) return true;
    return false;
}

// True if the expression writes to a parameter variable itself or takes its
// address. Substituting an r-value argument for such a parameter would be
// unsound (e.g. `x++` on a by-value param, or escaping `&x`). Writes made
// *through* a pointer parameter are unaffected and intentionally allowed.
static bool writes_param(Node *n, LVar **params, int nparams) {
    if (!n) return false;
    switch (n->kind) {
    case ND_ASSIGN:
    case ND_PRE_INC:
    case ND_POST_INC:
    case ND_PRE_DEC:
    case ND_POST_DEC:
    case ND_ADDR:
        if (n->lhs && n->lhs->kind == ND_LVAR)
            for (int i = 0; i < nparams; i++)
                if (n->lhs->var == params[i]) return true;
        break;
    default:
        break;
    }
    if (writes_param(n->lhs, params, nparams) || writes_param(n->rhs, params, nparams) ||
        writes_param(n->cond, params, nparams) || writes_param(n->then, params, nparams) ||
        writes_param(n->els, params, nparams) || writes_param(n->init, params, nparams) ||
        writes_param(n->inc, params, nparams) || writes_param(n->stmt_expr_result, params, nparams))
        return true;
    for (Node *b = n->body; b; b = b->next)
        if (writes_param(b, params, nparams)) return true;
    for (Node *a = n->args; a; a = a->next)
        if (writes_param(a, params, nparams)) return true;
    return false;
}

// True if the expression references a local variable of the callee that is
// not one of its parameters (e.g. a statement-expression or compound-literal
// temporary). Such a variable lives in the callee's frame; cloning it into
// the caller would alias an unrelated stack slot, so those bodies are not
// inlined.
static bool refs_nonparam_local(Node *n, LVar **params, int nparams) {
    if (!n) return false;
    if (n->kind == ND_LVAR && n->var && n->var->is_local) {
        bool is_param = false;
        for (int i = 0; i < nparams; i++)
            if (n->var == params[i]) is_param = true;
        if (!is_param) return true;
    }
    if (refs_nonparam_local(n->lhs, params, nparams) || refs_nonparam_local(n->rhs, params, nparams) ||
        refs_nonparam_local(n->cond, params, nparams) || refs_nonparam_local(n->then, params, nparams) ||
        refs_nonparam_local(n->els, params, nparams) || refs_nonparam_local(n->init, params, nparams) ||
        refs_nonparam_local(n->inc, params, nparams) || refs_nonparam_local(n->stmt_expr_result, params, nparams))
        return true;
    for (Node *b = n->body; b; b = b->next)
        if (refs_nonparam_local(b, params, nparams)) return true;
    for (Node *a = n->args; a; a = a->next)
        if (refs_nonparam_local(a, params, nparams)) return true;
    return false;
}

// Rewrite parameter references to their argument expressions, in place.
static void subst_params(Node *n, LVar **params, Node **args, int nparams) {
    if (!n) return;
    if (n->kind == ND_LVAR) {
        for (int i = 0; i < nparams; i++) {
            if (n->var == params[i]) {
                Node *saved_next = n->next;
                Node *rep = clone_expr(args[i]);
                *n = *rep;
                n->next = saved_next; // keep our position in any list
                return; // substituted subtree is argument-owned
            }
        }
        return;
    }
    subst_params(n->lhs, params, args, nparams);
    subst_params(n->rhs, params, args, nparams);
    subst_params(n->cond, params, args, nparams);
    subst_params(n->then, params, args, nparams);
    subst_params(n->els, params, args, nparams);
    subst_params(n->init, params, args, nparams);
    subst_params(n->inc, params, args, nparams);
    subst_params(n->stmt_expr_result, params, args, nparams);
    for (Node *b = n->body; b; b = b->next)
        subst_params(b, params, args, nparams);
    for (Node *a = n->args; a; a = a->next)
        subst_params(a, params, args, nparams);
}

// If `fn`'s whole body is a single "return EXPR;" (optionally wrapped in one
// { ... } block), return EXPR; otherwise NULL.
static Node *inlinable_return_expr(Function *fn) {
    Node *b = fn->body;
    if (!b || b->next) return NULL;
    if (b->kind == ND_BLOCK) {
        b = b->body;
        if (!b || b->next) return NULL;
    }
    if (b->kind != ND_RETURN || !b->lhs) return NULL;
    return b->lhs;
}

// Try to inline a call. On success returns the replacement expression;
// otherwise NULL (leaving the call unchanged).
static Node *try_inline(Program *prog, Node *call) {
    // A direct call carries its target either in funcname or, for an
    // in-scope function, as an ND_LVAR in lhs. Names are interned, so a
    // pointer compare identifies the callee.
    char *name = call->funcname;
    if (!name && call->lhs && call->lhs->kind == ND_LVAR && call->lhs->var)
        name = call->lhs->var->name;
    if (!name || name == bi_s_printf) return NULL;

    // The call must yield a scalar value; struct/union/void returns are not
    // safe to splice as a plain expression here.
    Type *rt = call->ty;
    if (!rt || rt->kind == TY_VOID || rt->kind == TY_STRUCT || rt->kind == TY_UNION)
        return NULL;

    Function *fn = NULL;
    for (TLItem *item = prog->items; item; item = item->next)
        if (item->kind == TL_FUNC && item->fn->name == name) {
            fn = item->fn;
            break;
        }
    if (!fn || !fn->body || fn->is_variadic || (fn->ty && fn->ty->is_variadic))
        return NULL;
    if (has_cleanup_local(fn)) return NULL;

    Node *ret_expr = inlinable_return_expr(fn);
    if (!ret_expr) return NULL;
    if (calls_name(ret_expr, fn->name)) return NULL; // directly recursive

    int cost = expr_cost(ret_expr);
    int budget = fn->is_inline ? INLINE_COST_INLINE : INLINE_COST_AUTO;
    if (cost > budget) return NULL;

    // Collect scalar parameters and match them to simple arguments.
    LVar *params[MAX_INLINE_PARAMS];
    Node *args[MAX_INLINE_PARAMS];
    int nparams = 0;
    for (LVar *p = fn->params; p; p = p->param_next) {
        if (nparams >= MAX_INLINE_PARAMS) return NULL;
        if (!(p->ty && (is_integer(p->ty) || is_flonum(p->ty) || p->ty->kind == TY_PTR)))
            return NULL;
        params[nparams++] = p;
    }
    int nargs = 0;
    for (Node *a = call->args; a; a = a->next) {
        if (nargs >= MAX_INLINE_PARAMS) return NULL;
        if (!arg_is_simple(a)) return NULL;
        args[nargs++] = a;
    }
    if (nargs != nparams) return NULL;
    if (writes_param(ret_expr, params, nparams)) return NULL;
    if (refs_nonparam_local(ret_expr, params, nparams)) return NULL;

    Node *inl = clone_expr(ret_expr);
    subst_params(inl, params, args, nparams);

    // Preserve the call's result type, matching the implicit return
    // conversion, by inserting a cast when the expression's type differs.
    if (inl->ty && (inl->ty->kind != rt->kind || inl->ty->size != rt->size)) {
        Node *cast = arena_alloc(sizeof(Node));
        memset(cast, 0, sizeof(Node));
        cast->kind = ND_CAST;
        cast->lhs = inl;
        cast->tok = call->tok;
        cast->ty = rt;
        return cast;
    }
    if (!inl->ty) inl->ty = rt;
    return inl;
}

// ---- -funroll: unroll const-sized loops ---------------------------------
//
// A fast loop unroller for for-loops with a constant, known iteration count.
// Only the simplest pattern is handled:
//   for (i = START; i < END; i++) BODY    or
//   for (i = START; i <= END; i++) BODY
// where START and END are compile-time integer constants, the induction
// variable is incremented (++i or i++), and the body carries no break or
// continue (which would have no enclosing loop to target after unrolling).

#define MAX_UNROLL_ITERS 16


// True if the subtree contains a break or continue (which would lose their
// enclosing loop after unrolling).
static bool has_break_or_continue(Node *n) {
    if (!n) return false;
    if (n->kind == ND_BREAK || n->kind == ND_CONTINUE) return true;
    if (has_break_or_continue(n->lhs)) return true;
    if (has_break_or_continue(n->rhs)) return true;
    if (has_break_or_continue(n->cond)) return true;
    if (has_break_or_continue(n->then)) return true;
    if (has_break_or_continue(n->els)) return true;
    if (has_break_or_continue(n->init)) return true;
    if (has_break_or_continue(n->inc)) return true;
    for (Node *c = n->body; c; c = c->next)
        if (has_break_or_continue(c)) return true;
    for (Node *c = n->args; c; c = c->next)
        if (has_break_or_continue(c)) return true;
    return false;
}

// Substitute every ND_LVAR reference to `var` with ND_NUM `val`.
static void subst_lvar(Node *n, LVar *var, long val) {
    if (!n) return;
    if (n->kind == ND_LVAR && n->var == var) {
        n->kind = ND_NUM;
        n->val = val;
        n->var = NULL;
        return;
    }
    subst_lvar(n->lhs, var, val);
    subst_lvar(n->rhs, var, val);
    subst_lvar(n->cond, var, val);
    subst_lvar(n->then, var, val);
    subst_lvar(n->els, var, val);
    subst_lvar(n->init, var, val);
    subst_lvar(n->inc, var, val);
    for (Node *c = n->body; c; c = c->next)
        subst_lvar(c, var, val);
    for (Node *c = n->args; c; c = c->next)
        subst_lvar(c, var, val);
}

// Compute the constant iteration count of a for-loop with the canonical form
//   for (i = START; i < END; i++)  or  for (i = START; i <= END; i++)
// Returns the count on success, -1 if the loop doesn't match this pattern.
static int loop_iteration_count(Node *node) {
    if (!node->init || !node->cond || !node->inc) return -1;

    // init must be: var = START (constant)
    if (node->init->kind != ND_ASSIGN) return -1;
    Node *ivar = node->init->lhs;
    Node *istart = node->init->rhs;
    if (!ivar || ivar->kind != ND_LVAR || !ivar->var) return -1;
    if (!istart || istart->kind != ND_NUM) return -1;
    long start = istart->val;

    // cond must be: var < END or var <= END
    int cmp; // 0 = <, 1 = <=
    if (node->cond->kind == ND_LT) cmp = 0;
    else if (node->cond->kind == ND_LE)
        cmp = 1;
    else
        return -1;
    Node *c_lhs = node->cond->lhs;
    Node *c_rhs = node->cond->rhs;
    if (!c_lhs || c_lhs->kind != ND_LVAR || c_lhs->var != ivar->var) return -1;
    if (!c_rhs || c_rhs->kind != ND_NUM) return -1;
    long end = c_rhs->val;

    // inc must be: i++ or ++i on the same variable
    if (node->inc->kind != ND_POST_INC && node->inc->kind != ND_PRE_INC)
        return -1;
    Node *inc_target = node->inc->lhs;
    if (!inc_target || inc_target->kind != ND_LVAR || inc_target->var != ivar->var)
        return -1;

    long count = cmp ? (end - start + 1) : (end - start);
    if (count <= 0 || count > MAX_UNROLL_ITERS) return -1;
    return (int)count;
}

// Try to unroll a const-sized for-loop. On success returns an ND_BLOCK
// containing the init followed by N copies of the body (with the induction
// variable substituted by its value in each copy). On failure returns NULL.
static Node *try_unroll(Node *node) {
    if (node->kind != ND_FOR) return NULL;

    int count = loop_iteration_count(node);
    if (count < 0) return NULL;

    // Safety: refuse to unroll if the body has break or continue.
    if (has_break_or_continue(node->then)) return NULL;

    long start_val = node->init->rhs->val;
    LVar *ivar = node->init->lhs->var;

    // tag the init so it isn't freed when node is replaced
    node->init->next = NULL;

    // Build the unrolled result as a flat statement list chained via `next`.
    // The first statement is the init; subsequent statements are the body
    // clones with the induction variable substituted.
    Node head = {0}, *tail = &head;

    // 1) init statement
    tail->next = node->init;
    tail = node->init;

    // 2) count copies of the body
    for (int k = 0; k < count; k++) {
        if (node->then->kind == ND_BLOCK) {
            // Clone each statement in the compound body
            for (Node *s = node->then->body; s; s = s->next) {
                Node *copy = clone_expr(s);
                subst_lvar(copy, ivar, start_val + k);
                tail->next = copy;
                tail = copy;
            }
        } else {
            // Single-statement body
            Node *copy = clone_expr(node->then);
            subst_lvar(copy, ivar, start_val + k);
            tail->next = copy;
            tail = copy;
        }
    }

    return head.next; // the first statement (init), with the rest chained
}

static Node *optimize_node(Program *prog, Node *node) {
    if (!node) return NULL;
    node->lhs = optimize_node(prog, node->lhs);
    node->rhs = optimize_node(prog, node->rhs);
    node->cond = optimize_node(prog, node->cond);
    node->then = optimize_node(prog, node->then);
    node->els = optimize_node(prog, node->els);
    node->init = optimize_node(prog, node->init);
    node->inc = optimize_node(prog, node->inc);

    // We can't easily map node->next without breaking lists potentially?
    // Wait, body is a list. args is a list.
    Node *prev_body = NULL;
    for (Node *n = node->body; n; n = n->next) {
        Node *o = optimize_node(prog, n);
        if (prev_body) prev_body->next = o;
        else
            node->body = o;
        prev_body = o;
    }
    Node *prev_arg = NULL;
    for (Node *n = node->args; n; n = n->next) {
        Node *o = optimize_node(prog, n);
        if (prev_arg) prev_arg->next = o;
        else
            node->args = o;
        prev_arg = o;
    }

    if (node->kind == ND_ADD || node->kind == ND_SUB || node->kind == ND_MUL || node->kind == ND_DIV || node->kind == ND_MOD) {
        if (node->lhs && node->lhs->kind == ND_NUM && node->rhs && node->rhs->kind == ND_NUM) {
            Node *fold = arena_alloc(sizeof(Node));
            fold->kind = ND_NUM;
            if (node->kind == ND_ADD) fold->val = node->lhs->val + node->rhs->val;
            if (node->kind == ND_SUB) fold->val = node->lhs->val - node->rhs->val;
            if (node->kind == ND_MUL) fold->val = node->lhs->val * node->rhs->val;
            if (node->kind == ND_DIV) {
                if (node->rhs->val == 0) return node; // avoid div by zero
                fold->val = node->lhs->val / node->rhs->val;
            }
            if (node->kind == ND_MOD) {
                if (node->rhs->val == 0) return node;
                fold->val = node->lhs->val % node->rhs->val;
            }
            fold->ty = node->ty;
            return fold;
        }
    }

    if (node->kind == ND_FUNCALL) {
        // Resolve function name from funcname or lhs->var for folding
        const char *fname = node->funcname;
        if (!fname && node->lhs && node->lhs->kind == ND_LVAR && node->lhs->var)
            fname = node->lhs->var->name;

        // First: try folding via known pure-function table (strlen, abs, etc.)
        if (fname && !has_addr_arg(node)) {
            long fold_val;
            if (try_fold_pure(fname, node->args, &fold_val)) {
                Node *fold = arena_alloc(sizeof(Node));
                fold->kind = ND_NUM;
                fold->val = fold_val;
                fold->ty = node->ty;
                return fold;
            }
        }

        // Second: -finline. Replace a call to a tiny "return EXPR;" function
        // with the substituted expression when the arguments are simple.
        // Runs before the funcname gate below because in-scope direct calls
        // carry their target in lhs, not funcname.
        if (opt_finline) {
            Node *inl = try_inline(prog, node);
            if (inl) return inl;
        }

        // Third: CTFE for user-defined functions with const args
        if (!node->funcname) return node;
        bool all_const = true;
        int args[10];
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next) {
            if (arg->kind == ND_NUM)
                args[nargs] = (int)arg->val;
            else if (arg->kind == ND_STR && arg->str)
                args[nargs] = arg->str_id;
            else {
                all_const = false;
                break;
            }
            if (nargs < 10) nargs++;
        }
        if (all_const && !has_addr_arg(node) && node->funcname != bi_s_printf) {
            Function *target = NULL;
            for (TLItem *item = prog->items; item; item = item->next) {
                if (item->kind == TL_FUNC && item->fn->name == node->funcname) {
                    target = item->fn;
                    break;
                }
            }
            if (target && target->body && !has_cleanup_local(target)) {
                bool success = true;
                int env[256] = {0};
                LVar *param = target->params;
                for (int i = 0; i < nargs; i++) {
                    if (param) {
                        env[param->offset / 8] = args[i];
                        param = param->param_next;
                    }
                }
                int dummy_cf = CF_NEXT;
                int result = 0;
                for (Node *stmt = target->body; stmt; stmt = stmt->next) {
                    result = eval_ast(prog, target, stmt, env, &dummy_cf, &success);
                    if (!success || dummy_cf == CF_RETURN) break;
                }
                if (success) {
                    Node *fold = arena_alloc(sizeof(Node));
                    fold->kind = ND_NUM;
                    fold->val = result;
                    fold->ty = node->ty;
                    return fold;
                }
            }
        }
    }

    // -funroll: unroll const-sized for-loops
    if (opt_funroll && node->kind == ND_FOR) {
        Node *unrolled = try_unroll(node);
        if (unrolled) return unrolled;
    }
    return node;
}

void optimize(Program *prog) {
    for (TLItem *item = prog->items; item; item = item->next) {
        if (item->kind != TL_FUNC)
            continue;
        Function *fn = item->fn;
        Node *prev = NULL;
        for (Node *n = fn->body; n; n = n->next) {
            Node *o = optimize_node(prog, n);
            if (prev) prev->next = o;
            else
                fn->body = o;
            prev = o;
        }
    }
}
