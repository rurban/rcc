/* GCC's fixed-width/signedness `__builtin_{s,u}{add,sub,mul}{,l,ll}_overflow`
 * family (as opposed to the type-generic 3-arg `__builtin_{add,sub,mul}_
 * overflow`, already supported) was entirely unrecognized: rcc's parser
 * silently treated the call as an ordinary implicit-declaration external
 * call, producing a valid .o that failed at *link* time with "undefined
 * reference" instead of expanding inline arithmetic + overflow-flag
 * codegen. cg_builtins.c already contained the add/sub/mul overflow
 * codegen (driven off the actual argument/result-pointer types, which
 * makes it width/signedness-correct for any of these names for free) --
 * only the eighteen name -> codegen-dispatch registrations were missing.
 *
 * Confirmed a genuine, real GCC builtin family via direct `gcc -c` check;
 * blocks test/third_party's test_libgit2 (src/util/alloc.c, filebuf.c,
 * fs_path.c).
 *
 * Separately: the type-generic 3-arg form with a destination NARROWER
 * than `int` (`int8_t`/`int16_t`/`uint8_t`/`uint16_t *result`) had two
 * stacked x86-64 codegen bugs. The result pointer's pointee size was
 * clamped to a minimum of 4 bytes, so (a) a narrow destination got
 * range-checked against 32-bit bounds instead of its own 8/16-bit range
 * -- overflow into a genuinely narrow type went undetected whenever the
 * mathematically exact result still fit in 32 bits (e.g. `int16_t`:
 * 32767*2 = 65534, which overflows int16 but not int32) -- and (b) the
 * same clamped size was then also used as the MEMORY STORE WIDTH, so
 * storing into a genuine `int8_t`/`int16_t *` wrote 4 bytes instead of
 * 1/2, corrupting whatever memory followed the narrow destination.
 * ARM64 had the identical clamp (already-written `res_sz == 1`/`== 2`
 * range-check paths existed but were permanently unreachable dead code
 * because of it).
 *
 * Found via postgres's `int2mul`/`int2pl`/`int2mi`
 * (`src/backend/utils/adt/int.c`), which rely on
 * `pg_mul_s16_overflow`/`pg_add_s16_overflow`/`pg_sub_s16_overflow`
 * (`src/include/common/int.h`) wrapping exactly these builtins to
 * implement SQL smallint arithmetic overflow detection ("smallint out
 * of range") -- every `SELECT ... * int2 '2' ...`-style overflow test
 * in postgres's own regression suite silently produced a wrapped-around
 * result instead of the expected error. Fixed by tracking the TRUE,
 * unclamped destination byte size separately from the clamped
 * operand-computation width, using it for both the final range check
 * (truncate + re-widen + compare against the always-exact >=32-bit
 * computed result, which cannot itself overflow for narrow inputs) and
 * the memory store.
 *
 * Separately: x86-64 codegen computed the native operation size `sz`
 * from arga's type ALONE, then only widened a narrower operand when
 * `sz_op > sz && sz == 4` (i.e. only when BOTH operands were assumed 4
 * bytes). When arga is wide (e.g. `long long`, making sz == 8) but argb
 * is a narrower type (e.g. a plain `int` literal like `-1`), that widen
 * check never fired: argb's register held only its 32-bit value, which
 * x86-64 leaves implicitly zero-extended (not sign-extended) in the
 * upper 32 bits after a 32-bit write. A later 64-bit imulq/add/sub then
 * read the register's full 64 bits as a large positive number instead
 * of the negative value, corrupting both the result and the overflow
 * flag. Found via test/third_party's test_vlc: compat/test/ckd.c's
 * `ckd_mul(&res, LLONG_MAX, -1)` (LLONG_MAX is `long long`, `-1` is
 * `int`).
 *
 * Separately again: GCC's `__builtin_add_overflow_p`/`__builtin_sub_
 * overflow_p` (the predicate-only siblings of `__builtin_mul_overflow_p`)
 * were entirely unrecognized: neither name was registered as a gperf
 * keyword, nor a `bi_*` interned pointer, nor a codegen dispatch case,
 * so a call like `__builtin_add_overflow_p(a, b, (int)0)` fell through
 * to an ordinary implicit-declaration external call -- producing a
 * valid .o that failed at *link* time with "undefined reference to
 * `__builtin_add_overflow_p'". Confirmed genuine real GCC builtins via
 * a direct `gcc -c` check; blocks test/third_party's test_bison
 * (gnulib's lib/canonicalize.c: `INT_ADD_OVERFLOW` macro from
 * intprops.h). Fixed by adding the two names alongside the existing
 * `__builtin_mul_overflow_p` in keywords.gperf/cg_builtins.c, reusing
 * the exact same add/sub-overflow codegen already used by the two-arg
 * overflow-detect + store form, just without the store to the (unused,
 * type-only) third argument.
 */

