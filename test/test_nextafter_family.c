/* rcc's bundled <math.h> must declare nextafter/nextafterf/nextafterl:
 * glibc declares them and real code calls them directly (jerryscript's
 * ecma-helpers-number.c uses __builtin_nextafter under __GNUC__, and
 * other code calls plain nextafter()). Without the declaration, each
 * call was implicit-int (matching the exact tgamma/lgamma bug this
 * file's test_gamma_family.c documents): the compiler assumed a 4/8-byte
 * integer return in RAX and never touched XMM0 at all, so the result was
 * whatever double bit pattern happened to already be sitting in XMM0
 * from an unrelated prior call -- e.g. two back-to-back calls
 * `f(1.0,2.0); nextafter(1.0,2.0);` silently returned f's own result a
 * second time, not nextafter's.
 *
 * A second, independent bug in the same area: `__builtin_nextafter[f|l]`
 * (real GCC/Clang's builtin form, gated on `#if defined(__GNUC__)` in
 * jerryscript and elsewhere since rcc defines __GNUC__ for
 * compatibility) had no dispatch entry at all, emitting a call to the
 * literal, nonexistent symbol "__builtin_nextafter" -- "undefined
 * reference to `__builtin_nextafter'" at link time. Fixed by declaring
 * the __builtin_ names (parser.c's declare_builtin_on_demand) and
 * renaming them to the real libm symbols at codegen (codegen.c's
 * __builtin_* rename table, matching __builtin_pow/__builtin_fabs/etc.).
 *
 * Platform notes: long double is 80-bit x87 extended only on x86-64;
 * rcc represents `long double` scalars captured in a GP register (e.g.
 * a function's return value) as 64-bit double precision internally
 * (a separate, pre-existing, documented architectural limitation --
 * see TODO.md), so nextafterl()'s result is checked only loosely here.
 */
#include <math.h>

#define EPS 1e-15

static int close_d(double a, double b) { return fabs(a - b) < EPS; }
static int close_f(float a, float b) { return fabsf(a - b) < 1e-6f; }

/* A prior double-returning call, to reproduce the "stale XMM0" symptom
 * an undeclared nextafter() exhibited: if nextafter falls back to
 * implicit-int codegen, this call's leftover XMM0 value bleeds through. */
static double prior_call(double a, double b) { return a + b * 1000.0; }

int main(void) {
    int ok = 1;

    double before = prior_call(1.0, 2.0); /* = 2001.0, left in xmm0 */
    double na = nextafter(1.0, 2.0); /* smallest double > 1.0 */
    ok = ok && na != before;
    ok = ok && na > 1.0 && close_d(na, 1.0000000000000002220446049250313);

    ok = ok && close_d(nextafter(1.0, -1.0 / 0.0), 0.99999999999999988898);
    ok = ok && nextafter(2.0, 2.0) == 2.0; /* from == to: no change */

    ok = ok && close_f(nextafterf(1.0f, 2.0f), 1.00000011920928955078f);

    /* __builtin_nextafter[f] -- the separate "undefined reference to
     * `__builtin_nextafter'" link bug, gated the same way real
     * jerryscript source gates it. */
#if defined(__GNUC__) || defined(__clang__)
    ok = ok && close_d(__builtin_nextafter(1.0, -1.0 / 0.0), 0.99999999999999988898);
    ok = ok && close_f(__builtin_nextafterf(1.0f, 2.0f), 1.00000011920928955078f);
#endif

#if !defined(_WIN32) && !defined(__aarch64__)
    /* long double: just confirm it links, runs, and moves in the right
     * direction (see the long-double-precision caveat above). */
    long double nal = nextafterl(1.0L, 2.0L);
    ok = ok && nal >= 1.0L;
    long double bnal = __builtin_nextafterl(1.0L, 2.0L);
    ok = ok && bnal >= 1.0L;
#endif

    return ok ? 0 : 1;
}
