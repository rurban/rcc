/* Regression test: GCC's __float128 keyword was not recognized as a type
 * at all (only the C23 spelling _Float128 was) -- rcc treated `__float128`
 * as an undeclared identifier. fftw3.h's quad-precision API section is
 * gated purely on `__GNUC__`/arch version checks (independent of whether
 * quad-precision support is actually built), so every x86_64 build hit
 * `typedef _Complex float __attribute__((mode(TC))) fftwq_complex;`-style
 * declarations naming `__float128`, and the whole header -- and every
 * translation unit that includes it -- failed to parse.
 *
 * rcc has no real 128-bit binary float arithmetic; __float128 is aliased
 * to `long double` (matching the existing _Float128 alias), which is
 * enough for type declarations, pointers, and struct members to parse
 * and size/align correctly -- real quad-precision codegen is out of
 * scope, same limitation _Float128 already has on x86_64.
 */

#include <stdio.h>

typedef __float128 quad_complex[2];
typedef struct { __float128 re, im; } quad_pair;

static __float128 identity(__float128 x) { return x; }

int main(void) {
    __float128 a = 1.0Q;
    __float128 b = a + 1.0Q;
    quad_complex c;
    quad_pair p;
    __float128 *ptr = &a;

    (void)c;
    p.re = a;
    p.im = b;

    if (sizeof(__float128) != sizeof(long double)) {
        printf("FAIL: sizeof(__float128)=%zu != sizeof(long double)=%zu\n",
               sizeof(__float128), sizeof(long double));
        return 1;
    }
    if ((double)*ptr != 1.0) {
        printf("FAIL: __float128 pointer roundtrip\n");
        return 1;
    }
    if ((double)identity(p.re) != 1.0 || (double)p.im != 2.0) {
        printf("FAIL: __float128 struct member / function arg\n");
        return 1;
    }

    printf("ALL __float128 KEYWORD TESTS PASSED\n");
    return 0;
}
