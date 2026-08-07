/* SSSE3 <tmmintrin.h> intrinsics: rcc had no bundled tmmintrin.h, so
 * including it fell through to the system's real header, which needs
 * <mmintrin.h>'s __m64 (never chained in) and GCC-internal builtins rcc
 * doesn't implement -- "expected ';' or ','" on __m64 params, and
 * "lvalue required" once a workaround got far enough to reach
 * _mm_shuffle_epi8. Found via bearssl's ghash_pclmul.c:
 * `(x) = _mm_shuffle_epi8((x), byteswap_index);`.
 *
 * _mm_shuffle_epi8 is the one intrinsic here backed by a real machine
 * instruction (PSHUFB via __builtin_ia32_pshufb128, x86-only): a
 * data-dependent per-byte-lane permute with no expressible-as-C-operators
 * equivalent, unlike abs/sign/hadd/hsub/alignr which are plain lane-wise
 * arithmetic. */
#include <tmmintrin.h>
#include <string.h>

static int veq32(__m128i v, int e0, int e1, int e2, int e3) {
    int buf[4];
    _mm_storeu_si128((__m128i *)buf, v);
    return buf[0] == e0 && buf[1] == e1 && buf[2] == e2 && buf[3] == e3;
}

int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    /* Byte-reverse each 4-byte lane via pshufb (the ghash_pclmul.c pattern).
     * __builtin_ia32_pshufb128 (and therefore _mm_shuffle_epi8) is x86-only
     * -- see tmmintrin.h's #ifndef ARCH_ARM64 guard. */
    __m128i idx = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
    unsigned char in[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    __m128i v = _mm_loadu_si128((const __m128i *)in);
    v = _mm_shuffle_epi8(v, idx); /* (x) = _mm_shuffle_epi8((x), idx); pattern */
    unsigned char out[16];
    _mm_storeu_si128((__m128i *)out, v);
    unsigned char expect[16] = {0x04, 0x03, 0x02, 0x01, 0x08, 0x07, 0x06, 0x05,
                                0x0c, 0x0b, 0x0a, 0x09, 0x10, 0x0f, 0x0e, 0x0d};
    if (memcmp(out, expect, 16) != 0) return 1;

    /* High bit of the index byte zeroes that output lane. */
    __m128i zidx = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0);
    __m128i z = _mm_shuffle_epi8(v, zidx);
    unsigned char zout[16];
    _mm_storeu_si128((__m128i *)zout, z);
    for (int i = 8; i < 16; i++)
        if (zout[i] != 0) return 2;
#endif
    if (!veq32(_mm_abs_epi32(_mm_set_epi32(-1, 5, -100, 0)), 0, 100, 5, 1)) return 3;
    if (!veq32(_mm_hadd_epi32(_mm_set_epi32(4, 3, 2, 1), _mm_set_epi32(8, 7, 6, 5)),
               3, 7, 11, 15))
        return 4;
    if (!veq32(_mm_hsub_epi32(_mm_set_epi32(4, 3, 2, 1), _mm_set_epi32(8, 7, 6, 5)),
               -1, -1, -1, -1))
        return 5;
    if (!veq32(_mm_sign_epi32(_mm_set_epi32(4, 3, 2, 1), _mm_set_epi32(-1, 0, 1, -1)),
               -1, 2, 0, -4))
        return 6;

    /* alignr(a, b, 4): bytes 4..19 of the 32-byte b:a concatenation. */
    __m128i a16 = _mm_set_epi8(31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16);
    __m128i b16 = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    __m128i al = _mm_alignr_epi8(a16, b16, 4);
    unsigned char albuf[16];
    _mm_storeu_si128((__m128i *)albuf, al);
    for (int i = 0; i < 16; i++)
        if (albuf[i] != (unsigned char)(i + 4)) return 7;

    return 0;
}
