/* Combined SSE2 <emmintrin.h> intrinsic coverage: baseline ops, unpack/pack,
 * and variable-count shifts -- all found missing from rcc's bundled header
 * across several third-party sessions.
 *
 * min/max/saturating-arithmetic/madd/mulhi/sad/whole-register-byte-shifts:
 * _mm_min_epi16/_mm_max_epi16/_mm_min_epu8/_mm_max_epu8, the saturating
 * _mm_adds_/_mm_subs_ family (epi8/epi16/epu8/epu16), _mm_avg_epu8/
 * _mm_avg_epu16, _mm_mulhi_epi16/_mm_mulhi_epu16, _mm_madd_epi16,
 * _mm_sad_epu8, and the whole-register byte shifts _mm_srli_si128/
 * _mm_slli_si128 (distinct from the per-lane _mm_s{r,l}li_epi{16,32,64}
 * that did exist). All ordinary SSE2 (no SSSE3+), yet entirely absent --
 * any code calling them saw an implicit-int undeclared-function call.
 * Found two ways: (1) GCC 15's own <avx2intrin.h> reduce-operator macros
 * call _mm_max_epi16/_mm_min_epi16 by name once __builtin_shufflevector
 * itself was fixed, cascading into "expected an expression" parse errors
 * at the call site; (2) libwebp's sharpyuv_sse2.c calls _mm_max_epi16/
 * _mm_min_epi16/_mm_madd_epi16 directly, and dec_sse2.c calls
 * _mm_srli_si128/_mm_slli_si128 directly. All results cross-checked
 * against real gcc's <emmintrin.h> output.
 *
 * shuffle/unpack/pack/maskmove: _mm_shufflelo_epi16/_mm_shufflehi_epi16
 * (compile-time-constant 16-bit half-lane permutes), the full
 * _mm_unpacklo/unpackhi_epi{8,16,32,64} interleave family, saturating
 * _mm_packs_epi16/_mm_packus_epi16/_mm_packs_epi32, and
 * _mm_maskmoveu_si128. All plain lane-wise arithmetic, no new machine
 * instructions needed. Found via blosc2's blosc/shuffle-sse2.c (bit/byte
 * shuffle network implementation), which failed with "lvalue required as
 * left operand of assignment" on `xmm0[k] = _mm_shufflelo_epi16(...)` /
 * `xmm0[k] = _mm_unpacklo_epi8(...)` -- these functions didn't exist, so
 * the calls were implicit-int declarations, and assigning their (bogus
 * int) result to a __m128i array element wasn't a valid lvalue-typed
 * assignment.
 *
 * variable-count shifts: _mm_sll_epi16/32/64, _mm_srl_epi16/32/64,
 * _mm_sra_epi16/32 -- the count-in-a-__m128i-register siblings of the
 * immediate _mm_s{r,l}li_epi{16,32,64}/_mm_srai_epi{16,32} that did
 * exist. Entirely absent from rcc's bundled header. Found via FLAC's
 * src/libFLAC/lpc_intrin_sse2.c, `summ = _mm_sra_epi32(summ, cnt);`
 * failing with "lvalue required as left operand of assignment" (same
 * implicit-int-call root cause as above). Semantics cross-checked
 * against real gcc: the count is read from the low 64 bits of the
 * count register; a count exceeding the element width zeroes the
 * logical shifts, while the arithmetic shift instead saturates to
 * width-1 (every lane filled with its own sign bit). */
#include <emmintrin.h>
#include <assert.h>
#include <string.h>

#if !defined(__aarch64__) && !defined(_M_ARM64)

static void store8(__m128i v, short *out) { memcpy(out, &v, 16); }
static void store16b(__m128i v, unsigned char *out) { memcpy(out, &v, 16); }
static void store16sb(__m128i v, signed char *out) { memcpy(out, &v, 16); }
static void store4(__m128i v, int *out) { memcpy(out, &v, 16); }

static int eq8(__m128i v, const short *e) {
    short buf[8];
    _mm_storeu_si128((__m128i *)buf, v);
    return memcmp(buf, e, sizeof(buf)) == 0;
}

