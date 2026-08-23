/* C11 6.5.3.3p2: "The result of the unary + operator is the value of
 * its (promoted) operand." A narrower-than-int operand (char/short/
 * bool/bit-field) undergoes ordinary integer promotion to `int`, same
 * as any other arithmetic operand -- unary `+` is not a pure no-op.
 *
 * Found via ggrep (GNU grep) 3.12's gnulib-tests/test-intprops.c:
 * gnulib's INT_PROMOTE(e) macro (intprops.h) is exactly `(+(e))`, used
 * to verify the compiler performs this promotion:
 *
 *   int check = _Generic(INT_PROMOTE((short int)0), int: 0);
 *   int check_size[2 * (sizeof(INT_PROMOTE((short int)0)) == sizeof(int)) - 1];
 *
 * rcc's unary `+` previously just returned its operand completely
 * unchanged (no wrapping node at all), keeping a `short`/`char`/`_Bool`
 * operand's original narrow type -- so both of the above failed
 * (_Generic selected the wrong association; the sizeof comparison, and
 * therefore the array size, came out wrong).
 */
#include <assert.h>
#include <limits.h>

/* A wider-than-int (or exactly int) operand is genuinely unaffected --
 * unary + stays a true no-op there. */
_Static_assert(sizeof(+(long)0) == sizeof(long), "long unaffected by unary +");
_Static_assert(sizeof(+(int)0) == sizeof(int), "int unaffected by unary +");

/* Narrower-than-int operands promote to int. */
_Static_assert(sizeof(+(short)0) == sizeof(int), "short promotes to int");
_Static_assert(sizeof(+(char)0) == sizeof(int), "char promotes to int");
_Static_assert(sizeof(+(signed char)0) == sizeof(int), "signed char promotes to int");
_Static_assert(sizeof(+(unsigned char)0) == sizeof(int), "unsigned char promotes to int");
_Static_assert(sizeof(+(_Bool)0) == sizeof(int), "_Bool promotes to int");

/* Promoted-to-int selects the `int` association, not the original
 * narrow type's. */
int check_short = _Generic(+(short)0, int: 1, short: 2, default: 3);
int check_char = _Generic(+(char)0, int: 1, char: 2, default: 3);

/* Sign is preserved through promotion: a negative short stays negative
 * as int, an unsigned short (same width as int here would stay
 * unsigned, but on rcc's LP64 targets `unsigned short` is narrower
 * than `int` and promotes to signed `int`). */
_Static_assert(+(short)-1 < 0, "promoted negative short stays negative");
_Static_assert(+(unsigned short)0xFFFFu == 65535, "promoted unsigned short value preserved");

int main(void) {
    assert(check_short == 1);
    assert(check_char == 1);

    /* Runtime: a promoted narrow operand participates in arithmetic as
     * a real int, e.g. doesn't wrap at the narrow type's width. */
    short s = 30000;
    int sum = +s + +s; /* 60000, must not wrap as a 16-bit short would allow */
    assert(sum == 60000);

    return 0;
}
