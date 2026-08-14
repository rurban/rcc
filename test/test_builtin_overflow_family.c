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

    return 0;
}
