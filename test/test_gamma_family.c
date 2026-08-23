/* rcc's bundled <math.h> must declare the whole gamma family
 * (tgamma/lgamma/gamma + float/long-double variants): glibc declares
 * these and real code calls them (zsh's math module). Without the
 * declarations rcc treats each call as implicit-int, reading the
 * integer return register instead of xmm0 — tgamma(3) came back 0
 * instead of 2.
 *
 * Platform notes: gamma()/gammaf()/gammal() are lgamma aliases on
 * glibc/macOS, but mingw-w64's libm does not export them at all.
 * long double is 80-bit x87 only on x86-64; rcc's ARM64/Windows
 * long-double ABI is not covered here (quad/80-bit differences).
 * All comparisons use epsilon precision.
 */
#include <math.h>

#define LN2 0.69314718055994530942
#define EPS 1e-12

static int close_d(double a, double b) { return fabs(a - b) < EPS; }
static int close_f(float a, float b) { return fabsf(a - b) < 1e-6f; }
static int close_ld_as_d(long double a, double b) { return fabs((double)a - b) < EPS; }

int main(void) {
    int ok = 1;
    ok = ok && close_d(tgamma(3.0), 2.0);
    ok = ok && close_d(lgamma(3.0), LN2);
    ok = ok && close_f(tgammaf(3.0f), 2.0f);
    ok = ok && close_f(lgammaf(3.0f), (float)LN2);
#if defined(__linux__)
    /* gamma()/gammaf()/gammal() are lgamma aliases on glibc; mingw-w64's
     * libm does not export them, and macOS's semantics differ. */
    ok = ok && (close_d(gamma(3.0), 2.0) || close_d(gamma(3.0), LN2));
    ok = ok && (close_f(gammaf(3.0f), 2.0f) || close_f(gammaf(3.0f), (float)LN2));
    ok = ok && (close_ld_as_d(gammal(3.0L), 2.0) || close_ld_as_d(gammal(3.0L), LN2));
#endif
#if !defined(_WIN32) && !defined(__aarch64__)
    /* long double (80-bit x87) return ABI */
    ok = ok && close_ld_as_d(tgammal(3.0L), 2.0);
    ok = ok && close_ld_as_d(lgammal(3.0L), LN2);
#endif
    return ok ? 0 : 1;
}
