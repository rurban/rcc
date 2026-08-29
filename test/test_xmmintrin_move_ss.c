/* _mm_move_ss (SSE1): sets the low single-precision lane of the result
 * from B's low lane; the upper three lanes come from A unchanged.
 *
 * Regression: rcc's bundled <xmmintrin.h> is a self-contained
 * reimplementation of the SSE intrinsics (not GCC's own header), and
 * simply never implemented _mm_move_ss at all -- every other single/
 * double lane-move intrinsic existed (_mm_move_sd in <emmintrin.h>,
 * _mm_movehl_ps/_mm_movelh_ps here), but this one was missing. A call
 * to it therefore had no declaration anywhere, fell through to an
 * implicit (K&R-style) function call, and linked as "undefined
 * reference to `_mm_move_ss`".
 *
 * Found via a real PHP build: ext/hash/hash_sha_sse2.c's SHA
 * implementation calls _mm_move_ss(_mm_castsi128_ps(a),
 * _mm_castsi128_ps(b)) directly.
 */
#include <xmmintrin.h>

#if !defined(__aarch64__) && !defined(_M_ARM64)
int main(void) {
    __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f); /* {1,2,3,4} */
    __m128 b = _mm_set_ps(40.0f, 30.0f, 20.0f, 10.0f); /* {10,20,30,40} */
    __m128 r = _mm_move_ss(a, b);
    float out[4];
    _mm_storeu_ps(out, r);
    if (out[0] != 10.0f) return 1; /* low lane from b */
    if (out[1] != 2.0f) return 2; /* upper lanes from a */
    if (out[2] != 3.0f) return 3;
    if (out[3] != 4.0f) return 4;
    return 0;
}
#else
int main(void) { return 0; }
#endif
