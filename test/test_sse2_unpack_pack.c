/* SSE2 <emmintrin.h> intrinsics missing from rcc's bundled header:
 * _mm_shufflelo_epi16/_mm_shufflehi_epi16 (compile-time-constant 16-bit
 * half-lane permutes), the full _mm_unpacklo/unpackhi_epi{8,16,32,64}
 * interleave family, saturating _mm_packs_epi16/_mm_packus_epi16/
 * _mm_packs_epi32, and _mm_maskmoveu_si128. All plain lane-wise
 * arithmetic, no new machine instructions needed. Found via blosc2's
 * blosc/shuffle-sse2.c (bit/byte shuffle network implementation),
 * which failed with "lvalue required as left operand of assignment"
 * on `xmm0[k] = _mm_shufflelo_epi16(...)` /
 * `xmm0[k] = _mm_unpacklo_epi8(...)` -- these functions didn't exist,
 * so the calls were implicit-int declarations, and assigning their
 * (bogus int) result to a __m128i array element wasn't a valid
 * lvalue-typed assignment. */
#include <emmintrin.h>
#include <string.h>

static int eq8(__m128i v, const short *e) {
    short buf[8];
    _mm_storeu_si128((__m128i *)buf, v);
    return memcmp(buf, e, sizeof(buf)) == 0;
}

int main(void) {
    short in[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    __m128i v = _mm_loadu_si128((const __m128i *)in);

    short lo_exp[8] = {3, 2, 1, 0, 4, 5, 6, 7};
    if (!eq8(_mm_shufflelo_epi16(v, 0x1B), lo_exp)) return 1;
    short hi_exp[8] = {0, 1, 2, 3, 7, 6, 5, 4};
    if (!eq8(_mm_shufflehi_epi16(v, 0x1B), hi_exp)) return 2;

    /* unpacklo/hi_epi8 */
    __m128i a8 = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    __m128i b8 = _mm_set_epi8(115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103,
                              102, 101, 100);
    unsigned char lo8[16], hi8[16];
    _mm_storeu_si128((__m128i *)lo8, _mm_unpacklo_epi8(a8, b8));
    _mm_storeu_si128((__m128i *)hi8, _mm_unpackhi_epi8(a8, b8));
    for (int i = 0; i < 8; i++) {
        if (lo8[i * 2] != i || lo8[i * 2 + 1] != 100 + i) return 3;
        if (hi8[i * 2] != i + 8 || hi8[i * 2 + 1] != 108 + i) return 4;
    }

    /* unpacklo/hi_epi32, unpacklo/hi_epi64 */
    __m128i a32 = _mm_set_epi32(3, 2, 1, 0);
    __m128i b32 = _mm_set_epi32(103, 102, 101, 100);
    int buf32[4];
    _mm_storeu_si128((__m128i *)buf32, _mm_unpacklo_epi32(a32, b32));
    if (buf32[0] != 0 || buf32[1] != 100 || buf32[2] != 1 || buf32[3] != 101) return 5;
    _mm_storeu_si128((__m128i *)buf32, _mm_unpackhi_epi32(a32, b32));
    if (buf32[0] != 2 || buf32[1] != 102 || buf32[2] != 3 || buf32[3] != 103) return 6;

    long long buf64[2];
    __m128i a64 = _mm_set_epi64x(1, 0), b64 = _mm_set_epi64x(101, 100);
    _mm_storeu_si128((__m128i *)buf64, _mm_unpacklo_epi64(a64, b64));
    if (buf64[0] != 0 || buf64[1] != 100) return 7;
    _mm_storeu_si128((__m128i *)buf64, _mm_unpackhi_epi64(a64, b64));
    if (buf64[0] != 1 || buf64[1] != 101) return 8;

    /* Saturating packs */
    __m128i s1 = _mm_set_epi32((300 << 16) | 300, 0, 0, 0); /* build via epi16 below instead */
    (void)s1;
    short w[8] = {200, -200, 30000, -30000, 0, 1, -1, 127};
    __m128i wv = _mm_loadu_si128((const __m128i *)w);
    signed char packed[16];
    _mm_storeu_si128((__m128i *)packed, _mm_packs_epi16(wv, wv));
    signed char exp_packed[8] = {127, -128, 127, -128, 0, 1, -1, 127};
    for (int i = 0; i < 8; i++) {
        if (packed[i] != exp_packed[i]) return 9;
        if (packed[i + 8] != exp_packed[i]) return 10;
    }

    /* maskmoveu: only lanes with mask high bit set are stored */
    unsigned char dst[16];
    memset(dst, 0xAA, sizeof(dst));
    __m128i data = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    __m128i mask = _mm_set_epi8(-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0);
    _mm_maskmoveu_si128(data, mask, (char *)dst);
    for (int i = 0; i < 16; i++) {
        if (i % 2 == 0) {
            if (dst[i] != 0xAA) return 11; /* untouched */
        } else {
            if (dst[i] != i) return 12; /* stored */
        }
    }

    return 0;
}
