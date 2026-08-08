/* SSE2 <emmintrin.h> baseline intrinsics missing entirely from rcc's
 * bundled header: _mm_min_epi16/_mm_max_epi16/_mm_min_epu8/_mm_max_epu8,
 * the saturating _mm_adds_/_mm_subs_ family (epi8/epi16/epu8/epu16),
 * _mm_avg_epu8/_mm_avg_epu16, _mm_mulhi_epi16/_mm_mulhi_epu16,
 * _mm_madd_epi16, _mm_sad_epu8, and the whole-register byte shifts
 * _mm_srli_si128/_mm_slli_si128 (distinct from the per-lane
 * _mm_s{r,l}li_epi{16,32,64} that did exist). All ordinary SSE2 (no
 * SSSE3+), yet entirely absent -- any code calling them saw an
 * implicit-int undeclared-function call. Found two ways: (1) GCC 15's
 * own <avx2intrin.h> reduce-operator macros call _mm_max_epi16/
 * _mm_min_epi16 by name once __builtin_shufflevector itself was fixed,
 * cascading into "expected an expression" parse errors at the call
 * site; (2) libwebp's sharpyuv_sse2.c calls _mm_max_epi16/
 * _mm_min_epi16/_mm_madd_epi16 directly, and dec_sse2.c calls
 * _mm_srli_si128/_mm_slli_si128 directly. All results below were
 * cross-checked against real gcc's <emmintrin.h> output. */
#include <emmintrin.h>
#include <assert.h>
#include <string.h>

#if !defined(__aarch64__) && !defined(_M_ARM64)

static void store8(__m128i v, short *out) { memcpy(out, &v, 16); }
static void store16b(__m128i v, unsigned char *out) { memcpy(out, &v, 16); }
static void store16sb(__m128i v, signed char *out) { memcpy(out, &v, 16); }
static void store4(__m128i v, int *out) { memcpy(out, &v, 16); }

#endif

int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    __m128i a = _mm_set_epi16(-5, -4, -3, -2, -1, 0, 1, 2);
    __m128i b = _mm_set_epi16(5, 4, 3, 2, 1, 0, -1, -2);

    short r8[8];
    store8(_mm_min_epi16(a, b), r8);
    short min_exp[8] = {-2, -1, 0, -1, -2, -3, -4, -5};
    for (int i = 0; i < 8; i++) assert(r8[i] == min_exp[i]);

    store8(_mm_max_epi16(a, b), r8);
    short max_exp[8] = {2, 1, 0, 1, 2, 3, 4, 5};
    for (int i = 0; i < 8; i++) assert(r8[i] == max_exp[i]);

    /* madd: pairwise 16x16->32 multiply-add. */
    int r4[4];
    store4(_mm_madd_epi16(a, b), r4);
    int madd_exp[4] = {-5, -1, -13, -41}; /* matches gcc's reference output */
    for (int i = 0; i < 4; i++) assert(r4[i] == madd_exp[i]);

    /* mulhi: high 16 bits of a 16x16 signed/unsigned multiply. */
    store8(_mm_mulhi_epi16(a, b), r8);
    short mulhi_exp[8] = {-1, -1, 0, -1, -1, -1, -1, -1};
    for (int i = 0; i < 8; i++) assert(r8[i] == mulhi_exp[i]);

    /* min/max epu8, avg_epu8, adds/subs saturation, sad_epu8. */
    unsigned char rb[16];
    store16b(_mm_min_epu8(_mm_set1_epi8((char)3), _mm_set1_epi8((char)200)), rb);
    assert(rb[0] == 3);
    store16b(_mm_max_epu8(_mm_set1_epi8((char)3), _mm_set1_epi8((char)200)), rb);
    assert(rb[0] == 200);
    store16b(_mm_avg_epu8(_mm_set1_epi8((char)3), _mm_set1_epi8((char)6)), rb);
    assert(rb[0] == 5); /* (3+6+1)/2 = 5 */

    signed char rsb[16];
    store16sb(_mm_adds_epi8(_mm_set1_epi8((char)120), _mm_set1_epi8((char)20)), rsb);
    assert(rsb[0] == 127); /* saturates */
    store16b(_mm_subs_epu8(_mm_set1_epi8((char)3), _mm_set1_epi8((char)10)), rb);
    assert(rb[0] == 0); /* unsigned saturation at 0 */

    long long rs[2];
    __m128i sad = _mm_sad_epu8(_mm_set1_epi8((char)10), _mm_set1_epi8((char)3));
    memcpy(rs, &sad, 16);
    assert(rs[0] == 56 && rs[1] == 56); /* 8 lanes * |10-3| */

    /* Whole-register byte shifts (distinct from per-lane epi16/epi32/epi64
     * shifts): srli/slli by BYTES, zero-filling the vacated end. */
    __m128i seq = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    store16b(_mm_srli_si128(seq, 3), rb);
    unsigned char srli_exp[16] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0};
    for (int i = 0; i < 16; i++) assert(rb[i] == srli_exp[i]);
    store16b(_mm_slli_si128(seq, 5), rb);
    unsigned char slli_exp[16] = {0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (int i = 0; i < 16; i++) assert(rb[i] == slli_exp[i]);
    /* Out-of-range shift counts yield an all-zero result. */
    store16b(_mm_srli_si128(seq, 16), rb);
    for (int i = 0; i < 16; i++) assert(rb[i] == 0);
#endif
    return 0;
}
