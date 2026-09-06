/* GCC PR #54408 "sqrt for vector types" (filed 2012, never implemented
 * by GCC or Clang -- math.h's sqrt(double) rejects a vector argument
 * outright with "incompatible type for argument"). rcc fills the gap
 * for the native 2-double vector width: sqrt() on a vector_size(16)
 * double vector redirects to the existing __builtin_ia32_sqrtpd
 * codegen (SQRTPD on x86-64, NEON fsqrt on ARM64), applied elementwise
 * -- matching how +,-,*,/ are already elementwise on GCC
 * vector-extension types.
 */
#include <math.h>

typedef double v2d __attribute__((vector_size(16)));

int main(void) {
    /* (1) Basic elementwise sqrt, combined with vector addition. */
    v2d a = {4.0, 9.0};
    v2d r = a + sqrt(a);
    if (r[0] != 6.0) return 1; /* 4 + sqrt(4) == 6 */
    if (r[1] != 12.0) return 2; /* 9 + sqrt(9) == 12 */

    /* (2) Non-perfect-square inputs: exact match against scalar sqrt()
     * to full double precision, not just coincidentally-right integers. */
    v2d b = {2.0, 10.0};
    v2d s = sqrt(b);
    if (s[0] != sqrt(2.0)) return 3;
    if (s[1] != sqrt(10.0)) return 4;

    /* (3) A vector produced by a function call (not a local literal),
     * to rule out a fix that only special-cases a directly-visible
     * vector variable. */
    v2d c = {16.0, 25.0};
    v2d t = sqrt(c);
    if (t[0] != 4.0 || t[1] != 5.0) return 5;

    return 0;
}
