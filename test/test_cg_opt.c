/* Constant-divisor strength reduction (src/cg_opt.c): `/` and `%` by a
 * compile-time-constant divisor are replaced with shifts and/or a
 * magic-number multiply instead of a runtime idiv/div (Hacker's Delight /
 * LLVM DivisionByConstantInfo technique). Differential test: for each
 * literal divisor D, compares rcc's own constant-folded `n / D` / `n % D`
 * (exercises cg_opt.c) against the same division through a volatile,
 * non-constant-foldable divisor (exercises rcc's plain idiv/div path) --
 * both compiled by rcc itself, so any codegen bug in either path shows up
 * as a runtime mismatch. Covers unsigned/signed, 32/64-bit, trivial (1,
 * -1), power-of-two (positive and negative), and general (odd and even
 * non-power-of-two, including large divisors needing a full-width magic
 * constant) divisors, over boundary and mid-range dividends.
 *
 * n == INT_MIN, D == -1 is skipped: that specific combination overflows
 * the quotient and traps (#DE / SIGFPE) on real x86 idiv hardware
 * regardless of compiler, so it cannot be used as a reference oracle.
 * rcc's optimized path avoids the trap by folding n/-1 to -n (matching
 * gcc/clang, since the C standard leaves this case undefined).
 */
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

volatile uint32_t u32_dividends[] = {0, 1, 2, 3, 7, 100, 12345, 0xFFFFFFFFu, 0x80000000u, 999999937u, 4294967294u};
volatile int32_t s32_dividends[] = {0, 1, -1, 2, -2, 7, -7, 100, -100, 12345, -12345, INT32_MIN, INT32_MAX, INT32_MIN + 1};
volatile uint64_t u64_dividends[] = {0, 1, 2, 1000000007ULL, 0xFFFFFFFFFFFFFFFFULL, 0x8000000000000000ULL, 12345678901234ULL};
volatile int64_t s64_dividends[] = {0, 1, -1, 1000000007LL, -1000000007LL, LLONG_MIN, LLONG_MAX, LLONG_MIN + 1};

static int fails;

#define CHECK_U32(D) do { \
    for (int _i = 0; _i < (int)(sizeof(u32_dividends) / sizeof(u32_dividends[0])); _i++) { \
        uint32_t n = u32_dividends[_i]; \
        volatile uint32_t vd = (D); \
        uint32_t qo = n / (D), ro = n % (D); \
        uint32_t qr = n / vd, rr = n % vd; \
        if (qo != qr || ro != rr) { \
            printf("FAIL u32 n=%u d=%u: opt q=%u r=%u ref q=%u r=%u\n", n, (unsigned)(D), qo, ro, qr, rr); \
            fails++; \
        } \
    } \
} while (0)

#define CHECK_S32(D) do { \
    for (int _i = 0; _i < (int)(sizeof(s32_dividends) / sizeof(s32_dividends[0])); _i++) { \
        int32_t n = s32_dividends[_i]; \
        if (n == INT32_MIN && (D) == -1) continue; \
        volatile int32_t vd = (D); \
        int32_t qo = n / (D), ro = n % (D); \
        int32_t qr = n / vd, rr = n % vd; \
        if (qo != qr || ro != rr) { \
            printf("FAIL s32 n=%d d=%d: opt q=%d r=%d ref q=%d r=%d\n", n, (int)(D), qo, ro, qr, rr); \
            fails++; \
        } \
    } \
} while (0)

#define CHECK_U64(D) do { \
    for (int _i = 0; _i < (int)(sizeof(u64_dividends) / sizeof(u64_dividends[0])); _i++) { \
        uint64_t n = u64_dividends[_i]; \
        volatile uint64_t vd = (D); \
        uint64_t qo = n / (D), ro = n % (D); \
        uint64_t qr = n / vd, rr = n % vd; \
        if (qo != qr || ro != rr) { \
            printf("FAIL u64 n=%llu d=%llu: opt q=%llu r=%llu ref q=%llu r=%llu\n", \
                   (unsigned long long)n, (unsigned long long)(D), (unsigned long long)qo, \
                   (unsigned long long)ro, (unsigned long long)qr, (unsigned long long)rr); \
            fails++; \
        } \
    } \
} while (0)

