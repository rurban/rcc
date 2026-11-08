/* A ## paste result's hide set is the INTERSECTION of its operands' (C99
 * 6.10.3.3 via Prosser's glue()), not the union. With a union, the pasted
 * name inherited every macro its right operand had been produced by:
 * metalang99's `ML99_PRIV_IF_##cond`, with cond = `1` coming out of a nested
 * ML99_PRIV_IF expansion (ML99_PRIV_IS_UNTUPLE), carried ML99_PRIV_IF_1 and
 * stayed unexpanded, so every ML99_EVAL (datatype99) failed to parse.
 */
#include <stdio.h>

#define IF(c, x, y) IF_O(c)(x, y)
#define IF_O(c) IF_##c
#define IF_0(a, b) b
#define IF_1(a, b) a
#define TRUE(x) 1
#define FAST(x) 0
#define T(x) IF(1, TRUE, FAST)(x)
#define U(x) IF(IF(1, 0, 1), TRUE, FAST)(x)

int main(void) {
    int r1 = IF(T(z), 11, 22);
    int r2 = IF(U(z), 11, 22);
    if (r1 != 11 || r2 != 22) {
        printf("FAIL: %d %d\n", r1, r2);
        return 1;
    }
    printf("OK\n");
    return 0;
}
