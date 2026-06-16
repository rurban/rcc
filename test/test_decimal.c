/* Test _Decimal / IEC 60559 feature macros.
 *
 * When a standard macro is defined the corresponding real features are tested.
 * When none of the three are defined (rcc's current state) we verify the
 * alias behaviour: _DecimalN maps to float/double/long double. */
#include <stdio.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

/* ── __STDC_IEC_60559_BFP__ ─────────────────────────────────────────────── */
#ifdef __STDC_IEC_60559_BFP__
#include <float.h>
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
static void test_bfp(void) {
    /* CR_DECIMAL_DIG must be at least DECIMAL_DIG + 3 */
    CHECK(CR_DECIMAL_DIG >= DECIMAL_DIG + 3);

    /* strfromd: convert double to string */
    char buf[32];
    strfromd(buf, sizeof(buf), "%.1f", 3.14);
    CHECK(buf[0] == '3');

    /* issignaling / iseqsig */
    CHECK(!issignaling(1.0));
    CHECK(iseqsig(1.0, 1.0) == 1);
    CHECK(iseqsig(1.0, 2.0) == 0);

    /* SNANF must produce a signaling NaN */
    CHECK(issignaling((float)SNANF));

    /* fesetexcept / fetestexceptflag */
    fexcept_t flag;
    fesetexcept(FE_INVALID);
    fetestexceptflag(&flag, FE_INVALID);

    /* fegetmode / fesetmode */
    femode_t mode;
    fegetmode(&mode);
    fesetmode(&mode);
}
#endif /* __STDC_IEC_60559_BFP__ */

/* ── __STDC_IEC_60559_TYPES__ ───────────────────────────────────────────── */
#ifdef __STDC_IEC_60559_TYPES__
static void test_types(void) {
    CHECK(sizeof(_Float32)  == 4);
    CHECK(sizeof(_Float64)  == 8);
    CHECK(sizeof(_Float32x) == 8);   /* _Float32x is at least double width */

    _Float32  f32  = 1.5F32;
    _Float64  f64  = 2.5F64;
    _Float32x f32x = 3.5F32x;
    CHECK((float)f32   == 1.5f);
    CHECK((double)f64  == 2.5);
    CHECK((double)f32x == 3.5);

    /* arithmetic */
    _Float64 a = 1.0F64, b = 2.0F64;
    CHECK((double)(a + b) == 3.0);
    CHECK((double)(b - a) == 1.0);
    CHECK((double)(a * b) == 2.0);
    CHECK((double)(b / a) == 2.0);
}
#endif /* __STDC_IEC_60559_TYPES__ */

/* ── __STDC_IEC_60559_DFP__ ─────────────────────────────────────────────── */
#ifdef __STDC_IEC_60559_DFP__
static void test_dfp(void) {
    CHECK(sizeof(_Decimal32)  == 4);
    CHECK(sizeof(_Decimal64)  == 8);
    CHECK(sizeof(_Decimal128) == 16);

    /* The definitive decimal test: 0.1 + 0.2 == 0.3 exactly in decimal FP */
    _Decimal64 x = 0.1DD, y = 0.2DD, z = 0.3DD;
    CHECK(x + y == z);

    /* negative zero */
    _Decimal64 neg = -0.0DD;
    CHECK(neg == 0.0DD);

    /* DD/DF/DL literal suffixes map to the correct decimal types */
    _Decimal32  df = 1.5DF;
    _Decimal64  dd = 1.5DD;
    _Decimal128 dl = 1.5DL;
    CHECK(df == (_Decimal32)1.5DF);
    CHECK(dd == (_Decimal64)1.5DD);
    CHECK(dl == (_Decimal128)1.5DL);

    /* struct and array */
    struct { _Decimal32 x; _Decimal64 y; } s;
    s.x = 10.0DF; s.y = 20.0DD;
    CHECK(s.x == 10.0DF);
    CHECK(s.y == 20.0DD);

    _Decimal32 arr[3] = {1.0DF, 2.0DF, 3.0DF};
    CHECK(arr[0] == 1.0DF);
    CHECK(arr[1] == 2.0DF);
    CHECK(arr[2] == 3.0DF);
}
#endif /* __STDC_IEC_60559_DFP__ */

/* ── alias fallback (rcc current state: none of the three defined) ──────── */
#if !defined(__STDC_IEC_60559_BFP__) && \
    !defined(__STDC_IEC_60559_TYPES__) && \
    !defined(__STDC_IEC_60559_DFP__)
static void test_aliases(void) {
    /* __DECIMAL_BID_FORMAT__ and __DEC_EVAL_METHOD__ still present */
#ifndef __DECIMAL_BID_FORMAT__
    fprintf(stderr, "FAIL: __DECIMAL_BID_FORMAT__ not defined\n"); fail++;
#endif
#ifndef __DEC_EVAL_METHOD__
    fprintf(stderr, "FAIL: __DEC_EVAL_METHOD__ not defined\n"); fail++;
#endif

    /* sizeof matches the aliased binary types */
    CHECK(sizeof(_Decimal32)  == sizeof(float));
    CHECK(sizeof(_Decimal64)  == sizeof(double));
    CHECK(sizeof(_Decimal128) == sizeof(long double));

    /* basic scalars */
    _Decimal32  a32  = 1.5;
    _Decimal64  a64  = 2.5;
    _Decimal128 a128 = 3.5;
    CHECK((float)a32        == 1.5f);
    CHECK((double)a64       == 2.5);
    CHECK((long double)a128 == 3.5L);

    /* DD/DF/DL suffixes (aliased to double/float/long double) */
    double      dd = 1.5DD;
    float       df = 1.5DF;
    long double dl = 1.5DL;
    CHECK(dd == 1.5);
    CHECK(df == 1.5f);
    CHECK(dl == 1.5L);

    /* negative zero */
    _Decimal64 neg = -0.0;
    CHECK(neg == 0.0);

    /* arithmetic */
    _Decimal64 x = 1.0, y = 2.0;
    CHECK((double)(x + y) == 3.0);
    CHECK((double)(y - x) == 1.0);
    CHECK((double)(x * y) == 2.0);
    CHECK((double)(y / x) == 2.0);

    /* struct fields */
    struct { _Decimal32 x; _Decimal64 y; _Decimal128 z; } s;
    s.x = 10.0; s.y = 20.0; s.z = 30.0;
    CHECK((float)s.x        == 10.0f);
    CHECK((double)s.y       == 20.0);
    CHECK((long double)s.z  == 30.0L);

    /* array */
    _Decimal32 arr[3] = {1.0, 2.0, 3.0};
    CHECK((float)arr[0] == 1.0f);
    CHECK((float)arr[1] == 2.0f);
    CHECK((float)arr[2] == 3.0f);
}
#endif

int main(void) {
#ifdef __STDC_IEC_60559_BFP__
    test_bfp();
#endif
#ifdef __STDC_IEC_60559_TYPES__
    test_types();
#endif
#ifdef __STDC_IEC_60559_DFP__
    test_dfp();
#endif
#if !defined(__STDC_IEC_60559_BFP__) && \
    !defined(__STDC_IEC_60559_TYPES__) && \
    !defined(__STDC_IEC_60559_DFP__)
    test_aliases();
#endif
    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
