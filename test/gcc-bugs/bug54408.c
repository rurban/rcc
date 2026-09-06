/* GCC Bug #54408 - sqrt for vector types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54408
 */
/* { dg-do run } */

/* The original report (Marc Glisse, 2012) is a pure feature-request
 * discussion with no attached testcase: "it would be nice to add sqrt
 * to the gcc vector extensions... Component 'other' because fixing
 * this might involve front, middle and back-ends." Neither GCC nor
 * Clang has a vector-aware sqrt: math.h's sqrt(double) rejects a
 * vector argument outright (the previously scraped file used C++
 * template syntax that was never part of the actual report and isn't
 * valid C at all -- this is a faithful minimal C illustration
 * instead).
 *
 * rcc fills the requested gap for the native 2-double vector width:
 * sqrt(vector_of_2_doubles) redirects to the existing
 * __builtin_ia32_sqrtpd codegen (SQRTPD on x86-64, NEON fsqrt on
 * ARM64), applied elementwise, matching how +,-,*,/ are already
 * elementwise on GCC vector-extension types. Verified against plain
 * scalar sqrt() to full double precision. */
#include <math.h>
typedef double v2d __attribute__ ((vector_size (16)));
v2d f(v2d x) { return x + sqrt(x); }
int main(void) {
    v2d a = {2.0, 10.0};
    v2d r = f(a);
    return (r[0] == 2.0 + sqrt(2.0) && r[1] == 10.0 + sqrt(10.0)) ? 0 : 1;
}