#endif

int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    /* --- min/max/madd/mulhi/sad/saturating arithmetic --- */
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

    /* --- shuffle/unpack/pack/maskmove --- */
    short in[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    __m128i v = _mm_loadu_si128((const __m128i *)in);

    short lo_exp[8] = {3, 2, 1, 0, 4, 5, 6, 7};
    assert(eq8(_mm_shufflelo_epi16(v, 0x1B), lo_exp));
    short hi_exp[8] = {0, 1, 2, 3, 7, 6, 5, 4};
    assert(eq8(_mm_shufflehi_epi16(v, 0x1B), hi_exp));

    /* unpacklo/hi_epi8 */
    __m128i a8 = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    __m128i b8 = _mm_set_epi8(115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103,
                              102, 101, 100);
    unsigned char lo8[16], hi8[16];
    _mm_storeu_si128((__m128i *)lo8, _mm_unpacklo_epi8(a8, b8));
    _mm_storeu_si128((__m128i *)hi8, _mm_unpackhi_epi8(a8, b8));
    for (int i = 0; i < 8; i++) {
        assert(lo8[i * 2] == i && lo8[i * 2 + 1] == 100 + i);
        assert(hi8[i * 2] == i + 8 && hi8[i * 2 + 1] == 108 + i);
    }

    /* unpacklo/hi_epi32, unpacklo/hi_epi64 */
    __m128i a32 = _mm_set_epi32(3, 2, 1, 0);
    __m128i b32 = _mm_set_epi32(103, 102, 101, 100);
    int buf32[4];
    _mm_storeu_si128((__m128i *)buf32, _mm_unpacklo_epi32(a32, b32));
    assert(buf32[0] == 0 && buf32[1] == 100 && buf32[2] == 1 && buf32[3] == 101);
    _mm_storeu_si128((__m128i *)buf32, _mm_unpackhi_epi32(a32, b32));
    assert(buf32[0] == 2 && buf32[1] == 102 && buf32[2] == 3 && buf32[3] == 103);

    long long buf64[2];
    __m128i a64 = _mm_set_epi64x(1, 0), b64 = _mm_set_epi64x(101, 100);
    _mm_storeu_si128((__m128i *)buf64, _mm_unpacklo_epi64(a64, b64));
    assert(buf64[0] == 0 && buf64[1] == 100);
    _mm_storeu_si128((__m128i *)buf64, _mm_unpackhi_epi64(a64, b64));
    assert(buf64[0] == 1 && buf64[1] == 101);

    /* Saturating packs */
    short w[8] = {200, -200, 30000, -30000, 0, 1, -1, 127};
    __m128i wv = _mm_loadu_si128((const __m128i *)w);
    signed char packed[16];
    _mm_storeu_si128((__m128i *)packed, _mm_packs_epi16(wv, wv));
    signed char exp_packed[8] = {127, -128, 127, -128, 0, 1, -1, 127};
    for (int i = 0; i < 8; i++) {
        assert(packed[i] == exp_packed[i]);
        assert(packed[i + 8] == exp_packed[i]);
    }

    /* maskmoveu: only lanes with mask high bit set are stored */
    unsigned char dst[16];
    memset(dst, 0xAA, sizeof(dst));
    __m128i data = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
    __m128i mask = _mm_set_epi8(-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0);
    _mm_maskmoveu_si128(data, mask, (char *)dst);
    for (int i = 0; i < 16; i++) {
        if (i % 2 == 0) assert(dst[i] == 0xAA); /* untouched */
        else
            assert(dst[i] == i); /* stored */
    }

    /* --- variable-count shifts (count in the low 64 bits of a __m128i) --- */
    __m128i s32 = _mm_set_epi32(-8, -4, 4, 8);
    int rs32[4];
    store4(_mm_sra_epi32(s32, _mm_cvtsi32_si128(1)), rs32);
    assert(rs32[0] == 4 && rs32[1] == 2 && rs32[2] == -2 && rs32[3] == -4);
    /* count > 31 saturates every lane to its own sign bit. */
    store4(_mm_sra_epi32(s32, _mm_cvtsi32_si128(40)), rs32);
    assert(rs32[0] == 0 && rs32[1] == 0 && rs32[2] == -1 && rs32[3] == -1);

    __m128i s16 = _mm_set_epi16(-16, -8, -4, -2, 2, 4, 8, 16);
    store8(_mm_sra_epi16(s16, _mm_cvtsi32_si128(1)), r8);
    short sra16_exp[8] = {8, 4, 2, 1, -1, -2, -4, -8};
    for (int i = 0; i < 8; i++) assert(r8[i] == sra16_exp[i]);

    __m128i u32 = _mm_set_epi32(0x80000000u, 4, 2, 1);
    _mm_storeu_si128((__m128i *)buf32, _mm_sll_epi32(u32, _mm_cvtsi32_si128(1)));
    assert(buf32[0] == 2 && buf32[1] == 4 && buf32[2] == 8 && buf32[3] == 0);
    _mm_storeu_si128((__m128i *)buf32, _mm_srl_epi32(u32, _mm_cvtsi32_si128(1)));
    assert(buf32[0] == 0 && buf32[1] == 1 && buf32[2] == 2 && buf32[3] == 0x40000000);
    /* count > width zeroes every lane for the logical shifts. */
    _mm_storeu_si128((__m128i *)buf32, _mm_sll_epi32(u32, _mm_cvtsi32_si128(32)));
    assert(buf32[0] == 0 && buf32[1] == 0 && buf32[2] == 0 && buf32[3] == 0);

    __m128i u16 = _mm_set_epi16(1, 2, 4, 8, 16, 32, 64, 0x4000);
    store8(_mm_sll_epi16(u16, _mm_cvtsi32_si128(1)), r8);
    short sll16_exp[8] = {(short)0x8000, 128, 64, 32, 16, 8, 4, 2};
    for (int i = 0; i < 8; i++) assert(r8[i] == sll16_exp[i]);
    store8(_mm_srl_epi16(u16, _mm_cvtsi32_si128(1)), r8);
    short srl16_exp[8] = {0x2000, 32, 16, 8, 4, 2, 1, 0};
    for (int i = 0; i < 8; i++) assert(r8[i] == srl16_exp[i]);

    __m128i u64 = _mm_set_epi64x((long long)1 << 63, 1);
    __m128i cnt64 = _mm_cvtsi32_si128(1);
    _mm_storeu_si128((__m128i *)buf64, _mm_sll_epi64(u64, cnt64));
    assert(buf64[0] == 2 && buf64[1] == 0);
    _mm_storeu_si128((__m128i *)buf64, _mm_srl_epi64(u64, cnt64));
    assert(buf64[0] == 0 && buf64[1] == ((unsigned long long)1 << 62));

    /* --- unaligned 32-/64-bit scalar load/store (zero-extend) --- */
    unsigned char raw[17] = {0, 0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44,
                             0, 0,    0,    0,    0,    0,    0,    0};
    int lo32[4];
    _mm_storeu_si128((__m128i *)lo32, _mm_loadu_si32(raw + 1));
    assert(lo32[0] == 0xDDCCBBAA && lo32[1] == 0 && lo32[2] == 0 && lo32[3] == 0);
    unsigned char out32[9] = {0};
    _mm_storeu_si32(out32 + 1, _mm_loadu_si32(raw + 1));
    assert(memcmp(out32 + 1, raw + 1, 4) == 0 && out32[0] == 0);
    unsigned long long lo64[2];
    _mm_storeu_si128((__m128i *)lo64, _mm_loadu_si64(raw + 1));
    assert(lo64[0] == 0x44332211DDCCBBAAULL && lo64[1] == 0);
    unsigned char out64b[10] = {0};
    _mm_storeu_si64(out64b + 1, _mm_loadu_si64(raw + 1));
    assert(memcmp(out64b + 1, raw + 1, 8) == 0 && out64b[0] == 0 && out64b[9] == 0);
#endif
    return 0;
}
