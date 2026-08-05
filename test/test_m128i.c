/* Unit tests for SSE2 __m128i intrinsics (include/emmintrin.h) added for
 * xxHash's XXH3/XXH128 SSE2 accumulate/scramble path: _mm_shuffle_epi32
 * (32-bit lane shuffle) and _mm_mul_epu32 (low-32x32->64 unsigned
 * multiply of each 64-bit lane). Verified bit-for-bit against GCC. */
#include <stdio.h>
#include <emmintrin.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

static int veqi32(__m128i v, int e0, int e1, int e2, int e3) {
    int o[4];
    _mm_storeu_si128((__m128i *)o, v);
    return o[0] == e0 && o[1] == e1 && o[2] == e2 && o[3] == e3;
}

static int veqi64(__m128i v, long long e0, long long e1) {
    long long o[2];
    _mm_storeu_si128((__m128i *)o, v);
    return o[0] == e0 && o[1] == e1;
}

int main(void) {
    __m128i a = _mm_set_epi32(4, 3, 2, 1); /* lanes: 0=1, 1=2, 2=3, 3=4 */

    /* _mm_shuffle_epi32: imm's 2-bit fields select the source lane for
     * each destination lane (dst[i] = src[(imm >> 2*i) & 3]). */
    CHECK(veqi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(0, 1, 2, 3)), 4, 3, 2, 1));
    CHECK(veqi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(3, 2, 1, 0)), 1, 2, 3, 4));
    CHECK(veqi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(0, 0, 0, 0)), 1, 1, 1, 1));
    /* Cross-checked against GCC: _MM_SHUFFLE(0,3,0,1) on {1,2,3,4}. */
    CHECK(veqi32(_mm_shuffle_epi32(a, _MM_SHUFFLE(0, 3, 0, 1)), 2, 1, 4, 1));

    /* _mm_mul_epu32: multiplies the low unsigned 32 bits of each 64-bit
     * lane, producing full 64-bit unsigned products (odd 32-bit lanes
     * of the inputs are ignored). */
    __m128i b = _mm_set_epi32(0, 3, 0, 3); /* lanes: 0=3, 1=0, 2=3, 3=0 */
    CHECK(veqi64(_mm_mul_epu32(a, b), 3, 9)); /* lane0: 1*3=3; lane2: 3*3=9 */

    /* Bit-exact cross-check against a value GCC was verified against. */
    __m128i c = _mm_set_epi32((int)0x11223344, (int)0x55667788, (int)0x99AABBCC, (int)0xDDEEFF00);
    __m128i d = _mm_set_epi32((int)0x11111111, (int)0x22222222, (int)0x33333333, (int)0x44444444);
    CHECK(veqi32(_mm_shuffle_epi32(c, _MM_SHUFFLE(0, 3, 0, 1)),
                 (int)0x99AABBCC, (int)0xDDEEFF00, (int)0x11223344, (int)0xDDEEFF00));
    CHECK(veqi64(_mm_mul_epu32(c, d), (long long)0x3B2EAA662B37BC00LL, (long long)0x0B62FEDEE38BF010LL));

    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
