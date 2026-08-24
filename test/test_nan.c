/* NaN/Infinity constant and libc-call regressions.
 *
 * 1) The C11 7.12p3/p5 NAN/INFINITY macros must be POSITIVE, float-typed
 * constants (matching glibc/GCC: __builtin_inff() / __builtin_nanf("")).
 * A previous `#define NAN (0.0/0.0)` computed the value via runtime
 * hardware division instead: on x86, the invalid-operand result of a
 * 0.0/0.0 division sets the sign bit, so NAN silently became a
 * NEGATIVE NaN -- printf'd as "-nan" instead of "nan" (found via
 * json-c's test_cast, which diffs %f output against a fixture file).
 *
 * 2) rcc's bundled <math.h> is self-contained (no #include_next chaining
 * to glibc) and never declared nan()/nanf()/nanl(). A call to the
 * undeclared function fell back to an implicit "int nan()" declaration,
 * so rcc read the return value out of %rax (integer register) via
 * cvtsi2sd instead of %xmm0 (double register) -- nan("") silently
 * returned 0.0 instead of a quiet NaN.
 *
 * Found via c3lang/c3c: its stdlib builds the `double::nan` / `float::nan`
 * compile-time constants with `nan("")` (src/compiler/sema_expr.c). With
 * the bug, `double::nan` was actually 0.0, so `double::nan == double::nan`
 * evaluated to *true* -- corrupting every NaN-aware comparison built on
 * top of it (c3's own `math::is_approx`/test::eq_approx unit tests). */
#include <math.h>
#include <stdio.h>

int main(void) {
    if (signbit(NAN)) return 1;
    if (signbit(INFINITY)) return 2;
    if (!isnan(NAN)) return 3;
    if (!isinf(INFINITY)) return 4;

    /* NOTE: deliberately not checking printf's %f string form here --
     * "nan" vs "-nan" is glibc's convention; MinGW's legacy MSVCRT
     * formats it entirely differently ("1.#QNAN0"/"-1.#IND00"). The
     * signbit() checks above are the portable, correct assertion. */

    double d = nan("");
    float f = nanf("");

    if (!isnan(d)) { printf("nan(\"\") not NaN: %g\n", d); return 5; }
    if (!isnan(f)) { printf("nanf(\"\") not NaN: %g\n", (double)f); return 6; }

    /* A NaN must never compare equal to itself (IEEE 754). This is what
     * actually broke downstream: with the bug, d/f were 0.0, so d==d and
     * f==f were (wrongly) true. */
    if (d == d) { printf("nan(\"\") == itself\n"); return 7; }
    if (f == f) { printf("nanf(\"\") == itself\n"); return 8; }

    long double ld = nanl("");
    if (!isnan(ld)) { printf("nanl(\"\") not NaN\n"); return 9; }
    if (ld == ld) { printf("nanl(\"\") == itself\n"); return 10; }

    printf("OK\n");
    return 0;
}
