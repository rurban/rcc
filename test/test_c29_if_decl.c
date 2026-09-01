/* C29 (WG14 N3356/N3473): `if` accepts a declaration as its controlling
 * clause. Two forms:
 *   1. `if (T D = X; expr)`   -- declaration + separate condition expr.
 *   2. `if (T D = X)`         -- "declaration-condition": always exactly
 *      as if written `if (T D = X; D)` (the declared value itself is
 *      the condition). In both forms `D`'s scope extends through every
 *      branch, including `else`.
 */
#include "test_common.h"

static int xmit_ch(char c) { return c == 'X' ? 0 : -1; }

/* Form 2: declaration-condition, scope persists into else. */
static int test_decl_condition_else_scope(int ok)
{
    if (int err = xmit_ch(ok ? 'X' : 'Y')) {
        return err; /* nonzero: err visible and equal to xmit_ch's result */
    } else {
        return err + 1000; /* err == 0 here; distinguishes the else path */
    }
}

/* Form 1: declaration + separate condition expression. */
static int test_decl_and_expr(int n)
{
    if (int m = n * 2; m > 10) {
        return m;
    }
    return -1;
}

/* The declared object must be usable inside the `then` branch too, not
 * just as the implicit condition. */
static int test_decl_used_in_then(int n)
{
    if (int doubled = n * 2) {
        return doubled + 1;
    }
    return 0;
}

/* A declaration-condition whose value is zero must take the else path,
 * exactly like an ordinary `if (0)`. */
static int test_decl_condition_false(void)
{
    if (int zero = 0) {
        (void)zero;
        return 1; /* wrongly took the then branch */
    }
    return 0;
}

/* else-if chain: each clause's own declaration scope is independent. */
static int test_else_if_chain(int n)
{
    if (int a = n - 1; a > 0) {
        return 100 + a;
    } else if (int b = n + 1; b > 0) {
        return 200 + b;
    } else {
        return 300;
    }
}

int main(void)
{
    /* xmit_ch('X') returns 0 (success) -- falsy, so the else branch
     * runs, and must still see err == 0. */
    if (test_decl_condition_else_scope(1) != 1000) {
        printf("FAIL: xmit_ch('X')==0 should take the else branch (want 1000, got %d)\n",
               test_decl_condition_else_scope(1));
        return 1;
    }
    /* xmit_ch('Y') returns -1 (failure) -- truthy, so the then branch
     * runs and returns err itself. */
    int r = test_decl_condition_else_scope(0);
    if (r != -1) {
        printf("FAIL: then-branch err scope wrong (got %d, want -1)\n", r);
        return 2;
    }

    if (test_decl_and_expr(6) != 12) {
        printf("FAIL: decl+expr form: n=6 should give m=12\n");
        return 3;
    }
    if (test_decl_and_expr(2) != -1) {
        printf("FAIL: decl+expr form: n=2 (m=4) should take the false path\n");
        return 4;
    }

    if (test_decl_used_in_then(5) != 11) {
        printf("FAIL: declared var not usable in then-branch body\n");
        return 5;
    }

    if (test_decl_condition_false() != 0) {
        printf("FAIL: zero-valued declaration-condition took then branch\n");
        return 6;
    }

    if (test_else_if_chain(5) != 104) {
        printf("FAIL: else-if chain, n=5: want 104\n");
        return 7;
    }
    if (test_else_if_chain(0) != 201) {
        printf("FAIL: else-if chain, n=0: want 201\n");
        return 8;
    }
    if (test_else_if_chain(-1) != 300) {
        printf("FAIL: else-if chain, n=-1: want 300\n");
        return 9;
    }

    return 0;
}
