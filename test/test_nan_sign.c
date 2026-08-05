/* The C11 7.12p3/p5 NAN/INFINITY macros must be POSITIVE, float-typed
 * constants (matching glibc/GCC: __builtin_inff() / __builtin_nanf("")).
 * A previous `#define NAN (0.0/0.0)` computed the value via runtime
 * hardware division instead: on x86, the invalid-operand result of a
 * 0.0/0.0 division sets the sign bit, so NAN silently became a
 * NEGATIVE NaN -- printf'd as "-nan" instead of "nan" (found via
 * json-c's test_cast, which diffs %f output against a fixture file). */
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

    printf("OK\n");
    return 0;
}
