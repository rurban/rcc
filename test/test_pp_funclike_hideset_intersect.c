/* Prosser's function-like invocation rule: the expansion's hide set is
 * HS(name) INTERSECT HS(')') UNION {name}. rcc used the name token's whole
 * hide set (only pasted names were intersected), so a macro name produced by
 * another macro's expansion carried that macro's name into its own body:
 * metalang99's ML99_appl_IMPL is `ML99_PRIV_IF(c, ML99_PRIV_APPL_F, ..)(f, ..)`
 * and ML99_PRIV_APPL_F's body calls ML99_PRIV_IF again, which stayed
 * unexpanded (`ML99_EVAL(ML99_appl(...))`, i.e. every datatype99 use).
 */
#include <stdio.h>

#define IF(c, x, y) IF_O(c)(x, y)
#define IF_O(c) IF_##c
#define IF_0(a, b) b
#define IF_1(a, b) a
#define APPL_F(f) IF(1, f, 0)
#define APPL(f) IF(1, APPL_F, 0)(f)

int main(void) {
    int r = APPL(5);
    if (r != 5) {
        printf("FAIL: %d\n", r);
        return 1;
    }
    printf("OK\n");
    return 0;
}
