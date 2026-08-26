/* A struct/union compound literal appearing as a plain runtime expression
 * (e.g. a function-call argument) triggered a speculative "maybe this is
 * actually foldable to a compile-time constant" attempt, so member access
 * on it could later constant-fold too (mirrors what a genuine `constexpr`
 * or file-scope object gets). That attempt's own "is everything here
 * plausibly constant" prescan blindly skips over any PARENTHESIZED group
 * without checking its contents, so `.member = (some_function_call(...))`
 * looked "all constant" (the whole call is swallowed as one opaque
 * parenthesized span) and the speculative fold was attempted anyway.
 *
 * Two stacked bugs followed from there:
 *
 * 1. The fold then failed for a genuine reason (the value is a runtime
 *    function call, or takes the address of a local variable) deep inside
 *    global_init_one()'s "expected constant expression in initializer"
 *    check -- but that check unconditionally treated any failure as a
 *    HARD COMPILE ERROR, with no way to say "this was only a speculative,
 *    best-effort attempt; failing is fine, just don't fold it."
 *
 * 2. Compounding it: attempting the fold sets `in_global_var_init = true`
 *    for its whole call tree (so a genuinely nested static/global compound
 *    literal reached through a REAL global initializer gets correctly
 *    classified as file-scope, C11 6.5.2.5p10). But when the OUTER
 *    attempt is itself only speculative (not a real global initializer),
 *    that flag wrongly reclassifies any compound literal nested inside it
 *    as file-scope too, forcing `is_local = false` on a genuinely-local
 *    object.
 *
 * Found via postgres's `pg_list.h`:
 *   #define list_make_ptr_cell(v)  ((ListCell) {.ptr_value = (v)})
 *   #define list_make1(x1) list_make1_impl(T_List, list_make_ptr_cell(x1))
 * and `dependency.c`'s `context.rtables = list_make1(list_make1(&rte));`
 * (rte a local `RangeTblEntry`) -- rcc hard-rejected this completely valid
 * C with "expected constant expression in initializer", even though
 * nothing here is used in a constant-expression context at all.
 *
 * Fixed by: (1) letting global_init_one()/global_initializer_impl()'s
 * "expected constant expression" checks fail quietly (via a new
 * `in_speculative_const_fold` flag) instead of hard-erroring when only
 * this speculative fold is in flight; (2) tracking whether the fold
 * actually succeeded end to end (`speculative_fold_failed`) so a partial
 * failure can't leave `var->has_init`/`var->is_constexpr` set with a mix
 * of correctly-folded and garbage bytes for later constant-expression
 * reads to silently pick up. */
#include <stdlib.h>

typedef union ListCell {
    void *ptr_value;
    int int_value;
} ListCell;

typedef struct List {
    int n;
    void *data;
} List;

static List *mk_impl(int t, ListCell datum1) {
    List *l = malloc(sizeof(*l));
    l->n = t;
    l->data = datum1.ptr_value;
    return l;
}

#define mk_cell(v)  ((ListCell) {.ptr_value = (v)})
#define mk1(x1) mk_impl(1, mk_cell(x1))

int main(void) {
    int local = 42;
    /* Doubly-nested: the OUTER mk1's compound-literal argument contains,
     * as its member value, a call to mk1 whose OWN compound-literal
     * argument takes the address of a genuine local. Must simply compile
     * and run correctly -- no constant-expression context involved. */
    List *outer = mk1(mk1(&local));
    if (!outer || outer->n != 1)
        return 1;
    List *inner = (List *)outer->data;
    if (!inner || inner->n != 1)
        return 1;
    int *back = (int *)inner->data;
    if (!back || *back != 42)
        return 1;
    return 0;
}