#include <limits.h>

int main(void)
{
    /* add: int/long/long long, signed/unsigned */
    { int a = 2000000000, b = 2000000000, r;
      if (!__builtin_sadd_overflow(a, b, &r)) return 1; }
    { int a = 1, b = 2, r;
      if (__builtin_sadd_overflow(a, b, &r) || r != 3) return 2; }
    { long a = LONG_MAX, b = 1, r;
      if (!__builtin_saddl_overflow(a, b, &r)) return 3; }
    { long long a = 0x7fffffffffffffffLL, b = 1, r;
      if (!__builtin_saddll_overflow(a, b, &r)) return 4; }
    { unsigned a = 0xffffffffU, b = 2, r;
      if (!__builtin_uadd_overflow(a, b, &r)) return 5; }
    { unsigned long a = -1UL, b = 2, r;
      if (!__builtin_uaddl_overflow(a, b, &r)) return 6; }
    { unsigned long long a = -1ULL, b = 2, r;
      if (!__builtin_uaddll_overflow(a, b, &r)) return 7; }

    /* sub */
    { int a = -2000000000, b = 2000000000, r;
      if (!__builtin_ssub_overflow(a, b, &r)) return 8; }
    { int a = 1, b = 2, r;
      if (__builtin_ssub_overflow(a, b, &r) || r != -1) return 9; }
    { long a = LONG_MIN, b = 1, r;
      if (!__builtin_ssubl_overflow(a, b, &r)) return 10; }
    { long long a = -0x7fffffffffffffffLL - 1, b = 1, r;
      if (!__builtin_ssubll_overflow(a, b, &r)) return 11; }
    { unsigned a = 0, b = 1, r;
      if (!__builtin_usub_overflow(a, b, &r)) return 12; }
    { unsigned long a = 0, b = 1, r;
      if (!__builtin_usubl_overflow(a, b, &r)) return 13; }
    { unsigned long long a = 0, b = 1, r;
      if (!__builtin_usubll_overflow(a, b, &r)) return 14; }

    /* mul */
    { int a = 100000, b = 100000, r;
      if (!__builtin_smul_overflow(a, b, &r)) return 15; }
    { unsigned a = 5, b = 3, r;
      if (__builtin_smul_overflow(a, b, &r) || r != 15) return 16; }
    { long a = LONG_MAX, b = 2, r;
      if (!__builtin_smull_overflow(a, b, &r)) return 17; }
    { long long a = 0x7fffffffffffffffLL, b = 2, r;
      if (!__builtin_smulll_overflow(a, b, &r)) return 18; }
    { unsigned a = 0xffffffffU, b = 2, r;
      if (!__builtin_umul_overflow(a, b, &r)) return 19; }
    { unsigned long a = -1UL, b = 2, r;
      if (!__builtin_umull_overflow(a, b, &r)) return 20; }
    { unsigned long long a = -1ULL, b = 2, r;
      if (!__builtin_umulll_overflow(a, b, &r)) return 21; }

    /* type-generic form still works (pre-existing) */
    { int a = 2000000000, b = 2000000000, r;
      if (!__builtin_add_overflow(a, b, &r)) return 22; }

    /* type-generic form, destination NARROWER than int */
    { short a = 32767, b = 2, r;
      if (!__builtin_mul_overflow(a, b, &r)) return 23; }
    { short a = 100, b = 5, r;
      if (__builtin_mul_overflow(a, b, &r) || r != 500) return 24; }
    { short a = 32767, b = 1, r;
      if (!__builtin_add_overflow(a, b, &r)) return 25; }
    { short a = 1000, b = 234, r;
      if (__builtin_add_overflow(a, b, &r) || r != 1234) return 26; }
    { short a = -32768, b = 1, r;
      if (!__builtin_sub_overflow(a, b, &r)) return 27; }
    { short a = 1000, b = 1, r;
      if (__builtin_sub_overflow(a, b, &r) || r != 999) return 28; }
    { unsigned short a = 65535, b = 1, r;
      if (!__builtin_add_overflow(a, b, &r)) return 29; }
    { unsigned short a = 0, b = 1, r;
      if (!__builtin_sub_overflow(a, b, &r)) return 30; }
    { signed char a = 100, b = 100, r;
      if (!__builtin_add_overflow(a, b, &r)) return 31; }
    { signed char a = 50, b = 50, r;
      if (__builtin_add_overflow(a, b, &r) || r != 100) return 32; }
    { signed char a = 10, b = 20, r;
      if (!__builtin_mul_overflow(a, b, &r)) return 33; }
    { unsigned char a = 255, b = 1, r;
      if (!__builtin_add_overflow(a, b, &r)) return 34; }

    /* No adjacent-memory corruption: the store must be exactly
     * sizeof(narrow field) wide, never touching a following field. */
    { struct { short r; int sentinel; } s;
      s.sentinel = 0x11223344;
      short a = 100, b = 5;
      __builtin_mul_overflow(a, b, &s.r);
      if (s.r != 500 || s.sentinel != 0x11223344) return 35; }
    { struct { signed char r; int sentinel; } s;
      s.sentinel = 0x55667788;
      signed char a = 10, b = 5;
      __builtin_add_overflow(a, b, &s.r);
      if (s.r != 15 || s.sentinel != 0x55667788) return 36; }

    /* Mixed-width operands: wide signed lhs, narrow-int rhs literal. */
    { long long res;
      if (__builtin_mul_overflow(LLONG_MAX, -1, &res) || res != -LLONG_MAX)
          return 37; }
    { long long res;
      if (!__builtin_mul_overflow(LLONG_MIN, -1, &res) || res != LLONG_MIN)
          return 38; }
    /* Same mismatch, operands swapped (narrow lhs, wide rhs). */
    { long long res;
      if (__builtin_mul_overflow(-1, LLONG_MAX, &res) || res != -LLONG_MAX)
          return 39; }
    /* Mixed width also applies to add/sub. */
    { long long res;
      if (__builtin_add_overflow(LLONG_MAX, -1, &res) || res != LLONG_MAX - 1)
          return 40; }
    { long long res;
      if (__builtin_sub_overflow(LLONG_MIN, -1, &res) || res != LLONG_MIN + 1)
          return 41; }
    /* Unsigned narrow rhs mixed with a wide signed lhs. */
    { long long res;
      unsigned rhs = 2;
      if (!__builtin_mul_overflow(LLONG_MAX, rhs, &res))
          return 42; }

    /* add_overflow_p/sub_overflow_p: predicate only, third arg's type
     * picks the width, its value is never written. */
    if (__builtin_add_overflow_p(1, 2, (int)0) != 0) return 43;
    if (__builtin_add_overflow_p(INT_MAX, 1, (int)0) != 1) return 44;
    if (__builtin_add_overflow_p(-1, 1, (int)0) != 0) return 45;
    if (__builtin_add_overflow_p(1L, 2L, (long)0) != 0) return 46;
    if (__builtin_add_overflow_p(LONG_MAX, 1L, (long)0) != 1) return 47;
    if (__builtin_add_overflow_p(0xffffffffU, 1U, (unsigned)0) != 1) return 48;
    if (__builtin_sub_overflow_p(2, 1, (int)0) != 0) return 49;
    if (__builtin_sub_overflow_p(INT_MIN, 1, (int)0) != 1) return 50;
    if (__builtin_sub_overflow_p(0U, 1U, (unsigned)0) != 1) return 51;
    if (__builtin_sub_overflow_p(LONG_MIN, 1L, (long)0) != 1) return 52;
    /* mul_overflow_p: pre-existing, verify it's still wired correctly
     * alongside the two new siblings. */
    if (__builtin_mul_overflow_p(100000, 100000, (int)0) != 1) return 53;
    if (__builtin_mul_overflow_p(3, 5, (int)0) != 0) return 54;

    return 0;
}
