/* uint64_t -> double/float conversion for values with the sign bit set
 * used a manual "halve, convert, double" fallback (cvtsi2sd only handles
 * signed 64-bit ranges). The halving step dropped the shifted-out bit
 * instead of folding it back in as a sticky/round-to-odd bit, so results
 * near a mantissa rounding boundary were silently corrupted by up to
 * 2 ULP. Found via yyjson's test_number (parsing "-9223372036854776833").
 */
#include <stdint.h>

static volatile uint64_t vals[] = {
    9223372036854776833ULL, /* the yyjson repro: rounds to ...777856, not ...775808 */
    18446744073709551615ULL, /* UINT64_MAX: rounds up to 2^64 */
    9223372036854775809ULL,  /* INT64_MAX+2: rounds down to 2^63 */
    9223372036854775808ULL,  /* exactly 2^63: exact, no rounding */
    0x8000000000000001ULL,
};

int main(void) {
    double expect[] = {
        9223372036854777856.0,
        18446744073709551616.0,
        9223372036854775808.0,
        9223372036854775808.0,
        9223372036854775808.0,
    };
    for (int i = 0; i < 5; i++) {
        double d = (double)vals[i];
        if (d != expect[i]) return i + 1;
    }

    volatile uint64_t big = 9223372036854776833ULL;
    float f = (float)big;
    if (f != 9223372036854775808.0f) return 10; /* float has fewer mantissa bits: rounds to 2^63 */

    return 0;
}
