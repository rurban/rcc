/* GCC Bug #123482 - in -frounding-math, GCC moves floating-point operation across fesetround call
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123482
 */
/* { dg-do compile } */
/* { dg-options "-O1 -frounding-math" } */

#include <stdio.h>
#include <stdlib.h>
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

/* The multiplication must not be moved across the fesetround call.
 * Under FE_UPWARD, 0.1 * 0.3 rounds up to 0.030000000000000002; gcc
 * (verified: gcc 16 at -O1/-O2 with -frounding-math) reorders the
 * multiply after fesetround(FE_TONEAREST) and computes the wrong
 * result 0.029999999999999999.  Compile-only here since the wrong code
 * is the bug being demonstrated. */
int main(void)
{
    const int rounding_modes[] = {
        FE_TONEAREST,
        FE_DOWNWARD,
        FE_UPWARD,
        FE_TOWARDZERO
    };

    size_t idx = 2;   /* FE_UPWARD */
    int mode = rounding_modes[idx];

    fesetround(mode);

    double a = 0.1;
    double b = 0.3;

    double result = a * b;   /* must round upward */

    fesetround(FE_TONEAREST);

    if (result != 0.030000000000000002)
        __builtin_abort();
    return 0;
}
