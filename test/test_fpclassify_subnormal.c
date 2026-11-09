/* fpclassify()/isnormal() must tell subnormals apart from zero and normals.
 * __builtin_fpclassify reloaded the exponent mask instead of the argument
 * before the mantissa test, so every subnormal double came back as FP_ZERO
 * (Tcl: [fpclassify 1e-314] gave "zero"). A float argument is promoted to
 * double on the way in, where float subnormals are ordinary normal numbers,
 * so float needs its own range check. Expected values are written with the
 * FP_* names, since their numbering differs between libcs.
 */
#include <math.h>
#include <stdio.h>

static int fails;
#define CHECK(expr, want)                                         \
    do {                                                          \
        int got_ = (expr);                                        \
        if (got_ != (want)) {                                     \
            printf("FAIL line %d: %s = %d, want %d\n", __LINE__, #expr, got_, (want)); \
            fails++;                                              \
        }                                                         \
    } while (0)

int main(void) {
    volatile double d_sub = 1e-314, d_nsub = -1e-314, d_min = 4.9406564584124654e-324;
    volatile double d_tiny = 9.997e-320, d_minnorm = 2.2250738585072014e-308;
    volatile double d_zero = 0.0, d_nzero = -0.0, d_one = 1.0, d_big = 1e300;
    volatile double d_inf = INFINITY, d_nan = NAN;

    CHECK(fpclassify(d_sub), FP_SUBNORMAL);
    CHECK(fpclassify(d_nsub), FP_SUBNORMAL);
    CHECK(fpclassify(d_min), FP_SUBNORMAL);
    CHECK(fpclassify(d_tiny), FP_SUBNORMAL);
    CHECK(fpclassify(d_minnorm), FP_NORMAL);
    CHECK(fpclassify(d_zero), FP_ZERO);
    CHECK(fpclassify(d_nzero), FP_ZERO);
    CHECK(fpclassify(d_one), FP_NORMAL);
    CHECK(fpclassify(d_big), FP_NORMAL);
    CHECK(fpclassify(d_inf), FP_INFINITE);
    CHECK(fpclassify(d_nan), FP_NAN);
    CHECK(isnormal(d_sub), 0);
    CHECK(isnormal(d_one), 1);
    CHECK(isnormal(d_zero), 0);

    volatile float f_sub = 1e-40f, f_nsub = -1e-40f, f_min = 1.0e-45f;
    volatile float f_minnorm = 1.17549435e-38f, f_zero = 0.0f, f_one = 1.0f, f_big = 3e38f;
    volatile float f_inf = INFINITY, f_nan = NAN;

    CHECK(fpclassify(f_sub), FP_SUBNORMAL);
    CHECK(fpclassify(f_nsub), FP_SUBNORMAL);
    CHECK(fpclassify(f_min), FP_SUBNORMAL);
    CHECK(fpclassify(f_minnorm), FP_NORMAL);
    CHECK(fpclassify(f_zero), FP_ZERO);
    CHECK(fpclassify(f_one), FP_NORMAL);
    CHECK(fpclassify(f_big), FP_NORMAL);
    CHECK(fpclassify(f_inf), FP_INFINITE);
    CHECK(fpclassify(f_nan), FP_NAN);
    CHECK(isnormal(f_sub), 0);
    CHECK(isnormal(f_min), 0);
    CHECK(isnormal(f_minnorm), 1);
    CHECK(isnormal(f_one), 1);

    if (fails) return 1;
    printf("OK\n");
    return 0;
}
