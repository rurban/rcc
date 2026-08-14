// F16C intrinsic regression test: __builtin_ia32_vcvtph2ps/vcvtph2ps256
// (half -> float) and __builtin_ia32_vcvtps2ph/vcvtps2ph256 (float ->
// half) — the pair GCC's <f16cintrin.h> wraps as _mm_cvtph_ps /
// _mm256_cvtph_ps / _mm_cvtps_ph / _mm256_cvtps_ph.
//
// Found via test_brotli (../brotli/c/enc/backward_references.c pulls in
// <immintrin.h>, which unconditionally parses <f16cintrin.h>'s
// __always_inline__ wrapper bodies): none of these four names start
// with "cvt" (the leading "v" of "vcvtph2ps" defeats the
// memcmp(n,"cvt",3) prefix check used to classify every OTHER cvt*
// builtin's return type) and none end in a b/w/d/q lane-size letter,
// so type.c's ia32_builtin_ret() fell through to its ty_int catch-all
// instead of the real vector return type. That misclassification then
// tripped a second, unrelated codegen bug: a vector-typed local
// declaration initialized from an int-returning call whose own
// argument is itself a vector (`__v8hi H = __builtin_ia32_vcvtps2ph(A,
// imm);`) mis-parsed as "expected an expression" instead of surfacing
// the real type mismatch. Fixing the return-type classification routes
// the call down the correct (working) vector-initializer path.
// x86-64 only (VEX encodings, F16C-specific opcodes); guarded for
// arm64/mingw. gcc/clang need -mf16c to expose <immintrin.h>'s F16C
// section; rcc has no -m target flags and always exposes every ISA
// extension's builtins/encoders.
#if !defined(__aarch64__) && !defined(_M_ARM64)
#include <stdio.h>
#include <string.h>

typedef short v8hi __attribute__((vector_size(16), aligned(16)));
typedef float v4sf __attribute__((vector_size(16), aligned(16)));
typedef float v8sf __attribute__((vector_size(32), aligned(32)));

static int fails = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fails++;                                                         \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                     \
    } while (0)

// IEEE 754 binary16 <-> binary32 reference conversion (software), used
// to compute expected values independently of the hardware path under
// test -- so a bug that corrupts BOTH directions identically still gets
// caught, unlike a pure round-trip check.
static unsigned short ref_f32_to_f16(float f) {
    unsigned int x;
    memcpy(&x, &f, 4);
    unsigned int sign = (x >> 16) & 0x8000u;
    int exp = (int)((x >> 23) & 0xff) - 127 + 15;
    unsigned int mant = x & 0x7fffffu;
    if (exp <= 0) return (unsigned short)sign; // flush to zero (subnormal skipped: test uses normal values)
    if (exp >= 31) return (unsigned short)(sign | 0x7c00u); // overflow -> inf
    return (unsigned short)(sign | ((unsigned int)exp << 10) | (mant >> 13));
}
static float ref_f16_to_f32(unsigned short h) {
    unsigned int sign = (unsigned int)(h & 0x8000u) << 16;
    unsigned int exp = (h >> 10) & 0x1fu;
    unsigned int mant = h & 0x3ffu;
    unsigned int x;
    if (exp == 0) x = sign; // zero (subnormal skipped: test uses normal values)
    else x = sign | ((exp - 15u + 127u) << 23) | (mant << 13);
    float f;
    memcpy(&f, &x, 4);
    return f;
}

int main(void) {
    // vcvtps2ph: narrow 4 packed floats to 4 packed halfs (imm=0: round
    // to nearest-even). Values chosen exactly representable in binary16.
    v4sf a = {1.0f, -2.5f, 100.25f, 0.0f};
    v8hi h = (v8hi)__builtin_ia32_vcvtps2ph(a, 0);
    unsigned short *hbits = (unsigned short *)&h;
    CHECK(hbits[0] == ref_f32_to_f16(1.0f));
    CHECK(hbits[1] == ref_f32_to_f16(-2.5f));
    CHECK(hbits[2] == ref_f32_to_f16(100.25f));
    CHECK(hbits[3] == ref_f32_to_f16(0.0f));

    // vcvtph2ps: widen those same 4 halfs back to floats -- must
    // round-trip exactly for values with no lost mantissa precision.
    v4sf back = (v4sf)__builtin_ia32_vcvtph2ps(h);
    float *fback = (float *)&back;
    CHECK(fback[0] == 1.0f);
    CHECK(fback[1] == -2.5f);
    CHECK(fback[2] == 100.25f);
    CHECK(fback[3] == 0.0f);

    // A value that genuinely loses precision in binary16 (24-bit float
    // mantissa narrowed to 10 bits): confirms real narrowing happens,
    // not a silent pass-through/zero-fill.
    v4sf prec = {1.0f / 3.0f, 0, 0, 0};
    v8hi hprec = (v8hi)__builtin_ia32_vcvtps2ph(prec, 0);
    unsigned short *hp = (unsigned short *)&hprec;
    CHECK(*hp == ref_f32_to_f16(1.0f / 3.0f));
    v4sf precback = (v4sf)__builtin_ia32_vcvtph2ps(hprec);
    float *fp = (float *)&precback;
    CHECK(*fp != 1.0f / 3.0f); // precision was actually lost
    CHECK(*fp == ref_f16_to_f32(ref_f32_to_f16(1.0f / 3.0f)));

    // 256-bit forms: vcvtps2ph256 narrows 8 floats to 8 halfs packed
    // into a 128-bit result; vcvtph2ps256 widens a 128-bit v8hi back to
    // 8 floats in a 256-bit result.
    v8sf a8 = {1.0f, -2.5f, 100.25f, 0.0f, 3.5f, -0.5f, 1024.0f, 7.0f};
    v8hi h8 = (v8hi)__builtin_ia32_vcvtps2ph256(a8, 0);
    unsigned short *h8b = (unsigned short *)&h8;
    float expect8[8] = {1.0f, -2.5f, 100.25f, 0.0f, 3.5f, -0.5f, 1024.0f, 7.0f};
    for (int i = 0; i < 8; i++)
        CHECK(h8b[i] == ref_f32_to_f16(expect8[i]));

    v8sf back8 = (v8sf)__builtin_ia32_vcvtph2ps256(h8);
    float *fback8 = (float *)&back8;
    for (int i = 0; i < 8; i++)
        CHECK(fback8[i] == expect8[i]);

    // Non-zero rounding-mode immediate (1 = round toward -inf / floor):
    // must actually thread the immediate through, not silently ignore
    // it. Binary16 mantissa steps near 1.0 are 2^-10 apart; halfway
    // between the mantissa=1 and mantissa=2 steps is 1+1.5*2^-10.
    // Round-to-nearest-even breaks the tie toward the EVEN mantissa
    // (2, rounding up); round-toward-negative-infinity always picks
    // the lower candidate (1, rounding down) -- the two modes must
    // disagree here.
    v4sf rnd = {1.0f + 1.5f / 1024.0f, 0, 0, 0};
    v8hi h_nearest = (v8hi)__builtin_ia32_vcvtps2ph(rnd, 0); // round to nearest-even (default)
    v8hi h_floor = (v8hi)__builtin_ia32_vcvtps2ph(rnd, 1); // round toward -inf
    unsigned short *hn = (unsigned short *)&h_nearest;
    unsigned short *hf = (unsigned short *)&h_floor;
    CHECK(*hn != *hf); // the immediate genuinely changed the result

    if (fails) {
        printf("%d check(s) failed\n", fails);
        return 1;
    }
    printf("OK\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
