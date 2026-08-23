/* Constant-folding an integer arithmetic expression (+, -, *, shifts,
 * bitwise ops) must truncate/sign-extend the result to the expression's
 * own type width after EVERY operation, exactly like real hardware and
 * rcc's own runtime codegen do -- not carry a full 64-bit `long long`
 * through the whole fold and only mask at the very end (or never).
 *
 * Found via ggrep (GNU grep) 3.12's gnulib-tests/test-intprops.c, via
 * gnulib's own `(1 ? 0 : (e)) - (v)` "ghost ternary" idiom
 * (_GL_INT_NEGATE_CONVERT in intprops-internal.h) used to get a zero
 * value of "the same type e would have": for an `unsigned int` e,
 * `(1 ? 0 : e) - 1` must wrap to UINT_MAX (0xFFFFFFFF), matching plain
 * `0u - 1`. Before this fix, eval_const_expr()'s ND_SUB (and ND_ADD/
 * ND_MUL) case computed `lhs - rhs` as a raw 64-bit `long long` with no
 * truncation, so the folded value stayed the 64-bit pattern -1
 * (0xFFFFFFFFFFFFFFFF) instead of the 32-bit-truncated 0xFFFFFFFF. A
 * subsequent `>>` on that value inspected the correct `unsigned int`
 * type but shifted the WRONG (un-truncated) 64-bit pattern, corrupting
 * every INT_LEFT_SHIFT_OVERFLOW-style compile-time check built on it.
 *
 * Fixed by wrapping eval_const_expr() in a truncating entry point: fold
 * via the (renamed, internal) implementation, then mask/sign-extend the
 * result to node->ty's width before returning -- applied once per node,
 * so every intermediate value in a deeply nested constant expression is
 * correctly truncated exactly where real arithmetic would truncate it.
 */
#include <assert.h>
#include <limits.h>

/* The core repro: an unsigned int wraparound subtraction, constant-
 * folded, must produce the truncated (wrapped) value -- not a sign-
 * extended 64-bit one. */
_Static_assert(0u - 1 == UINT_MAX, "0u - 1 wraps to UINT_MAX");
_Static_assert((0u - 1) >> 1 == (UINT_MAX >> 1), "wrapped value shifts as UINT_MAX would");

/* The exact gnulib ghost-ternary idiom. */
#define GHOST_MINUS(e, v) ((1 ? 0 : (e)) - (v))
_Static_assert(GHOST_MINUS(UINT_MAX, 1) == UINT_MAX, "ghost ternary minus one wraps to UINT_MAX");
_Static_assert((GHOST_MINUS(UINT_MAX, 1) >> 1) == (UINT_MAX >> 1),
               "ghost ternary minus one shifts identically to plain UINT_MAX");

/* Addition and multiplication wraparound must also truncate correctly
 * at narrower-than-long-long widths. */
_Static_assert((unsigned char)(0xFFu + 2) == 1, "unsigned char add wraps at 8 bits");
_Static_assert(((unsigned short)(0xFFFFu * 3) >> 1) == (unsigned short)(0xFFFFu * 3) / 2,
               "unsigned short multiply wraps at 16 bits before shifting");
_Static_assert((0xFFFFFFFFu * 3u) == (unsigned)(0xFFFFFFFFu * 3u), "unsigned int multiply wraps at 32 bits");

/* Signed narrow-width overflow wraps (implementation-defined, but must
 * match what the same expression evaluates to at runtime -- two's
 * complement, matching rcc's own codegen and every real target). */
_Static_assert((signed char)(127 + 1) == -128, "signed char add wraps at 8 bits, two's complement");

int main(void) {
    /* Runtime must agree with every compile-time fold above. */
    volatile unsigned int one = 1;
    assert((0u - one) == UINT_MAX);
    assert(((0u - one) >> 1) == (UINT_MAX >> 1));

    volatile unsigned char uc = 0xFF;
    assert((unsigned char)(uc + 2) == 1);

    volatile signed char sc = 127;
    assert((signed char)(sc + 1) == -128);

    return 0;
}
