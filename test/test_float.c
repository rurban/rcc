/* Test _FloatN / _FloatNx type aliases and F32/F64/F128/F32x/F64x suffixes.
 *
 * When __STDC_IEC_60559_TYPES__ is defined the real extended float types are
 * tested.  When it is absent (rcc's current state) we verify the alias
 * behaviour:
 *   _Float32  -> float
 *   _Float64  -> double
 *   _Float32x -> double   (F32x suffix)
 *   _Float64x -> long double (F64x suffix)
 *   _Float128 -> long double on ARM64; stub+warning on x86 */
#include <stdio.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

/* ── __STDC_IEC_60559_TYPES__: real extended float types ────────────────── */
#ifdef __STDC_IEC_60559_TYPES__
static void test_types(void) {
    CHECK(sizeof(_Float32)  == 4);
    CHECK(sizeof(_Float64)  == 8);
    CHECK(sizeof(_Float32x) >= 8);   /* at least double width */
    CHECK(sizeof(_Float64x) >= 8);

    /* F32 / F64 / F32x / F64x literal suffixes */
    _Float32  a = 1.5F32;
    _Float64  b = 2.5F64;
    _Float32x c = 3.5F32x;
    _Float64x d = 4.5F64x;
    CHECK((float)a         == 1.5f);
    CHECK((double)b        == 2.5);
    CHECK((double)c        == 3.5);
    CHECK((long double)d   == 4.5L);

    /* lowercase suffixes */
    _Float32 e = 1.5f32;
    _Float64 f = 2.5f64;
    CHECK((float)e  == 1.5f);
    CHECK((double)f == 2.5);

    /* arithmetic */
    _Float64 x = 1.0F64, y = 2.0F64;
    CHECK((double)(x + y) == 3.0);
    CHECK((double)(y - x) == 1.0);
    CHECK((double)(x * y) == 2.0);
    CHECK((double)(y / x) == 2.0);

    /* struct and array */
    struct { _Float32 x; _Float64 y; } s = {1.0F32, 2.0F64};
    CHECK((float)s.x  == 1.0f);
    CHECK((double)s.y == 2.0);

    _Float32 arr[3] = {1.0F32, 2.0F32, 3.0F32};
    CHECK((float)arr[0] == 1.0f);
    CHECK((float)arr[1] == 2.0f);
    CHECK((float)arr[2] == 3.0f);

#ifdef __SIZEOF_FLOAT128__
    /* _Float128 when the compiler supports it */
    CHECK(sizeof(_Float128) == 16);
    _Float128 g = 1.5F128;
    CHECK((long double)g == 1.5L);
#endif
}

#else /* alias fallback */

static void test_aliases(void) {
    /* sizeof matches the aliased binary types */
    CHECK(sizeof(_Float32)  == sizeof(float));
    CHECK(sizeof(_Float64)  == sizeof(double));

    /* F32 / F64 suffixes */
    _Float32 a = 1.5F32;
    _Float64 b = 2.5F64;
    CHECK((float)a  == 1.5f);
    CHECK((double)b == 2.5);

    /* lowercase suffixes */
    _Float32 c = 1.5f32;
    _Float64 d = 2.5f64;
    CHECK((float)c  == 1.5f);
    CHECK((double)d == 2.5);

    /* _Float32x -> double, _Float64x -> long double */
    CHECK(sizeof(_Float32x) == sizeof(double));
    CHECK(sizeof(_Float64x) == sizeof(long double));

    _Float32x fx32 = 1.25F32x;
    _Float64x fx64 = 1.25F64x;
    CHECK((double)fx32      == 1.25);
    CHECK((long double)fx64 == 1.25L);

    /* lowercase F32x / F64x */
    _Float32x fx32l = 1.25f32x;
    _Float64x fx64l = 1.25f64x;
    CHECK((double)fx32l      == 1.25);
    CHECK((long double)fx64l == 1.25L);

    /* F128: long double on all platforms as stub */
    CHECK(sizeof(_Float128) == sizeof(long double));
    _Float128 e = 1.5F128;
    CHECK((long double)e == 1.5L);

    /* arithmetic */
    _Float32 x = 1.0, y = 2.0;
    CHECK((float)(x + y) == 3.0f);
    CHECK((float)(y - x) == 1.0f);
    CHECK((float)(x * y) == 2.0f);
    CHECK((float)(y / x) == 2.0f);

    /* struct fields */
    struct { _Float32 x; _Float64 y; } s;
    s.x = 10.0; s.y = 20.0;
    CHECK((float)s.x  == 10.0f);
    CHECK((double)s.y == 20.0);

    /* array */
    _Float32 arr[3] = {1.0, 2.0, 3.0};
    CHECK((float)arr[0] == 1.0f);
    CHECK((float)arr[1] == 2.0f);
    CHECK((float)arr[2] == 3.0f);
}
#endif /* __STDC_IEC_60559_TYPES__ */

int main(void) {
#ifdef __STDC_IEC_60559_TYPES__
    test_types();
#else
    test_aliases();
#endif
    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
