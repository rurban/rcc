/* __builtin_{add,sub,mul}_overflow's x86-64 codegen computed the native
 * operation size `sz` from arga's type ALONE, then only widened a narrower
 * operand when `sz_op > sz && sz == 4` (i.e. only when BOTH operands were
 * assumed 4 bytes). When arga is wide (e.g. `long long`, making sz == 8)
 * but argb is a narrower type (e.g. a plain `int` literal like `-1`), that
 * widen check never fired: argb's register held only its 32-bit value,
 * which x86-64 leaves implicitly zero-extended (not sign-extended) in the
 * upper 32 bits after a 32-bit write. A later 64-bit imulq/add/sub then
 * read the register's full 64 bits as a large positive number instead of
 * the negative value, corrupting both the result and the overflow flag.
 *
 * Found via test/third_party's test_vlc: compat/test/ckd.c's
 * `ckd_mul(&res, LLONG_MAX, -1)` (LLONG_MAX is `long long`, `-1` is `int`).
 */

#include <limits.h>

int main(void)
{
    /* The exact failing case: wide signed lhs, narrow-int rhs literal. */
    { long long res;
      if (__builtin_mul_overflow(LLONG_MAX, -1, &res) || res != -LLONG_MAX)
          return 1; }
    { long long res;
      if (!__builtin_mul_overflow(LLONG_MIN, -1, &res) || res != LLONG_MIN)
          return 2; }

    /* Same mismatch, operands swapped (narrow lhs, wide rhs). */
    { long long res;
      if (__builtin_mul_overflow(-1, LLONG_MAX, &res) || res != -LLONG_MAX)
          return 3; }

    /* Mixed width also applies to add/sub. */
    { long long res;
      if (__builtin_add_overflow(LLONG_MAX, -1, &res) || res != LLONG_MAX - 1)
          return 4; }
    { long long res;
      if (__builtin_sub_overflow(LLONG_MIN, -1, &res) || res != LLONG_MIN + 1)
          return 5; }

    /* Unsigned narrow rhs mixed with a wide signed lhs. */
    { long long res;
      unsigned rhs = 2;
      if (!__builtin_mul_overflow(LLONG_MAX, rhs, &res))
          return 6; }

    return 0;
}
