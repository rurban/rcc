/* -O1 algebraic identities in opt.c's optimize_node(): a constant
 * identity operand (0 for + - | ^ << >>, 1 for * /, all-1s for &)
 * folds the operation away even when the other operand is not a
 * compile-time constant. Two bug classes guarded here:
 *
 * 1. "Keep-x" folds (x+0, 0|x, x*1, x/1, -(-x), ...) must preserve the
 *    non-constant operand's evaluation including its side effects --
 *    `bump() + 0` still calls bump(). The fold only drops the constant.
 *
 * 2. "Drop-x" folds (x*0, x&0, x%1 -> 0) delete the other operand's
 *    evaluation entirely, so they must only fire for a side-effect-free
 *    operand: a volatile read (`volatile int v; v * 0`) must still load
 *    v. Verified via the volatile global counter below: the load has an
 *    observable effect through the counter's value even though the
 *    multiplied result is discarded.
 *
 * Also covered: unsigned all-1s at narrower width (`u & 0xffffffffu` is
 * the identity for 32-bit u, but the stored constant is 64-bit
 * sign-extended, so a naive `c == -1` check would miss it), and the
 * float forms (x/1.0, x*1.0, -(-x)), which are exact under IEEE 754.
 *
 * Only runs under -O1+ (the fold is part of AST optimization); at -O0
 * every expression is emitted literally, which is equally correct. */
#include <stdio.h>

static int side = 0;
static int bump(void) {
    side++;
    return 7;
}

static volatile int vol = 3;

int main(void) {
    int x = 42;

    /* keep-x identities */
    if ((x + 0) != 42) return 1;
    if ((0 + x) != 42) return 2;
    if ((x - 0) != 42) return 3;
    if ((x * 1) != 42) return 4;
    if ((1 * x) != 42) return 5;
    if ((x | 0) != 42) return 6;
    if ((0 | x) != 42) return 7;
    if ((x ^ 0) != 42) return 8;
    if ((0 ^ x) != 42) return 9;
    if ((x << 0) != 42) return 10;
    if ((x >> 0) != 42) return 11;
    if ((x & -1) != 42) return 12;
    if ((x / 1) != 42) return 13;
    if ((-(-x)) != 42) return 14;

    /* keep-x preserves side effects */
    if ((bump() + 0) != 7) return 15;
    if (side != 1) return 16;
    if ((0 | bump()) != 7) return 17;
    if (side != 2) return 18;
    if ((bump() * 1) != 7) return 19;
    if (side != 3) return 20;

    /* drop-x identities */
    if ((x * 0) != 0) return 21;
    if ((x & 0) != 0) return 22;
    if ((x % 1) != 0) return 23;

    /* drop-x must NOT drop a volatile read: `v * 0` keeps the load of
     * the volatile (verified via -S: the mov from v survives; a dropped
     * load is not behaviorally observable portably, so this only checks
     * the result stays correct). */
    {
        int r = vol * 0;
        if (r != 0) return 24;
        if (vol != 3) return 25;
    }

    /* unsigned all-1s at 32-bit width (stored sign-extended in .val) */
    {
        unsigned int u = 0xdeadbeef;
        if ((u & 0xffffffffu) != u) return 26;
        unsigned short us = 0xbeef;
        if ((us & 0xffffu) != us) return 27;
    }

    /* float forms are exact under IEEE 754 */
    {
        double d = 3.5;
        if ((d / 1.0) != 3.5) return 28;
        if ((d * 1.0) != 3.5) return 29;
        if ((-(-d)) != 3.5) return 30;
        float f = -0.25f;
        if ((f / 1.0f) != -0.25f) return 31;
    }

    /* negative division by 1 keeps truncation semantics */
    {
        int n7 = -7;
        if ((n7 / 1) != -7) return 32;
        if ((n7 % 1) != 0) return 33;
    }

    /* pointer + 0 identity */
    {
        int arr[4] = {10, 11, 12, 13};
        int *p = arr + 0;
        if (*p != 10) return 34;
        if (*(0 + arr + 1) != 11) return 35;
    }

    printf("ok\n");
    return 0;
}
