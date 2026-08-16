/* GCC's `sizeof(char[1-2*COND])`-negative-array-size static-assert trick
 * (used by ruby's rb_scan_args_verify(), walking a format-string literal
 * through nested-ternary macros to compute COND at compile time) requires
 * two things rcc previously lacked entirely:
 *
 *  1. eval_const_expr() folding a VLA's (non-strict-ICE) dimension
 *     expression when it's still provably constant -- specifically
 *     string-literal indexing at a constant offset (`"foo"[N]`), which
 *     6.6p6 excludes from a real integer-constant-expression, so the
 *     array stays classified TY_VLA (matching real GCC's own frontend),
 *     but its *runtime* size expression must still be foldable.
 *  2. ND_COND (ternary) and ND_IF codegen skipping the untaken branch
 *     entirely when the condition folds constant -- previously ND_COND
 *     always generated BOTH branches (runtime branching only), so a call
 *     to a __attribute__((error(...)))-marked function guarded behind a
 *     provably-false condition still reached codegen and wrongly
 *     triggered the diagnostic. ND_IF only recognized a bare ND_NUM
 *     condition (e.g. `if (1)`), not a folded comparison/sizeof
 *     expression like `if (1 != 1)`.
 *
 * Two regressions surfaced fixing this and are covered here too:
 *  - String-literal indexing must reject wide/char16_t/char32_t strings
 *    (multi-byte code units, not raw bytes) -- GCC torture's
 *    20010325-1.c, `L"a" "b"[1] != L'b'`.
 *  - A flonum-operand comparison (==, !=, <, <=) must fold in genuine
 *    floating point, not truncate to `long long` first (INFINITY is UB
 *    to convert) -- GCC torture's c23-float-3.c, `INFINITY > FLT_MAX`.
 */

#include <limits.h>

void bad_format(void) __attribute__((error("bad format")));
void length_mismatch(void) __attribute__((error("length mismatch")));

/* Mirrors ruby's own rb_scan_args_count_* nested-ternary macro chain
 * (include/ruby/internal/scan_args.h): walks a format string literal at
 * compile-time-constant offsets via pure macro expansion (no function
 * calls) -- one required-arg digit, one optional-arg digit, then NUL. */
#define isdig(c) (((unsigned char)((c) - '0')) < 10)
#define count_end(fmt, ofs, vari) ((fmt)[ofs] ? -1 : (vari))
#define count_var(fmt, ofs, vari) count_end(fmt, ofs, vari)
#define count_opt(fmt, ofs, vari) \
    (!isdig((fmt)[ofs]) ? count_var(fmt, ofs, vari) : \
     count_var(fmt, (ofs) + 1, (vari) + (fmt)[ofs] - '0'))
#define count_lead(fmt, ofs, vari) \
    (!isdig((fmt)[ofs]) ? count_var(fmt, ofs, vari) : \
     count_opt(fmt, (ofs) + 1, (vari) + (fmt)[ofs] - '0'))

#define scan_count(fmt) count_lead(fmt, 0, 0)

#define verify(fmt, varc) \
    (sizeof(char[1 - 2 * (scan_count(fmt) < 0)]) != 1 ? \
     (bad_format(), 0) : \
     sizeof(char[1 - 2 * (scan_count(fmt) != (varc))]) != 1 ? \
     (length_mismatch(), 0) : \
     0)

#define call_verify(fmt, varc) verify(fmt, varc)

int main(void)
{
    /* "12": one required ('1') + one optional ('2') = vari 1+2 = 3. Both
     * ternary conditions provably fold false at compile time; neither
     * bad_format() nor length_mismatch() may reach codegen. */
    call_verify("12", 3);
    if (scan_count("12") != 3)
        return 1;

    /* Plain `if` form of the same static-assert-via-negative-array-size
     * idiom: must ALSO skip the untaken branch (ND_IF, not just ND_COND). */
    if (sizeof(char[1 - 2 * (scan_count("5") != 5)]) != 1)
        bad_format();
    if (1 != 1)
        bad_format();

    /* Regression: wide string indexing must not fold via the narrow
     * string-literal byte path (would misread multi-byte code units). */
    if (L"a" "b"[1] != L'b')
        return 2;

    /* Regression: flonum comparisons must fold in genuine floating
     * point, not via UB truncation to `long long`. */
    if (!(__builtin_inff() > 3.4028235e38f /* FLT_MAX */))
        return 3;

    return 0;
}
