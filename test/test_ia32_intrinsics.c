/* __builtin_ia32_* SIMD intrinsics: the real GCC headers implement every
 * intrinsic as a thin inline wrapper over one of these calls, and until
 * this session rcc could neither type them (they fell through
 * declare_builtin_on_demand to implicit int, so `(__m128)call` became a
 * scalar-int->vector cast) nor codegen them (only sqrtps/sqrtss/rsqrtps
 * were implemented). Now the return type is derived from the name suffix
 * (see type.c's ia32_builtin_ret) and gen_ia32_builtin emits the packed
 * SSE instruction directly. Also covers the scalar->vector cast splat
 * (`(__v8qi)0LL` used to ICE with "Invalid register -1") and the GNU
 * `extern __inline __gnu_inline__` local-copy path (how the real glibc
 * headers' wrappers link at -O0 without an inliner).
 *
 * Found via the "include the real glibc <immintrin.h>" investigation:
 * the real headers failed to parse at all, then failed to link, then
 * produced wrong code (haddps prefix, addsubps matched "add" first,
 * unary 0F38 ops loaded a second argument, cvtpi2pd arity, vec_init
 * element width). Every case here is verified byte-identical against
 * gcc -O0/-O2 with the real headers. */
#if !defined(__aarch64__) && !defined(_M_ARM64)
#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <mmintrin.h>
#include <string.h>

static int eqf4(__m128 v, const float *e) {
    float b[4];
    _mm_storeu_ps(b, v);
    return memcmp(b, e, sizeof(b)) == 0;
}
static int eqd2(__m128d v, const double *e) {
    double b[2];
    _mm_storeu_pd(b, v);
    return memcmp(b, e, sizeof(b)) == 0;
}
static int eqi4(__m128i v, const int *e) {
    int b[4];
    _mm_storeu_si128((__m128i *)b, v);
    return memcmp(b, e, sizeof(b)) == 0;
}

/* GNU extern-inline wrapper exactly like the real headers' (rcc has no
 * inliner guarantee, so this must compile as a per-TU local copy). */
extern __inline__ __attribute__((__gnu_inline__, __always_inline__))
__m128
my_wrap(__m128 a, __m128 b) { return (__m128)__builtin_ia32_addps(a, b); }

