/* rcc ships its own minimal include/xmmintrin.h (real quad-precision-free,
 * MMX-free by design) that only implemented the *packed* SSE compares
 * (_mm_cmpeq_ps etc.) -- the *scalar* family (_mm_cmpeq_ss, _mm_cmplt_ss,
 * ..., operating on lane 0 only and passing lanes 1-3 through from the
 * first operand unchanged) was entirely missing, so any third-party code
 * using them failed to link with "undefined reference".
 *
 * Found via a real SDL3 build: src/audio/SDL_audiotypecvt.c uses
 * _mm_cmpge_ss directly.
 *
 * The "not" predicates (cmpnlt/cmpnle/cmpngt/cmpnge) and cmpord/cmpunord
 * are the trickiest to get right: the mask they produce must be a
 * genuine all-bits-set/all-bits-clear pattern (0xFFFFFFFF, not the
 * floating-point value -1.0f, whose bit pattern is 0xBF800000) since
 * callers routinely feed the result straight into _mm_and_ps/
 * _mm_andnot_ps as a bitwise selector -- and they must return "true"
 * (all-bits-set) whenever either operand is NaN, exactly like the real
 * CMPNLTSS/CMPNLESS/CMPNGTSS/CMPNGESS hardware predicates, not like a
 * naive C "!(a < b)" evaluated then converted to a float value.
 */
#include <stdio.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <string.h>
#include <xmmintrin.h>

static unsigned int bits32(float f) {
    unsigned int u;
    memcpy(&u, &f, 4);
    return u;
}

static int check_lane0(const char *name, __m128 r, unsigned int want, float a1, float a2, float a3) {
    unsigned int got = bits32(r[0]);
    if (got != want) {
        printf("FAIL: %s lane0 = %08x, want %08x\n", name, got, want);
        return 0;
    }
    /* Lanes 1-3 must pass through from `a` completely unmodified. */
    if (r[1] != a1 || r[2] != a2 || r[3] != a3) {
        printf("FAIL: %s clobbered upper lanes: got [%g,%g,%g], want [%g,%g,%g]\n",
               name, (double)r[1], (double)r[2], (double)r[3], (double)a1, (double)a2, (double)a3);
        return 0;
    }
    return 1;
}

int main(void) {
    const unsigned int T = 0xffffffffu, F = 0u;
    float nan = 0.0f / 0.0f;
    int ok = 1;

    /* {value pair, then the 12 expected lane-0 masks in declaration order}. */
    struct {
        float a0, b0;
        unsigned int eq, lt, le, gt, ge, neq, nlt, nle, ngt, nge, ord, unord;
    } cases[] = {
        /* a < b */
        {1.0f, 2.0f, F, T, T, F, F, T, F, F, T, T, T, F},
        /* a > b */
        {2.0f, 1.0f, F, F, F, T, T, T, T, T, F, F, T, F},
        /* a == b */
        {1.0f, 1.0f, T, F, T, F, T, F, T, F, T, F, T, F},
        /* NaN on either side: every ordered predicate is false, every
         * "not"/unordered predicate is true (matches real hardware). */
        {nan, 1.0f, F, F, F, F, F, T, T, T, T, T, F, T},
        {1.0f, nan, F, F, F, F, F, T, T, T, T, T, F, T},
        {nan, nan, F, F, F, F, F, T, T, T, T, T, F, T},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        __m128 a = {cases[i].a0, 99.0f, 98.0f, 97.0f};
        __m128 b = {cases[i].b0, 0.0f, 0.0f, 0.0f};
        ok &= check_lane0("cmpeq_ss", _mm_cmpeq_ss(a, b), cases[i].eq, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmplt_ss", _mm_cmplt_ss(a, b), cases[i].lt, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmple_ss", _mm_cmple_ss(a, b), cases[i].le, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpgt_ss", _mm_cmpgt_ss(a, b), cases[i].gt, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpge_ss", _mm_cmpge_ss(a, b), cases[i].ge, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpneq_ss", _mm_cmpneq_ss(a, b), cases[i].neq, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpnlt_ss", _mm_cmpnlt_ss(a, b), cases[i].nlt, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpnle_ss", _mm_cmpnle_ss(a, b), cases[i].nle, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpngt_ss", _mm_cmpngt_ss(a, b), cases[i].ngt, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpnge_ss", _mm_cmpnge_ss(a, b), cases[i].nge, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpord_ss", _mm_cmpord_ss(a, b), cases[i].ord, 99.0f, 98.0f, 97.0f);
        ok &= check_lane0("cmpunord_ss", _mm_cmpunord_ss(a, b), cases[i].unord, 99.0f, 98.0f, 97.0f);
    }

    if (!ok)
        return 1;
    printf("OK all 12 _mm_cmpXX_ss predicates match hardware semantics, including NaN\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
