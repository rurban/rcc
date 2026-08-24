/* Regression test: a deeply nested ternary chain (5+ levels) passed as
 * one argument of a multi-argument call corrupted an earlier, already-
 * computed sibling argument.
 *
 * gen_funcall's x86-64 argument-evaluation loop only reserved a FIXED
 * 2-register headroom before deciding whether to stage each argument
 * to a stack slot (`use_staging`). A later argument's own expression
 * can legitimately need more than 2 live registers at its peak -- a
 * jemalloc-style `LG_FLOOR_64(x)` macro (`x < N ? F(x) : k + F(x>>k)`,
 * nested 5-6 levels deep) needs roughly one register per nesting
 * level. Under that pressure, alloc_reg()'s spill-victim search
 * repeatedly borrowed the register still holding an EARLIER argument's
 * already-computed value; the deep expression's own spill/restore
 * bookkeeping didn't perfectly balance across every branch, so the
 * earlier argument's true value was lost with no fault raised.
 *
 * Found via jemalloc's own bit_util.h: `LG_CEIL(x)`/`LG_FLOOR(x)`
 * (both macros, plus the equivalent lg_ceil()/lg_floor() runtime
 * functions built on __builtin_clzll) passed as the *second* argument
 * of a two-argument call alongside the plain first argument `x`
 * itself, once `x` crossed 2^15: the first argument silently read a
 * stale value from a few loop iterations back instead of its own `x`.
 *
 * Fix: gen_funcall now computes a real (if coarse) worst-case register
 * pressure per argument expression (expr_reg_pressure()) instead of a
 * fixed constant, so `use_staging` reliably kicks in whenever a
 * sibling argument's expression could plausibly outgrow the headroom.
 */
#include <stdio.h>

/* Mirrors jemalloc's LG_FLOOR_64/LG_CEIL macro chain (bit_util.h) --
 * a compile-time popcount-style ternary recursion, 6 levels deep for
 * a 64-bit value. */
#define F1(x) 0
#define F2(x) ((x) < (1ULL << 1) ? F1(x) : 1 + F1((x) >> 1))
#define F4(x) ((x) < (1ULL << 2) ? F2(x) : 2 + F2((x) >> 2))
#define F8(x) ((x) < (1ULL << 4) ? F4(x) : 4 + F4((x) >> 4))
#define F16(x) ((x) < (1ULL << 8) ? F8(x) : 8 + F8((x) >> 8))
#define F32(x) ((x) < (1ULL << 16) ? F16(x) : 16 + F16((x) >> 16))
#define F64(x) ((x) < (1ULL << 32) ? F32(x) : 32 + F32((x) >> 32))

static int failures;

#ifndef _WIN32
/* Win64: the #ifdef _WIN32 marshal path in gen_funcall's register-arg
 * loop has no staging/reload mechanism at all (unlike SysV's
 * use_staging or ARM64's use_staging_arm64) -- see test_call_pressure.c
 * for the identical, previously-documented gap. This test exercises
 * that same class of protection, so it is SysV/ARM64-only until the
 * Windows marshal path gets its own staging fix.
 */
static void expect_pair(size_t input, unsigned answer) {
    /* `input` is the plain, unmodified loop variable -- the earlier
     * argument that must survive evaluating the deep ternary chain
     * passed alongside it as the second argument. */
    if (input == 1)
        return;
    if (input > (((size_t)1) << answer) || input <= (((size_t)1) << (answer - 1))) {
        printf("FAIL: input=%zu corrupted (got answer=%u)\n", input, answer);
        failures++;
    }
}
#endif

int main(void) {
    /* The corruption only manifested once `i` crossed 2^15 (32768),
     * where F16's own nested-ternary recursion first needed more than
     * the old fixed 2-register headroom. */
#ifndef _WIN32
    for (size_t i = 32760; i < 32790; i++) {
        expect_pair(i, F16(i) + (((i) & ((i) - 1)) == 0 ? 0 : 1));
        expect_pair(i, F32(i) + (((i) & ((i) - 1)) == 0 ? 0 : 1));
    }
#endif

    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL NESTED TERNARY CALL ARG TESTS PASSED\n");
    return failures ? 1 : 0;
}
