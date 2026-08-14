/* `_mm_cvt_ss2si`/`_mm_cvt_si2ss`/`_mm_cvtt_ss2si` (the original,
 * pre-2001 SSE intrinsic names) and their modern equivalents
 * (`_mm_cvtss_si32`/`_mm_cvtsi32_ss`/`_mm_cvttss_si32`) were entirely
 * missing from xmmintrin.h -- the underlying codegen
 * (__builtin_ia32_cvtss2si/cvttss2si/cvtsi2ss, cg_vectors.c's scalar
 * int<->float conversion dispatch) already existed, only the header
 * wrappers were missing. Found via test_libopus.
 */
#include <xmmintrin.h>

int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    __m128 a = _mm_set_ss(3.7f);

    if (_mm_cvt_ss2si(a) != 4) return 1;    /* round to nearest */
    if (_mm_cvtss_si32(a) != 4) return 2;
    if (_mm_cvtt_ss2si(a) != 3) return 3;   /* truncate */
    if (_mm_cvttss_si32(a) != 3) return 4;

    __m128 b = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f); /* lanes: 4,3,2,1 */
    __m128 c = _mm_cvt_si2ss(b, 99);
    if (c[0] != 99.0f || c[1] != 3.0f || c[2] != 2.0f || c[3] != 1.0f)
        return 5;
    __m128 d = _mm_cvtsi32_ss(b, 77);
    if (d[0] != 77.0f) return 6;

#if defined(__x86_64__) || defined(_M_X64)
    if (_mm_cvtss_si64(a) != 4) return 7;
    if (_mm_cvttss_si64(a) != 3) return 8;
    __m128 e = _mm_cvtsi64_ss(b, 55);
    if (e[0] != 55.0f) return 9;
#endif
#endif

    return 0;
}
