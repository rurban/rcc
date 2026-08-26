/* opt.c's own peephole constant-folding pass for ND_DIV/ND_MOD (a
 * SEPARATE, independent fold from parser.c's eval_const_expr_impl,
 * applied to already-ND_NUM operands during -O1+ AST optimization)
 * always did PLAIN SIGNED C division/modulo on the operands' `long
 * long .val` fields, with no unsigned check at all -- unlike
 * eval_const_expr_impl, which correctly reinterprets as
 * `unsigned long long` first when the operand type is unsigned.
 *
 * A huge unsigned constant like SIZE_MAX/UINT64_MAX
 * (18446744073709551615, all-1s bit pattern) stores as -1 when the
 * `long long` field is read as signed. Plain signed `-1 / 8` truncates
 * toward zero to 0 (C's signed-division truncation rule) instead of
 * the correct unsigned result 0x1FFFFFFFFFFFFFFF -- silently
 * corrupting any `SIZE_MAX / sizeof(x)`-style overflow-guard divisor
 * computed at -O1 or higher.
 *
 * Only reproduces at -O1+ (this fold pass is part of AST optimization,
 * never runs at -O0, where the division is correctly emitted as a
 * genuine runtime `div` instruction instead) -- found via postgres's
 * fe_utils/print.c:
 *
 *   uint64 total_cells = (uint64) ncolumns * nrows;
 *   if (total_cells >= SIZE_MAX / sizeof(*content->cells))
 *       ... "Cannot print table contents ... maximum 0" ...
 *
 * which made literally every psql-driven regression test fail
 * identically once postgres's build itself was fixed enough to
 * bootstrap. Fixed by mirroring eval_const_expr_impl's unsigned
 * handling in opt.c's fold. */
#include <stdint.h>

int main(void) {
    int ok = 1;

    /* The exact shape from print.c: SIZE_MAX / sizeof(pointer).
     * Use uint64_t/int64_t explicitly throughout (not `unsigned
     * long`/`long`, which are only 32-bit on LLP64/Windows -- assigning
     * a genuine 64-bit SIZE_MAX-derived value to a 32-bit `unsigned
     * long` would silently truncate it there, corrupting the test's
     * own expectations independent of the compiler bug being checked). */
    uint64_t a = SIZE_MAX / sizeof(int *);
    ok = ok && a == 2305843009213693951ULL;

    /* Plain UINT64_MAX / small divisor. */
    uint64_t b = 18446744073709551615ULL / 3ULL;
    ok = ok && b == 6148914691236517205ULL;

    /* Modulo needs the same fix. */
    uint64_t c = SIZE_MAX % 7ULL;
    ok = ok && c == 1ULL;

    /* A signed huge-magnitude divide must still behave like real GCC's
     * truncating signed division (sanity check the fix didn't flip
     * signed division to also go through the unsigned path). */
    int64_t d = -100LL / 3LL;
    ok = ok && d == -33LL;

    return ok ? 0 : 1;
}
