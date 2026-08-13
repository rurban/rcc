// AVX-512 (EVEX) regression test: the `_mm512_*` wrappers used by
// blake3_avx512.c lower to either 64-byte C vector ops (add/and/xor/
// andnot) or `__builtin_ia32_*512_mask` calls (shifts, rotates,
// permutes, converts, compares). Compiling this exercises the EVEX
// encoders and the 64-byte gen_vector path; the runtime checks run
// only when the CPU actually has AVX-512F (the local dev machines may
// not — the test must PASS everywhere, CI runners included).
// x86-64 only; guarded for arm64/mingw like the AVX2 test.
#if !defined(__aarch64__) && !defined(_M_ARM64) && defined(__RCC__)
#include <stdio.h>

typedef int v16si __attribute__((vector_size(64), aligned(64)));
typedef long long v8di __attribute__((vector_size(64), aligned(64)));
typedef int v8si __attribute__((vector_size(32), aligned(32)));
typedef int v4si __attribute__((vector_size(16), aligned(16)));
typedef double v4df __attribute__((vector_size(32), aligned(32)));

static int fails = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fails++;                                                          \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);             \
        }                                                                     \
    } while (0)

int main(void) {
    // 64-byte C vector ops (gen_vector64_x86) — these must compile even
    // without AVX-512 (the encoders are emitted but never run).
    v16si a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    v16si b = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    v16si c = a + b;
    v16si d = a ^ b;
    v16si e = a & b;
    v16si f = ~a & b;
    v8di g = {1, 2, 3, 4, 5, 6, 7, 8};
    v8di h = {8, 7, 6, 5, 4, 3, 2, 1};
    v8di i = g + h;

    if (__builtin_cpu_supports("avx512f")) {
        CHECK(c[0] == 17 && c[15] == 17);
        CHECK(d[0] == 17 && d[15] == 17);
        CHECK(e[0] == 0 && e[15] == 0);
        CHECK(f[0] == 16 && f[15] == 1);
        CHECK(i[0] == 9 && i[7] == 9);
        // the masked builtins blake3 uses
        v16si r = (v16si)__builtin_ia32_psrldi512_mask((v16si){16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, 4, (v16si)-1);
        CHECK(r[0] == 1 && r[1] == 0);
        v16si ro = (v16si)__builtin_ia32_prord512_mask((v16si){1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}, 4, (v16si)-1);
        CHECK(ro[0] == 0x10000000 && ro[15] == 1);
        v16si u = (v16si)__builtin_ia32_punpckldq512_mask(a, b, (v16si)-1);
        CHECK(u[0] == 1 && u[1] == 16 && u[2] == 2 && u[3] == 15);
        v16si sh = (v16si)__builtin_ia32_shuf_i32x4_mask(a, b, 0x88, (v16si)-1);
        CHECK(sh[0] == 1 && sh[4] == 5);
        v16si an = (v16si)__builtin_ia32_pandnd512_mask(a, b, (v16si)-1);
        CHECK(an[0] == 16 && an[15] == 1);
        v8si lo = (v8si)__builtin_ia32_pmovqd512_mask((v8di){0x100000001, 0x300000002, 0x500000003, 0x700000004, 0x900000005, 0xB00000006, 0xD00000007, 0xF00000008}, (v8si)-1);
        CHECK(lo[0] == 1 && lo[7] == 8);
        // cmp_epu32_mask -> __mmask16
        int m = __builtin_ia32_ucmpd512_mask(a, b, 1); // LT: a[i] < b[i] for i<8
        CHECK(m == 0xff);
        // 256-bit masked store + 512->256 extract
        v16si val = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
        int buf[8] = {0};
        __builtin_ia32_storedqusi256_mask(buf, (v8si)val, -1);
        CHECK(buf[0] == 7 && buf[7] == 7);
        v4si ext = (v4si)__builtin_ia32_extractf64x4_mask((v4df)val, 1, (v4df)-1);
        CHECK(ext[0] == 7);
    }
    printf(fails ? "%d FAILURES\n" : "ALL PASS\n", fails);
    return fails != 0;
}
#elif !defined(__aarch64__) && !defined(_M_ARM64)
// gcc build: compile-only verification (rcc runs the runtime checks).
int main(void) { return 0; }
#else
int main(void) { return 0; }
#endif
