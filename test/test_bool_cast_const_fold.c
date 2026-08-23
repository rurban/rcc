/* Constant-folding a cast to `_Bool`/`bool` must implement C11 6.3.1.2
 * ("converting any scalar value to _Bool: the result is 0 if the value
 * compares equal to 0, 1 otherwise"), not a truncating bit-copy/mask.
 * Found via ggrep (GNU grep) 3.12's gnulib-tests/test-bool.c:
 *
 *   char d[(bool) 0.5 == true ? 1 : -1];
 *   bool e = &s;
 *
 * Two separate rcc bugs:
 *
 * 1. eval_const_expr()'s ND_CAST case recursed into
 *    eval_const_expr(node->lhs, val) unconditionally first. For a
 *    flonum operand, that recursive call already truncates toward
 *    zero (0.5 -> 0) via the earlier is_flonum() fast path, before the
 *    _Bool-specific truthiness check ever sees the original value --
 *    so `(bool) 0.5` folded to 0 instead of 1 (only a runtime cast, in
 *    codegen.c's gen_cast_reg, already had the correct truthiness
 *    semantics). Fixed by special-casing TY_BOOL first in the ND_CAST
 *    case and evaluating a flonum operand via eval_const_fexpr()
 *    directly, comparing the un-truncated value against 0.0.
 *
 * 2. A file-scope `bool e = &s;` initializer: `&s` is a relocatable
 *    address, not resolvable to a numeric constant until link time,
 *    but its *truthiness* is always known at compile time (an
 *    address-of expression is never null). The existing
 *    "relocatable address stored in a non-pointer scalar" global-
 *    initializer fallback only fired for scalars at least pointer-
 *    width (`var->ty->size >= 8`), so a 1-byte `_Bool` never reached
 *    it and hit "unsupported global initializer". Fixed by adding a
 *    TY_BOOL-specific fallback that folds any looks_like_address_expr()
 *    initializer straight to the constant 1, without needing the
 *    actual address.
 */
#include <assert.h>
#include <stdbool.h>

/* Bug 1: cast-to-bool of a nonzero, sub-1 float constant, in an actual
 * integer-constant-expression context (array size). */
char fractional_is_true[(bool) 0.5 == true ? 1 : -1];
char zero_is_false[(bool) 0.0 == false ? 1 : -1];
char neg_fractional_is_true[(bool) -0.25 == true ? 1 : -1];

/* Same fold, reached through an enum constant-expression. */
enum { HALF_TRUE = ((bool) 0.5 == true) ? 1 : -1 };

/* Bug 2: address-of a global, stored directly in a bool global. */
struct holder { int x; };
struct holder holder_instance;
bool has_address = &holder_instance;
bool null_is_false = false;

int main(void) {
    assert(sizeof(fractional_is_true) == 1);
    assert(sizeof(zero_is_false) == 1);
    assert(sizeof(neg_fractional_is_true) == 1);
    assert(HALF_TRUE == 1);

    assert(has_address == true);
    assert(null_is_false == false);

    /* Runtime equivalents must agree with the constant-folded ones
     * (regression guard: this path was already correct). */
    volatile double half = 0.5;
    assert((bool) half == true);
    volatile double zero = 0.0;
    assert((bool) zero == false);

    return 0;
}
