// GNU `a ?: b` (omitted then-operand) extension: this means `a ? a : b`
// except `a` is evaluated exactly once. Two related bugs:
//
//  1. The RESULT type of the expression must go through the same usual
//     arithmetic conversions as an ordinary `a ? a : b` ternary (e.g.
//     `int ?: unsigned long long` promotes to `unsigned long long`), not
//     default to the bare type of the condition.
//  2. `a` must really only be evaluated once: rcc's codegen generated the
//     condition's side effects once for the branch test and then, on the
//     true path, generated `node->then` completely independently -- since
//     the omitted form's `then` is the exact same subexpression object as
//     `cond` (or a cast wrapping it, inserted for the usual-arithmetic-
//     conversion above), that re-ran the condition's side effects a second
//     time.
//
// Mirrors michaelforney/cproc's test/conditional-omit.c, plus additional
// coverage for the single-evaluation requirement and mixed float/int
// promotion.
#include <assert.h>

int main(void) {
    // cproc's own case: int ?: unsigned long long must promote to ULL, and
    // ++x must fire exactly once (x must end up 2, not 3).
    int x = 1;
    if ((++x ?: 0ull) != 2)
        return 1;
    if (x != 2)
        return 2;

    // Plain matching-type case: still must evaluate the condition once.
    int y = 1;
    int r = (++y ?: 0);
    if (r != 2 || y != 2)
        return 3;

    // Promotion to a floating type.
    int z = 1;
    double d = (++z ?: 0.0);
    if (d != 2.0 || z != 2)
        return 4;

    // Constant-expression use must still fold (no runtime evaluation issue).
    static_assert((5 ?: 3) == 5, "constant omitted-ternary must fold");
    int arr[(0 ?: 7)];
    if (sizeof(arr) / sizeof(arr[0]) != 7)
        return 5;

    return 0;
}