#define CHECK_S64(D) do { \
    for (int _i = 0; _i < (int)(sizeof(s64_dividends) / sizeof(s64_dividends[0])); _i++) { \
        int64_t n = s64_dividends[_i]; \
        if (n == LLONG_MIN && (D) == -1) continue; \
        volatile int64_t vd = (D); \
        int64_t qo = n / (D), ro = n % (D); \
        int64_t qr = n / vd, rr = n % vd; \
        if (qo != qr || ro != rr) { \
            printf("FAIL s64 n=%lld d=%lld: opt q=%lld r=%lld ref q=%lld r=%lld\n", \
                   (long long)n, (long long)(D), (long long)qo, (long long)ro, (long long)qr, (long long)rr); \
            fails++; \
        } \
    } \
} while (0)

int main(void) {
    /* unsigned 32-bit: trivial, powers of two, odd and even general */
    CHECK_U32(1); CHECK_U32(2); CHECK_U32(3); CHECK_U32(4); CHECK_U32(5);
    CHECK_U32(6); CHECK_U32(7); CHECK_U32(8); CHECK_U32(9); CHECK_U32(10);
    CHECK_U32(11); CHECK_U32(13); CHECK_U32(16); CHECK_U32(17); CHECK_U32(100);
    CHECK_U32(255); CHECK_U32(641); CHECK_U32(1000); CHECK_U32(2147483648u);
    CHECK_U32(999999937u); CHECK_U32(4294967295u);

    /* signed 32-bit: trivial (1,-1), powers of two (both signs), general */
    CHECK_S32(1); CHECK_S32(-1); CHECK_S32(2); CHECK_S32(-2); CHECK_S32(3);
    CHECK_S32(-3); CHECK_S32(4); CHECK_S32(-4); CHECK_S32(5); CHECK_S32(-5);
    CHECK_S32(6); CHECK_S32(-6); CHECK_S32(7); CHECK_S32(-7); CHECK_S32(8);
    CHECK_S32(-8); CHECK_S32(9); CHECK_S32(-9); CHECK_S32(11); CHECK_S32(-11);
    CHECK_S32(13); CHECK_S32(-13); CHECK_S32(16); CHECK_S32(-16);
    CHECK_S32(100); CHECK_S32(-100); CHECK_S32(255); CHECK_S32(-255);
    CHECK_S32(641); CHECK_S32(-641); CHECK_S32(1000); CHECK_S32(-1000);
    CHECK_S32(1073741824); CHECK_S32(-1073741824);
    CHECK_S32(INT32_MIN); CHECK_S32(INT32_MAX);

    /* unsigned 64-bit: including divisors needing the full 64-bit magic
     * constant (no small 32-bit-or-less magic number exists) */
    CHECK_U64(1); CHECK_U64(2); CHECK_U64(3); CHECK_U64(4); CHECK_U64(5);
    CHECK_U64(6); CHECK_U64(7); CHECK_U64(9); CHECK_U64(13); CHECK_U64(16);
    CHECK_U64(255); CHECK_U64(641); CHECK_U64(1000000007ULL);
    CHECK_U64(3000000019ULL); CHECK_U64(9223372036854775783ULL);
    CHECK_U64(18446744073709551557ULL); CHECK_U64(4294967296ULL);
    CHECK_U64(9223372036854775808ULL);

    /* signed 64-bit */
    CHECK_S64(1); CHECK_S64(-1); CHECK_S64(2); CHECK_S64(-2); CHECK_S64(3);
    CHECK_S64(-3); CHECK_S64(6); CHECK_S64(-6); CHECK_S64(7); CHECK_S64(-7);
    CHECK_S64(9); CHECK_S64(-9); CHECK_S64(255); CHECK_S64(-255);
    CHECK_S64(641); CHECK_S64(-641);
    CHECK_S64(1000000007LL); CHECK_S64(-1000000007LL);
    CHECK_S64(4611686018427387903LL); CHECK_S64(-4611686018427387903LL);
    CHECK_S64(1099511627776LL); CHECK_S64(-1099511627776LL); CHECK_S64(LLONG_MIN);

    if (fails) {
        printf("%d failure(s)\n", fails);
        return 1;
    }
    printf("OK\n");
    return 0;
}
