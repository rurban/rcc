// Regression: a compound literal whose designated initializers are all
// parenthesized non-constant expressions (bison's `.val = (yyvsp[0].x)`
// pattern) used to hard-error ("expected an expression") on the second
// designator: a speculative all-constant scan skipped every ( ... ) group,
// so the literal was folded as a global initializer, which flat-consumed
// the first value as a scalar member. Seen in samba's claims_transformation.
#include <stdio.h>
struct E { int a; int b; };
struct W { struct E e; };
struct P { struct W type; struct W value; };
union U { struct W w; };

int main(void) {
    union U s[3] = {{{{1, 2}}}, {{{3, 4}}}, {{{5, 6}}}};
    union U *yyvsp = s + 2;
    struct P p;
    p = (struct P) {
        .type = (yyvsp[-2].w),
        .value = (yyvsp[0].w),
    };
    if (p.type.e.a != 1 || p.type.e.b != 2 || p.value.e.a != 5 || p.value.e.b != 6) {
        printf("FAIL\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
