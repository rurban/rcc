// AVX/AVX2 intrinsic regression test: direct __builtin_ia32_*256 calls and
// the gcc-header lowering (addps256 etc.), verified byte-identical to gcc
// -mavx2 at -O0..-O3. The header wrappers are thin casts over these names,
// so testing the names directly covers the real <immintrin.h> chain.
// x86-64 only (VEX encodings); guarded for arm64/mingw.
#if !defined(__aarch64__) && !defined(_M_ARM64)
#include <stdio.h>

typedef float v8sf __attribute__((vector_size(32), aligned(32)));
typedef double v4df __attribute__((vector_size(32), aligned(32)));
typedef int v8si __attribute__((vector_size(32), aligned(32)));
typedef long long v4di __attribute__((vector_size(32), aligned(32)));
typedef short v16hi __attribute__((vector_size(32), aligned(32)));
typedef signed char v32qi __attribute__((vector_size(32), aligned(32)));
typedef int v4si __attribute__((vector_size(16), aligned(16)));
typedef float v4sf __attribute__((vector_size(16), aligned(16)));

static int fails = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fails++;                                                          \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                     \
    } while (0)

int main(void) {
    v8sf a = {1, 2, 3, 4, 5, 6, 7, 8};
    v8sf b = {2, 3, 4, 5, 6, 7, 8, 9};
    v8sf c = (v8sf)__builtin_ia32_addps256(a, b);
    CHECK(c[0] == 3 && c[1] == 5 && c[7] == 17);
    c = (v8sf)__builtin_ia32_subps256(b, a);
    CHECK(c[0] == 1 && c[7] == 1);
    c = (v8sf)__builtin_ia32_mulps256(a, b);
    CHECK(c[0] == 2 && c[7] == 72);
    c = (v8sf)__builtin_ia32_divps256(b, a);
    CHECK(c[0] == 2 && c[7] == 1.125);
    c = (v8sf)__builtin_ia32_sqrtps256(a);
    CHECK(c[0] == 1 && c[7] > 2.82 && c[7] < 2.83);
    c = (v8sf)__builtin_ia32_maxps256(a, b);
    CHECK(c[0] == 2 && c[7] == 9);
    // cmpps: all-equal -> all-ones bit pattern (=-nan as float)
    c = (v8sf)__builtin_ia32_cmpps256(a, a, 0);
    CHECK(((v8si)c)[0] == -1 && ((v8si)c)[7] == -1);
    // rcpp: 12-bit approximate reciprocal
    c = (v8sf)__builtin_ia32_rcpps256(b);
    CHECK(c[0] > 0.49f && c[0] < 0.51f && c[7] > 0.10f && c[7] < 0.12f);

    v4df ad = {1.5, 2.5, 3.5, 4.5}, bd = {10, 20, 30, 40};
    v4df cd = (v4df)__builtin_ia32_addpd256(ad, bd);
    CHECK(cd[0] == 11.5 && cd[3] == 44.5);

    v8si x = {1, 2, 3, 4, 5, 6, 7, 8};
    v8si y = {8, 7, 6, 5, 4, 3, 2, 1};
    v8si z = (v8si)__builtin_ia32_paddd256(x, y);
    CHECK(z[0] == 9 && z[7] == 9);
    z = (v8si)__builtin_ia32_psubd256(x, y);
    CHECK(z[0] == -7 && z[7] == 7);
    z = (v8si)__builtin_ia32_pcmpeqd256(x, x);
    CHECK(z[0] == -1 && z[7] == -1);
    z = (v8si)__builtin_ia32_pcmpgtd256(y, x);
    CHECK(z[0] == -1 && z[7] == 0);
    z = (v8si)__builtin_ia32_pmaxsw256(x, y);
    CHECK(z[0] == 8 && z[7] == 8);
    z = (v8si)__builtin_ia32_pmullw256(x, y);
    CHECK(z[0] == 8 && z[7] == 8);
    // vpshufd shuffles within each 128-bit lane: 0x1b reverses each lane
    z = (v8si)__builtin_ia32_pshufd256(x, 0x1b);
    CHECK(z[0] == 4 && z[3] == 1 && z[4] == 8 && z[7] == 5);
    z = (v8si)__builtin_ia32_pslldi256(x, 3);
    CHECK(z[0] == 8 && z[7] == 64);
    z = (v8si)__builtin_ia32_psrldi256(x, 2);
    CHECK(z[0] == 0 && z[7] == 2);
    {
        v4di q = (v4di)__builtin_ia32_psrlqi256((v4di)x, 2);
        CHECK(q[0] == (0x200000001LL >> 2) && q[3] == (0x800000007LL >> 2));
    }
    {
        // runtime count: the headers' _mm256_slli_epi32 passes a plain int
        // to the "i" builtin; must fall back to the variable-count form
        v4si cnt = {1, 0, 0, 0};
        z = (v8si)__builtin_ia32_pslld256(x, cnt);
        CHECK(z[0] == 2 && z[7] == 16);
    }
    // vpermq: full-256 permute of qwords (not per-lane)
    z = (v8si)__builtin_ia32_permdi256((v4di)x, 0x4e);
    CHECK(z[0] == 5 && z[1] == 6 && z[2] == 7 && z[3] == 8 && z[4] == 1 && z[5] == 2);
    // palignr's builtin imm is in BITS (the header passes __N*8)
    z = (v8si)__builtin_ia32_palignr256(x, y, 64);
    CHECK(z[0] == 6 && z[1] == 5 && z[7] == 6); // gcc-verified
    z = (v8si)__builtin_ia32_pmaddwd256(x, y);
    // 16-bit words: x={1,0,2,0,..}, y={8,0,7,0,..} as shorts
    CHECK(z[0] == 8 && z[1] == 14 && z[3] == 20);
    z = (v8si)__builtin_ia32_packssdw256(x, y);
    CHECK(z[0] == 0x00020001 && z[7] == 0x00010002);
    z = (v8si)__builtin_ia32_pabsd256(x);
    CHECK(z[0] == 1 && z[7] == 8);
    z = (v8si)__builtin_ia32_paddb256((v32qi)x, (v32qi)x);
    CHECK(z[0] == 2 && z[7] == 16);
    {
        // x as 16-bit words is {1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0}
        v16hi hw = (v16hi)__builtin_ia32_paddw256((v16hi)x, (v16hi)x);
        CHECK(hw[0] == 2 && hw[1] == 0 && hw[15] == 0);
    }
    {
        // x as qwords is {0x200000001, 0x400000003, 0x600000005, 0x800000007}
        v4di qq = (v4di)__builtin_ia32_paddq256((v4di)x, (v4di)x);
        CHECK(qq[0] == 0x400000002 && qq[3] == 0x100000000ELL);
    }
    // integer vector multiply: vpmulld (mullo_epi32 lowering)
    {
        v8si m = (v8si)__builtin_ia32_pmulld256(x, y);
        CHECK(m[0] == 8 && m[7] == 8);
    }
    // converts
    {
        v8sf f = (v8sf)__builtin_ia32_cvtdq2ps256(x);
        CHECK(f[0] == 1 && f[7] == 8);
    }
    z = (v8si)__builtin_ia32_cvtps2dq256(a);
    CHECK(z[0] == 1 && z[7] == 8);
    z = (v8si)__builtin_ia32_cvttps2dq256(b);
    CHECK(z[0] == 2 && z[7] == 9);
    {
        v4sf half = {1, 2, 3, 4};
        v4df d = (v4df)__builtin_ia32_cvtps2pd256(half);
        CHECK(d[0] == 1 && d[3] == 4);
    }
    // move masks
    {
        v4df m = {-1.0, 0.5, 2.0, -3.0};
        CHECK(__builtin_ia32_movmskpd256(m) == 0b1001); // sign bits of {-1,0.5,2,-3}
    }
    {
        v8sf f = {-1, 0.5, 2, -3, 4, -5, 6, 7};
        CHECK(__builtin_ia32_movmskps256(f) == 0b00101001); // sign bits
    }
    CHECK(__builtin_ia32_pmovmskb256(x) == 0);
    // permutevar: vpermd ymm = table[indices]
    z = (v8si)__builtin_ia32_permvarsi256(x, y);
    CHECK(z[0] == 1 && z[1] == 8 && z[7] == 2);
    // vpermilps imm + var
    {
        v8sf p = (v8sf)__builtin_ia32_vpermilps256(a, 0x1b);
        CHECK(p[0] == 4 && p[3] == 1 && p[4] == 8 && p[7] == 5);
        v8sf q = (v8sf)__builtin_ia32_vpermilvarps256(a, x);
        CHECK(q[0] == 2 && q[7] == 5);
        if (fails) printf("QACTUAL %g %g %g %g %g %g %g %g\n", q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7]);
    }
    // blend imm + var
    {
        v8sf p = (v8sf)__builtin_ia32_blendps256(a, b, 0x55);
        CHECK(p[0] == 2 && p[1] == 2 && p[7] == 8); // 0x55 picks b on even lanes
        v8sf q = (v8sf)__builtin_ia32_blendvps256(a, b, a);
        CHECK(q[0] == 1 && q[7] == 8); // mask=a: all sign bits 0 -> pick a
    }
    // round
    {
        v8sf r = (v8sf)__builtin_ia32_roundps256(b, 0);
        CHECK(r[0] == 2 && r[7] == 9);
    }
    // dpps
    {
        v8sf p = (v8sf)__builtin_ia32_dpps256(a, b, 0xff);
        CHECK(p[0] == 40 && p[4] == 200); // 4-lane dots, broadcast per lane
    }
    // 128-bit extract/insert
    {
        v4si e = (v4si)__builtin_ia32_vextractf128_si256((v4di)x, 1);
        CHECK(e[0] == 5 && e[3] == 8);
        v8si ins = (v8si)__builtin_ia32_vinsertf128_si256((v4di)x, (v4si){9, 9, 9, 9}, 1);
        CHECK(ins[0] == 1 && ins[4] == 9 && ins[7] == 9);
    }
    printf(fails ? "%d FAILURES\n" : "ALL PASS\n", fails);
    return fails != 0;
}
#else
int main(void) { return 0; }
#endif