int main(void) {
    __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    __m128 b = _mm_set_ps(40.0f, 30.0f, 20.0f, 10.0f);
    __m128 c = __builtin_ia32_addps(a, b); /* {11, 22, 33, 44} */

    /* packed + scalar (lane-0) float ALU */
    float add_e[4] = {11, 22, 33, 44};
    if (!eqf4(c, add_e)) return 1;
    float mul_e[4] = {10, 40, 90, 160};
    if (!eqf4(__builtin_ia32_mulps(a, b), mul_e)) return 2;
    float ss_e[4] = {11, 2, 3, 4}; /* addss: lane 0 only */
    if (!eqf4(__builtin_ia32_addss(a, b), ss_e)) return 3;
    float sqrt_e[4] = {3.3166248f, 4.6904158f, 5.7445626f, 6.6332498f};
    if (!eqf4(__builtin_ia32_sqrtps(c), sqrt_e)) return 4;
    float ms_e[4] = {10, 2, 3, 4}; /* maxss: lane 0 = max(1, 10) */
    if (!eqf4(__builtin_ia32_maxss(a, b), ms_e)) return 5;

    /* packed double */
    __m128d g = _mm_set_pd(1.5, 2.5);
    __m128d h = _mm_set_pd(3.5, 4.5);
    double pd_e[2] = {7.0, 5.0}; /* set_pd stores {hi,lo}: {1.5+4.5, 2.5+3.5} */
    if (!eqd2(__builtin_ia32_addpd(g, h), pd_e)) return 6;

    /* SSE3: haddps needs the F2 prefix, addsubps must not match "add" */
    __m128 as = __builtin_ia32_addsubps(c, b); /* {11-10, 22+20, 33-30, 44+40} */
    float as_e[4] = {1, 42, 3, 84};
    if (!eqf4(as, as_e)) return 7;
    float hs_e[4] = {43, 87, 43, 87}; /* haddps(as, as) = {as0+as1, as2+as3, ...} */
    if (!eqf4(__builtin_ia32_haddps(as, as), hs_e)) return 8;

    /* integer ALU (128-bit forms) */
    __m128i i1 = _mm_set_epi32(4, 3, 2, 1);
    __m128i i2 = _mm_set1_epi32(10);
    int pd_i[4] = {11, 12, 13, 14};
    if (!eqi4(__builtin_ia32_paddd128(i1, i2), pd_i)) return 9;
    int pandn_e[4] = {10, 8, 8, 10}; /* ~i1 & i2 */
    if (!eqi4(__builtin_ia32_pandn128(i1, i2), pandn_e)) return 10;

    /* compares: predicates map to cmpps immediates, gt swaps operands */
    unsigned all0[4] = {0, 0, 0, 0};
    float cmp[4];
    _mm_storeu_ps(cmp, __builtin_ia32_cmpeqps(a, b)); /* all false */
    if (memcmp(cmp, all0, sizeof(all0)) != 0) return 11;
    _mm_storeu_ps(cmp, __builtin_ia32_cmpgeps(a, b)); /* a >= b: all false */
    if (memcmp(cmp, all0, sizeof(all0)) != 0) return 12;
    _mm_storeu_ps(cmp, __builtin_ia32_cmpgtps(b, a)); /* b > a: all true */
    for (int i = 0; i < 4; i++) {
        unsigned bits;
        memcpy(&bits, &cmp[i], sizeof(bits));
        if (bits != 0xFFFFFFFFu) return 13;
    }

    /* shuffles */
    float sh_e[4] = {3, 4, 20, 10};
    if (!eqf4(__builtin_ia32_shufps(a, b, 0x1e), sh_e)) return 14;
    int ps_e[4] = {4, 3, 2, 1};
    if (!eqi4(__builtin_ia32_pshufd(i1, 0x1b), ps_e)) return 15;

    /* conversions */
    int cvt_e[4] = {11, 22, 33, 44};
    if (!eqi4(__builtin_ia32_cvtps2dq(c), cvt_e)) return 16;
    if (!eqf4(__builtin_ia32_cvtdq2ps(__builtin_ia32_cvtps2dq(c)), add_e)) return 17;
    if (__builtin_ia32_cvtss2si(c) != 11) return 18;

    /* move masks */
    if (__builtin_ia32_movmskps(a) != 0) return 19;

    /* SSSE3: pshufb128 byte permute + unary pabsd128 */
    __m128i shuf_mask = _mm_set_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12);
    int shufb_e[4] = {4, 3, 2, 1}; /* verified identical to gcc -mssse3 */
    if (!eqi4(__builtin_ia32_pshufb128(i1, shuf_mask), shufb_e)) return 20;
    __m128i neg = _mm_set_epi32(-4, -3, -2, -1); /* lanes {-1,-2,-3,-4} */
    int abs_e[4] = {1, 2, 3, 4};
    if (!eqi4(__builtin_ia32_pabsd128(neg), abs_e)) return 21;

    /* SSE4.1: blendps (imm), roundps (unary imm), ptestz128 */
    float bl_e[4] = {10, 2, 3, 40}; /* imm 0x9: lanes 0,3 from b */
    if (!eqf4(__builtin_ia32_blendps(a, b, 0x9), bl_e)) return 22;
    if (!eqf4(__builtin_ia32_roundps(c, 0), add_e)) return 23;
    __m128i zz = _mm_setzero_si128();
    if (!__builtin_ia32_ptestz128(zz, zz)) return 24;

    /* MMX (8-byte) via vec_init_v4hi + paddw */
    __m64 m1 = __builtin_ia32_vec_init_v4hi(1, 2, 3, 4);
    __m64 m2 = __builtin_ia32_vec_init_v4hi(5, 6, 7, 8);
    long long mm = (long long)__builtin_ia32_paddw(m1, m2);
    if ((mm & 0xffff) != 6 || ((mm >> 16) & 0xffff) != 8) return 25;
    if ((mm >> 32) != 0x000C000ALL) return 26; /* high lanes {10, 12} */

    /* scalar -> vector cast splat (was: "Invalid register -1" ICE) */
    __v8qi spl_b = (__v8qi)7LL;
    if (spl_b[0] != 7 || spl_b[7] != 7) return 27;
    __v4sf spl_f = (__v4sf)2.5f;
    if (spl_f[0] != 2.5f || spl_f[3] != 2.5f) return 28;
    double dv = 1.25;
    __v2df spl_d = (__v2df)dv;
    if (spl_d[0] != 1.25 || spl_d[1] != 1.25) return 29;

    /* crc32 (mingw's smmintrin.h compiles these wrappers as local copies
     * even when unused, so the builtins must exist) */
    if (__builtin_ia32_crc32qi(0xFFFFFFFFu, 0xAA) != 0x642B3130u) return 30;
    if (__builtin_ia32_crc32si(0xFFFFFFFFu, 0xCCCCCCCCu) != 0x70B16A3Du) return 31;

    /* SSE4.1: vec_set_* (used by _mm_insert_epi{8,16,32,64}). Was
     * misimplemented as vec_init_*, corrupting the unchanged lane. */
    __m128i ins = _mm_set_epi64x(0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    __m128i ins1 = _mm_insert_epi64(ins, 0xaaaaaaaaaaaaaaaaULL, 1);
    if ((unsigned long long)_mm_extract_epi64(ins1, 0) != 0xfedcba9876543210ULL) return 33;
    if ((unsigned long long)_mm_extract_epi64(ins1, 1) != 0xaaaaaaaaaaaaaaaaULL) return 34;
    __m128i ins0 = _mm_insert_epi64(ins, 0xbbbbbbbbbbbbbbbbULL, 0);
    if ((unsigned long long)_mm_extract_epi64(ins0, 0) != 0xbbbbbbbbbbbbbbbbULL) return 35;
    if ((unsigned long long)_mm_extract_epi64(ins0, 1) != 0x123456789abcdef0ULL) return 36;

    __m128i ins32 = _mm_set_epi32(4, 3, 2, 1);
    ins32 = _mm_insert_epi32(ins32, 0xccccccccU, 2);
    int ins32_e[4] = {1, 2, (int)0xccccccccU, 4};
    if (memcmp(&ins32, ins32_e, sizeof(ins32_e)) != 0) return 37;

    __builtin_ia32_emms();
    return 0;
}
#else
int main(void) { return 0; }
#endif
