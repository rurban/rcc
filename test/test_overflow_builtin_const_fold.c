/* __builtin_{add,sub,mul}_overflow_p(a, b, (T)0) must constant-fold in a
 * static_assert/array-size context, matching real GCC/Clang. Found via
 * ggrep (GNU grep) 3.12's gnulib-tests/test-intprops.c, whose VERIFY()
 * macro picks `static_assert` over a runtime ASSERT specifically when
 * `__GNUC__ || __clang__` is defined (rcc defines __GNUC__=16 for ABI/
 * feature compatibility) -- and separately, gnulib's own
 * _GL_HAS_BUILTIN_OVERFLOW_P gate is `__has_builtin
 * (__builtin_mul_overflow_p)`, which rcc's __has_builtin table already
 * (correctly) reports as available, since cg_builtins.c does implement
 * the RUNTIME codegen for these -- but eval_const_expr() (parser.c's
 * constant-expression folder) had no case for them at all, so any
 * compile-time use hit "condition must be a constant expression" even
 * though every operand was already a compile-time constant.
 *
 * Two sub-bugs fixed together:
 *  1. No fold at all for these three builtins (added the ND_FUNCALL case).
 *  2. The two value operands must be treated as their OWN type's exact
 *     mathematical value (matching GCC's documented "infinite-precision
 *     signed math per operand" contract) before checking against the
 *     third argument's type range -- naively reinterpreting through the
 *     result type first breaks e.g. `INT_MIN * ULONG_MAX` (INT_MIN must
 *     stay the negative value -2147483648, not become a huge unsigned
 *     value, even though the result type is unsigned long).
 */
#include <assert.h>
#include <limits.h>

_Static_assert(__builtin_add_overflow_p(2147483647, 1, (int)0),
               "INT_MAX + 1 overflows int");
_Static_assert(!__builtin_add_overflow_p(2147483646, 1, (int)0),
               "INT_MAX - 1 + 1 does not overflow int");
_Static_assert(__builtin_sub_overflow_p(INT_MIN, 1, (int)0),
               "INT_MIN - 1 overflows int");
_Static_assert(!__builtin_sub_overflow_p(INT_MIN + 1, 1, (int)0),
               "(INT_MIN+1) - 1 does not overflow int");
_Static_assert(__builtin_mul_overflow_p(INT_MAX, 2, (int)0),
               "INT_MAX * 2 overflows int");
_Static_assert(!__builtin_mul_overflow_p(1000, 1000, (int)0),
               "1000*1000 does not overflow int");
_Static_assert(__builtin_add_overflow_p(4294967295u, 1u, (unsigned)0),
               "UINT_MAX + 1 overflows unsigned");
_Static_assert(!__builtin_add_overflow_p(4294967294u, 1u, (unsigned)0),
               "UINT_MAX - 1 + 1 does not overflow unsigned");

/* A negative signed operand must keep its true (negative) value, not
 * get reinterpreted through the unsigned result type first. */
_Static_assert(__builtin_mul_overflow_p(INT_MIN, 4294967295UL, (unsigned long)0),
               "INT_MIN * ULONG_MAX overflows unsigned long");
_Static_assert(__builtin_mul_overflow_p(INT_MIN, 4294967295u, (unsigned)0),
               "INT_MIN * UINT_MAX overflows unsigned");

int main(void) {
    /* Runtime equivalents must agree with the constant-folded ones. */
    volatile int a = INT_MAX, one = 1;
    assert(__builtin_add_overflow_p(a, one, (int)0));
    volatile int b = 1000;
    assert(!__builtin_mul_overflow_p(b, b, (int)0));
    return 0;
}
