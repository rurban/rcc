/* Regression test for eval_const_expr()'s ND_MEMBER fold (enabled at -O1+
 * via CTFE): a plain (non-`const`) global struct/union with a literal-only
 * initializer must NOT be treated as a compile-time constant just because
 * it *starts* as one.
 *
 * Root cause (parser.c's eval_const_expr(), ND_MEMBER case): the fold's
 * guard was `root_var->is_constexpr || !root_var->is_local` -- for any
 * *global* variable (`!is_local` is true for every global, `const` or not)
 * with a static initializer, a `global.member`-style read got folded to
 * the value baked into the *initializer*, permanently, for the rest of
 * compilation -- even though an ordinary mutable global is written to at
 * runtime throughout the program. Once one `if (global.member == X)` folds
 * to a fixed compile-time answer, that branch is gone forever: no later
 * write to `global.member` can ever change which side of the `if` runs.
 *
 * Found via bash's `struct dstack dstack = { NULL, 0, 0 };` (parse.y):
 * `current_delimiter(dstack)` (`dstack.delimiter_depth ? ... : 0`) folded
 * to a permanent 0, so every `if (current_delimiter(dstack) == '\'') ...`
 * quote-tracking check throughout the hand-written parser silently always
 * took the "not quoted" branch. That broke the delicate alias-recursion
 * guard in shell_getc() (parse.y) badly enough that alias expansion never
 * terminated: `test/third_party/test_bash`'s own `tests/alias4.sub` (via
 * `shopt -s expand_aliases; alias command=command; eval 'command true'`)
 * hung at any `-O1`+ build, though `-O0` (which skips CTFE) passed.
 *
 * Fixed by requiring genuine immutability: `is_constexpr` (real
 * `constexpr` declarations and constant-valued compound literals), or a
 * non-local variable that is *also* `const`-qualified.
 */
#include <assert.h>

struct flags_t {
    char flags;
};

static struct flags_t g_flags = {0};
#define FLAG_BIT 0x2

static int check(void) {
    if (g_flags.flags & FLAG_BIT)
        return 100;
    return 200;
}

/* A genuinely `const` global with a literal initializer must still fold --
 * this is the case the `!is_local` disjunct was meant to preserve, and the
 * fix must not regress it. */
struct pair_t {
    int a;
    int b;
};
static const struct pair_t g_pair = {3, 4};

int main(void) {
    /* g_flags.flags starts at 0: the bit is clear. */
    assert(check() == 200);

    /* Mutate the *global* at runtime -- a later read of the same member
     * must observe this write, not the stale static-initializer value. */
    g_flags.flags |= FLAG_BIT;
    assert(check() == 100);

    g_flags.flags &= ~FLAG_BIT;
    assert(check() == 200);

    /* Real const globals must still constant-fold (e.g. usable as an
     * array bound via sizeof/static_assert-style contexts elsewhere). */
    int arr[g_pair.a + g_pair.b];
    assert(sizeof(arr) / sizeof(arr[0]) == 7);

    return 0;
}
