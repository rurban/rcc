/* Unit tests for SSE __m128 intrinsics (include/xmmintrin.h), which lower to
 * native packed SSE via rcc's __attribute__((vector_size)) support. */
#include <stdio.h>
#include <xmmintrin.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

static int eqf(float a, float b) { return a == b; }

static int veq(__m128 v, float e0, float e1, float e2, float e3) {
    float o[4];
    _mm_storeu_ps(o, v);
    return eqf(o[0], e0) && eqf(o[1], e1) && eqf(o[2], e2) && eqf(o[3], e3);
}

int main(void) {
    __m128 a = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 b = _mm_setr_ps(10.0f, 20.0f, 30.0f, 40.0f);

    /* set / setr / set1 / setzero */
    CHECK(veq(a, 1, 2, 3, 4));
    CHECK(veq(_mm_set_ps(4, 3, 2, 1), 1, 2, 3, 4));
    CHECK(veq(_mm_set1_ps(7.0f), 7, 7, 7, 7));
    CHECK(veq(_mm_setzero_ps(), 0, 0, 0, 0));

    /* load / store round-trip */
    float buf[4] = {5, 6, 7, 8};
    CHECK(veq(_mm_loadu_ps(buf), 5, 6, 7, 8));

    /* packed arithmetic (native addps/subps/mulps/divps) */
    CHECK(veq(_mm_add_ps(a, b), 11, 22, 33, 44));
    CHECK(veq(_mm_sub_ps(b, a), 9, 18, 27, 36));
    CHECK(veq(_mm_mul_ps(a, b), 10, 40, 90, 160));
    CHECK(veq(_mm_div_ps(b, a), 10, 10, 10, 10));

    /* scalar (lane-0) variants keep the upper lanes of a */
    CHECK(veq(_mm_add_ss(a, b), 11, 2, 3, 4));
    CHECK(veq(_mm_mul_ss(a, b), 10, 2, 3, 4));

    /* min / max */
    __m128 c = _mm_setr_ps(2.5f, 2.5f, 2.5f, 2.5f);
    CHECK(veq(_mm_min_ps(a, c), 1, 2, 2.5f, 2.5f));
    CHECK(veq(_mm_max_ps(a, c), 2.5f, 2.5f, 3, 4));

    /* reciprocal (1/x, exact for powers of two) */
    CHECK(veq(_mm_rcp_ps(_mm_setr_ps(1, 2, 4, 8)), 1.0f, 0.5f, 0.25f, 0.125f));

    /* bitwise (native andps/orps/xorps) on float bit patterns */
    __m128 x = _mm_set1_ps(6.0f), y = _mm_set1_ps(3.0f);
    CHECK(veq(_mm_and_ps(x, y), 3, 3, 3, 3));   /* 0x40C00000 & 0x40400000 = 0x40400000 */
    CHECK(veq(_mm_or_ps(x, y), 6, 6, 6, 6));    /* = 0x40C00000 */

    /* comparisons (native cmpps) + movemask */
    __m128 lt = _mm_cmplt_ps(a, _mm_set1_ps(3.0f)); /* 1<3,2<3 true; 3,4 false */
    CHECK(_mm_movemask_ps(lt) == 0x3);
    __m128 gt = _mm_cmpgt_ps(a, _mm_set1_ps(2.0f)); /* 3,4 > 2 */
    CHECK(_mm_movemask_ps(gt) == 0xC);
    __m128 eq = _mm_cmpeq_ps(a, _mm_setr_ps(1, 0, 3, 0));
    CHECK(_mm_movemask_ps(eq) == 0x5);

    /* andnot: (~a) & b */
    __m128 all1 = _mm_cmpeq_ps(a, a); /* all lanes 0xFFFFFFFF */
    CHECK(_mm_movemask_ps(all1) == 0xF);
    CHECK(_mm_movemask_ps(_mm_andnot_ps(all1, all1)) == 0x0);

    /* shuffle / unpack / move */
    CHECK(veq(_mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 2, 1, 0)), 1, 2, 30, 40));
    CHECK(veq(_mm_unpacklo_ps(a, b), 1, 10, 2, 20));
    CHECK(veq(_mm_unpackhi_ps(a, b), 3, 30, 4, 40));
    CHECK(veq(_mm_movelh_ps(a, b), 1, 2, 10, 20));
    CHECK(veq(_mm_movehl_ps(a, b), 30, 40, 3, 4));

    /* extraction */
    CHECK(_mm_cvtss_f32(a) == 1.0f);

    /* pass/return __m128 by value through a function */
    __m128 sum = _mm_add_ps(_mm_mul_ps(a, b), a);
    CHECK(veq(sum, 11, 42, 93, 164));

    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
