/* Test builtin inline expansion: memset, memcpy, memcmp, strlen, strcmp, strchr */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int failures;

#define assert_eq(a, b, msg) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        printf("FAIL %s: got %lld, expected %lld\n", msg, _a, _b); \
        failures++; \
    } \
} while (0)

#define assert_ok(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        failures++; \
    } \
} while (0)

int main(void) {
    char buf[128], ref[128];

    /* --- memset --- */
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < 128; i++) assert_eq(buf[i], 0, "memset zero");

    memset(buf, 0xAA, 64);
    for (int i = 0; i < 64; i++) assert_eq((unsigned char)buf[i], 0xAA, "memset partial");
    assert_eq(buf[64], 0, "memset past end");

    char *r = memset(buf, 0xBB, 32);
    assert_eq(r, buf, "memset retval");

    /* --- memcpy --- */
    memset(ref, 'X', 128);
    memcpy(buf, ref, 64);
    for (int i = 0; i < 64; i++) assert_eq(buf[i], 'X', "memcpy fwd");
    assert_eq(buf[64], 0, "memcpy past end");

    r = memcpy(buf, ref, 32);
    assert_eq(r, buf, "memcpy retval");

    /* --- memcmp --- */
    memset(buf, 'A', 10);
    memset(ref, 'A', 10);
    assert_eq(memcmp(buf, ref, 10), 0, "memcmp eq");
    buf[5] = 'B';
    assert_ok(memcmp(buf, ref, 10) > 0, "memcmp gt");
    buf[5] = '@';
    assert_ok(memcmp(buf, ref, 10) < 0, "memcmp lt");
    assert_eq(memcmp(buf, ref, 0), 0, "memcmp zero");
    /* memcmp with bytes > 127: the inline expansion must use zero-extend
     * (movzbl), not sign-extend (movsbl), on the differing bytes.  A signed
     * load of 0xFF gives -1, flipping the sign of the difference.  Seen in
     * rcc-compiled perl5: "\0" cmp "\xFF" returned 1 instead of -1. */
    memset(buf, 0, 10);
    memset(ref, 0xFF, 10);
    assert_ok(memcmp(buf, ref, 10) < 0, "memcmp lt hi-byte: 0 < 0xFF");
    assert_ok(memcmp(ref, buf, 10) > 0, "memcmp gt hi-byte: 0xFF > 0");
    buf[0] = 0x80;
    assert_ok(memcmp(buf, ref, 10) < 0, "memcmp lt hi-byte: 0x80 < 0xFF");
    ref[0] = 0x7F;
    assert_ok(memcmp(buf, ref, 10) > 0, "memcmp gt hi-byte: 0x80 > 0x7F");
    assert_ok(memcmp(ref, buf, 10) < 0, "memcmp lt hi-byte: 0x7F < 0x80");

    /* --- strlen --- */
    memcpy(buf, "hello, world!", 14);
    assert_eq(strlen(buf), 13, "strlen");
    assert_eq(strlen(""), 0, "strlen empty");
    assert_eq(strlen("x"), 1, "strlen single");
    for (int i = 0; i < 100; i++) buf[i] = 'X';
    buf[100] = 0;
    assert_eq(strlen(buf), 100, "strlen long");

    /* --- strcmp --- */
    assert_eq(strcmp("abc", "abc"), 0, "strcmp eq");
    assert_ok(strcmp("abc", "abb") > 0, "strcmp gt");
    assert_ok(strcmp("abc", "abd") < 0, "strcmp lt");
    assert_ok(strcmp("ab", "abc") < 0, "strcmp prefix");
    assert_ok(strcmp("abc", "ab") > 0, "strcmp longer");
    assert_eq(strcmp("", ""), 0, "strcmp empty");

    /* --- strchr --- */
    memcpy(buf, "hello world", 12);
    char *p = strchr(buf, 'w');
    assert_eq(p - buf, 6, "strchr find");
    p = strchr(buf, 'h');
    assert_eq(p - buf, 0, "strchr first");
    p = strchr(buf, 'd');
    assert_eq(p - buf, 10, "strchr last");
    p = strchr(buf, 'z');
    assert_eq((long long)p, 0, "strchr miss");
    p = strchr(buf, '\0');
    assert_eq(*p, 0, "strchr terminator");

    /* --- builtin aliases --- */
    memset(buf, 0, 128);
    __builtin_memcpy(buf, "ABCD", 5);
    assert_eq(buf[0], 'A', "alias memcpy");
    assert_eq(__builtin_strlen(buf), 4, "alias strlen");
    assert_eq(__builtin_memcmp(buf, "ABCD", 4), 0, "alias memcmp");
    r = __builtin_memset(buf, 0, 16);
    assert_eq(r, buf, "alias memset");
    assert_eq(__builtin_strcmp("xyz", "xyz"), 0, "alias strcmp");
    p = __builtin_strchr("abcaba", 'b');
    assert_eq(*p, 'b', "alias strchr");

    /* __builtin_isfinite */
    assert_eq(__builtin_isfinite(3.14), 1, "isfinite(3.14)");
    assert_eq(__builtin_isfinite(0.0), 1, "isfinite(0.0)");
    assert_eq(__builtin_isfinite(-0.0), 1, "isfinite(-0.0)");
    assert_eq(__builtin_isfinite(1.0/0.0), 0, "isfinite(inf)");
    assert_eq(__builtin_isfinite(-1.0/0.0), 0, "isfinite(-inf)");
    assert_eq(__builtin_isfinite(0.0/0.0), 0, "isfinite(NaN)");
    /* __builtin_isfinitef */
    assert_eq(__builtin_isfinitef(3.14f), 1, "isfinitef(3.14)");
    assert_eq(__builtin_isfinitef(0.0f), 1, "isfinitef(0.0)");
    assert_eq(__builtin_isfinitef(1.0f/0.0f), 0, "isfinitef(inf)");
    assert_eq(__builtin_isfinitef(0.0f/0.0f), 0, "isfinitef(NaN)");
    /* __builtin_isfinitel */
    assert_eq(__builtin_isfinitel(3.14L), 1, "isfinitel(3.14)");
    assert_eq(__builtin_isfinitel(0.0L), 1, "isfinitel(0.0)");
    assert_eq(__builtin_isfinitel(1.0L/0.0L), 0, "isfinitel(inf)");
    assert_eq(__builtin_isfinitel(0.0L/0.0L), 0, "isfinitel(NaN)");

    /* __builtin_isnormal */
    assert_eq(__builtin_isnormal(3.14), 1, "isnormal(3.14)");
    assert_eq(__builtin_isnormal(0.0), 0, "isnormal(0.0)");
    assert_eq(__builtin_isnormal(1.0/0.0), 0, "isnormal(inf)");
    assert_eq(__builtin_isnormal(0.0/0.0), 0, "isnormal(NaN)");
    /* __builtin_isnormalf */
    assert_eq(__builtin_isnormalf(3.14f), 1, "isnormalf(3.14)");
    assert_eq(__builtin_isnormalf(0.0f), 0, "isnormalf(0.0)");
    assert_eq(__builtin_isnormalf(1.0f/0.0f), 0, "isnormalf(inf)");
    assert_eq(__builtin_isnormalf(0.0f/0.0f), 0, "isnormalf(NaN)");
    /* __builtin_isnormall */
    assert_eq(__builtin_isnormall(3.14L), 1, "isnormall(3.14)");
    assert_eq(__builtin_isnormall(0.0L), 0, "isnormall(0.0)");
    assert_eq(__builtin_isnormall(1.0L/0.0L), 0, "isnormall(inf)");
    assert_eq(__builtin_isnormall(0.0L/0.0L), 0, "isnormall(NaN)");
    /* __builtin_fpclassify */
    assert_eq(__builtin_fpclassify(3.14), FP_NORMAL, "fpclassify(3.14)");
    assert_eq(__builtin_fpclassify(0.0), FP_ZERO, "fpclassify(0.0)");
    assert_eq(__builtin_fpclassify(1.0/0.0), FP_INFINITE, "fpclassify(inf)");
    assert_eq(__builtin_fpclassify(0.0/0.0), FP_NAN, "fpclassify(NaN)");

    /* __builtin_fpclassifyf */
    assert_eq(__builtin_fpclassifyf(3.14f), FP_NORMAL, "fpclassifyf(3.14)");
    assert_eq(__builtin_fpclassifyf(0.0f), FP_ZERO, "fpclassifyf(0.0)");
    assert_eq(__builtin_fpclassifyf(1.0f/0.0f), FP_INFINITE, "fpclassifyf(inf)");
    assert_eq(__builtin_fpclassifyf(0.0f/0.0f), FP_NAN, "fpclassifyf(NaN)");
    /* __builtin_fpclassifyl */
    assert_eq(__builtin_fpclassifyl(3.14L), FP_NORMAL, "fpclassifyl(3.14)");
    assert_eq(__builtin_fpclassifyl(0.0L), FP_ZERO, "fpclassifyl(0.0)");
    assert_eq(__builtin_fpclassifyl(1.0L/0.0L), FP_INFINITE, "fpclassifyl(inf)");
    assert_eq(__builtin_fpclassifyl(0.0L/0.0L), FP_NAN, "fpclassifyl(NaN)");

    /* Regression: a conditional operator whose branches are void-typed
     * (e.g. `(void)0`, __builtin_unreachable()) yields no value register.
     * Codegen used to move the (nonexistent) branch result into the result
     * register and abort with "Invalid register -1" (perl's Perl_yyparse
     * ASSUME/UNREACHABLE macro triggered this). */
    {
        volatile int sel = 8;
        int hits = 0;
        switch (sel) {
        case 8: hits++; break;
        default:
            /* constant-false condition: always takes the `(void)0` arm */
            ((!"UNREACHABLE") ? (void)0 : __builtin_unreachable());
            __builtin_unreachable();
        }
        assert_eq(hits, 1, "void ternary in switch");

        /* ASSUME-style guard: condition true, unreachable arm not taken */
        int x = sel;
        (x == 8) ? (void)0 : __builtin_unreachable();
        assert_eq(x, 8, "void ternary assume guard");
    }

    /* --- alloca (inline expansion; helper-call fallback is unreachable
     * in ordinary code because the allocator keeps >= 1 register free at
     * the call site — max 6 held arg regs on x86-64, so free >= 2) --- */
    {
        /* basic: allocation returns 16-byte-aligned usable memory */
        char *p = alloca(64);
        for (int i = 0; i < 64; i++) p[i] = (char)i;
        assert_eq(p[0], 0, "alloca basic write/read");
        assert_eq(p[63], 63, "alloca basic last byte");
        assert_eq((long)p % 16, 0, "alloca 16-byte aligned");
    }
    {
        /* large allocation crossing the 4K page-probe loop: each 4K chunk
         * must touch the guard page before subtracting, so a single big
         * alloca can't skip over the stack guard page */
        char *big = alloca(1 << 20); /* 1 MiB */
        big[0] = 1;
        big[(1 << 20) - 1] = 2;
        assert_eq(big[0], 1, "alloca large first byte");
        assert_eq(big[(1 << 20) - 1], 2, "alloca large last byte");
    }
    {
        /* many live alloca results at once (register pressure): all are
         * allocated before any is used, then all are written/read back */
        char *a = alloca(16), *b = alloca(16), *c = alloca(16), *d = alloca(16);
        char *e = alloca(16), *f = alloca(16), *g = alloca(16), *h = alloca(16);
        a[0] = b[1] = c[2] = d[3] = e[4] = f[5] = g[6] = h[7] = 1;
        assert_eq(a[0] + b[1] + c[2] + d[3] + e[4] + f[5] + g[6] + h[7], 8,
                  "alloca many live");
    }
    {
        /* alloca inside a loop body: each iteration gets fresh memory */
        volatile int ok = 1;
        for (int i = 0; i < 8; i++) {
            char *q = alloca(32);
            q[0] = (char)i;
            if (q[0] != i) ok = 0;
        }
        assert_ok(ok, "alloca in loop");
    }

    /* --- alloca helper-call fallback: force free_reg_count()==0 by
     * occupying all 8 x86-64 VRegs with live right-deep-expression
     * values so the inline alloca path falls through to the per-TU
     * __rcc_alloca helper call.  Verifies the use_staging spill-slot
     * preservation (the outer values survive across the call via
     * spilled_regs bits + same-register binary-op combining).
     * Win64 lacks staging-reload in its marshal path (#ifdef _WIN32
     * at the reg-args placement loop), so the spill-preservation fix
     * can't work there yet; skip for now. */
#ifndef _WIN32
    {
        volatile long v = 3;
        char *p;
        /* Right-deep: (v+1)+((v+2)+...+((v+8)+(long)(p=alloca(40))...).
         * With all 8 VRegs busy, free_reg_count()==0, use_staging=1. */
        long r = (v + 1) + ((v + 2) + ((v + 3) + ((v + 4) +
                 ((v + 5) + ((v + 6) + ((v + 7) + ((v + 8) +
                 (long)(p = alloca(40)))))))));
        if (!p) return 2;
        p[0] = 42; p[39] = 7;
        assert_eq(r, 60 + (long)p, "alloca fallback sum = v-sum + ptr");
        assert_eq(p[0], 42, "alloca fallback p[0]");
        assert_eq(p[39], 7, "alloca fallback p[39]");
    }
#endif
    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL BUILTIN TESTS PASSED\n");
    return failures ? 1 : 0;
}
