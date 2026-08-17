/* GCC's `__builtin_add_overflow_p`/`__builtin_sub_overflow_p` (the
 * predicate-only siblings of the already-supported `__builtin_mul_
 * overflow_p`) were entirely unrecognized: neither name was registered
 * as a gperf keyword, nor a `bi_*` interned pointer, nor a codegen
 * dispatch case, so a call like `__builtin_add_overflow_p(a, b, (int)0)`
 * fell through to an ordinary implicit-declaration external call --
 * producing a valid .o that failed at *link* time with "undefined
 * reference to `__builtin_add_overflow_p'". Confirmed genuine real GCC
 * builtins via a direct `gcc -c` check; blocks test/third_party's
 * test_bison (gnulib's lib/canonicalize.c: `INT_ADD_OVERFLOW` macro from
 * intprops.h). Fixed by adding the two names alongside the existing
 * `__builtin_mul_overflow_p` in keywords.gperf/cg_builtins.c, reusing
 * the exact same add/sub-overflow codegen already used by the two-arg
 * overflow-detect + store form, just without the store to the (unused,
 * type-only) third argument.
 */
#include <limits.h>

int main(void)
{
    /* add_overflow_p: predicate only, third arg's type picks the width,
     * its value is never written. */
    if (__builtin_add_overflow_p(1, 2, (int)0) != 0) return 1;
    if (__builtin_add_overflow_p(INT_MAX, 1, (int)0) != 1) return 2;
    if (__builtin_add_overflow_p(-1, 1, (int)0) != 0) return 3;
    if (__builtin_add_overflow_p(1L, 2L, (long)0) != 0) return 4;
    if (__builtin_add_overflow_p(LONG_MAX, 1L, (long)0) != 1) return 5;
    if (__builtin_add_overflow_p(0xffffffffU, 1U, (unsigned)0) != 1) return 6;

    /* sub_overflow_p */
    if (__builtin_sub_overflow_p(2, 1, (int)0) != 0) return 7;
    if (__builtin_sub_overflow_p(INT_MIN, 1, (int)0) != 1) return 8;
    if (__builtin_sub_overflow_p(0U, 1U, (unsigned)0) != 1) return 9;
    if (__builtin_sub_overflow_p(LONG_MIN, 1L, (long)0) != 1) return 10;

    /* mul_overflow_p: pre-existing, verify it's still wired correctly
     * alongside the two new siblings. */
    if (__builtin_mul_overflow_p(100000, 100000, (int)0) != 1) return 11;
    if (__builtin_mul_overflow_p(3, 5, (int)0) != 0) return 12;

    return 0;
}
