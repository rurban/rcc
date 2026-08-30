/* A `static`/global floating-point initializer whose expression contains
 * an *integer* sub-division (both operands of `/` are integers, so C
 * requires truncating integer division there, even though the overall
 * expression's outer operation and final type are floating-point) must
 * fold that inner division with C's integer semantics, not plain
 * floating-point division.
 *
 * Regression: parser.c's eval_double_const_expr() (used to fold global/
 * static initializers at compile time) evaluated every ND_DIV node --
 * including ones whose own type was `int` -- as plain `lhs / rhs` in
 * `double` precision, only ever narrowing the *outer* result to `float`
 * when the outermost node's type was TY_FLOAT. An inner integer division
 * nested inside a larger float expression silently kept its fractional
 * part instead of truncating it, producing a value measurably different
 * from what identical code computes at runtime (where the integer
 * division genuinely truncates, per real codegen).
 *
 * Found via a real SDL3 build: test/testautomation_stdlib.c computes an
 * NTSC-rate float constant as `((100 * 60 * 1000) / 1001) / 100.0f`
 * (integer 6000000/1001 truncates to 5994, /100.0f == 59.94...) in a
 * `static struct {...} f_and_g_test_cases[] = {...}` initializer --
 * folded to 0x426fc29f (leaving 6000000.0/1001.0 = 5994.0059... un-
 * truncated) instead of the correct 0x426fc28f that both real GCC and
 * this same expression evaluated at *runtime* by rcc itself produce,
 * breaking every SDL_snprintf/SDL_vsnprintf `%f`/`%g` precision test
 * built on it.
 */
#include <stdio.h>
#include <string.h>

/* Global/static storage: folded at compile time by eval_double_const_expr. */
static float g_const = ((100 * 60 * 1000) / 1001) / 100.0f;

int main(void) {
    /* Local: computed by ordinary runtime codegen, never constant-folded
     * through eval_double_const_expr -- the ground truth to compare
     * against (and independently verified to match real GCC bit-for-
     * bit: 0x426fc28f). */
    float g_runtime = ((100 * 60 * 1000) / 1001) / 100.0f;

    unsigned int const_bits, runtime_bits;
    memcpy(&const_bits, &g_const, 4);
    memcpy(&runtime_bits, &g_runtime, 4);

    if (const_bits != runtime_bits) {
        printf("FAIL: compile-time-folded constant = %08x, runtime value = %08x (want equal, expect 426fc28f)\n",
               const_bits, runtime_bits);
        return 1;
    }
    if (const_bits != 0x426fc28fu) {
        printf("FAIL: both compile-time and runtime values are %08x, want 426fc28f\n", const_bits);
        return 1;
    }

    printf("OK integer sub-division inside a float constant expression truncates correctly (%.6f)\n",
           (double)g_const);
    return 0;
}
