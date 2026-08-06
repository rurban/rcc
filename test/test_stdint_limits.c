/* Regression test: C99 stdint.h fast/least width limit macros.
 *
 * stdint.h defined int_fastN_t / uint_fastN_t / int_leastN_t /
 * uint_leastN_t typedefs but was missing the corresponding MAX/MIN
 * limit macros required by C99 7.18.2.2 and 7.18.2.3.  Affected
 * every project that used UINT_FAST64_MAX (coreutils, diffutils,
 * gpatch, gsed, gtar, and others via lib/tempname.c).
 */

#include <stdint.h>
#include <stdio.h>

static int failures;

#define assert_eq(a, b, msg) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        printf("FAIL: %s: expected %lld, got %lld\n", msg, _b, _a); \
        failures++; \
    } \
} while (0)

int main(void) {
    /* Least-width limits */
    assert_eq(INT_LEAST8_MIN,  INT8_MIN,  "INT_LEAST8_MIN");
    assert_eq(INT_LEAST8_MAX,  INT8_MAX,  "INT_LEAST8_MAX");
    assert_eq(UINT_LEAST8_MAX, UINT8_MAX, "UINT_LEAST8_MAX");
    assert_eq(INT_LEAST16_MIN, INT16_MIN, "INT_LEAST16_MIN");
    assert_eq(INT_LEAST16_MAX, INT16_MAX, "INT_LEAST16_MAX");
    assert_eq(UINT_LEAST16_MAX, UINT16_MAX, "UINT_LEAST16_MAX");
    assert_eq(INT_LEAST32_MIN, INT32_MIN, "INT_LEAST32_MIN");
    assert_eq(INT_LEAST32_MAX, INT32_MAX, "INT_LEAST32_MAX");
    assert_eq(UINT_LEAST32_MAX, UINT32_MAX, "UINT_LEAST32_MAX");
    assert_eq(INT_LEAST64_MIN, INT64_MIN, "INT_LEAST64_MIN");
    assert_eq(INT_LEAST64_MAX, INT64_MAX, "INT_LEAST64_MAX");
    assert_eq(UINT_LEAST64_MAX, UINT64_MAX, "UINT_LEAST64_MAX");

    /* Fast-width limits */
    assert_eq(INT_FAST8_MIN,  INT8_MIN,  "INT_FAST8_MIN");
    assert_eq(INT_FAST8_MAX,  INT8_MAX,  "INT_FAST8_MAX");
    assert_eq(UINT_FAST8_MAX, UINT8_MAX, "UINT_FAST8_MAX");
    assert_eq(INT_FAST16_MIN, INT16_MIN, "INT_FAST16_MIN");
    assert_eq(INT_FAST16_MAX, INT16_MAX, "INT_FAST16_MAX");
    assert_eq(UINT_FAST16_MAX, UINT16_MAX, "UINT_FAST16_MAX");
    assert_eq(INT_FAST32_MIN, INT32_MIN, "INT_FAST32_MIN");
    assert_eq(INT_FAST32_MAX, INT32_MAX, "INT_FAST32_MAX");
    assert_eq(UINT_FAST32_MAX, UINT32_MAX, "UINT_FAST32_MAX");
    assert_eq(INT_FAST64_MIN, INT64_MIN, "INT_FAST64_MIN");
    assert_eq(INT_FAST64_MAX, INT64_MAX, "INT_FAST64_MAX");
    assert_eq(UINT_FAST64_MAX, UINT64_MAX, "UINT_FAST64_MAX");

    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL STDINT LIMIT TESTS PASSED\n");
    return failures ? 1 : 0;
}
