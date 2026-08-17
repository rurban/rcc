/* rcc's own bundled include/math.h declared most float-precision (`f`-
 * suffixed) <math.h> functions but was missing 27 of them present for
 * `double`/`long double`: log2f, truncf, asinhf, acoshf, atanhf, exp2f,
 * expm1f, log1pf, cbrtf, hypotf, erff, erfcf, copysignf, remainderf,
 * fdimf, fmaxf, fminf, nearbyintf, rintf, lroundf, lrintf, llroundf,
 * llrintf, scalbnf, ldexpf, frexpf, modff. Calling any of these with no
 * other visible prototype falls back to an implicit `int` return type
 * (same class of bug as erf()/erfc(), see test_math_erf.c): the caller
 * reads the return value out of RAX instead of XMM0, producing garbage
 * silently accepted at both compile and link time.
 *
 * Found via box2d's src/math_functions.c: b2UnwindAngle() calls
 * remainderf(radians, 2*pi) with no explicit prototype in scope --
 * remainderf(-1.0f, 2*pi) returned 0 instead of -1.0f (and other inputs
 * returned values in the billions), so every body's initial rotation
 * came out as identity instead of the real angle. The falling-hinges
 * determinism test's tightly-packed initial layout then overlapped
 * badly, the contact solver exploded positions into the millions
 * within one step, and the simulation never settled (the test's outer
 * loop has no step cap, waiting for bodies to sleep) -- an apparent
 * infinite hang, not an obvious crash.
 *
 * Fixed by adding the missing float-precision declarations to
 * include/math.h, matching the existing double/long-double lists.
 * Each assertion below is a tight bound around the true mathematical
 * answer (not just "in some plausible range"), since implicit-int
 * garbage can itself look deceptively plausible (see test_math_erf.c's
 * own note on this). */
#include <math.h>
#include <assert.h>

static int close_to(float a, float b) { return fabsf(a - b) < 1e-6f; }

int main(void) {
    /* The exact real-world trigger: remainderf must give IEEE-754
     * remainder semantics, not fmodf's. */
    assert(close_to(remainderf(-1.0f, 2.0f * 3.14159265f), -1.0f));
    assert(close_to(remainderf(-0.9f, 2.0f * 3.14159265f), -0.9f));
    assert(close_to(remainderf(5.0f, 3.0f), -1.0f));
    assert(close_to(remainderf(5.5f, 2.0f), -0.5f));

    assert(close_to(fdimf(5.0f, 3.0f), 2.0f));
    assert(close_to(fdimf(3.0f, 5.0f), 0.0f));
    assert(close_to(fmaxf(3.0f, 5.0f), 5.0f));
    assert(close_to(fminf(3.0f, 5.0f), 3.0f));
    assert(close_to(copysignf(3.0f, -1.0f), -3.0f));
    assert(close_to(copysignf(-3.0f, 1.0f), 3.0f));

    assert(close_to(log2f(8.0f), 3.0f));
    assert(close_to(truncf(3.7f), 3.0f));
    assert(close_to(truncf(-3.7f), -3.0f));
    assert(close_to(exp2f(4.0f), 16.0f));
    assert(close_to(expm1f(0.0f), 0.0f));
    assert(close_to(log1pf(0.0f), 0.0f));
    assert(close_to(cbrtf(27.0f), 3.0f));
    assert(close_to(hypotf(3.0f, 4.0f), 5.0f));
    assert(close_to(erff(0.0f), 0.0f));
    assert(close_to(erfcf(0.0f), 1.0f));
    assert(close_to(asinhf(0.0f), 0.0f));
    assert(close_to(acoshf(1.0f), 0.0f));
    assert(close_to(atanhf(0.0f), 0.0f));
    assert(close_to(nearbyintf(2.5f), 2.0f)); /* round-to-even default mode */
    assert(close_to(rintf(2.5f), 2.0f));

    assert(lroundf(2.6f) == 3);
    assert(lrintf(2.4f) == 2);
    assert(llroundf(-2.6f) == -3);
    assert(llrintf(-2.4f) == -2);

    assert(close_to(scalbnf(1.0f, 4), 16.0f));
    assert(close_to(ldexpf(1.0f, 4), 16.0f));

    int exp;
    float frac = frexpf(8.0f, &exp);
    assert(close_to(frac, 0.5f) && exp == 4);

    float ip;
    float fp = modff(3.75f, &ip);
    assert(close_to(ip, 3.0f) && close_to(fp, 0.75f));

    return 0;
}
