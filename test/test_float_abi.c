// SPDX-License-Identifier: LGPL-2.1-or-later
// Regression test: x86-64 `float`-typed function-call arguments and
// parameters crossed the ABI boundary as rcc's internal double-widened
// representation (see "Float values are always stored as double in GP
// regs" in codegen.c) instead of a genuine 32-bit value in the low bits
// of an XMM register/slot. This happened to be self-consistent for
// direct rcc-to-rcc calls (both sides agreed on the same non-standard
// convention) but broke on any call crossing into externally-compiled
// code that follows the real ABI -- most visibly, any call into libm
// with a `float` argument (sinf, cosf, sqrtf, ...), silently producing
// wrong results instead of a compile error.
//
// Four distinct, independently-broken cases are covered here (found via
// real third-party builds, esp. HandmadeMath's math library):
//  1. A `float` argument/return crossing into an externally-compiled
//     (libm) function.
//  2. K&R oldstyle (no-prototype) function definitions, where the ABI
//     requires the CALLEE to expect a promoted double regardless of its
//     own declared `float` parameter type.
//  3. Genuine C99 `...` variadic float arguments, which the language
//     requires promoting to double at the call site -- x86-64 codegen
//     had this for K&R oldstyle calls but never wired it up for real
//     prototyped variadic functions.
//  4. Multiple float parameters/arguments spanning a platform's
//     register-argument limit (4 on Win64, 8 on SysV Linux) into the
//     stack-overflow path, which needed the same narrow-to-genuine-float
//     fix applied to BOTH the register path and the (separate, easy to
//     miss) stack-argument path.
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)
static int eqf_tol(float a, float b, float tol) {
    float d = a - b;
    return d < tol && d > -tol;
}

// --- 1. float args/return crossing into externally-compiled libm ---
static void test_libm_float_args(void) {
    volatile float half_pi = 1.5707963267948966f; /* not a compile-time constant to the optimizer */
    CHECK(eqf_tol(sinf(half_pi), 1.0f, 0.0001f));
    CHECK(eqf_tol(cosf(half_pi), 0.0f, 0.0001f));
    volatile float sixteen = 16.0f;
    CHECK(eqf_tol(sqrtf(sixteen), 4.0f, 0.0001f));
}

// --- 2. K&R oldstyle (no-prototype) function definition ---
static float kr_double_it(x)
float x;
{
    return 2.0f * x;
}
static void test_kr_oldstyle(void) {
    /* No prototype in scope: the caller must promote the float argument
     * to double (K&R/no-prototype default argument promotion), and the
     * oldstyle callee must narrow it back down on receipt. */
    CHECK(eqf_tol(kr_double_it(3.5f), 7.0f, 0.0001f));
    CHECK(eqf_tol(kr_double_it(3.5), 7.0f, 0.0001f)); /* literal is already double */
}

// --- 3. genuine C99 `...` variadic float promotion ---
static double sum_variadic_floats(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double total = 0.0;
    for (int i = 0; i < n; i++)
        total += va_arg(ap, double); /* C requires float varargs promoted to double */
    va_end(ap);
    return total;
}
static void test_vararg_float_promotion(void) {
    float a = 1.0f, b = 2.0f, c = 3.0f;
    double s = sum_variadic_floats(3, a, b, c);
    CHECK(eqf_tol((float)s, 6.0f, 0.0001f));
}

// --- 4. many float args crossing register->stack boundary on either ABI ---
static float sum9(float a, float b, float c, float d, float e,
                   float f, float g, float h, float i) {
    return a + b + c + d + e + f + g + h + i;
}
static void test_many_float_args(void) {
    /* 9 args: exceeds Win64's 4 combined register slots AND SysV's 8 XMM
     * argument registers, exercising both platforms' register-argument
     * path and their (separately broken) stack-overflow path together. */
    float r = sum9(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f);
    CHECK(eqf_tol(r, 45.0f, 0.0001f));
}

int main(void) {
    test_libm_float_args();
    test_kr_oldstyle();
    test_vararg_float_promotion();
    test_many_float_args();
    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
