/* Test #pragma STDC FENV_ACCESS ON/OFF/DEFAULT, __STDC_FENV_ACCESS__
 * macro, block-scoped save/restore, and constant-folding suppression. */
#include <stdio.h>
#include <math.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

/* ── __STDC_FENV_ACCESS__ macro ─────────────────────────────────────── */
static void test_macro(void) {
#ifdef __STDC_FENV_ACCESS__
    CHECK(__STDC_FENV_ACCESS__ == 1);
#else
    fprintf(stderr, "FAIL: __STDC_FENV_ACCESS__ not defined\n");
    fail++;
#endif
}

/* ── Pragma syntax: ON / OFF / DEFAULT ───────────────────────────────── */
#pragma STDC FENV_ACCESS ON
#pragma STDC FENV_ACCESS OFF
#pragma STDC FENV_ACCESS DEFAULT

static void test_pragma_syntax(void) {
    /* All three pragma forms must compile without error.
     * If we reached this point the parser accepted them. */
    CHECK(1); /* keep the test-counter happy */
}

/* ── Block-scoped save/restore ──────────────────────────────────────── */
static int block_scoped_value(void) {
#pragma STDC FENV_ACCESS ON
    {
        int outer = 0;
        volatile double x = 3.0;
        volatile double y = 4.0;
        double z = x + y;
        outer = (int)(z + 0.5);
        return outer;
    }
}

static void test_block_scope(void) {
    CHECK(block_scoped_value() == 7);
}

/* ── Constant-folding suppression ────────────────────────────────────── */
/* With FENV_ACCESS OFF (default), 1.0/0.0 could be folded to Inf at
 * compile time — no runtime FP exception.  With it ON, the division
 * must happen at runtime.  We verify the result is still correct;
 * actual FE_DIVBYZERO raising is platform-dependent. */

static double folded_div(void) {
    /* no FENV_ACCESS pragma → may constant-fold */
    return 1.0 / 0.0;
}

#pragma STDC FENV_ACCESS ON
static double runtime_div(void) {
    /* FENV_ACCESS ON → NO constant-folding */
    return 1.0 / 0.0;
}
#pragma STDC FENV_ACCESS OFF

static void test_folding(void) {
    double a = folded_div();
    double b = runtime_div();
    CHECK(isinf(a));
    CHECK(isinf(b));
    /* Both paths produce +inf; the difference is that runtime_div()
     * must evaluate at runtime (no compile-time fold). */
}

#pragma STDC FENV_ACCESS ON
static double runtime_add(double x, double y) {
    return x + y;
}
#pragma STDC FENV_ACCESS OFF

static double folded_add(double x, double y) {
    return x + y;
}

static void test_add(void) {
    CHECK(runtime_add(2.0, 3.0) == 5.0);
    CHECK(folded_add(2.0, 3.0) == 5.0);
}

#pragma STDC FENV_ACCESS ON
static double runtime_mul(double x, double y) {
    return x * y;
}
#pragma STDC FENV_ACCESS OFF

static double folded_mul(double x, double y) {
    return x * y;
}

static void test_mul(void) {
    CHECK(runtime_mul(2.5, 4.0) == 10.0);
    CHECK(folded_mul(2.5, 4.0) == 10.0);
}

#pragma STDC FENV_ACCESS ON
static double runtime_neg(double x) {
    return -x;
}
#pragma STDC FENV_ACCESS OFF

static void test_neg(void) {
    CHECK(runtime_neg(3.0) == -3.0);
}

/* ── Struct initializers with float members ──────────────────────────── */
/* Even with FENV_ACCESS ON, static initializers should still work
 * (they are translated at compile time in a separate environment). */

#pragma STDC FENV_ACCESS ON
struct f3 { float a; double b; };
static struct f3 s = { 1.5f, 2.5 };
#pragma STDC FENV_ACCESS OFF

static void test_static_init(void) {
    CHECK(s.a == 1.5f);
    CHECK(s.b == 2.5);
}

/* ── main ────────────────────────────────────────────────────────────── */
int main(void) {
    test_macro();
    test_pragma_syntax();
    test_block_scope();
    test_folding();
    test_add();
    test_mul();
    test_neg();
    test_static_init();
    if (fail)
        fprintf(stderr, "FAIL: %d failures\n", fail);
    return fail;
}
