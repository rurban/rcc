/* Minimal __m64 support: _mm_storeh_pi/_mm_storel_pi store the high/low
 * 64 bits (a packed pair of floats) of an __m128 through a `__m64 *`
 * pointer -- the pre-SSE2 (movlps/movhps-era) way some third-party code
 * still spells a 64-bit float-pair store. Found via blosc2's
 * blosc/bitshuffle-sse2.c: "undeclared variable" on `(__m64 *)` casts,
 * since rcc had no __m64 type at all. */
#include <xmmintrin.h>

int main(void) {
    __m128 v = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f); /* lanes: 1,2,3,4 */
    float lo[2] = {0, 0}, hi[2] = {0, 0};
    _mm_storel_pi((__m64 *)lo, v);
    _mm_storeh_pi((__m64 *)hi, v);
    if (lo[0] != 1.0f || lo[1] != 2.0f) return 1;
    if (hi[0] != 3.0f || hi[1] != 4.0f) return 2;
    return 0;
}
