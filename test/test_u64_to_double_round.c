/* uint64_t -> double/float conversion for values with the sign bit set
 * used a manual "halve, convert, double" fallback (cvtsi2sd only handles
 * signed 64-bit ranges). The halving step dropped the shifted-out bit
 * instead of folding it back in as a sticky/round-to-odd bit, so results
 * near a mantissa rounding boundary were silently corrupted by up to
 * 2 ULP. Found via yyjson's test_number (parsing "-9223372036854776833").
 *
 * Also covers i64/u64 -> f32, which must round ONCE: converting via f64
 * (cvtsi2sd + cvtsd2ss) double-rounds and lands 1 ULP off for |v| >= 2^53
 * (found via wasm3's spec-test f32.convert_i64_s/u).
 */
#include <stdint.h>
#include <string.h>

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

    /* i64 -> f32 single rounding (exact bit pattern from wasm3's spec tests) */
    {
        float g = (float)9007199791611905LL;
        uint32_t bits;
        memcpy(&bits, &g, 4);
        if (bits != 0x5A000001) return 20;
    }

    /* u64 -> f32 single rounding */
    static const uint64_t u64v[] = {
        9007199791611905ULL,
        9223371761976868863ULL,
        9223372586610589697ULL,
        18446744073709551615ULL,
    };
    static const uint32_t u64f32exp[] = { 0x5A000001, 0x5EFFFFFF, 0x5F000001, 0x5F800000 };
    for (int i = 0; i < 4; i++) {
        float g = (float)u64v[i];
        uint32_t bits;
        memcpy(&bits, &g, 4);
        if (bits != u64f32exp[i]) return 30 + i;
    }

    return 0;
}
