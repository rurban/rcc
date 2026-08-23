/* C11 6.3.1.8p1's usual arithmetic conversions for a signed/unsigned
 * pair of DIFFERENT rank: the three-way rule, not "any unsigned operand
 * makes the result unsigned":
 *
 *   1. If the unsigned operand's rank >= the signed operand's rank,
 *      the signed operand converts to the unsigned type.
 *   2. Else, if the signed type can represent every value of the
 *      unsigned type (true whenever it's strictly WIDER, which every
 *      higher-rank standard type before the same-width long/long long
 *      pair on LP64 always is), the unsigned operand converts to the
 *      signed type -- the result stays SIGNED.
 *   3. Else (same width, e.g. `long` vs `long long` on LP64), both
 *      convert to the unsigned type corresponding to the signed
 *      operand's type.
 *
 * rcc's usual_arith_type() previously computed
 * `is_unsigned = lhs->is_unsigned || rhs->is_unsigned` unconditionally
 * -- correct only for rule 1 and rule 3, but wrong for rule 2: e.g.
 * `long + unsigned int` incorrectly became `unsigned long` instead of
 * staying `long` (signed), silently reinterpreting any negative `long`
 * operand as a huge positive value before the arithmetic even ran.
 *
 * Found via ggrep (GNU grep) 3.12's gnulib-tests/test-intprops.c:
 * `_GL_EXPR_SIGNED(_GL_INT_CONVERT(a, b))` with `a` an `intmax_t`
 * (INTMAX_MIN) and `b` an `unsigned int` (UINT_MAX) computed the common
 * type of the pair as unsigned, so every subsequent MIN/MAX/overflow
 * computation built on it (INT_DIVIDE_OVERFLOW, INT_REMAINDER_OVERFLOW)
 * silently discarded intmax_t's sign.
 */
#include <assert.h>
#include <limits.h>
#include <stdint.h>

/* The core repro, at both compile time (type-driven _Generic/sizeof)
 * and runtime (value-driven comparison). `long`'s width itself is
 * platform-dependent (8 bytes on LP64 Linux/macOS, only 4 bytes -- the
 * same as `unsigned int` -- on LLP64 Windows/mingw), so which rule
 * applies to `long + unsigned int` differs: LP64's wider `long` hits
 * rule 2 (stays signed); LLP64's same-width `long` hits rule 3
 * (becomes unsigned long, like the long/long long pair below). Guard
 * on LONG_MAX vs INT_MAX (true LP64-width test) so both platforms
 * exercise the fix with their own platform-correct expected outcome. */
#if LONG_MAX > INT_MAX
_Static_assert(_Generic((long)0 + (unsigned int)0, long: 1, default: 0),
               "long + unsigned int stays (signed) long on LP64");
_Static_assert(((long)-1 + (unsigned int)0) < 0,
               "a negative long stays negative when added to unsigned int on LP64");
#else
_Static_assert(_Generic((long)0 + (unsigned int)0, unsigned long: 1, default: 0),
               "long + unsigned int becomes unsigned long on LLP64 (same width)");
#endif

_Static_assert(_Generic((long long)0 + (unsigned int)0, long long: 1, default: 0),
               "long long + unsigned int stays (signed) long long");

/* Same rank, mixed sign: the unsigned type correctly wins (rule 1) --
 * regression guard, this direction already worked. */
_Static_assert(_Generic((int)0 + (unsigned int)0, unsigned int: 1, default: 0),
               "int + unsigned int becomes unsigned int");

/* Same width, different rank (long vs long long on LP64): the result
 * must become the UNSIGNED type of the signed operand (rule 3) --
 * neither operand's own type suffices to hold the other's full range. */
#if ULONG_MAX == ULLONG_MAX
_Static_assert(_Generic((unsigned long)0 + (long long)0, unsigned long long: 1, default: 0),
               "unsigned long + long long becomes unsigned long long on LP64");
#endif

/* intmax_t (gnulib's own repro shape) + unsigned int stays signed. */
_Static_assert(_Generic((intmax_t)0 + (unsigned int)0, intmax_t: 1, default: 0),
               "intmax_t + unsigned int stays signed intmax_t");
_Static_assert(((intmax_t) INTMAX_MIN + (unsigned int) 0) < 0,
               "INTMAX_MIN stays negative when added to an unsigned int 0");

int main(void) {
#if LONG_MAX > INT_MAX
    volatile long a = -1;
    volatile unsigned int b = 0;
    assert((a + b) < 0);
#endif

    volatile intmax_t im = INTMAX_MIN;
    volatile unsigned int ub = UINT_MAX;
    /* Must not reinterpret im as a huge positive value first: the sum
     * stays within 1 ULP of INTMAX_MIN, nowhere near a huge unsigned
     * wraparound result. */
    assert(im + (intmax_t) ub == INTMAX_MIN + (intmax_t) UINT_MAX);

    return 0;
}
