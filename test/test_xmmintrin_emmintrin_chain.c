/* Real GCC/Clang's <xmmintrin.h> auto-chains to <emmintrin.h> once SSE2 is
 * enabled (the default on x86-64): third-party code commonly includes only
 * <xmmintrin.h> and then freely uses SSE2-only intrinsics/types
 * (_mm_storeu_si128, _mm_loadu_si128, __m128i, ...) without including
 * <emmintrin.h> itself, relying on that chain. rcc's bundled <xmmintrin.h>
 * didn't chain to <emmintrin.h> at all, so any such SSE2-only reference
 * after only `#include <xmmintrin.h>` failed as an undeclared
 * identifier/type -- confirmed via pixman's test/utils/utils-prng.c
 * (`_mm_storeu_si128 (addr, _mm_loadu_si128 ((__m128i *)d));`), which
 * blocked the whole test_pixman third-party build.
 *
 * Fixed by adding the same `#ifdef __SSE2__ #include <emmintrin.h> #endif`
 * chain real GCC/Clang's <xmmintrin.h> has, at the end of rcc's own copy.
 */
#include <xmmintrin.h>

int main(void) {
    /* SSE1 type/intrinsic, works without any chaining. */
    __m128 f = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    if (_mm_cvtss_f32(f) != 1.0f) return 1;

#ifdef __SSE2__    
    /* SSE2-only type/intrinsics, only visible if <xmmintrin.h> chained to
     * <emmintrin.h> as real GCC/Clang do -- this is the exact pattern that
     * failed before the fix. */
    __m128i src = _mm_set1_epi32(0x12345678);
    __m128i buf;
    _mm_storeu_si128(&buf, _mm_loadu_si128(&src));

    int lanes[4];
    _mm_storeu_si128((__m128i *)lanes, buf);
    for (int i = 0; i < 4; i++)
        if (lanes[i] != 0x12345678) return 2;
#endif

    return 0;
}
