/* Regression test: a scalar initializer wrapped in "superfluous but
 * legal" braces with a trailing comma -- `T x = { expr, };` -- failed
 * to parse ("expected specific operator"), for both local and (nested,
 * e.g. inside a designated struct-member initializer) global/static
 * scalars. C11 6.7.9p11 explicitly permits enclosing a scalar
 * initializer in braces ("the initializer for a scalar shall be a
 * single expression, optionally enclosed in braces"); 6.7.9p19's
 * general trailing-comma allowance for a brace-enclosed initializer
 * list is not restricted to array/struct targets, so `{ expr, }` is
 * exactly as legal as `{ expr }` for a scalar.
 *
 * `local_init_one()`/`global_init_one()` (parser.c) both had a
 * "superfluous braces around scalar" fallback that recursively parsed
 * the inner value, then unconditionally called `skip(tok, "}")` --
 * without first consuming an optional trailing comma, so `tok` was
 * still sitting on the `,` and `skip()` reported "expected specific
 * operator" pointing at it. Fixed by consuming an optional `,` between
 * the recursive parse and the closing-brace skip, in both functions.
 *
 * Found via util-linux's isosize.c-adjacent codebase, libgit2:
 * `network/remote/remotes.c`'s
 * `char *specs = { "refs/heads/master", };`.
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Local scalar, brace + trailing comma. */
static int local_int(void) {
    int x = {5,};
    return x;
}
static const char *local_ptr(void) {
    char *specs = {"refs/heads/master",};
    return specs;
}

/* Global/static scalar, brace + trailing comma (direct top-level form). */
int g_int = {7,};
char *g_ptr = {"origin",};

/* Nested: a designated struct-member scalar initializer with braces +
 * trailing comma -- the shape that specifically exercises
 * global_init_one()'s (as opposed to local_init_one()'s) copy of this
 * fallback. */
struct S {
    char *specs;
    int y;
};
struct S g_struct = {.specs = {"refs/heads/master",}, .y = 1};

int main(void) {
    assert(local_int() == 5);
    assert(strcmp(local_ptr(), "refs/heads/master") == 0);
    assert(g_int == 7);
    assert(strcmp(g_ptr, "origin") == 0);
    assert(strcmp(g_struct.specs, "refs/heads/master") == 0);
    assert(g_struct.y == 1);
    printf("OK\n");
    return 0;
}
