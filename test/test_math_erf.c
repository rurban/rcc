/* rcc's own bundled include/math.h (used in preference to the system
 * <math.h> the same way emmintrin.h/tmmintrin.h are) declared most of
 * ISO C99's <math.h> surface but was missing erf()/erfc() entirely.
 * Calling an undeclared external function falls back to an implicit
 * `int` return type (K&R legacy behavior, silently accepted, no
 * diagnostic) -- so `double y = erf(x);` treated erf's return value as
 * coming back in RAX (the integer-return register) and converted it to
 * double via cvtsi2sd, instead of reading the real double result glibc
 * placed in XMM0. The call itself succeeded and glibc computed the
 * correct value; only the *caller's* interpretation of the return
 * register was wrong. Any single external call, with no other code
 * around it, reproduced this -- no mruby/VM machinery needed.
 *
 * Found via mruby 4.0.0's mrbtest suite: Math.erf(1) returned
 * 1072693248.0 instead of ~0.8427 (its bit pattern happens to be a
 * "recognizable-looking" large integer, not obviously garbage, which
 * is what made this particular miscompile easy to spot in test output
 * but easy to miss by inspection otherwise).
 *
 * Fixed by adding `double erf(double);` / `double erfc(double);` to
 * include/math.h. Regression-tests against implicit "looks-plausible"
 * garbage, not just "not obviously wrong": both bounds-checks below
 * would trivially pass for values in a huge wrong range too, so the
 * comparisons are tight around the true mathematical answer. */
#include <math.h>
#include <assert.h>

static int close_to(double a, double b) { return fabs(a - b) < 1e-9; }

int main(void) {
    assert(close_to(erf(0.0), 0.0));
    assert(close_to(erf(1.0), 0.8427007929497149));
    assert(close_to(erf(-1.0), -0.8427007929497149));
    assert(close_to(erfc(0.0), 1.0));
    assert(close_to(erfc(1.0), 0.15729920705028513));
    /* erf + erfc == 1 for all real x (the actual mathematical identity,
     * not just "some value in a plausible range"). */
    assert(close_to(erf(2.0) + erfc(2.0), 1.0));
    return 0;
}
