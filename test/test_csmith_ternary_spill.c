/* csmith 2278747 (reduced_optmismatch_O2_10.c, trimmed of dead functions):
 * rcc -O0 vs -O2/-O3 mismatch. In func_8, `(*l_2136) = safe_div_func_int32_t_s_s(p_10, p_10)`
 * -O2's inliner substitutes the ternary body (si2==0||...?si1:si1/si2)
 * directly at the call site; under the surrounding giant expression's
 * register pressure, ND_COND's shared result register `r` (still live
 * from evaluating the earlier "then" branch) got picked as alloc_reg()'s
 * spill victim while generating the "else" branch's own division
 * operand, and the div's real result -- landed in that same physical
 * register -- was then wrongly overwritten by free_reg()'s restore of
 * the stale spilled (pre-division) value. Fixed by cond_finish_branch()
 * in codegen.c: discard the spill instead of restoring when the branch's
 * own result register aliases the ternary's shared result register.
 * Manual minimization lost the exact register-pressure trigger (the
 * bug depends on -O2's whole-function register allocation), so this
 * keeps the full reduced csmith source with only the crc printout
 * replaced by an assertion against the gcc -O2 reference checksum.
 */
# 0 "rcc_optlevels_work/2278747-10.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3
# 0 "<command-line>" 2
# 1 "rcc_optlevels_work/2278747-10.c"
# 10 "rcc_optlevels_work/2278747-10.c"
# 1 "/usr/local/include/csmith.h" 1 3
# 40 "/usr/local/include/csmith.h" 3
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/float.h" 1 3
# 41 "/usr/local/include/csmith.h" 2 3

#include <stdlib.h>
#include <math.h>

# 42 "/usr/local/include/csmith.h" 2 3

#include <string.h>

# 51 "/usr/local/include/random_inc.h" 2 3

#include <stdint.h>

# 55 "/usr/local/include/random_inc.h" 2 3

#include <assert.h>


# 59 "/usr/local/include/random_inc.h" 2 3
# 88 "/usr/local/include/random_inc.h" 3
# 1 "/usr/local/include/platform_generic.h" 1 3
# 39 "/usr/local/include/platform_generic.h" 3

#include <stdio.h>

# 40 "/usr/local/include/platform_generic.h" 2 3


static void platform_main_begin(void) { }

static void platform_main_end(uint32_t crc, int flag) {
  // too lazy to catch the platform-isms
#ifdef _WIN32
  uint32_t expected = 0x96022241;
#else
  uint32_t expected = 0xED0CDF0E;
#endif
  if (crc != expected) {
      printf("FAIL: got checksum = %X, expected %X\n", crc, expected);
    exit(1);
  }


  printf("checksum = %X\n", crc);
# 114 "/usr/local/include/platform_generic.h" 3
}
# 89 "/usr/local/include/random_inc.h" 2 3
# 99 "/usr/local/include/random_inc.h" 3
# 1 "/usr/local/include/safe_math.h" 1 3
# 13 "/usr/local/include/safe_math.h" 3
static int8_t
(safe_unary_minus_func_int8_t_s)(int8_t si )
{
 
  return






    -si;
}

static int8_t
(safe_add_func_int8_t_s_s)(int8_t si1, int8_t si2 )
{
 
  return






    (si1 + si2);
}

static int8_t
(safe_sub_func_int8_t_s_s)(int8_t si1, int8_t si2 )
{
 
  return






    (si1 - si2);
}

static int8_t
(safe_mul_func_int8_t_s_s)(int8_t si1, int8_t si2 )
{
 
  return






    si1 * si2;
}

static int8_t
(safe_mod_func_int8_t_s_s)(int8_t si1, int8_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-128)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 % si2);
}

static int8_t
(safe_div_func_int8_t_s_s)(int8_t si1, int8_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-128)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 / si2);
}

static int8_t
(safe_lshift_func_int8_t_s_s)(int8_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32) || (left > ((127) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static int8_t
(safe_lshift_func_int8_t_s_u)(int8_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32) || (left > ((127) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static int8_t
(safe_rshift_func_int8_t_s_s)(int8_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32))?
    ((left)) :

    (left >> ((int)right));
}

static int8_t
(safe_rshift_func_int8_t_s_u)(int8_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32)) ?
    ((left)) :

    (left >> ((unsigned int)right));
}



static int16_t
(safe_unary_minus_func_int16_t_s)(int16_t si )
{
 
  return






    -si;
}

static int16_t
(safe_add_func_int16_t_s_s)(int16_t si1, int16_t si2 )
{
 
  return






    (si1 + si2);
}

static int16_t
(safe_sub_func_int16_t_s_s)(int16_t si1, int16_t si2 )
{
 
  return






    (si1 - si2);
}

static int16_t
(safe_mul_func_int16_t_s_s)(int16_t si1, int16_t si2 )
{
 
  return






    si1 * si2;
}

static int16_t
(safe_mod_func_int16_t_s_s)(int16_t si1, int16_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-32767-1)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 % si2);
}

static int16_t
(safe_div_func_int16_t_s_s)(int16_t si1, int16_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-32767-1)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 / si2);
}

static int16_t
(safe_lshift_func_int16_t_s_s)(int16_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32) || (left > ((32767) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static int16_t
(safe_lshift_func_int16_t_s_u)(int16_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32) || (left > ((32767) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static int16_t
(safe_rshift_func_int16_t_s_s)(int16_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32))?
    ((left)) :

    (left >> ((int)right));
}

static int16_t
(safe_rshift_func_int16_t_s_u)(int16_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32)) ?
    ((left)) :

    (left >> ((unsigned int)right));
}



static int32_t
(safe_unary_minus_func_int32_t_s)(int32_t si )
{
 
  return


    (si==(-2147483647-1)) ?
    ((si)) :


    -si;
}

static int32_t
(safe_add_func_int32_t_s_s)(int32_t si1, int32_t si2 )
{
 
  return


    (((si1>0) && (si2>0) && (si1 > ((2147483647)-si2))) || ((si1<0) && (si2<0) && (si1 < ((-2147483647-1)-si2)))) ?
    ((si1)) :


    (si1 + si2);
}

static int32_t
(safe_sub_func_int32_t_s_s)(int32_t si1, int32_t si2 )
{
 
  return


    (((si1^si2) & (((si1 ^ ((si1^si2) & (~(2147483647))))-si2)^si2)) < 0) ?
    ((si1)) :


    (si1 - si2);
}

static int32_t
(safe_mul_func_int32_t_s_s)(int32_t si1, int32_t si2 )
{
 
  return


    (((si1 > 0) && (si2 > 0) && (si1 > ((2147483647) / si2))) || ((si1 > 0) && (si2 <= 0) && (si2 < ((-2147483647-1) / si1))) || ((si1 <= 0) && (si2 > 0) && (si1 < ((-2147483647-1) / si2))) || ((si1 <= 0) && (si2 <= 0) && (si1 != 0) && (si2 < ((2147483647) / si1)))) ?
    ((si1)) :


    si1 * si2;
}

static int32_t
(safe_mod_func_int32_t_s_s)(int32_t si1, int32_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-2147483647-1)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 % si2);
}

static int32_t
(safe_div_func_int32_t_s_s)(int32_t si1, int32_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-2147483647-1)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 / si2);
}

static int32_t
(safe_lshift_func_int32_t_s_s)(int32_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32) || (left > ((2147483647) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static int32_t
(safe_lshift_func_int32_t_s_u)(int32_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32) || (left > ((2147483647) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static int32_t
(safe_rshift_func_int32_t_s_s)(int32_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32))?
    ((left)) :

    (left >> ((int)right));
}

static int32_t
(safe_rshift_func_int32_t_s_u)(int32_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32)) ?
    ((left)) :

    (left >> ((unsigned int)right));
}




static int64_t
(safe_unary_minus_func_int64_t_s)(int64_t si )
{
 
  return


    (si==(-9223372036854775807L -1)) ?
    ((si)) :


    -si;
}

static int64_t
(safe_add_func_int64_t_s_s)(int64_t si1, int64_t si2 )
{
 
  return


    (((si1>0) && (si2>0) && (si1 > ((9223372036854775807L)-si2))) || ((si1<0) && (si2<0) && (si1 < ((-9223372036854775807L -1)-si2)))) ?
    ((si1)) :


    (si1 + si2);
}

static int64_t
(safe_sub_func_int64_t_s_s)(int64_t si1, int64_t si2 )
{
 
  return


    (((si1^si2) & (((si1 ^ ((si1^si2) & (~(9223372036854775807L))))-si2)^si2)) < 0) ?
    ((si1)) :


    (si1 - si2);
}

static int64_t
(safe_mul_func_int64_t_s_s)(int64_t si1, int64_t si2 )
{
 
  return


    (((si1 > 0) && (si2 > 0) && (si1 > ((9223372036854775807L) / si2))) || ((si1 > 0) && (si2 <= 0) && (si2 < ((-9223372036854775807L -1) / si1))) || ((si1 <= 0) && (si2 > 0) && (si1 < ((-9223372036854775807L -1) / si2))) || ((si1 <= 0) && (si2 <= 0) && (si1 != 0) && (si2 < ((9223372036854775807L) / si1)))) ?
    ((si1)) :


    si1 * si2;
}

static int64_t
(safe_mod_func_int64_t_s_s)(int64_t si1, int64_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-9223372036854775807L -1)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 % si2);
}

static int64_t
(safe_div_func_int64_t_s_s)(int64_t si1, int64_t si2 )
{
 
  return

    ((si2 == 0) || ((si1 == (-9223372036854775807L -1)) && (si2 == (-1)))) ?
    ((si1)) :

    (si1 / si2);
}

static int64_t
(safe_lshift_func_int64_t_s_s)(int64_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32) || (left > ((9223372036854775807L) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static int64_t
(safe_lshift_func_int64_t_s_u)(int64_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32) || (left > ((9223372036854775807L) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static int64_t
(safe_rshift_func_int64_t_s_s)(int64_t left, int right )
{
 
  return

    ((left < 0) || (((int)right) < 0) || (((int)right) >= 32))?
    ((left)) :

    (left >> ((int)right));
}

static int64_t
(safe_rshift_func_int64_t_s_u)(int64_t left, unsigned int right )
{
 
  return

    ((left < 0) || (((unsigned int)right) >= 32)) ?
    ((left)) :

    (left >> ((unsigned int)right));
}







static uint8_t
(safe_unary_minus_func_uint8_t_u)(uint8_t ui )
{
 
  return -ui;
}

static uint8_t
(safe_add_func_uint8_t_u_u)(uint8_t ui1, uint8_t ui2 )
{
 
  return ui1 + ui2;
}

static uint8_t
(safe_sub_func_uint8_t_u_u)(uint8_t ui1, uint8_t ui2 )
{
 
  return ui1 - ui2;
}

static uint8_t
(safe_mul_func_uint8_t_u_u)(uint8_t ui1, uint8_t ui2 )
{
 
  return ((unsigned int)ui1) * ((unsigned int)ui2);
}

static uint8_t
(safe_mod_func_uint8_t_u_u)(uint8_t ui1, uint8_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 % ui2);
}

static uint8_t
(safe_div_func_uint8_t_u_u)(uint8_t ui1, uint8_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 / ui2);
}

static uint8_t
(safe_lshift_func_uint8_t_u_s)(uint8_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32) || (left > ((255) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static uint8_t
(safe_lshift_func_uint8_t_u_u)(uint8_t left, unsigned int right )
{
 
  return

    ((((unsigned int)right) >= 32) || (left > ((255) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static uint8_t
(safe_rshift_func_uint8_t_u_s)(uint8_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32)) ?
    ((left)) :

    (left >> ((int)right));
}

static uint8_t
(safe_rshift_func_uint8_t_u_u)(uint8_t left, unsigned int right )
{
 
  return

    (((unsigned int)right) >= 32) ?
    ((left)) :

    (left >> ((unsigned int)right));
}



static uint16_t
(safe_unary_minus_func_uint16_t_u)(uint16_t ui )
{
 
  return -ui;
}

static uint16_t
(safe_add_func_uint16_t_u_u)(uint16_t ui1, uint16_t ui2 )
{
 
  return ui1 + ui2;
}

static uint16_t
(safe_sub_func_uint16_t_u_u)(uint16_t ui1, uint16_t ui2 )
{
 
  return ui1 - ui2;
}

static uint16_t
(safe_mul_func_uint16_t_u_u)(uint16_t ui1, uint16_t ui2 )
{
 
  return ((unsigned int)ui1) * ((unsigned int)ui2);
}

static uint16_t
(safe_mod_func_uint16_t_u_u)(uint16_t ui1, uint16_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 % ui2);
}

static uint16_t
(safe_div_func_uint16_t_u_u)(uint16_t ui1, uint16_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 / ui2);
}

static uint16_t
(safe_lshift_func_uint16_t_u_s)(uint16_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32) || (left > ((65535) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static uint16_t
(safe_lshift_func_uint16_t_u_u)(uint16_t left, unsigned int right )
{
 
  return

    ((((unsigned int)right) >= 32) || (left > ((65535) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static uint16_t
(safe_rshift_func_uint16_t_u_s)(uint16_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32)) ?
    ((left)) :

    (left >> ((int)right));
}

static uint16_t
(safe_rshift_func_uint16_t_u_u)(uint16_t left, unsigned int right )
{
 
  return

    (((unsigned int)right) >= 32) ?
    ((left)) :

    (left >> ((unsigned int)right));
}



static uint32_t
(safe_unary_minus_func_uint32_t_u)(uint32_t ui )
{
 
  return -ui;
}

static uint32_t
(safe_add_func_uint32_t_u_u)(uint32_t ui1, uint32_t ui2 )
{
 
  return ui1 + ui2;
}

static uint32_t
(safe_sub_func_uint32_t_u_u)(uint32_t ui1, uint32_t ui2 )
{
 
  return ui1 - ui2;
}

static uint32_t
(safe_mul_func_uint32_t_u_u)(uint32_t ui1, uint32_t ui2 )
{
 
  return ((unsigned int)ui1) * ((unsigned int)ui2);
}

static uint32_t
(safe_mod_func_uint32_t_u_u)(uint32_t ui1, uint32_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 % ui2);
}

static uint32_t
(safe_div_func_uint32_t_u_u)(uint32_t ui1, uint32_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 / ui2);
}

static uint32_t
(safe_lshift_func_uint32_t_u_s)(uint32_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32) || (left > ((4294967295U) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static uint32_t
(safe_lshift_func_uint32_t_u_u)(uint32_t left, unsigned int right )
{
 
  return

    ((((unsigned int)right) >= 32) || (left > ((4294967295U) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static uint32_t
(safe_rshift_func_uint32_t_u_s)(uint32_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32)) ?
    ((left)) :

    (left >> ((int)right));
}

static uint32_t
(safe_rshift_func_uint32_t_u_u)(uint32_t left, unsigned int right )
{
 
  return

    (((unsigned int)right) >= 32) ?
    ((left)) :

    (left >> ((unsigned int)right));
}




static uint64_t
(safe_unary_minus_func_uint64_t_u)(uint64_t ui )
{
 
  return -ui;
}

static uint64_t
(safe_add_func_uint64_t_u_u)(uint64_t ui1, uint64_t ui2 )
{
 
  return ui1 + ui2;
}

static uint64_t
(safe_sub_func_uint64_t_u_u)(uint64_t ui1, uint64_t ui2 )
{
 
  return ui1 - ui2;
}

static uint64_t
(safe_mul_func_uint64_t_u_u)(uint64_t ui1, uint64_t ui2 )
{
 
  return ((unsigned long long)ui1) * ((unsigned long long)ui2);
}

static uint64_t
(safe_mod_func_uint64_t_u_u)(uint64_t ui1, uint64_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 % ui2);
}

static uint64_t
(safe_div_func_uint64_t_u_u)(uint64_t ui1, uint64_t ui2 )
{
 
  return

    (ui2 == 0) ?
    ((ui1)) :

    (ui1 / ui2);
}

static uint64_t
(safe_lshift_func_uint64_t_u_s)(uint64_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32) || (left > ((18446744073709551615UL) >> ((int)right)))) ?
    ((left)) :

    (left << ((int)right));
}

static uint64_t
(safe_lshift_func_uint64_t_u_u)(uint64_t left, unsigned int right )
{
 
  return

    ((((unsigned int)right) >= 32) || (left > ((18446744073709551615UL) >> ((unsigned int)right)))) ?
    ((left)) :

    (left << ((unsigned int)right));
}

static uint64_t
(safe_rshift_func_uint64_t_u_s)(uint64_t left, int right )
{
 
  return

    ((((int)right) < 0) || (((int)right) >= 32)) ?
    ((left)) :

    (left >> ((int)right));
}

static uint64_t
(safe_rshift_func_uint64_t_u_u)(uint64_t left, unsigned int right )
{
 
  return

    (((unsigned int)right) >= 32) ?
    ((left)) :

    (left >> ((unsigned int)right));
}







float fabsf(float);
double fabs(double);


static float
(safe_add_func_float_f_f)(float sf1, float sf2 )
{
 
  return

    (fabsf((0.5f * sf1) + (0.5f * sf2)) > (0.5f * 3.40282346638528859811704183484516925e+38F)) ?
    (sf1) :

    (sf1 + sf2);
}

static float
(safe_sub_func_float_f_f)(float sf1, float sf2 )
{
 
  return

    (fabsf((0.5f * sf1) - (0.5f * sf2)) > (0.5f * 3.40282346638528859811704183484516925e+38F)) ?
    (sf1) :

    (sf1 - sf2);
}

static float
(safe_mul_func_float_f_f)(float sf1, float sf2 )
{
 
  return


    (fabsf((0x1.0p-100f * sf1) * (0x1.0p-28f * sf2)) > (0x1.0p-100f * (0x1.0p-28f * 3.40282346638528859811704183484516925e+38F))) ?



    (sf1) :

    (sf1 * sf2);
}

static float
(safe_div_func_float_f_f)(float sf1, float sf2 )
{
 
  return


    ((fabsf(sf2) < 1.0f) && (((sf2 == 0.0f) || (fabsf((0x1.0p-49f * sf1) / (0x1.0p100f * sf2))) > (0x1.0p-100f * (0x1.0p-49f * 3.40282346638528859811704183484516925e+38F))))) ?



    (sf1) :

    (sf1 / sf2);
}




static double
(safe_add_func_double_f_f)(double sf1, double sf2 )
{
 
  return

    (fabs((0.5 * sf1) + (0.5 * sf2)) > (0.5 * ((double)1.79769313486231570814527423731704357e+308L))) ?
    (sf1) :

    (sf1 + sf2);
}

static double
(safe_sub_func_double_f_f)(double sf1, double sf2 )
{
 
  return

    (fabs((0.5 * sf1) - (0.5 * sf2)) > (0.5 * ((double)1.79769313486231570814527423731704357e+308L))) ?
    (sf1) :

    (sf1 - sf2);
}

static double
(safe_mul_func_double_f_f)(double sf1, double sf2 )
{
 
  return


    (fabs((0x1.0p-100 * sf1) * (0x1.0p-924 * sf2)) > (0x1.0p-100 * (0x1.0p-924 * ((double)1.79769313486231570814527423731704357e+308L)))) ?



    (sf1) :

    (sf1 * sf2);
}

static double
(safe_div_func_double_f_f)(double sf1, double sf2 )
{
 
  return


    ((fabs(sf2) < 1.0) && (((sf2 == 0.0) || (fabs((0x1.0p-974 * sf1) / (0x1.0p100 * sf2))) > (0x1.0p-100 * (0x1.0p-974 * ((double)1.79769313486231570814527423731704357e+308L)))))) ?



    (sf1) :

    (sf1 / sf2);
}
# 1195 "/usr/local/include/safe_math.h" 3
static int32_t
(safe_convert_func_float_to_int32_t)(float sf1 )
{
 
  return

    ((sf1 <= (-2147483647-1)) || (sf1 >= (2147483647))) ?
    ((2147483647)) :

    ((int32_t)(sf1));
}
# 100 "/usr/local/include/random_inc.h" 2 3
# 46 "/usr/local/include/csmith.h" 2 3

static uint32_t crc32_tab[256];
static uint32_t crc32_context = 0xFFFFFFFF;

static void crc32_gentab(void) {
  uint32_t crc;
  const uint32_t poly = 0xEDB88320;
  int i, j;

  for (i = 0; i < 256; i++) {
    crc = i;
    for (j = 8; j > 0; j--) {
      if (crc & 1) {
        crc = (crc >> 1) ^ poly;
      } else {
        crc >>= 1;
      }
    }
    crc32_tab[i] = crc;
  }
}

static void crc32_byte(uint8_t b) {
  crc32_context = ((crc32_context >> 8) & 0x00FFFFFF) ^
                  crc32_tab[(crc32_context ^ b) & 0xFF];
}
# 89 "/usr/local/include/csmith.h" 3
static void crc32_8bytes(uint64_t val) {
  crc32_byte((val >> 0) & 0xff);
  crc32_byte((val >> 8) & 0xff);
  crc32_byte((val >> 16) & 0xff);
  crc32_byte((val >> 24) & 0xff);
  crc32_byte((val >> 32) & 0xff);
  crc32_byte((val >> 40) & 0xff);
  crc32_byte((val >> 48) & 0xff);
  crc32_byte((val >> 56) & 0xff);
}

static void transparent_crc(uint64_t val, char *vname, int flag) {
  crc32_8bytes(val);
  if (flag) {
    printf("...checksum after hashing %s : %lX\n", vname,
           crc32_context ^ 0xFFFFFFFF);
  }
}



static void transparent_crc_bytes(char *ptr, int nbytes, char *vname,
                                  int flag) {
  int i;
  for (i = 0; i < nbytes; i++) {
    crc32_byte(ptr[i]);
  }
  if (flag) {
    printf("...checksum after hashing %s : %lX\n", vname,
           crc32_context ^ 0xFFFFFFFF);
  }
}
# 11 "rcc_optlevels_work/2278747-10.c" 2



# 13 "rcc_optlevels_work/2278747-10.c"
static long __undefined;



static int32_t g_2 = 0x40D7F825L;
static int32_t g_5 = 0xE9D05BA7L;
static int16_t g_15[10] = {1L,0x039DL,1L,1L,0x039DL,1L,1L,0x039DL,1L,1L};
static uint64_t g_28[9][9] = {{18446744073709551606UL,0x05ABA9A984365721LL,18446744073709551610UL,18446744073709551610UL,0x05ABA9A984365721LL,18446744073709551606UL,0x05ABA9A984365721LL,18446744073709551610UL,18446744073709551610UL},{0UL,0UL,0xCB4448110CBBBC25LL,4UL,0xCB4448110CBBBC25LL,0UL,0UL,0xCB4448110CBBBC25LL,4UL},{0xE7F13BCF59F89F2ELL,0x05ABA9A984365721LL,0xE7F13BCF59F89F2ELL,18446744073709551606UL,18446744073709551606UL,0xE7F13BCF59F89F2ELL,0x05ABA9A984365721LL,0xE7F13BCF59F89F2ELL,18446744073709551606UL},{3UL,0xCB4448110CBBBC25LL,0xCB4448110CBBBC25LL,3UL,0x4E2B79DC86695CD1LL,3UL,0xCB4448110CBBBC25LL,0xCB4448110CBBBC25LL,3UL},{18446744073709551606UL,0xE7F13BCF59F89F2ELL,0x05ABA9A984365721LL,0xE7F13BCF59F89F2ELL,18446744073709551606UL,18446744073709551606UL,0xE7F13BCF59F89F2ELL,0x05ABA9A984365721LL,0xE7F13BCF59F89F2ELL},{3UL,0UL,0x4E2B79DC86695CD1LL,0x4E2B79DC86695CD1LL,0UL,3UL,0UL,0x4E2B79DC86695CD1LL,0x4E2B79DC86695CD1LL},{18446744073709551606UL,18446744073709551606UL,0xE7F13BCF59F89F2ELL,0x05ABA9A984365721LL,0xE7F13BCF59F89F2ELL,18446744073709551606UL,18446744073709551606UL,0xE7F13BCF59F89F2ELL,0x05ABA9A984365721LL},{4UL,0UL,4UL,3UL,3UL,4UL,0UL,4UL,3UL},{18446744073709551610UL,0xE7F13BCF59F89F2ELL,0xE7F13BCF59F89F2ELL,18446744073709551610UL,0x2A6F9A4D91F06E85LL,18446744073709551610UL,0xE7F13BCF59F89F2ELL,0xE7F13BCF59F89F2ELL,18446744073709551610UL}};
static int32_t g_62 = (-2L);
static int32_t g_65 = 5L;
static int32_t g_69 = 0L;
static int32_t *g_75[4][2][4] = {{{&g_69,&g_2,&g_69,&g_2},{&g_2,&g_2,&g_69,&g_69}},{{&g_69,&g_69,&g_2,&g_69},{(void*)0,&g_2,(void*)0,&g_2}},{{(void*)0,&g_2,&g_2,(void*)0},{&g_69,&g_2,&g_69,&g_2}},{{&g_2,&g_2,&g_69,&g_69},{&g_69,&g_69,&g_2,&g_69}}};
static int32_t ** volatile g_74 = &g_75[2][0][2];
static int8_t g_117 = 0xFEL;
static uint8_t g_120 = 255UL;
static int8_t g_124 = 1L;
static uint16_t g_171 = 1UL;
static uint16_t g_187 = 0x514BL;
static volatile int32_t g_197 = 0xC1661776L;
static volatile int32_t *g_196 = &g_197;
static volatile int32_t * volatile *g_195[10] = {&g_196,&g_196,&g_196,&g_196,&g_196,&g_196,&g_196,&g_196,&g_196,&g_196};
static volatile int32_t * volatile ** volatile g_194 = &g_195[2];
static volatile int32_t * volatile ** volatile *g_193 = &g_194;
static uint64_t *g_218 = &g_28[5][1];
static uint64_t **g_217[4] = {&g_218,&g_218,&g_218,&g_218};
static int32_t ***g_225 = (void*)0;
static int32_t **g_227 = &g_75[3][0][1];
static int32_t ***g_226 = &g_227;
static volatile uint32_t g_245 = 0x7E161316L;
static uint32_t g_285 = 4294967292UL;
static volatile int16_t g_313 = 0x20F8L;
static volatile int16_t *g_312 = &g_313;
static volatile int16_t **g_311 = &g_312;
static int64_t g_330 = 0x7B2CC81F5611FC3BLL;
static int64_t *g_370 = &g_330;
static const int32_t **g_382 = (void*)0;
static const int32_t ***g_381 = &g_382;
static int16_t g_490 = 0xFE7EL;
static int16_t *g_502 = &g_15[0];
static int16_t **g_501 = &g_502;
static const uint64_t g_524 = 18446744073709551612UL;
static const uint64_t g_526 = 18446744073709551609UL;
static volatile uint32_t g_534 = 4294967295UL;
static volatile uint32_t *g_533[8][1][3] = {{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}},{{&g_534,&g_534,&g_534}}};
static volatile uint32_t * volatile * const volatile g_532 = &g_533[0][0][2];
static volatile int32_t * volatile g_550 = &g_197;
static volatile uint8_t g_554 = 5UL;
static int32_t g_600 = 0xB51E6FFAL;
static uint64_t g_645 = 3UL;
static uint64_t **g_667 = &g_218;
static uint32_t ***g_683 = (void*)0;
static uint32_t ***g_685[4] = {(void*)0,(void*)0,(void*)0,(void*)0};
static int64_t g_715 = 0L;
static volatile int8_t g_762 = 0xDBL;
static uint16_t g_849 = 0xDCECL;
static int16_t g_894[5] = {0xC289L,0xC289L,0xC289L,0xC289L,0xC289L};
static volatile uint32_t g_919 = 2UL;
static int8_t g_922 = 0x97L;
static int32_t ** const *g_927 = &g_227;
static int32_t ** const **g_926 = &g_927;
static int32_t ** const ***g_925 = &g_926;
static const int32_t *g_966 = &g_69;
static const int32_t ** volatile g_965[6] = {&g_966,&g_966,&g_966,&g_966,&g_966,&g_966};
static volatile int64_t g_1012[8] = {0x148E9A67D171DB31LL,0x7C555C65136A77EELL,0x7C555C65136A77EELL,0x148E9A67D171DB31LL,0x7C555C65136A77EELL,0x7C555C65136A77EELL,0x148E9A67D171DB31LL,0x7C555C65136A77EELL};
static int32_t g_1088 = 0x69428C92L;
static int32_t g_1090 = 1L;
static uint32_t g_1125 = 1UL;
static int8_t *g_1181 = &g_922;
static int8_t **g_1180[7] = {&g_1181,&g_1181,&g_1181,&g_1181,&g_1181,&g_1181,&g_1181};
static uint8_t g_1208 = 0xC2L;
static const uint64_t *g_1246 = &g_524;
static const uint64_t **g_1245[2][1][6] = {{{&g_1246,&g_1246,&g_1246,&g_1246,&g_1246,&g_1246}},{{&g_1246,&g_1246,&g_1246,&g_1246,(void*)0,&g_1246}}};
static const uint64_t ***g_1244 = &g_1245[0][0][5];
static const uint64_t *** volatile *g_1243 = &g_1244;
static uint32_t ****g_1445 = &g_685[2];
static uint32_t *****g_1444 = &g_1445;
static const int8_t g_1480 = 0xCBL;
static const int8_t *g_1479 = &g_1480;
static const int8_t **g_1478 = &g_1479;
static const int8_t **g_1481 = &g_1479;
static const int8_t **g_1482 = &g_1479;
static int32_t ** const volatile g_1518[8][9] = {{&g_75[3][1][0],&g_75[2][0][3],&g_75[2][0][2],&g_75[2][0][0],&g_75[1][1][1],&g_75[2][0][2],&g_75[2][1][0],&g_75[2][0][2],(void*)0},{(void*)0,&g_75[0][0][2],&g_75[2][0][3],&g_75[0][1][3],&g_75[0][1][3],&g_75[2][0][3],&g_75[0][0][2],(void*)0,&g_75[3][1][3]},{&g_75[0][1][0],&g_75[0][0][2],&g_75[2][0][2],&g_75[2][0][3],(void*)0,(void*)0,&g_75[2][0][2],&g_75[3][1][3],&g_75[1][0][1]},{&g_75[0][0][2],&g_75[2][0][3],&g_75[1][1][1],&g_75[2][0][3],&g_75[3][1][3],&g_75[2][0][0],&g_75[2][0][2],&g_75[2][0][2],&g_75[1][0][1]},{(void*)0,&g_75[2][1][1],&g_75[2][1][1],(void*)0,&g_75[2][1][0],&g_75[2][0][3],&g_75[3][1][0],&g_75[3][1][3],&g_75[1][1][1]},{&g_75[2][1][0],&g_75[0][1][0],(void*)0,&g_75[0][1][3],&g_75[2][1][1],&g_75[2][0][2],&g_75[0][1][2],&g_75[2][0][3],&g_75[2][0][2]},{(void*)0,&g_75[2][0][2],&g_75[0][1][3],&g_75[3][1][3],&g_75[2][1][0],&g_75[3][1][3],&g_75[0][1][3],&g_75[2][0][2],(void*)0},{&g_75[2][0][0],&g_75[2][1][0],&g_75[0][1][3],&g_75[0][0][2],&g_75[1][0][1],&g_75[2][0][2],&g_75[2][1][1],(void*)0,&g_75[3][1][3]}};
static int16_t ***g_1548 = &g_501;
static volatile int32_t * volatile g_1604 = &g_197;
static volatile int32_t g_1633 = 0x38E6F8DBL;
static uint32_t g_1690 = 1UL;
static int32_t ** volatile g_1727 = &g_75[2][0][2];
static int64_t **g_1730 = &g_370;
static int32_t ** const *g_1744 = &g_227;
static uint8_t g_1823[9] = {255UL,6UL,255UL,6UL,255UL,6UL,255UL,6UL,255UL};
static uint32_t g_1860 = 0xAA6EC582L;
static uint32_t *g_1895 = (void*)0;
static uint32_t **g_1894 = &g_1895;
static uint32_t *** const g_1893 = &g_1894;
static uint32_t *** const *g_1892 = &g_1893;
static uint32_t *** const **g_1891[9] = {&g_1892,&g_1892,&g_1892,&g_1892,&g_1892,&g_1892,&g_1892,&g_1892,&g_1892};
static int16_t g_2019[5][5] = {{(-5L),(-1L),0x7E8EL,(-1L),(-5L)},{(-8L),(-1L),0x5711L,(-5L),0x5711L},{0x5711L,0x5711L,0x7E8EL,(-5L),(-1L)},{(-1L),(-8L),(-8L),(-1L),0x5711L},{(-1L),(-5L),0x2790L,0x2790L,(-5L)}};
static uint64_t ***g_2060 = &g_217[0];
static volatile uint32_t g_2175 = 0xC893FE44L;
static const int32_t ****g_2249 = &g_381;
static uint16_t * volatile g_2255 = (void*)0;
static uint16_t * volatile *g_2254 = &g_2255;
static uint32_t g_2268 = 1UL;
static uint16_t *g_2296 = &g_849;
static uint16_t **g_2295[1][2][4] = {{{&g_2296,&g_2296,&g_2296,&g_2296},{&g_2296,&g_2296,&g_2296,&g_2296}}};
static uint16_t ***g_2294 = &g_2295[0][0][2];
static int32_t g_2446 = (-6L);
static uint32_t * const *g_2452 = (void*)0;
static uint32_t * const ** volatile g_2451 = &g_2452;



static int32_t func_1(void);
static int32_t func_8(int64_t p_9, int32_t p_10);
static int32_t func_16(int32_t p_17, uint32_t p_18, uint32_t p_19, int64_t p_20);
static uint64_t func_31(uint64_t * p_32, int32_t p_33, int16_t p_34, uint32_t p_35);
static uint64_t * func_36(uint64_t * p_37, uint32_t p_38, uint64_t * p_39);
static uint16_t func_40(uint64_t * p_41, int8_t p_42);
static uint32_t func_44(int16_t p_45, uint64_t * p_46, uint64_t * p_47);
static uint64_t * func_48(const uint64_t * p_49, uint64_t * p_50, uint16_t p_51, int16_t p_52);
static const uint64_t * func_53(uint64_t * p_54, uint64_t * p_55);
static uint16_t func_56(const uint64_t * const p_57, uint64_t * p_58, uint32_t p_59, uint8_t p_60, const uint64_t * p_61);
# 143 "rcc_optlevels_work/2278747-10.c"
static int32_t func_1(void)
{
    int32_t l_11 = 0x9951EA66L;
    int32_t l_2169 = (-1L);
    int16_t l_2209 = 0L;
    int32_t l_2265 = (-3L);
    int32_t l_2266 = 0x8DED3336L;
    uint32_t *l_2273 = &g_285;
    uint16_t ****l_2297 = (void*)0;
    uint16_t ****l_2298 = (void*)0;
    uint16_t ****l_2299[7][10] = {{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294},{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294},{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294},{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294},{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294},{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294},{&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294,&g_2294}};
    uint8_t *l_2300 = &g_120;
    int16_t **l_2325 = &g_502;
    uint32_t ***l_2333[9] = {&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894};
    int16_t l_2394 = 0x0E3BL;
    int32_t l_2395 = 0x9DED98A7L;
    const int8_t *l_2416[10][9][2] = {{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}},{{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117},{&g_124,&g_117}}};
    int32_t l_2447 = 1L;
    int8_t l_2456[7] = {9L,1L,9L,9L,1L,9L,9L};
    int i, j, k;
    for (g_2 = 21; (g_2 > (-16)); g_2 = safe_sub_func_int64_t_s_s(g_2, 5))
    {
        int16_t l_2157 = 1L;
        int32_t ** const l_2163[5] = {&g_75[2][0][2],&g_75[2][0][2],&g_75[2][0][2],&g_75[2][0][2],&g_75[2][0][2]};
        int32_t **l_2164 = &g_75[0][1][2];
        int i;
        for (g_5 = 0; (g_5 <= 0); ++g_5)
        {
            const int32_t l_13 = 0xE704F3D4L;
            int32_t ****l_2148 = (void*)0;
            int32_t l_2167 = 0xB4600257L;
            int32_t l_2174 = (-9L);
            int8_t l_2198[3];
            int i;
            for (i = 0; i < 3; i++)
                l_2198[i] = (-1L);
            if (func_8(l_11, (safe_unary_minus_func_int32_t_s(l_13))))
            {
                if (l_13)
                    break;
            }
            else
            {
                int32_t *****l_2149 = (void*)0;
                int32_t *****l_2150 = &l_2148;
                int32_t l_2168 = (-10L);
                uint32_t *l_2170 = &g_1125;
                uint16_t *l_2171[6] = {&g_849,&g_849,&g_849,&g_849,&g_849,&g_849};
                uint64_t l_2203 = 18446744073709551607UL;
                int i;
                if (((g_849 = ((safe_sub_func_int8_t_s_s(((((*l_2170) = (((*l_2150) = l_2148) != ((safe_mod_func_int8_t_s_s((safe_mod_func_uint16_t_u_u(1UL, ((l_2157 |= ((safe_sub_func_uint16_t_u_u(l_11, l_11)) , (-1L))) ^ (((!(safe_mod_func_int32_t_s_s((l_2169 = ((safe_sub_func_int8_t_s_s((**g_1478), ((l_2163[0] != (l_2164 = ((*g_226) = (*g_226)))) & (safe_sub_func_int32_t_s_s((18446744073709551614UL ^ l_2167), l_11))))) , l_2168)), l_11))) > (*g_218)) , 0x735107EFL)))), l_11)) , (*g_925)))) , (*g_370)) | (*g_370)), (**g_1478))) , g_715)) | (**g_311)))
                {
                    int32_t l_2182 = 0x44B38E53L;
                    uint32_t *l_2183 = &g_1860;
                    int64_t ***l_2188 = &g_1730;
                    uint8_t *l_2199 = (void*)0;
                    uint8_t *l_2200 = &g_120;
                    int16_t *l_2201[5] = {&g_894[1],&g_894[1],&g_894[1],&g_894[1],&g_894[1]};
                    uint32_t ****l_2202 = &g_685[2];
                    int i;
                    for (g_187 = (-5); (g_187 != 23); g_187 = safe_add_func_uint64_t_u_u(g_187, 7))
                    {
                        ++g_2175;
                        return l_11;
                    }
                    l_2174 |= ((safe_mul_func_int8_t_s_s((safe_mul_func_int32_t_s_s(l_2167, l_2182)), (l_2182 , ((((*l_2183)++) , ((safe_rshift_func_int16_t_s_u((g_2019[2][2] |= ((*g_502) = (((((*l_2188) = &g_370) != &g_370) == ((((*l_2200) = ((safe_add_func_uint16_t_u_u(((g_245 , (((safe_mul_func_int8_t_s_s(((((!(((g_187 == g_1860) == (safe_div_func_uint64_t_u_u((safe_lshift_func_int8_t_s_u(0xEAL, g_1823[5])), (*g_370)))) || g_554)) | l_11) != (*g_218)) | (***g_1548)), 0x8FL)) >= l_2169) && l_2182)) > l_2198[1]), (*g_502))) , g_1012[4])) || (**g_1481)) | (*g_966))) != l_11))), g_1125)) >= (*g_370))) && 0x638F954F99866C25LL)))) >= 0x3912L);
                    l_2202 = (*g_1444);
                    (****g_925) = &l_2169;
                }
                else
                {
                    l_2203 &= l_11;
                }
            }
            if (l_11)
                continue;
        }
    }
    for (g_62 = 3; (g_62 >= 0); g_62 -= 1)
    {
        uint32_t **l_2205[9];
        int32_t l_2206 = 0x85F900EBL;
        const uint64_t *l_2235 = &g_526;
        uint64_t l_2259 = 18446744073709551613UL;
        int32_t *l_2262 = &l_2169;
        int32_t *l_2263 = &l_2206;
        int32_t *l_2264[1];
        int32_t l_2267 = (-3L);
        int i;
        for (i = 0; i < 9; i++)
            l_2205[i] = &g_1895;
        for (i = 0; i < 1; i++)
            l_2264[i] = &l_2169;
    }
    if (((safe_add_func_uint64_t_u_u((((*l_2273) = l_2265) ^ (((***g_194) >= ((safe_mod_func_uint8_t_u_u((safe_add_func_uint8_t_u_u((safe_sub_func_int64_t_s_s(0x715C670C354DB652LL, (safe_lshift_func_int16_t_s_u(((***g_1548) |= (safe_lshift_func_uint64_t_u_u((safe_lshift_func_int8_t_s_s((safe_add_func_uint32_t_u_u((((l_2266 = (safe_mod_func_uint32_t_u_u(((l_2169 = ((safe_mul_func_uint32_t_u_u(l_2265, (safe_add_func_uint8_t_u_u(((18446744073709551615UL | 0L) , ((*l_2300) = ((g_2294 = g_2294) == (void*)0))), ((safe_sub_func_uint16_t_u_u((((0x0C77L ^ l_2266) && l_2266) & l_2209), l_11)) && l_2209))))) , 0x8928B453L)) || 0x81EDDCB0L), l_2209))) ^ l_2265) , l_2169), l_2265)), (**g_1478))), l_2209))), 4)))), l_2265)), l_2265)) < 0UL)) <= 5UL)), (*g_370))) >= (*g_2296)))
    {
        int32_t *l_2307 = &l_2265;
        int32_t l_2328 = 0L;
        uint32_t * const *l_2339 = &g_1895;
        int32_t ***l_2340 = &g_227;
        uint8_t l_2361 = 0x55L;
        uint32_t l_2362 = 0xB8439B9DL;
        int32_t l_2400[2];
        int16_t ***l_2403 = &l_2325;
        int16_t ***l_2404 = &l_2325;
        const int8_t *l_2417 = (void*)0;
        uint16_t *l_2432 = (void*)0;
        int8_t l_2433 = 8L;
        uint16_t l_2448 = 0x9C04L;
        int i;
        for (i = 0; i < 2; i++)
            l_2400[i] = 1L;
        if (((*l_2307) ^= (safe_lshift_func_int64_t_s_s(1L, (((safe_mul_func_uint64_t_u_u((*g_1246), ((void*)0 == (*g_925)))) < 0UL) <= (*g_312))))))
        {
            int16_t l_2324 = 0x44B5L;
            const uint32_t l_2331 = 0xF5E56BA8L;
            for (g_1860 = 0; (g_1860 > 12); g_1860 = safe_add_func_uint16_t_u_u(g_1860, 2))
            {
                uint32_t **l_2316[3];
                int32_t l_2321 = 0x6D149394L;
                int64_t l_2326[10][1] = {{0L},{5L},{0x2053EDE2EEAF4791LL},{5L},{0L},{0L},{5L},{0x2053EDE2EEAF4791LL},{5L},{0L}};
                uint64_t *l_2327[6] = {&g_645,&g_645,&g_645,&g_645,&g_645,&g_645};
                int i, j;
                for (i = 0; i < 3; i++)
                    l_2316[i] = (void*)0;
                (*l_2307) |= 0x8364EAC2L;
                l_2328 |= ((l_2169 = ((***g_2060) ^= (((*l_2307) <= (l_2326[3][0] = (!(((safe_rshift_func_uint16_t_u_u((((safe_unary_minus_func_int32_t_s((safe_mul_func_int32_t_s_s(((l_2316[0] == l_2316[0]) || ((((((9L ^ ((((safe_lshift_func_int32_t_s_u(((**g_532) <= (safe_unary_minus_func_int32_t_s(((**g_501) , (~(l_2321 && (*l_2307))))))), 30)) <= (safe_add_func_uint64_t_u_u((*l_2307), (**g_1730)))) | l_2324) < 255UL)) , l_2321) == (***g_1548)) && l_2209) != g_1823[3]) , 0x086B05DCL)), l_2169)))) > 0x5F20L) & l_2321), 6)) , (*g_1548)) != l_2325)))) , l_2324))) >= l_2266);
            }
            for (g_120 = 15; (g_120 > 26); g_120 = safe_add_func_int16_t_s_s(g_120, 7))
            {
                uint32_t ***l_2332[4][1];
                int i, j;
                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 1; j++)
                        l_2332[i][j] = &g_1894;
                }
                l_2169 = ((((l_2331 , l_2332[2][0]) != l_2333[4]) == 0x6D7CL) == (**g_501));
            }
            (*l_2307) = (((+(safe_mul_func_int16_t_s_s((safe_sub_func_uint8_t_u_u(((**g_1892) == l_2339), (*l_2307))), (l_2340 == (*g_926))))) , (void*)0) != &g_1088);
            for (g_65 = 0; (g_65 < 25); g_65 = safe_add_func_int8_t_s_s(g_65, 4))
            {
                int32_t l_2354 = (-1L);
                for (g_1090 = 0; (g_1090 == (-14)); g_1090--)
                {
                    uint64_t * const *l_2358 = &g_218;
                    uint64_t * const ** const l_2357 = &l_2358;
                    uint64_t * const ** const *l_2356 = &l_2357;
                    uint64_t * const ** const **l_2355 = &l_2356;
                    int32_t l_2363 = (-1L);
                    uint8_t l_2364 = 0x98L;
                    int16_t ****l_2368 = &g_1548;
                    for (g_922 = 0; (g_922 >= (-6)); --g_922)
                    {
                        int8_t l_2365 = (-2L);
                        (*g_196) ^= ((***g_2294) , (((0xE5A7L == (safe_lshift_func_int8_t_s_u(l_2169, (safe_unary_minus_func_int64_t_s(5L))))) && (((safe_mul_func_uint8_t_u_u(0xB4L, (((safe_div_func_int16_t_s_s(((((l_2354 , l_2355) != (void*)0) <= (((safe_rshift_func_int32_t_s_s((((0x9E603A07L > l_2361) && l_2362) > l_2324), 25)) >= 65534UL) >= l_2363)) != l_2364), (***g_2294))) || g_69) , g_1823[6]))) | (-1L)) || 0x4E253639L)) ^ (*g_370)));
                        if (l_2365)
                            continue;
                    }
                    for (l_2209 = 0; (l_2209 >= 0); l_2209 -= 1)
                    {
                        return l_2363;
                    }
                    (*l_2307) &= ((((safe_sub_func_uint8_t_u_u(0x6AL, (((l_2368 != &g_1548) > (safe_lshift_func_uint8_t_u_u(((-3L) <= ((safe_lshift_func_uint32_t_u_u(((safe_add_func_uint8_t_u_u((~(*g_1181)), 0xADL)) & (0x802AL ^ (((safe_add_func_int16_t_s_s((l_2354 || g_117), (*g_502))) <= 1L) ^ l_2354))), 12)) != 0x21L)), g_1633))) & g_922))) != (**g_1478)) , (**g_1730)) ^ (***g_2060));
                }
                l_2169 |= ((*g_502) == ((safe_add_func_int64_t_s_s((l_2354 == (2UL != (safe_mul_func_uint16_t_u_u((safe_mod_func_int8_t_s_s((**g_1482), g_1823[5])), (*l_2307))))), ((*l_2307) , ((*g_2296) > ((++(***g_2060)) != (safe_rshift_func_int32_t_s_s((((safe_mod_func_int32_t_s_s(((**g_1482) & (*l_2307)), (-1L))) > (*g_370)) < l_2394), 20))))))) > 1L));
                return l_2395;
            }
        }
        else
        {
            uint32_t ***l_2399[8] = {&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894,&g_1894};
            const int32_t l_2405 = 0x80263760L;
            uint16_t l_2406[8] = {0x33CFL,0x33CFL,0x33CFL,0x33CFL,0x33CFL,0x33CFL,0x33CFL,0x33CFL};
            int32_t *l_2407 = &g_600;
            const int32_t **l_2408 = &g_966;
            int i;
            (*l_2407) ^= ((0xDCL <= (((*l_2307) ^ (0xDE274B73L > (safe_mod_func_int64_t_s_s((!(((*g_1181) = ((l_2399[1] != (*g_1445)) , ((l_2266 = (((l_2400[0] | (*l_2307)) , (safe_sub_func_int8_t_s_s(((((0xD1L | ((l_2403 == l_2404) != 0UL)) != 0xCBAB417EADFE59A7LL) & l_2405) >= 2UL), (*l_2307)))) < (*g_196))) , 2L))) , 18446744073709551609UL)), (-7L))))) != l_2406[1])) , (-1L));
            (*l_2408) = &l_2405;
        }
        if ((((safe_div_func_int32_t_s_s(((void*)0 != (*g_2254)), (safe_add_func_uint8_t_u_u((g_1823[5] = (+(safe_div_func_uint8_t_u_u((((*g_1482) = l_2416[0][5][1]) == l_2417), (safe_add_func_int16_t_s_s(((**g_501) = ((*l_2307) || (safe_rshift_func_int64_t_s_s((((void*)0 != (**g_1548)) <= (++(*l_2273))), (((safe_add_func_uint64_t_u_u((((((safe_sub_func_uint8_t_u_u((safe_mul_func_int16_t_s_s((safe_rshift_func_uint64_t_u_s(((-1L) <= ((*g_502) && 0x3CB1L)), l_2169)), (**g_501))), g_171)) , (**g_2294)) == l_2432) | 65535UL) != 0xA77FB13204137121LL), l_2266)) || l_2394) == g_15[0]))))), (***g_2294))))))), 247UL)))) | g_2019[2][2]) , 0L))
        {
            (***g_193) = (***g_193);
        }
        else
        {
            int32_t *l_2434 = &g_5;
            int32_t *l_2435 = (void*)0;
            int32_t *l_2436 = &l_2395;
            int32_t *l_2437 = &l_2266;
            int32_t *l_2438 = &l_2265;
            int32_t *l_2439 = &g_65;
            int32_t *l_2440 = (void*)0;
            int32_t *l_2441 = &g_5;
            int32_t *l_2442 = (void*)0;
            int32_t *l_2443 = &l_2265;
            int32_t *l_2444 = &l_2395;
            int32_t *l_2445[6];
            int i;
            for (i = 0; i < 6; i++)
                l_2445[i] = &g_5;
            ++l_2448;
            return l_2395;
        }
        (*g_2451) = l_2339;
        (**g_226) = &l_2266;
        if (g_1480)
            goto lbl_2455;
    }
    else
    {
lbl_2455:
        for (g_645 = 0; (g_645 >= 21); g_645 = safe_add_func_uint64_t_u_u(g_645, 6))
        {
            l_2169 = 0L;
        }
        (**g_927) = (*g_1727);
        l_2456[6] = l_2265;
    }
    return l_2395;
}







static int32_t func_8(int64_t p_9, int32_t p_10)
{
    int16_t l_14[3][10][6] = {{{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L}},{{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L}},{{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x2456L,0xBE13L,0x2456L,0x7059L,0xBE13L},{0x2456L,0x7059L,0xBE13L,(-1L),(-1L),0xBE13L},{(-1L),(-1L),0xBE13L,0x7059L,0x2456L,0xBE13L},{0x7059L,0x9C21L,(-1L),0x9C21L,0x0842L,(-1L)},{0x9C21L,0x0842L,(-1L),0xB969L,0xB969L,(-1L)}}};
    uint64_t *l_43 = &g_28[5][1];
    uint32_t *l_2070[6][6][2] = {{{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125}},{{(void*)0,&g_1125},{&g_1125,&g_1125},{&g_1125,(void*)0},{(void*)0,&g_1125},{(void*)0,&g_1125},{(void*)0,(void*)0}},{{&g_1125,&g_1125},{&g_1125,&g_1125},{(void*)0,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125}},{{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{(void*)0,&g_1125},{&g_1125,&g_1125},{&g_1125,(void*)0}},{{(void*)0,&g_1125},{(void*)0,&g_1125},{(void*)0,(void*)0},{&g_1125,&g_1125},{&g_1125,&g_1125},{(void*)0,&g_1125}},{{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125},{&g_1125,&g_1125}}};
    int32_t l_2096 = 0xA476E1ABL;
    int32_t l_2097 = (-8L);
    int32_t ****l_2105[3];
    int32_t *****l_2104 = &l_2105[2];
    int32_t ****l_2106 = &g_226;
    uint64_t l_2107 = 18446744073709551614UL;
    int16_t *l_2108 = (void*)0;
    int16_t *l_2109[4][10][6] = {{{&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1],&g_2019[0][4]},{&l_14[2][4][3],&l_14[2][4][3],&g_15[1],&g_894[0],&g_15[5],&g_894[0]},{&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1]},{&g_894[2],&g_894[1],&g_894[1],&g_894[2],&l_14[2][4][3],&g_894[0]},{&g_894[0],&g_894[2],&g_15[1],&g_894[2],&g_894[0],&g_2019[0][4]},{&g_894[2],&g_894[0],&g_2019[0][4],&g_2019[0][4],&g_894[0],&g_894[2]},{&g_894[1],&g_894[2],&l_14[2][4][3],&g_894[0],&l_14[2][4][3],&g_894[2]},{&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1],&g_2019[0][4]},{&l_14[2][4][3],&l_14[2][4][3],&g_15[1],&g_894[0],&g_15[5],&g_894[0]},{&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1]}},{{&g_894[2],&g_894[1],&g_894[1],&g_894[2],&l_14[2][4][3],&g_894[0]},{&g_894[0],&g_894[2],&g_15[1],&g_894[2],&g_894[0],&g_2019[0][4]},{&g_894[2],&g_894[0],&g_2019[0][4],&g_2019[0][4],&g_894[0],&g_894[2]},{&g_894[1],&g_894[2],&l_14[2][4][3],&g_894[0],&l_14[2][4][3],&g_894[2]},{&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1],&g_2019[0][4]},{&l_14[2][4][3],&l_14[2][4][3],&g_15[1],&g_894[0],&g_15[5],&g_894[0]},{&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1]},{&g_894[2],&g_894[1],&g_894[1],&g_894[2],&l_14[2][4][3],&g_894[0]},{&g_2019[0][4],&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1]},{&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1],&g_2019[0][4],&g_894[1]}},{{&g_894[0],&g_894[1],&g_15[5],&g_2019[0][4],&g_15[5],&g_894[1]},{&g_15[5],&g_894[0],&g_15[1],&l_14[2][4][3],&l_14[2][4][3],&g_15[1]},{&g_15[5],&g_15[5],&l_14[2][4][3],&g_2019[0][4],&g_894[2],&g_2019[0][4]},{&g_894[0],&g_15[5],&g_894[0],&g_15[1],&l_14[2][4][3],&l_14[2][4][3]},{&g_894[1],&g_894[0],&g_894[0],&g_894[1],&g_15[5],&g_2019[0][4]},{&g_2019[0][4],&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1]},{&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1],&g_2019[0][4],&g_894[1]},{&g_894[0],&g_894[1],&g_15[5],&g_2019[0][4],&g_15[5],&g_894[1]},{&g_15[5],&g_894[0],&g_15[1],&l_14[2][4][3],&l_14[2][4][3],&g_15[1]},{&g_15[5],&g_15[5],&l_14[2][4][3],&g_2019[0][4],&g_894[2],&g_2019[0][4]}},{{&g_894[0],&g_15[5],&g_894[0],&g_15[1],&l_14[2][4][3],&l_14[2][4][3]},{&g_894[1],&g_894[0],&g_894[0],&g_894[1],&g_15[5],&g_2019[0][4]},{&g_2019[0][4],&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1]},{&g_894[1],&g_2019[0][4],&g_15[1],&g_15[1],&g_2019[0][4],&g_894[1]},{&g_894[0],&g_894[1],&g_15[5],&g_2019[0][4],&g_15[5],&g_894[1]},{&g_15[5],&g_894[0],&g_15[1],&l_14[2][4][3],&l_14[2][4][3],&g_15[1]},{&g_15[5],&g_15[5],&l_14[2][4][3],&g_2019[0][4],&g_894[2],&g_2019[0][4]},{&g_894[0],&g_15[5],&g_894[0],&g_15[1],&l_14[2][4][3],&l_14[2][4][3]},{&g_894[1],&g_894[0],&g_894[0],&g_894[1],&g_15[5],&g_2019[0][4]},{&g_2019[0][4],&g_894[1],&l_14[2][4][3],&g_894[1],&g_2019[0][4],&g_15[1]}}};
    int16_t l_2116 = 1L;
    int8_t l_2132 = 0x8FL;
    uint16_t *l_2135 = (void*)0;
    uint16_t *l_2136 = &g_171;
    int8_t l_2142[5];
    uint32_t l_2143 = 0xBB69EA11L;
    int16_t l_2144 = 1L;
    int16_t l_2145 = 1L;
    int i, j, k;
    for (i = 0; i < 3; i++)
        l_2105[i] = &g_226;
    for (i = 0; i < 5; i++)
        l_2142[i] = 0L;
    for (p_9 = 2; (p_9 >= 0); p_9 -= 1)
    {
        uint64_t *l_27 = &g_28[5][1];
        const uint64_t *l_214 = (void*)0;
        const uint64_t **l_213 = &l_214;
        uint64_t *l_644 = &g_645;
        int32_t l_961 = 5L;
        int16_t l_1688[9] = {0x07E0L,0x07E0L,0x07E0L,0x07E0L,0x07E0L,0x07E0L,0x07E0L,0x07E0L,0x07E0L};
        uint32_t *l_1689 = &g_1690;
        uint64_t ***l_2063 = &g_667;
        int32_t l_2066 = 0x21B83128L;
        int i;
        g_15[0] = l_14[1][4][1];
    }
    (***l_2106) = ((safe_add_func_uint16_t_u_u(g_171, (g_2019[1][0] ^= ((9L > (p_9 || (safe_rshift_func_int64_t_s_u(((l_2096 = g_1823[5]) & l_14[1][9][5]), ((**g_667) ^= (*g_1246)))))) & (((l_2097 = (-10L)) , ((***g_1548) ^ (((((*l_2104) = &g_225) != l_2106) , p_10) & p_10))) == l_2107))))) , (void*)0);
    p_10 = ((((g_534 <= (g_849 = ((g_1088 = (safe_lshift_func_int64_t_s_u(((((safe_mod_func_int32_t_s_s((((((safe_div_func_uint32_t_u_u((((l_2116 <= (safe_rshift_func_uint16_t_u_s(g_1823[5], ((safe_lshift_func_int32_t_s_u((((+(safe_mod_func_int8_t_s_s(((safe_rshift_func_uint16_t_u_u((safe_div_func_uint8_t_u_u((safe_mod_func_int64_t_s_s((g_2019[2][2] , (safe_div_func_int32_t_s_s((((*g_1181) = l_2132) , (((*l_2136) = (safe_div_func_int32_t_s_s(p_10, p_10))) , (safe_lshift_func_int8_t_s_s((((***g_2060) = ((p_10 > (!(safe_sub_func_int64_t_s_s((*g_370), 0x9BD4E5E22F4319FALL)))) , 1UL)) != 0x2BC51397EB43B871LL), l_2142[2])))), 0x3C0EF0C6L))), p_10)), 4UL)), 3)) > 0x0D326C4010C40897LL), g_15[3]))) < 0xAF55B03BL) < g_1823[6]), 24)) > 0xBA67L)))) , p_9) && p_9), 0x8323AD6FL)) < p_9) || p_10) ^ (-10L)) , (*g_966)), l_2143)) == g_600) == p_10) >= l_2144), 49))) , p_9))) , 2L) >= p_10) > l_2145);
    return p_10;
}







static int32_t func_16(int32_t p_17, uint32_t p_18, uint32_t p_19, int64_t p_20)
{
    int32_t *l_1798 = &g_62;
    int8_t **l_1814 = &g_1181;
    int64_t * const *l_1839 = &g_370;
    int32_t l_1845 = 0xA4208FB1L;
    int32_t l_1846 = 0L;
    int32_t l_1848[4];
    int8_t l_1854 = 0x3DL;
    uint32_t *l_1877 = &g_1860;
    uint32_t **l_1876 = &l_1877;
    uint16_t l_1911 = 6UL;
    uint16_t *l_1947[2];
    int8_t *l_1950[7][6][4] = {{{(void*)0,&l_1854,(void*)0,&g_117},{&g_124,&g_124,&g_124,&g_124},{(void*)0,&g_124,&g_117,&g_124},{&g_117,&l_1854,&g_117,(void*)0},{(void*)0,&g_124,&g_124,(void*)0},{&g_124,&g_117,(void*)0,(void*)0}},{{(void*)0,(void*)0,&g_124,&l_1854},{(void*)0,&l_1854,&g_117,&l_1854},{&g_124,(void*)0,(void*)0,(void*)0},{&g_117,&g_117,&l_1854,(void*)0},{&g_124,&g_124,&g_124,(void*)0},{&l_1854,&l_1854,(void*)0,&g_124}},{{&l_1854,&g_124,(void*)0,(void*)0},{(void*)0,(void*)0,&l_1854,&g_124},{&g_117,&l_1854,&g_117,(void*)0},{(void*)0,&l_1854,(void*)0,&g_117},{&g_117,&l_1854,(void*)0,(void*)0},{&l_1854,&l_1854,&l_1854,&g_124}},{{&g_124,(void*)0,&g_117,(void*)0},{&g_117,&g_124,&g_124,(void*)0},{(void*)0,&l_1854,&g_124,&g_124},{&g_117,&g_117,&g_117,&g_117},{&g_124,(void*)0,&l_1854,&g_124},{&l_1854,&g_124,(void*)0,&l_1854}},{{&g_117,(void*)0,(void*)0,&l_1854},{(void*)0,&g_124,&g_117,&g_124},{&g_117,(void*)0,&l_1854,&g_117},{(void*)0,&g_117,(void*)0,&g_124},{(void*)0,&l_1854,&g_124,(void*)0},{(void*)0,&g_124,(void*)0,(void*)0}},{{(void*)0,(void*)0,&l_1854,&g_124},{&g_117,&l_1854,&g_117,(void*)0},{(void*)0,&l_1854,(void*)0,&g_117},{&g_117,&l_1854,(void*)0,(void*)0},{&l_1854,&l_1854,&l_1854,&g_124},{&g_124,(void*)0,&g_117,(void*)0}},{{&g_117,&g_124,&g_124,(void*)0},{(void*)0,&l_1854,&g_124,&g_124},{&g_117,&g_117,&g_117,&g_117},{&g_124,(void*)0,&l_1854,&g_124},{&l_1854,&g_124,(void*)0,&l_1854},{&g_117,(void*)0,(void*)0,&l_1854}}};
    uint32_t l_1954 = 18446744073709551611UL;
    int8_t l_2021 = 9L;
    int32_t l_2022 = 0x28A6F30DL;
    int16_t l_2024 = 0x6FEEL;
    int64_t l_2026 = 0L;
    int i, j, k;
    for (i = 0; i < 4; i++)
        l_1848[i] = 0x3FF1228AL;
    for (i = 0; i < 2; i++)
        l_1947[i] = &g_849;
    for (g_1208 = 0; (g_1208 > 33); g_1208 = safe_add_func_int32_t_s_s(g_1208, 8))
    {
        int32_t l_1809 = 0xBD4A7636L;
        int32_t l_1816 = 0x39463794L;
        int32_t l_1820 = 0x09175737L;
        int32_t l_1824[8] = {9L,0x7DE3262EL,9L,0x7DE3262EL,9L,0x7DE3262EL,9L,0x7DE3262EL};
        int32_t l_1825 = 0xC3334079L;
        int32_t l_1833 = 0x04A2BACCL;
        int8_t l_1852 = 4L;
        int32_t l_1858 = 0xB2708C1CL;
        int64_t l_1859 = (-4L);
        uint16_t l_1863 = 4UL;
        uint32_t l_1868[8][2] = {{0UL,0UL},{0UL,0UL},{0UL,0UL},{0UL,0UL},{0UL,0UL},{0UL,0UL},{0UL,0UL},{0UL,0UL}};
        uint32_t *****l_1897 = &g_1445;
        uint16_t **l_1929 = (void*)0;
        int i, j;
        for (g_330 = 0; (g_330 != 23); g_330 = safe_add_func_uint32_t_u_u(g_330, 1))
        {
            int32_t *l_1797 = (void*)0;
            int8_t ***l_1815 = &l_1814;
            int64_t *l_1819[8] = {&g_330,(void*)0,&g_330,(void*)0,&g_330,(void*)0,&g_330,(void*)0};
            uint8_t *l_1821 = &g_120;
            uint8_t *l_1822 = &g_1823[5];
            int32_t l_1826 = 0x119D5B1DL;
            uint32_t *l_1832[3][6][4] = {{{&g_285,(void*)0,&g_285,&g_1690},{&g_285,(void*)0,&g_285,(void*)0},{&g_1690,&g_285,(void*)0,(void*)0},{&g_1690,(void*)0,&g_1690,&g_1690},{(void*)0,(void*)0,&g_285,&g_1690},{&g_285,&g_285,&g_285,&g_285}},{{&g_285,&g_285,&g_1690,&g_285},{&g_1690,&g_285,(void*)0,&g_285},{(void*)0,&g_285,&g_1690,(void*)0},{(void*)0,&g_1690,(void*)0,&g_1690},{&g_285,(void*)0,(void*)0,(void*)0},{(void*)0,&g_285,&g_285,&g_285}},{{&g_285,&g_285,(void*)0,(void*)0},{&g_285,&g_1690,(void*)0,(void*)0},{&g_285,(void*)0,(void*)0,(void*)0},{&g_1690,(void*)0,&g_1690,(void*)0},{(void*)0,&g_1690,&g_285,(void*)0},{(void*)0,&g_285,&g_1690,&g_285}}};
            uint16_t *l_1834 = &g_187;
            int32_t l_1847 = 1L;
            int32_t l_1849 = 0x31FBF179L;
            int32_t l_1850 = 3L;
            int32_t l_1851[3];
            int8_t l_1853 = 1L;
            int32_t l_1855 = 0L;
            int16_t l_1856 = 0xA61BL;
            int32_t l_1857[9] = {0L,0L,0L,0L,0L,0L,0L,0L,0L};
            uint16_t l_1871[4][9][3] = {{{0xA531L,1UL,65535UL},{0xD3B8L,65535UL,65531UL},{0UL,0xA531L,0x4241L},{0x13A3L,0x4241L,0xE7A3L},{65528UL,0x180DL,0x01CCL},{0x3715L,65533UL,0x01CCL},{65535UL,1UL,0xE7A3L},{65535UL,0UL,0x4241L},{1UL,0x0570L,65531UL}},{{0x1F21L,65535UL,65535UL},{0UL,0x1EBFL,0x8EF7L},{65528UL,1UL,0x847FL},{65535UL,0x13A3L,0xB5C5L},{0x0570L,1UL,0x6015L},{0x1EBFL,0x13A3L,1UL},{0x34ECL,1UL,65531UL},{0x180DL,0x1EBFL,0xD3B8L},{1UL,65535UL,0xE8FFL}},{{65528UL,0x0570L,0x286CL},{65531UL,0UL,65535UL},{1UL,1UL,0x78D3L},{0x4241L,65533UL,0xA531L},{0x4241L,0x180DL,65531UL},{1UL,0x4241L,0x34ECL},{65531UL,0xA531L,65531UL},{65528UL,65535UL,65535UL},{1UL,1UL,0xC7A3L}},{{0x180DL,1UL,0xB8B7L},{0x34ECL,0x3715L,0x1EBFL},{0x1EBFL,0x1F21L,65531UL},{0x0570L,0x34ECL,0x1EBFL},{65535UL,0xD3B8L,0xB8B7L},{65528UL,0xA182L,0xC7A3L},{0UL,65531UL,65535UL},{0x1F21L,1UL,65531UL},{1UL,65535UL,0x34ECL}}};
            int i, j, k;
            for (i = 0; i < 3; i++)
                l_1851[i] = (-1L);
            l_1798 = ((**g_1744) = l_1797);
            l_1825 = (safe_lshift_func_int64_t_s_s((l_1824[1] = (18446744073709551608UL == (l_1809 = (safe_rshift_func_uint8_t_u_s(((*l_1822) ^= ((0x7AB08C1EBF41F800LL <= (((*l_1821) &= (safe_lshift_func_uint8_t_u_u((((((*g_502) = (safe_lshift_func_uint32_t_u_u((safe_div_func_uint32_t_u_u(l_1809, (-6L))), 0))) >= (safe_add_func_uint32_t_u_u(0UL, (((*g_218) ^= 6UL) < (l_1820 = ((l_1816 = (((*l_1815) = l_1814) == &g_1479)) == (safe_mul_func_int64_t_s_s((((0x7496E008A43B350DLL || (*g_370)) >= (*g_966)) > g_313), p_20)))))))) , p_17) > p_18), p_20))) == (-6L))) >= 0xA4C0AE47L)), 5))))), 29));
            (*g_196) = (l_1826 , (((safe_unary_minus_func_int8_t_s((safe_mod_func_uint32_t_u_u((safe_mul_func_uint32_t_u_u((l_1825 = 6UL), p_19)), ((((*g_312) | l_1833) > ((((*l_1834) = 0xC64AL) && (((safe_mul_func_int16_t_s_s((safe_mod_func_uint16_t_u_u(((0x720320FCL >= p_18) && ((p_19 == p_20) == 0x9EL)), (***g_1548))), p_17)) && p_18) <= 0x1A9A286CC51837D8LL)) , g_1125)) || p_19))))) , (void*)0) != l_1839));
            l_1848[1] &= (l_1846 = (((~((0xA33EL < (safe_add_func_uint8_t_u_u((safe_mod_func_uint32_t_u_u((g_1860++), l_1863)), 1UL))) & ((p_17 | p_20) >= ((void*)0 == &p_20)))) , ((safe_mod_func_uint64_t_u_u(l_1868[4][0], (safe_mul_func_int32_t_s_s((-4L), (*g_550))))) | p_20)) || l_1871[1][2][2]));
        }
        for (p_20 = 0; (p_20 != 20); p_20 = safe_add_func_uint16_t_u_u(p_20, 5))
        {
            int32_t l_1880 = 0xBF2F845FL;
            int32_t *l_1887 = &g_600;
            uint32_t *** const **l_1890 = (void*)0;
            uint32_t *****l_1913 = &g_1445;
            int32_t *** const *l_1923 = &g_225;
            (*l_1887) |= (((*g_218) = p_19) , ((safe_mul_func_uint32_t_u_u((l_1876 != ((((safe_rshift_func_uint8_t_u_s(l_1880, ((*g_502) != ((safe_mod_func_int8_t_s_s((l_1880 , p_20), 0x10L)) ^ (((safe_lshift_func_uint64_t_u_u(p_17, (safe_lshift_func_int16_t_s_u((**g_311), p_20)))) , p_19) | 7UL))))) || p_17) >= p_17) , (void*)0)), l_1824[1])) > l_1824[1]));
            for (g_62 = 0; (g_62 < 22); g_62 = safe_add_func_int16_t_s_s(g_62, 5))
            {
                uint16_t *l_1903[1];
                int32_t l_1904 = 0L;
                int32_t l_1905 = 8L;
                int32_t l_1906 = (-3L);
                uint32_t l_1934 = 0x8639AC9EL;
                int i;
                for (i = 0; i < 1; i++)
                    l_1903[i] = &l_1863;
                (*g_1604) ^= l_1852;
                l_1798 = ((****g_925) = &l_1848[2]);
                if ((*g_196))
                    break;
                if (((g_1891[0] = l_1890) == (((**g_1730) || (+((l_1897 != (void*)0) && (!(safe_mul_func_int8_t_s_s((safe_mul_func_uint16_t_u_u((g_187--), ((*g_1479) != (0x776F37A7L < 0x5EA74421L)))), ((safe_div_func_int64_t_s_s(l_1911, ((**g_1730) &= (!(p_17 , 4294967295UL))))) & 0x505031ECL))))))) , l_1913)))
                {
                    int16_t l_1930 = (-5L);
                    uint16_t l_1931[4][8] = {{65528UL,0UL,0xBDD7L,0xC0A9L,0x533DL,65535UL,0x533DL,0xC0A9L},{0x533DL,65535UL,0x533DL,0xC0A9L,0xBDD7L,0UL,65528UL,9UL},{0x565CL,0xBDD7L,0x2778L,0UL,0UL,0x2778L,0xBDD7L,0x565CL},{0x565CL,0xC0A9L,65535UL,0x533DL,0xBDD7L,0x0711L,0x2778L,0x0711L}};
                    int32_t l_1933 = 1L;
                    int i, j;
                    for (g_120 = (-7); (g_120 <= 24); g_120 = safe_add_func_int32_t_s_s(g_120, 4))
                    {
                        int64_t l_1918 = 7L;
                        const uint16_t *l_1927 = (void*)0;
                        const uint16_t * const *l_1926 = &l_1927;
                        const uint16_t * const **l_1928 = &l_1926;
                        int32_t l_1932 = (-10L);
                        (*l_1887) = ((*l_1798) = (safe_add_func_uint64_t_u_u(l_1918, 0x4D68BE7748DD009DLL)));
                        (*g_196) = ((((safe_div_func_uint64_t_u_u((l_1863 || (safe_mod_func_uint16_t_u_u(((((*g_925) == l_1923) < (*g_502)) >= (*l_1798)), (l_1933 ^= (safe_sub_func_uint16_t_u_u(0xF7A9L, ((((l_1932 = (((((*l_1928) = l_1926) == l_1929) ^ (4294967289UL & l_1930)) > l_1931[1][6])) | 0x47L) & p_19) < (-7L)))))))), 0xF2D6892E62722E7FLL)) , l_1932) | l_1934) != p_18);
                        if (l_1934)
                            continue;
                        (****g_925) = &l_1932;
                    }
                }
                else
                {
                    for (l_1904 = 0; (l_1904 > (-3)); --l_1904)
                    {
                        l_1905 = l_1904;
                        (****g_193) = l_1852;
                        if (p_17)
                            break;
                        (*l_1798) = (****g_193);
                    }
                }
            }
            return p_18;
        }
        return p_20;
    }
    if ((safe_lshift_func_int32_t_s_u((p_20 , (safe_add_func_int8_t_s_s(((*l_1798) = (l_1848[2] ^= (safe_mod_func_int32_t_s_s(((p_20 | ((safe_mul_func_int16_t_s_s((safe_add_func_int64_t_s_s(((***g_1548) >= ((g_849 &= (*l_1798)) | p_17)), 8UL)), (((safe_add_func_int8_t_s_s((l_1845 = ((*g_1181) = p_20)), (((safe_lshift_func_uint64_t_u_s(p_17, (~((**g_667) , p_17)))) == (*l_1798)) && (**g_501)))) <= g_245) < 9L))) && (*l_1798))) > (**g_501)), 1UL)))), p_18))), l_1954)))
    {
        int8_t l_1958 = 0xA5L;
        uint16_t *l_1984 = &l_1911;
        int32_t l_1999 = (-1L);
        int32_t l_2000 = 0x7DFDB041L;
        if ((safe_mod_func_int8_t_s_s((+l_1958), (((((safe_sub_func_int8_t_s_s(((*l_1798) >= (*l_1798)), (safe_rshift_func_int32_t_s_u((l_1845 ^= 0xFEADFE60L), 9)))) == ((+(safe_add_func_uint8_t_u_u(((safe_sub_func_int32_t_s_s((safe_add_func_uint32_t_u_u(((safe_add_func_int64_t_s_s(((safe_mod_func_uint8_t_u_u(((((*l_1798) && (safe_sub_func_uint32_t_u_u(0xA5B181B7L, (p_19 != (safe_sub_func_int16_t_s_s(((*l_1798) && (safe_mul_func_int16_t_s_s(((***g_1548) = (((safe_div_func_uint16_t_u_u((safe_div_func_int8_t_s_s(((((g_534 && 255UL) < 0x0FL) != (**g_1481)) || 0L), (-1L))), 0x6663L)) < l_1958) && l_1958)), p_18))), 0xDD4DL)))))) || (*l_1798)) != 0UL), p_18)) || (**g_501)), (-1L))) > (**g_1730)), 0xF495C789L)), 0xEC5795BCL)) > l_1958), 3L))) <= (*l_1798))) <= p_18) < g_894[1]) && g_245))))
        {
            int32_t ***l_1995[2];
            int i;
            for (i = 0; i < 2; i++)
                l_1995[i] = &g_227;
            l_2000 |= ((((*l_1798) , l_1984) != (void*)0) <= (safe_mul_func_uint64_t_u_u(((safe_add_func_int64_t_s_s(((safe_lshift_func_int32_t_s_u((safe_rshift_func_int32_t_s_s((l_1999 = (safe_add_func_int64_t_s_s(((void*)0 == l_1995[0]), ((safe_mul_func_int32_t_s_s((l_1958 , (l_1848[2] ^= ((((safe_unary_minus_func_int8_t_s(0x87L)) , &l_1995[0]) != (void*)0) && p_17))), p_18)) <= p_19)))), p_18)), 29)) < 6UL), l_1958)) , 0x806B59E96EFF7C9ALL), (-10L))));
            l_1798 = &l_2000;
        }
        else
        {
            uint16_t l_2014 = 0UL;
            for (p_18 = 0; (p_18 < 10); p_18++)
            {
                uint16_t *l_2013 = &g_849;
                (*l_1798) ^= ((p_20 | (-1L)) > (g_894[1] > (safe_add_func_uint32_t_u_u((safe_sub_func_int32_t_s_s((((safe_lshift_func_int32_t_s_u(((((g_600 & ((**g_311) < p_18)) != (p_18 > ((p_17 >= (((safe_lshift_func_uint8_t_u_s((l_2013 == &g_171), 6)) ^ (*g_966)) != p_19)) && l_2014))) <= (*g_1479)) && p_18), 2)) > g_1823[2]) , p_20), 0L)), 0xAFB7D9E3L))));
            }
        }
        (*g_196) |= ((*l_1798) != g_65);
    }
    else
    {
        int16_t l_2015 = 0xA373L;
        int32_t l_2016 = (-2L);
        int32_t l_2017[5][4] = {{0xB0ED909CL,0xB0ED909CL,0xB0ED909CL,0xB0ED909CL},{0xB0ED909CL,0xB0ED909CL,0xB0ED909CL,0xB0ED909CL},{0xB0ED909CL,0xB0ED909CL,0xB0ED909CL,0xB0ED909CL},{0xB0ED909CL,0xB0ED909CL,0xB0ED909CL,0xB0ED909CL},{0xB0ED909CL,0xB0ED909CL,0xB0ED909CL,0xB0ED909CL}};
        int32_t *l_2018[1][5][1];
        int16_t l_2020[8] = {0xC935L,0xC935L,0xC935L,0xC935L,0xC935L,0xC935L,0xC935L,0xC935L};
        int32_t l_2023 = 1L;
        int16_t l_2025[8][8] = {{(-1L),0xD857L,(-1L),(-4L),0L,0x3F66L,(-4L),0x1F61L},{0xD857L,(-5L),0x3F66L,0L,0xD184L,0xFF8DL,0L,(-5L)},{0xD857L,0x1F61L,0xB73FL,0L,0L,0xB73FL,0x1F61L,0xD857L},{(-1L),0x8067L,(-5L),0xFF8DL,0xD857L,0x3635L,(-1L),0x1F61L},{0x8067L,0xFF8DL,(-4L),1L,0xFF8DL,0x3635L,0L,0x3635L},{0L,0x8067L,4L,0x8067L,0L,0xB73FL,1L,0L},{0x3635L,0x1F61L,(-1L),0x3635L,0xD857L,0xFF8DL,(-5L),0x8067L},{0x1F61L,(-5L),(-1L),1L,0x3F66L,0x3F66L,1L,(-1L)}};
        uint32_t l_2027 = 7UL;
        int i, j, k;
        for (i = 0; i < 1; i++)
        {
            for (j = 0; j < 5; j++)
            {
                for (k = 0; k < 1; k++)
                    l_2018[i][j][k] = &g_62;
            }
        }
        l_2027++;
    }
    return l_1846;
}







static uint64_t func_31(uint64_t * p_32, int32_t p_33, int16_t p_34, uint32_t p_35)
{
    int32_t l_1750 = 0xC9B714D1L;
    int32_t *l_1757 = &g_600;
    uint64_t l_1758 = 1UL;
    uint16_t *l_1777 = (void*)0;
    uint64_t *l_1778 = &g_645;
    int32_t *****l_1782 = (void*)0;
    int32_t *** const *l_1784 = &g_226;
    int32_t *** const **l_1783[10] = {&l_1784,&l_1784,&l_1784,&l_1784,&l_1784,&l_1784,&l_1784,&l_1784,&l_1784,&l_1784};
    int32_t *** const **l_1785 = &l_1784;
    uint16_t *l_1786 = &g_187;
    int8_t l_1787 = 2L;
    uint8_t *l_1788 = (void*)0;
    uint8_t *l_1789[2];
    int8_t l_1792[8][10][3] = {{{0xF7L,0xF7L,0x9DL},{0x43L,0xDCL,6L},{0x9DL,0xD0L,1L},{0x43L,9L,0x43L},{0xF7L,0x9DL,1L},{0x02L,(-2L),6L},{1L,0x9DL,0x9DL},{6L,9L,(-1L)},{1L,0xD0L,1L},{0x02L,0xDCL,(-1L)}},{{0xF7L,0xF7L,0x9DL},{0x43L,0xDCL,6L},{0x9DL,0xD0L,1L},{0x43L,9L,0x43L},{0xF7L,0x9DL,1L},{0x02L,(-2L),6L},{1L,0x9DL,0x9DL},{6L,9L,(-1L)},{1L,0xD0L,1L},{0x02L,0xDCL,(-1L)}},{{0xF7L,0xF7L,0x9DL},{0x43L,0xDCL,6L},{0x9DL,0xD0L,1L},{0x43L,9L,0x43L},{0xF7L,0x9DL,1L},{0x02L,(-2L),6L},{1L,0x9DL,0x9DL},{6L,9L,(-1L)},{1L,0xD0L,1L},{0x02L,0xDCL,(-1L)}},{{0xF7L,0xF7L,0x9DL},{0x43L,0xDCL,6L},{0x9DL,0xD0L,0xD0L},{(-1L),(-2L),(-1L)},{0x9DL,1L,0xD0L},{6L,0xDCL,0x43L},{1L,1L,1L},{0x43L,(-2L),(-1L)},{1L,0xF7L,1L},{6L,1L,(-1L)}},{{0x9DL,0x9DL,1L},{(-1L),1L,0x43L},{1L,0xF7L,0xD0L},{(-1L),(-2L),(-1L)},{0x9DL,1L,0xD0L},{6L,0xDCL,0x43L},{1L,1L,1L},{0x43L,(-2L),(-1L)},{1L,0xF7L,1L},{6L,1L,(-1L)}},{{0x9DL,0x9DL,1L},{(-1L),1L,0x43L},{1L,0xF7L,0xD0L},{(-1L),(-2L),(-1L)},{0x9DL,1L,0xD0L},{6L,0xDCL,0x43L},{1L,1L,1L},{0x43L,(-2L),(-1L)},{1L,0xF7L,1L},{6L,1L,(-1L)}},{{0x9DL,0x9DL,1L},{(-1L),1L,0x43L},{1L,0xF7L,0xD0L},{(-1L),(-2L),(-1L)},{0x9DL,1L,0xD0L},{6L,0xDCL,0x43L},{1L,1L,1L},{0x43L,(-2L),(-1L)},{1L,0xF7L,1L},{6L,1L,(-1L)}},{{0x9DL,0x9DL,1L},{(-1L),1L,0x43L},{1L,0xF7L,0xD0L},{(-1L),(-2L),(-1L)},{0x9DL,1L,0xD0L},{6L,0xDCL,0x43L},{1L,1L,1L},{0x43L,(-2L),(-1L)},{1L,0xF7L,1L},{6L,1L,(-1L)}}};
    int i, j, k;
    for (i = 0; i < 2; i++)
        l_1789[i] = &g_120;
lbl_1791:
    (****g_193) |= (safe_sub_func_int32_t_s_s(1L, ((safe_mod_func_uint64_t_u_u(((p_35 , l_1750) >= l_1750), p_33)) || (p_33 , ((safe_rshift_func_int64_t_s_s((**g_1730), (**g_1730))) == (safe_rshift_func_int32_t_s_u(((*l_1757) = (safe_sub_func_uint16_t_u_u(g_1208, p_34))), l_1758)))))));
    if ((((safe_mod_func_uint64_t_u_u(((*p_32)--), p_34)) && ((g_120 = (g_1208 = (safe_div_func_int32_t_s_s((((safe_div_func_int64_t_s_s((safe_sub_func_uint64_t_u_u((*p_32), (l_1787 |= (safe_div_func_uint64_t_u_u(0x077F7C17B42BAC66LL, (((((((*l_1786) = (((void*)0 != &g_187) > ((((safe_mod_func_uint16_t_u_u(((*l_1757) = 65535UL), (((*l_1778) = 0xF76458F701C1DDD7LL) , (safe_sub_func_uint8_t_u_u((safe_unary_minus_func_uint8_t_u((l_1782 != (l_1785 = (l_1783[0] = l_1783[0]))))), p_35))))) && (-4L)) ^ g_524) <= p_34))) >= (*g_312)) , (**g_501)) >= 0UL) ^ 0L) & p_33)))))), (*g_218))) && 0L) >= p_35), 1L)))) < (**g_1481))) && 1L))
    {
        int32_t l_1790[10] = {0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L,0x2CEC03B8L};
        int i;
        return l_1790[3];
    }
    else
    {
        if (p_35)
            goto lbl_1791;
    }
    return l_1792[4][1][0];
}







static uint64_t * func_36(uint64_t * p_37, uint32_t p_38, uint64_t * p_39)
{
    uint32_t *****l_1692 = &g_1445;
    uint32_t *****l_1693 = &g_1445;
    int32_t l_1696 = 8L;
    int32_t l_1710 = 1L;
    int32_t l_1715[7][2][8] = {{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}},{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}},{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}},{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}},{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}},{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}},{{0L,0L,3L,3L,0L,0L,3L,3L},{0L,0L,3L,3L,0L,0L,3L,3L}}};
    int64_t **l_1731 = &g_370;
    int64_t l_1737 = 4L;
    int32_t ** const *l_1743 = &g_227;
    int i, j, k;
    if (((!g_15[8]) | ((l_1692 != (l_1693 = &g_1445)) || p_38)))
    {
        int8_t l_1697 = 0x60L;
        uint8_t *l_1708 = (void*)0;
        uint8_t *l_1709[4];
        int32_t l_1711 = 0x6B14B950L;
        int32_t l_1713 = 0x64250249L;
        int32_t l_1714 = 0xC8F44560L;
        int32_t l_1716 = (-1L);
        int32_t l_1717 = 0xA78B426FL;
        uint8_t l_1718 = 0x3EL;
        int i;
        for (i = 0; i < 4; i++)
            l_1709[i] = &g_1208;
        if ((safe_div_func_uint16_t_u_u(g_1480, (p_38 || ((g_1125 ^= (l_1697 |= l_1696)) , (l_1711 = (safe_div_func_int64_t_s_s(l_1696, (safe_mod_func_uint32_t_u_u(p_38, (p_38 , ((safe_sub_func_uint8_t_u_u(((safe_sub_func_int32_t_s_s(((((**g_667) = 18446744073709551607UL) , (((***g_1548) = ((safe_add_func_uint8_t_u_u((l_1710 &= g_524), g_187)) != g_524)) , (void*)0)) == &g_1243), p_38)) > p_38), g_245)) || 18446744073709551614UL))))))))))))
        {
            int32_t *l_1712[7];
            uint64_t *l_1721 = &g_28[5][1];
            int i;
            for (i = 0; i < 7; i++)
                l_1712[i] = &g_600;
            l_1718--;
            return l_1721;
        }
        else
        {
            uint64_t l_1722 = 18446744073709551615UL;
            (****g_925) = (void*)0;
            --l_1722;
            return p_37;
        }
    }
    else
    {
        int32_t * const l_1725 = &g_600;
        int32_t **l_1726[4] = {&g_75[2][0][2],&g_75[2][0][2],&g_75[2][0][2],&g_75[2][0][2]};
        int16_t ****l_1732[8][8][4] = {{{&g_1548,(void*)0,&g_1548,&g_1548},{(void*)0,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{(void*)0,&g_1548,(void*)0,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{(void*)0,&g_1548,&g_1548,(void*)0}},{{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,(void*)0},{&g_1548,(void*)0,(void*)0,(void*)0},{(void*)0,(void*)0,&g_1548,(void*)0},{&g_1548,(void*)0,&g_1548,(void*)0},{(void*)0,(void*)0,&g_1548,(void*)0},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,(void*)0}},{{&g_1548,&g_1548,(void*)0,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{(void*)0,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{(void*)0,&g_1548,(void*)0,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548}},{{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{(void*)0,&g_1548,&g_1548,(void*)0},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,(void*)0,&g_1548},{&g_1548,(void*)0,&g_1548,(void*)0},{&g_1548,(void*)0,(void*)0,&g_1548}},{{(void*)0,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{&g_1548,&g_1548,(void*)0,&g_1548},{&g_1548,&g_1548,&g_1548,(void*)0},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{(void*)0,(void*)0,(void*)0,&g_1548}},{{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,(void*)0,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,(void*)0},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548}},{{&g_1548,&g_1548,(void*)0,&g_1548},{&g_1548,(void*)0,&g_1548,(void*)0},{&g_1548,(void*)0,(void*)0,&g_1548},{(void*)0,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,&g_1548,&g_1548},{&g_1548,&g_1548,(void*)0,&g_1548},{&g_1548,&g_1548,&g_1548,(void*)0}},{{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{(void*)0,(void*)0,(void*)0,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,(void*)0,(void*)0,&g_1548},{&g_1548,&g_1548,&g_1548,&g_1548},{&g_1548,&g_1548,&g_1548,(void*)0},{&g_1548,&g_1548,&g_1548,&g_1548}}};
        int i, j, k;
lbl_1745:
        (*g_1727) = (g_1125 , l_1725);
        if ((safe_mod_func_uint8_t_u_u((p_38 >= (((***g_1548) = ((g_1730 = &g_370) == (p_38 , l_1731))) == (((void*)0 == l_1732[3][0][3]) | (safe_sub_func_uint64_t_u_u((safe_lshift_func_uint32_t_u_s(l_1737, (safe_div_func_int32_t_s_s((safe_add_func_int16_t_s_s((((*g_1181) = (*g_1479)) & ((g_285 | l_1715[2][0][3]) & p_38)), p_38)), p_38)))), 0L))))), l_1696)))
        {
            int32_t *l_1742 = &l_1710;
            l_1742 = &l_1710;
        }
        else
        {
            g_1744 = ((*g_926) = l_1743);
        }
        if (l_1737)
            goto lbl_1745;
    }
    return p_37;
}







static uint16_t func_40(uint64_t * p_41, int8_t p_42)
{
    const int32_t *l_968 = &g_600;
    int64_t **l_981 = &g_370;
    int32_t l_990 = 0xB318A031L;
    int32_t l_992 = 8L;
    int32_t l_993 = 1L;
    int32_t l_994 = 0x7B591F0BL;
    int32_t l_998[4][3][5] = {{{0x64A3AC90L,0xDC6421C7L,0x64A3AC90L,0x64A3AC90L,0xDC6421C7L},{(-2L),(-1L),0x0AB02E0DL,0L,0x0AB02E0DL},{0xDC6421C7L,0xDC6421C7L,1L,0xDC6421C7L,0xDC6421C7L}},{{0x0AB02E0DL,0L,0x0AB02E0DL,(-1L),(-2L)},{0xDC6421C7L,0x64A3AC90L,0x64A3AC90L,0xDC6421C7L,0x64A3AC90L},{(-2L),0L,0x9CC64314L,0L,(-2L)}},{{0x64A3AC90L,0xDC6421C7L,0x64A3AC90L,0x64A3AC90L,0xDC6421C7L},{(-2L),(-1L),0x0AB02E0DL,0L,0x0AB02E0DL},{0xDC6421C7L,0xDC6421C7L,1L,0xDC6421C7L,0xDC6421C7L}},{{0x0AB02E0DL,0L,0x0AB02E0DL,(-5L),0x0AB02E0DL},{0x64A3AC90L,1L,1L,0x64A3AC90L,1L},{0x0AB02E0DL,(-1L),(-2L),(-1L),0x0AB02E0DL}}};
    int64_t l_1013 = 0x16BD7CEE4FA11A2DLL;
    int16_t l_1014 = 0xCC9FL;
    uint16_t *l_1022 = &g_849;
    uint32_t ****l_1063 = &g_683;
    int32_t **l_1085[3];
    int8_t **l_1168 = (void*)0;
    int64_t l_1189 = 0x3E7D9DC55E3B76E5LL;
    int64_t l_1283 = 0xACA20F104F7789DCLL;
    int32_t l_1296 = (-8L);
    int16_t ***l_1321 = &g_501;
    int16_t l_1327 = 0x7883L;
    uint32_t l_1332 = 6UL;
    const int16_t l_1355 = 0L;
    uint32_t *l_1421 = &g_1125;
    uint32_t *l_1422[7] = {&g_1125,&g_1125,&g_1125,&g_1125,&g_1125,&g_1125,&g_1125};
    int32_t * const l_1517 = &g_5;
    int32_t ****l_1557 = (void*)0;
    uint8_t *l_1601 = &g_1208;
    int16_t l_1642 = 0L;
    int64_t l_1646 = 0x83898915E1B305ACLL;
    uint8_t l_1679 = 3UL;
    int i, j, k;
    for (i = 0; i < 3; i++)
        l_1085[i] = &g_75[2][0][2];
    for (p_42 = 12; (p_42 <= (-4)); p_42 = safe_sub_func_uint64_t_u_u(p_42, 9))
    {
        const int32_t *l_964[8] = {&g_5,&g_5,&g_5,&g_5,&g_5,&g_5,&g_5,&g_5};
        int32_t l_975 = 1L;
        int32_t l_1000 = 0x1C7490ECL;
        int32_t l_1001 = 3L;
        int32_t l_1002 = (-1L);
        int32_t l_1003 = 0x55821F81L;
        int32_t l_1004 = 4L;
        int32_t l_1005 = 2L;
        int32_t l_1007[9] = {4L,(-7L),4L,4L,(-7L),4L,4L,(-7L),4L};
        int16_t *l_1035[1];
        uint32_t l_1039 = 0xB7B57AFCL;
        int32_t **l_1084 = &g_75[2][0][2];
        int8_t * const l_1099 = &g_117;
        int8_t * const *l_1098 = &l_1099;
        int32_t l_1146[9][7][4] = {{{0x03D00FA4L,(-2L),0xFFFC9C79L,0xEF0FA3C7L},{0x03D00FA4L,0xBB309ADDL,0x201CB553L,(-1L)},{(-2L),(-1L),(-1L),(-1L)},{0xBB309ADDL,0xBB309ADDL,1L,0xEF0FA3C7L},{(-1L),(-2L),1L,0x93512381L},{0xBB309ADDL,0x03D00FA4L,(-1L),1L},{(-2L),0x03D00FA4L,0x201CB553L,0x93512381L}},{{0x03D00FA4L,(-2L),0xFFFC9C79L,0xEF0FA3C7L},{0x03D00FA4L,0xBB309ADDL,0x201CB553L,(-1L)},{(-2L),(-1L),(-1L),(-1L)},{0xBB309ADDL,0xBB309ADDL,1L,0xEF0FA3C7L},{(-1L),(-2L),1L,0x93512381L},{0xBB309ADDL,0x03D00FA4L,(-1L),1L},{(-2L),0x03D00FA4L,0x201CB553L,0x93512381L}},{{0x03D00FA4L,(-2L),0xFFFC9C79L,0xEF0FA3C7L},{0x03D00FA4L,0xBB309ADDL,0x201CB553L,(-1L)},{(-2L),(-1L),(-1L),(-1L)},{0xBB309ADDL,0xBB309ADDL,1L,0xEF0FA3C7L},{(-1L),(-2L),1L,0x93512381L},{0xBB309ADDL,0x03D00FA4L,(-1L),1L},{(-2L),0x03D00FA4L,0x201CB553L,0x93512381L}},{{0x03D00FA4L,(-2L),0xFFFC9C79L,0xEF0FA3C7L},{0x03D00FA4L,0xBB309ADDL,0x201CB553L,(-1L)},{(-2L),(-1L),(-1L),(-1L)},{0xBB309ADDL,0xBB309ADDL,1L,0xEF0FA3C7L},{(-1L),(-2L),1L,0x93512381L},{0xBB309ADDL,0x03D00FA4L,(-1L),1L},{(-2L),0x03D00FA4L,0x201CB553L,0x93512381L}},{{0x03D00FA4L,(-2L),0xFFFC9C79L,0xEF0FA3C7L},{0x03D00FA4L,0xBB309ADDL,0x201CB553L,(-1L)},{(-2L),(-1L),(-1L),(-1L)},{0xBB309ADDL,0xBB309ADDL,1L,0xEF0FA3C7L},{(-1L),(-2L),1L,0x93512381L},{0xBB309ADDL,0x03D00FA4L,(-1L),1L},{(-2L),0x03D00FA4L,0x201CB553L,0x93512381L}},{{0x03D00FA4L,(-2L),0xFFFC9C79L,0xEF0FA3C7L},{0x03D00FA4L,0xBB309ADDL,0x201CB553L,(-1L)},{(-2L),(-1L),(-1L),(-1L)},{0xBB309ADDL,0xBB309ADDL,1L,(-1L)},{(-9L),0x03D00FA4L,0xFFFC9C79L,0x201CB553L},{0x2A8AE91BL,(-1L),1L,0xFFFC9C79L},{0x03D00FA4L,(-1L),0xEF0FA3C7L,0x201CB553L}},{{(-1L),0x03D00FA4L,0xBCA1CCFFL,(-1L)},{(-1L),0x2A8AE91BL,0xEF0FA3C7L,1L},{0x03D00FA4L,(-9L),1L,1L},{0x2A8AE91BL,0x2A8AE91BL,0xFFFC9C79L,(-1L)},{(-9L),0x03D00FA4L,0xFFFC9C79L,0x201CB553L},{0x2A8AE91BL,(-1L),1L,0xFFFC9C79L},{0x03D00FA4L,(-1L),0xEF0FA3C7L,0x201CB553L}},{{(-1L),0x03D00FA4L,0xBCA1CCFFL,(-1L)},{(-1L),0x2A8AE91BL,0xEF0FA3C7L,1L},{0x03D00FA4L,(-9L),1L,1L},{0x2A8AE91BL,0x2A8AE91BL,0xFFFC9C79L,(-1L)},{(-9L),0x03D00FA4L,0xFFFC9C79L,0x201CB553L},{0x2A8AE91BL,(-1L),1L,0xFFFC9C79L},{0x03D00FA4L,(-1L),0xEF0FA3C7L,0x201CB553L}},{{(-1L),0x03D00FA4L,0xBCA1CCFFL,(-1L)},{(-1L),0x2A8AE91BL,0xEF0FA3C7L,1L},{0x03D00FA4L,(-9L),1L,1L},{0x2A8AE91BL,0x2A8AE91BL,0xFFFC9C79L,(-1L)},{(-9L),0x03D00FA4L,0xFFFC9C79L,0x201CB553L},{0x2A8AE91BL,(-1L),1L,0xFFFC9C79L},{0x03D00FA4L,(-1L),0xEF0FA3C7L,0x201CB553L}}};
        uint8_t l_1153 = 0xA1L;
        int32_t *l_1163 = &g_600;
        int8_t *l_1179[8][1] = {{&g_922},{&g_117},{&g_922},{&g_117},{&g_922},{&g_117},{&g_922},{&g_117}};
        int8_t **l_1178 = &l_1179[1][0];
        int i, j, k;
        for (i = 0; i < 1; i++)
            l_1035[i] = &g_894[0];
        for (g_285 = 2; (g_285 <= 9); g_285 += 1)
        {
            const int32_t **l_967[1][10][2] = {{{(void*)0,&l_964[0]},{&g_966,(void*)0},{&g_966,&g_966},{&g_966,(void*)0},{&g_966,&l_964[0]},{(void*)0,&l_964[0]},{&g_966,(void*)0},{&g_966,&g_966},{&g_966,(void*)0},{&g_966,&l_964[0]}}};
            int32_t l_977 = 0xD23575B9L;
            int64_t **l_982 = (void*)0;
            int32_t l_991 = 0xA649531CL;
            int32_t l_997[8][4] = {{0xA7ACABEBL,(-7L),2L,0xB9D45F77L},{0xA7ACABEBL,2L,0xA7ACABEBL,2L},{(-7L),0xB9D45F77L,2L,2L},{2L,2L,0x3828F2BAL,0xB9D45F77L},{0xB9D45F77L,(-7L),0x3828F2BAL,(-7L)},{2L,0xA7ACABEBL,2L,0x3828F2BAL},{(-7L),0xA7ACABEBL,0xA7ACABEBL,(-7L)},{0xA7ACABEBL,(-7L),2L,0xB9D45F77L}};
            int64_t l_1015 = 0L;
            uint8_t l_1016 = 1UL;
            int8_t *l_1019 = &g_117;
            volatile int16_t l_1055 = 0x06D8L;
            uint32_t ****l_1061 = (void*)0;
            uint64_t *** const *l_1132 = (void*)0;
            int32_t *l_1150[6][5][3] = {{{&g_2,&l_998[0][1][0],&l_993},{&l_993,&l_1003,&l_1004},{&l_1003,&l_1003,&l_1003},{&l_1004,&l_998[0][1][0],&l_1002},{(void*)0,&l_994,&l_998[0][1][0]}},{{&l_992,&l_993,(void*)0},{&l_993,(void*)0,&l_1007[1]},{&l_992,&l_1005,&l_1003},{(void*)0,(void*)0,&g_69},{&l_1004,&l_994,&l_1002}},{{&l_1003,&l_1002,&l_1002},{&l_993,&l_1004,&l_1005},{&l_1004,(void*)0,&l_1007[1]},{(void*)0,&l_992,&l_1005},{&l_1004,&l_993,&g_2}},{{&g_69,&l_992,&l_1003},{&g_65,(void*)0,(void*)0},{&l_1002,&l_1004,&l_1007[1]},{&l_993,&l_1003,&l_1003},{&l_993,&l_993,&g_65}},{{&l_1002,&g_2,&l_992},{&g_65,&l_1002,&l_990},{&g_69,(void*)0,&l_1007[1]},{&l_1004,&g_65,&l_990},{(void*)0,&l_993,&l_992}},{{&l_1004,&l_1003,&g_65},{&g_65,&l_994,&l_1003},{&l_1007[1],&l_994,&l_1007[1]},{&l_998[0][1][0],&l_1003,(void*)0},{&l_1002,&l_993,&l_1003}}};
            int i, j, k;
            l_968 = l_964[0];
            for (g_187 = 1; (g_187 <= 9); g_187 += 1)
            {
                int32_t *l_976[8][1][8] = {{{&g_600,&g_5,&g_5,&g_600,(void*)0,&l_975,(void*)0,&g_600}},{{&g_5,(void*)0,&g_5,(void*)0,&g_65,&g_65,(void*)0,&g_5}},{{(void*)0,(void*)0,&g_65,&l_975,&l_975,&l_975,&g_65,(void*)0}},{{(void*)0,&g_5,(void*)0,&g_65,&g_65,(void*)0,&g_5,(void*)0}},{{&g_5,&g_600,(void*)0,&l_975,(void*)0,&g_600,&g_5,&g_5}},{{&g_600,&l_975,(void*)0,(void*)0,&l_975,&g_600,&g_65,&g_600}},{{&l_975,&g_600,&g_65,&g_600,&l_975,(void*)0,(void*)0,&l_975}},{{&g_600,&g_5,&g_5,&g_600,(void*)0,&l_975,(void*)0,&g_600}}};
                int i, j, k;
                if (g_15[g_187])
                    break;
                for (g_490 = 0; (g_490 <= 4); g_490 += 1)
                {
                    int32_t *l_971 = &g_600;
                    int64_t l_972 = 0xA8B45FC35EFCC589LL;
                    if (((*l_971) |= ((safe_mod_func_int64_t_s_s(p_42, ((*g_370) ^= p_42))) & 0x6DL)))
                    {
                        return (*l_971);
                    }
                    else
                    {
                        if (p_42)
                            break;
                    }
                    if ((*g_966))
                        break;
                    (****g_193) = l_972;
                }
                for (g_171 = 3; (g_171 <= 9); g_171 += 1)
                {
                    return p_42;
                }
                if (((safe_sub_func_int32_t_s_s((l_975 = p_42), 1L)) , (l_977 = p_42)))
                {
                    int32_t l_989 = (-1L);
                    int32_t l_995 = 0L;
                    int32_t l_996 = 0xDA887BA4L;
                    int32_t l_999[5][5] = {{8L,(-1L),0L,(-1L),8L},{8L,(-1L),0L,(-1L),8L},{8L,(-1L),0L,(-1L),8L},{8L,(-1L),0L,(-1L),8L},{8L,(-1L),0L,(-1L),8L}};
                    int64_t l_1006 = 0L;
                    uint8_t l_1008 = 0x9AL;
                    int i, j;
                    if ((safe_add_func_uint32_t_u_u((0x3E05L && ((g_313 <= p_42) , (-1L))), (~(p_42 , (*g_370))))))
                    {
                        if (p_42)
                            break;
                        l_976[3][0][1] = &l_975;
                        l_982 = l_981;
                    }
                    else
                    {
                        uint16_t l_985[6][9][4] = {{{1UL,65533UL,0x6B89L,3UL},{0xA316L,65528UL,65533UL,65533UL},{1UL,1UL,0x8FE2L,1UL},{65528UL,65535UL,0x6B89L,1UL},{0x6044L,9UL,65535UL,0x6044L},{9UL,1UL,65526UL,1UL},{9UL,0xA316L,65535UL,3UL},{0x6044L,1UL,0x6B89L,0x6B89L},{65528UL,65528UL,0x8FE2L,1UL}},{{1UL,0x6044L,65533UL,1UL},{0xA316L,9UL,0x6B89L,65533UL},{1UL,9UL,1UL,65535UL},{65533UL,65529UL,8UL,0x6B89L},{0xD218L,65530UL,0xD218L,0x8FE2L},{65535UL,0x6B89L,1UL,65533UL},{65530UL,8UL,65526UL,0x6B89L},{0x0940L,65535UL,65526UL,65529UL},{65530UL,65533UL,1UL,0xA316L}},{{65535UL,0xD218L,0xD218L,65535UL},{0xD218L,65535UL,8UL,0x0940L},{65533UL,65530UL,1UL,65533UL},{65535UL,0x0940L,0x8FE2L,65533UL},{8UL,65530UL,65526UL,0x0940L},{0x6B89L,65535UL,65528UL,65535UL},{65530UL,0xD218L,0x8FE2L,0xA316L},{65529UL,65533UL,0xD218L,65529UL},{65533UL,65535UL,2UL,0x6B89L}},{{65533UL,8UL,0xD218L,65533UL},{65529UL,0x6B89L,0x8FE2L,0x8FE2L},{65530UL,65530UL,65528UL,0x6B89L},{0x6B89L,65529UL,65526UL,65535UL},{8UL,65533UL,0x8FE2L,65526UL},{65535UL,65533UL,1UL,65535UL},{65533UL,65529UL,8UL,0x6B89L},{0xD218L,65530UL,0xD218L,0x8FE2L},{65535UL,0x6B89L,1UL,65533UL}},{{65530UL,8UL,65526UL,0x6B89L},{0x0940L,65535UL,65526UL,65529UL},{65530UL,65533UL,1UL,0xA316L},{65535UL,0xD218L,0xD218L,65535UL},{0xD218L,65535UL,8UL,0x0940L},{65533UL,65530UL,1UL,65533UL},{65535UL,0x0940L,0x8FE2L,65533UL},{8UL,65530UL,65526UL,0x0940L},{0x6B89L,65535UL,65528UL,65535UL}},{{65530UL,0xD218L,0x8FE2L,0xA316L},{65529UL,65533UL,0xD218L,65529UL},{65533UL,65535UL,2UL,0x6B89L},{65533UL,8UL,0xD218L,65533UL},{65529UL,0x6B89L,0x8FE2L,0x8FE2L},{65530UL,65530UL,65528UL,0x6B89L},{0x6B89L,65529UL,65526UL,65535UL},{8UL,65533UL,0x8FE2L,65526UL},{65535UL,65533UL,1UL,65535UL}}};
                        int32_t l_986 = 0xC8415F0CL;
                        int32_t l_987 = 0xF3237787L;
                        int32_t l_988[9][9] = {{0xFCB13D26L,9L,1L,1L,0xCF4F51E5L,9L,0xFCB13D26L,9L,0xCF4F51E5L},{0xADE12816L,0x77823DC1L,0x77823DC1L,0xADE12816L,0x91066FC0L,0x7A76C37FL,0xADE12816L,0x7A76C37FL,0x91066FC0L},{0xFCB13D26L,9L,0xFD5C9457L,9L,0x0B1DBE8CL,0x76D8D06BL,1L,0x76D8D06BL,0x0B1DBE8CL},{0x77823DC1L,0L,0L,0x77823DC1L,0x1027181EL,0xEA8EFD78L,0x77823DC1L,0xEA8EFD78L,0x1027181EL},{1L,(-4L),0xFD5C9457L,9L,0x0B1DBE8CL,0x76D8D06BL,1L,0x76D8D06BL,0x0B1DBE8CL},{0x77823DC1L,0L,0L,0x77823DC1L,0x1027181EL,0xEA8EFD78L,0x77823DC1L,0xEA8EFD78L,0x1027181EL},{1L,(-4L),0xFD5C9457L,9L,0x0B1DBE8CL,0x76D8D06BL,1L,0x76D8D06BL,0x0B1DBE8CL},{0x77823DC1L,0L,0L,0x77823DC1L,0x1027181EL,0xEA8EFD78L,0x77823DC1L,0xEA8EFD78L,0x1027181EL},{1L,(-4L),0xFD5C9457L,9L,0x0B1DBE8CL,0x76D8D06BL,1L,0x76D8D06BL,0x0B1DBE8CL}};
                        int i, j, k;
                        (****g_925) = (void*)0;
                        if (p_42)
                            break;
                        l_985[2][1][1] |= (0xCBL >= ((p_42 | p_42) < 18446744073709551615UL));
                        --l_1008;
                    }
                }
                else
                {
                    int32_t l_1011[8][9][3] = {{{(-6L),0L,0xB5CD281EL},{0x3B393EE1L,0x90E6F5BBL,0x6EBCDA95L},{1L,0xB5CD281EL,0xB5CD281EL},{(-2L),9L,0x36AFC1CDL},{0x10BBB863L,0x806CE959L,1L},{(-2L),0x6EBCDA95L,8L},{1L,0x7BFC7C85L,0xE6228FF4L},{0x3B393EE1L,0x6EBCDA95L,9L},{(-6L),0x806CE959L,0L}},{{1L,9L,9L},{0xCA37641AL,0xB5CD281EL,0xE6228FF4L},{4L,0x90E6F5BBL,8L},{0xCA37641AL,0L,1L},{1L,0x3F73AD4EL,0x36AFC1CDL},{(-6L),0L,0xB5CD281EL},{0x3B393EE1L,0x90E6F5BBL,0x6EBCDA95L},{1L,0xB5CD281EL,0xB5CD281EL},{(-2L),9L,0x36AFC1CDL}},{{0x10BBB863L,0x806CE959L,1L},{(-2L),0x6EBCDA95L,8L},{1L,0x7BFC7C85L,0xE6228FF4L},{0x3B393EE1L,0x6EBCDA95L,9L},{(-6L),0x806CE959L,0L},{1L,9L,9L},{0xCA37641AL,0xB5CD281EL,0xE6228FF4L},{4L,0x90E6F5BBL,8L},{0xCA37641AL,0L,1L}},{{1L,0x3F73AD4EL,0x36AFC1CDL},{(-6L),0L,0xB5CD281EL},{0x3B393EE1L,0x90E6F5BBL,0x6EBCDA95L},{1L,0xB5CD281EL,0xB5CD281EL},{(-2L),9L,0x36AFC1CDL},{0x10BBB863L,0x806CE959L,1L},{(-2L),0x6EBCDA95L,8L},{1L,0x7BFC7C85L,0xE6228FF4L},{0x3B393EE1L,0x6EBCDA95L,9L}},{{(-6L),0x806CE959L,0L},{1L,9L,9L},{0xCA37641AL,0xB5CD281EL,0xE6228FF4L},{4L,0x90E6F5BBL,8L},{0xCA37641AL,0L,1L},{1L,0x3F73AD4EL,0x36AFC1CDL},{(-6L),0L,0xB5CD281EL},{0x3B393EE1L,0x38178BA9L,(-1L)},{0xB5CD281EL,0xC757E82BL,0xC757E82BL}},{{0x0D38A207L,0x20DF789BL,0x83AC685EL},{0xE6228FF4L,(-1L),8L},{0x0D38A207L,(-1L),(-1L)},{0xB5CD281EL,0x69DB4274L,0x079059CDL},{0x3F73AD4EL,(-1L),0x20DF789BL},{0x7BFC7C85L,(-1L),0xA3DDD031L},{9L,0x20DF789BL,0x20DF789BL},{0x73401A9BL,0xC757E82BL,0x079059CDL},{0x36AFC1CDL,0x38178BA9L,(-1L)}},{{0x73401A9BL,0xA3DDD031L,8L},{9L,0x77E49CF3L,0x83AC685EL},{0x7BFC7C85L,0xA3DDD031L,0xC757E82BL},{0x3F73AD4EL,0x38178BA9L,(-1L)},{0xB5CD281EL,0xC757E82BL,0xC757E82BL},{0x0D38A207L,0x20DF789BL,0x83AC685EL},{0xE6228FF4L,(-1L),8L},{0x0D38A207L,(-1L),(-1L)},{0xB5CD281EL,0x69DB4274L,0x079059CDL}},{{0x3F73AD4EL,(-1L),0x20DF789BL},{0x7BFC7C85L,(-1L),0xA3DDD031L},{9L,0x20DF789BL,0x20DF789BL},{0x73401A9BL,0xC757E82BL,0x079059CDL},{0x36AFC1CDL,0x38178BA9L,(-1L)},{0x73401A9BL,0xA3DDD031L,8L},{9L,0x77E49CF3L,0x83AC685EL},{0x7BFC7C85L,0xA3DDD031L,0xC757E82BL},{0x3F73AD4EL,0x38178BA9L,(-1L)}}};
                    int i, j, k;
                    l_1016--;
                    if ((*g_550))
                        continue;
                }
            }
        }
        for (g_69 = 0; (g_69 <= (-8)); g_69--)
        {
            int8_t l_1175 = 0L;
            int32_t l_1177 = (-6L);
            uint32_t *l_1188 = (void*)0;
            int32_t l_1190 = 0x274EAE68L;
            (*l_1084) = l_1163;
            l_1177 |= (safe_add_func_int32_t_s_s((safe_mul_func_int32_t_s_s(p_42, (((l_1168 == l_1168) ^ (3UL & (((**g_532) || ((*g_370) > (safe_sub_func_int16_t_s_s(p_42, ((((safe_rshift_func_int8_t_s_s(((**l_1098) &= ((safe_lshift_func_int16_t_s_s(((l_1175 > ((((~l_1175) != (*g_370)) <= (*g_370)) , g_2)) , p_42), p_42)) , p_42)), 3)) <= g_894[4]) && p_42) || l_1175))))) , (*l_1163)))) > 0x942FE7C2516DA5EBLL))), p_42));
            l_1190 = ((0x35F7L & (((*l_1022) = ((g_1180[2] = l_1178) == (((((p_42 ^ (((((l_1177 = (p_42 || ((*l_1163) = ((safe_div_func_int16_t_s_s(((*g_502) ^= (&g_532 == (void*)0)), (-10L))) != (((safe_mul_func_uint16_t_u_u(p_42, ((((((safe_mod_func_uint16_t_u_u((p_42 , l_1177), g_1012[7])) , (-1L)) ^ p_42) == (-9L)) , (void*)0) == l_1188))) , p_41) != (void*)0))))) , (*p_41)) && 18446744073709551615UL) && 0x4EL) >= l_1189)) | 0x44L) != p_42) , l_1177) , l_1168))) || g_1012[7])) , 0x5A405336L);
            (*g_227) = &l_1190;
        }
    }
    return g_922;
}







static uint32_t func_44(int16_t p_45, uint64_t * p_46, uint64_t * p_47)
{
    int16_t l_647 = 9L;
    int32_t l_648 = 1L;
    int32_t l_649 = 0xC28DB288L;
    int32_t l_650[6] = {0x380EE092L,0x380EE092L,0x380EE092L,0x380EE092L,0x380EE092L,0x380EE092L};
    uint32_t l_670 = 0x6BCE681EL;
    uint32_t ***l_689 = (void*)0;
    int32_t l_692 = 0x264366C1L;
    int16_t *l_702 = &g_15[0];
    int32_t l_744 = (-4L);
    uint32_t l_763 = 4294967290UL;
    const int32_t ****l_828 = &g_381;
    int32_t l_832 = 0x4AEC63EAL;
    uint32_t l_864 = 1UL;
    uint32_t l_866 = 0xEB0712D3L;
    int32_t l_883 = 8L;
    uint32_t *l_915[10][10] = {{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864},{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864},{&l_864,&l_864,(void*)0,&l_864,&l_864,(void*)0,&l_864,&l_864,(void*)0,&l_864},{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864},{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864},{&l_864,&l_864,(void*)0,&l_864,&l_864,(void*)0,&l_864,&l_864,(void*)0,&l_864},{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864},{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864},{&l_864,&l_864,(void*)0,&l_864,&l_864,(void*)0,&l_864,&l_864,(void*)0,&l_864},{&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864,&l_864}};
    uint32_t **l_914 = &l_915[8][4];
    uint32_t ***l_913 = &l_914;
    int8_t *l_952 = (void*)0;
    int8_t *l_955 = (void*)0;
    int i, j;
    (*g_196) = 1L;
    for (g_330 = 0; (g_330 <= 8); g_330 += 1)
    {
        int32_t *l_646[9] = {&g_62,&g_62,&g_62,&g_62,&g_62,&g_62,&g_62,&g_62,&g_62};
        int32_t l_651[1][10] = {{(-7L),(-7L),0xF40B181DL,(-1L),0xF40B181DL,(-7L),(-7L),0xF40B181DL,(-1L),0xF40B181DL}};
        uint8_t l_652 = 0x4FL;
        uint16_t l_655 = 0x5C13L;
        uint32_t *l_709 = &g_285;
        uint32_t ** const l_708 = &l_709;
        uint32_t ** const * const l_707[7][5] = {{&l_708,(void*)0,(void*)0,(void*)0,&l_708},{(void*)0,(void*)0,&l_708,&l_708,&l_708},{&l_708,&l_708,(void*)0,&l_708,&l_708},{(void*)0,(void*)0,(void*)0,(void*)0,&l_708},{(void*)0,&l_708,&l_708,&l_708,&l_708},{&l_708,(void*)0,&l_708,(void*)0,(void*)0},{(void*)0,&l_708,(void*)0,&l_708,(void*)0}};
        uint64_t *l_778 = &g_645;
        int16_t l_831 = 0x0EA5L;
        int16_t **l_834 = &l_702;
        int32_t **** const l_930 = &g_226;
        int32_t **** const *l_929 = &l_930;
        int i, j;
        ++l_652;
        --l_655;
    }
    return l_649;
}







static uint64_t * func_48(const uint64_t * p_49, uint64_t * p_50, uint16_t p_51, int16_t p_52)
{
    int8_t l_219 = (-1L);
    int32_t **l_224 = &g_75[1][0][0];
    int32_t ***l_223[9] = {&l_224,&l_224,&l_224,&l_224,&l_224,&l_224,&l_224,&l_224,&l_224};
    int32_t ****l_222[10] = {(void*)0,&l_223[0],(void*)0,&l_223[0],(void*)0,&l_223[0],(void*)0,&l_223[0],(void*)0,&l_223[0]};
    uint16_t *l_228 = (void*)0;
    uint16_t *l_229[5][1][5] = {{{(void*)0,&g_187,&g_171,&g_187,(void*)0}},{{&g_187,&g_171,&g_187,&g_187,&g_171}},{{(void*)0,(void*)0,&g_171,&g_187,&g_171}},{{&g_171,&g_171,&g_171,&g_171,&g_171}},{{&g_171,&g_187,&g_171,(void*)0,(void*)0}}};
    uint64_t **l_252 = &g_218;
    uint8_t l_263 = 0UL;
    int32_t l_319 = 0x5483BA15L;
    uint64_t l_333 = 18446744073709551611UL;
    int32_t l_352 = 0x098E4881L;
    uint16_t l_353 = 0x00D3L;
    uint32_t l_383 = 0x1EF2F5BEL;
    uint8_t l_474 = 0xC2L;
    uint32_t l_489 = 0x1B8B4179L;
    const int32_t l_545 = 0L;
    int i, j, k;
    if ((((g_69 != (g_187 < (((g_217[2] != &g_218) & l_219) >= (safe_mul_func_int16_t_s_s(p_52, 0x6F07L))))) < (g_171 = ((g_226 = (g_225 = (void*)0)) == (void*)0))) > (*g_218)))
    {
        uint16_t l_246 = 1UL;
        int32_t l_247[8][2] = {{9L,0xC724748EL},{9L,9L},{0xC724748EL,9L},{9L,0xC724748EL},{9L,9L},{0xC724748EL,9L},{9L,0xC724748EL},{9L,9L}};
        uint8_t *l_248[3];
        int32_t l_249 = 0x316402E3L;
        uint8_t l_288 = 248UL;
        uint64_t *l_291 = &g_28[5][1];
        uint64_t ***l_292 = &g_217[2];
        int32_t **l_318 = &g_75[1][1][0];
        int8_t l_322 = 0xDEL;
        int32_t l_332[4];
        uint32_t l_339 = 0x9F79172DL;
        int16_t l_349 = 0xCE5EL;
        int i, j;
        for (i = 0; i < 3; i++)
            l_248[i] = &g_120;
        for (i = 0; i < 4; i++)
            l_332[i] = 0x874800EBL;
        if (((*p_50) , (((safe_lshift_func_uint8_t_u_u((l_249 = (safe_sub_func_int8_t_s_s((safe_div_func_int8_t_s_s((((g_15[0] >= ((safe_div_func_uint64_t_u_u((safe_mod_func_uint32_t_u_u(g_171, g_15[6])), (*g_218))) == (safe_sub_func_int32_t_s_s((l_247[2][1] |= ((safe_div_func_int32_t_s_s((p_51 , ((safe_unary_minus_func_uint64_t_u(g_245)) ^ 0x8D2CL)), (((0L < l_246) >= p_51) || g_15[0]))) , p_52)), l_246)))) == 0xFE36482BL) | l_246), 0x16L)), g_15[0]))), 4)) & g_124) & 0L)))
        {
            uint64_t **l_253[7][5] = {{&g_218,&g_218,&g_218,&g_218,&g_218},{&g_218,&g_218,&g_218,&g_218,&g_218},{(void*)0,&g_218,(void*)0,&g_218,&g_218},{&g_218,&g_218,&g_218,&g_218,&g_218},{&g_218,&g_218,&g_218,&g_218,&g_218},{&g_218,&g_218,(void*)0,&g_218,(void*)0},{&g_218,(void*)0,&g_218,&g_218,(void*)0}};
            uint32_t *l_282 = (void*)0;
            uint32_t *l_283 = (void*)0;
            uint32_t *l_284 = &g_285;
            int16_t *l_286 = &g_15[5];
            int32_t l_287 = 0xA94D3F08L;
            int32_t ***l_307 = &g_227;
            int8_t l_309 = 0x7BL;
            int64_t *l_329 = &g_330;
            int32_t *****l_331 = &l_222[8];
            int i, j;
            (****g_193) = ((safe_rshift_func_int8_t_s_u(g_28[8][2], (g_120 = ((l_252 == l_253[5][2]) == (safe_lshift_func_int32_t_s_u((safe_unary_minus_func_uint16_t_u(((safe_mod_func_int8_t_s_s((safe_mod_func_int32_t_s_s((****g_193), (safe_lshift_func_int8_t_s_s(p_51, (l_263 != (safe_rshift_func_int8_t_s_u(((l_288 = (safe_rshift_func_int16_t_s_s(((safe_div_func_int64_t_s_s(((safe_mul_func_int16_t_s_s(((*l_286) ^= (safe_mod_func_int8_t_s_s(((((*l_284) &= ((((void*)0 == &g_225) || ((safe_sub_func_uint8_t_u_u(((((safe_div_func_int32_t_s_s((safe_sub_func_uint32_t_u_u((safe_lshift_func_int16_t_s_s(0xF78AL, 5)), p_52)), p_51)) | (-8L)) || 0xC94A2A389E9D286BLL) , p_52), g_28[2][8])) , l_249)) < p_51)) <= p_51) & g_69), l_247[3][1]))), 1L)) ^ l_249), g_28[3][7])) , l_287), p_51))) != l_247[2][1]), 1))))))), g_62)) ^ 0UL))), 14)))))) || 0L);
            if (l_287)
            {
                uint64_t ** const l_289 = &g_218;
                uint64_t ***l_290 = &g_217[2];
                (*l_290) = l_289;
                return l_291;
            }
            else
            {
                int64_t l_308[6][7] = {{0x91E0ECAFB3B25368LL,1L,1L,1L,0x91E0ECAFB3B25368LL,0x53A0D33AFCE1F6F2LL,0xEA0178CA02759C4DLL},{0x5694DDA16972C5E3LL,0x64DD62C0C1F5A1C2LL,0x800197DB5B27B66ELL,0x71850E58691E9792LL,0x91E0ECAFB3B25368LL,0xEA0178CA02759C4DLL,0x91E0ECAFB3B25368LL},{(-9L),0L,0L,(-9L),1L,0x71850E58691E9792LL,0x5694DDA16972C5E3LL},{0x5694DDA16972C5E3LL,0x71850E58691E9792LL,1L,(-9L),0L,0L,(-9L)},{0x91E0ECAFB3B25368LL,0xEA0178CA02759C4DLL,0x91E0ECAFB3B25368LL,0x71850E58691E9792LL,0x800197DB5B27B66ELL,0x64DD62C0C1F5A1C2LL,0x5694DDA16972C5E3LL},{0xEA0178CA02759C4DLL,0x53A0D33AFCE1F6F2LL,0x91E0ECAFB3B25368LL,1L,1L,0xEA0178CA02759C4DLL,0x64DD62C0C1F5A1C2LL}};
                int32_t l_310 = 0x1C36ACF2L;
                volatile int16_t ***l_314 = &g_311;
                int i, j;
                l_249 &= ((g_124 != ((p_51 , &l_252) == l_292)) == (p_51 || ((safe_rshift_func_int8_t_s_s(((l_310 |= (safe_mul_func_uint8_t_u_u(((safe_div_func_uint8_t_u_u((l_309 &= ((safe_mul_func_uint8_t_u_u((safe_lshift_func_int8_t_s_u((safe_div_func_int8_t_s_s((((((safe_div_func_uint8_t_u_u((g_120 = (((l_307 != l_307) || ((g_285 & (g_171 = g_245)) < p_52)) == g_285)), g_15[0])) , p_52) , (void*)0) == &p_51) > p_51), 0x7EL)), l_308[3][2])), (-8L))) || (****g_193))), g_62)) <= (*p_50)), g_124))) == g_117), l_308[3][2])) ^ p_52)));
                (*l_314) = g_311;
            }
            (****g_193) = (+((((0xCE8B87F2F1D17CBELL ^ (safe_mod_func_int64_t_s_s(((*l_329) &= ((l_318 != l_318) > (l_319 , ((safe_add_func_uint64_t_u_u((((**l_252) = l_309) != l_322), p_51)) , (safe_add_func_int8_t_s_s((safe_div_func_int32_t_s_s(((((safe_mul_func_uint32_t_u_u(p_51, (&l_318 == (void*)0))) != g_5) <= 0x9CL) == p_52), p_52)), p_52)))))), 0x7314760B1892D0D0LL))) ^ g_313) , l_331) != &g_193));
        }
        else
        {
            (*g_196) = 1L;
        }
        ++l_333;
        (*g_196) = ((safe_mod_func_uint8_t_u_u((~((l_339 >= ((safe_mod_func_uint16_t_u_u((((l_229[0][0][4] != (((safe_mul_func_uint32_t_u_u(g_171, 1L)) , (l_332[2] = (safe_mod_func_int64_t_s_s((safe_sub_func_uint8_t_u_u((!g_5), 0L)), (-10L))))) , (((((**g_311) != p_51) >= g_28[3][1]) < 0xC73BL) , (void*)0))) || l_349) , 65528UL), p_51)) == 0x24DEL)) == (*g_218))), 0xDEL)) < g_5);
    }
    else
    {
        int16_t l_350 = 0L;
        int16_t l_351 = 0x28F9L;
        int32_t l_387[6][1];
        uint16_t *l_403[7][5] = {{(void*)0,&l_353,&l_353,&l_353,(void*)0},{&g_171,&g_171,(void*)0,&g_171,&g_171},{(void*)0,&l_353,&l_353,&l_353,(void*)0},{&g_171,&g_171,(void*)0,&g_171,&g_171},{(void*)0,&l_353,&l_353,&l_353,(void*)0},{&g_171,&g_171,(void*)0,&g_171,&g_171},{(void*)0,&l_353,&l_353,&l_353,(void*)0}};
        uint32_t l_426 = 0x353CE2C3L;
        int32_t l_460[4][5][6] = {{{(-7L),0L,(-3L),0x3E965E27L,(-7L),(-7L)},{0xDF328BAFL,0x3E965E27L,0x3E965E27L,0xDF328BAFL,(-5L),0L},{0xDF328BAFL,(-5L),0L,0x3E965E27L,0L,0xDF328BAFL},{(-7L),0x16BE7BD5L,0x3E965E27L,9L,0L,0x16BE7BD5L},{0L,(-5L),(-3L),(-3L),(-5L),0L}},{{(-7L),0x3E965E27L,(-3L),0L,(-7L),0x16BE7BD5L},{0xDF328BAFL,0L,0x3E965E27L,0L,(-5L),0xDF328BAFL},{0xDF328BAFL,0x0911154BL,0L,0L,0L,0L},{(-7L),(-7L),0x3E965E27L,(-3L),0L,(-7L)},{0L,0x0911154BL,(-3L),9L,(-5L),0x3E965E27L}},{{(-7L),0L,(-3L),0x3E965E27L,(-7L),(-7L)},{0xDF328BAFL,0x3E965E27L,0x3E965E27L,0xDF328BAFL,(-5L),0L},{0xDF328BAFL,(-5L),0L,0x3E965E27L,0L,0xDF328BAFL},{(-7L),0x16BE7BD5L,0x3E965E27L,9L,0L,0x16BE7BD5L},{0L,(-5L),(-3L),(-3L),(-5L),0L}},{{(-7L),0x3E965E27L,(-3L),0L,(-7L),0x16BE7BD5L},{0xDF328BAFL,0L,0x3E965E27L,0L,(-5L),0xDF328BAFL},{0xDF328BAFL,0x0911154BL,0L,0L,0L,0L},{(-7L),(-7L),0x3E965E27L,(-3L),0L,(-7L)},{0L,0x0911154BL,(-3L),9L,(-5L),0x3E965E27L}}};
        int16_t **l_503 = &g_502;
        uint64_t *l_521 = &g_28[2][4];
        uint8_t l_530 = 0x48L;
        int32_t **l_541 = &g_75[2][0][2];
        uint8_t l_622 = 0x41L;
        int i, j, k;
        for (i = 0; i < 6; i++)
        {
            for (j = 0; j < 1; j++)
                l_387[i][j] = 0xC922BD29L;
        }
        ++l_353;
        (****g_193) = (-1L);
        for (l_351 = 0; (l_351 <= 1); l_351 += 1)
        {
            uint64_t l_359 = 0xEB88B96E72EE3003LL;
            const int32_t *l_378 = &g_62;
            int32_t l_385 = 0xBEBC1D7FL;
            if (p_52)
                break;
            for (l_319 = 1; (l_319 >= 0); l_319 -= 1)
            {
                uint8_t *l_367 = &l_263;
                const int32_t **l_380 = &l_378;
                const int32_t ***l_379[7][6][6] = {{{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380}},{{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380}},{{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380}},{{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380}},{{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380}},{{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380}},{{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380},{(void*)0,(void*)0,&l_380,&l_380,&l_380,&l_380},{(void*)0,&l_380,&l_380,(void*)0,&l_380,&l_380},{&l_380,(void*)0,&l_380,&l_380,(void*)0,&l_380},{&l_380,&l_380,&l_380,(void*)0,(void*)0,&l_380}}};
                int8_t *l_384[3][3][2] = {{{&g_117,&g_117},{&l_219,&g_117},{&g_117,&g_124}},{{&g_117,&l_219},{&l_219,&g_117},{&l_219,&g_124}},{{&l_219,&g_117},{&l_219,&l_219},{&g_117,&g_124}}};
                uint8_t *l_386 = &g_120;
                int32_t *l_388 = &l_387[5][0];
                int i, j, k;
                l_387[5][0] |= (safe_unary_minus_func_uint8_t_u(((*l_386) = ((safe_lshift_func_uint8_t_u_s(l_359, g_117)) == (l_385 = (safe_div_func_uint16_t_u_u((!((((--p_51) & (safe_lshift_func_uint8_t_u_u(((*l_367)--), g_330))) == ((((**g_311) , p_50) != g_370) , ((l_359 && (safe_mod_func_int8_t_s_s((~(safe_sub_func_uint64_t_u_u(((g_381 = (((safe_rshift_func_uint32_t_u_u((((void*)0 != l_378) ^ (*l_378)), (*l_378))) > 7L) , l_379[1][2][4])) == &l_380), 18446744073709551608UL))), l_383))) > l_351))) & 1UL)), l_350)))))));
                for (g_285 = 0; (g_285 <= 1); g_285 += 1)
                {
                    int i, j, k;
                    if ((*l_378))
                        break;
                    g_75[(l_351 + 2)][l_319][l_351] = l_388;
                }
                return p_50;
            }
        }
        for (l_333 = 0; (l_333 <= 0); l_333 += 1)
        {
            uint8_t l_389 = 9UL;
            int32_t *l_408 = (void*)0;
            int16_t l_409 = 1L;
            int32_t l_411 = (-6L);
            int32_t l_413 = 6L;
            int32_t l_416 = (-8L);
            int32_t l_422 = 0xDBF1C60DL;
            int32_t l_425 = 7L;
            uint32_t l_546 = 0x38D38D20L;
            uint8_t l_582 = 5UL;
            int32_t **l_631 = &l_408;
            if (l_389)
                break;
            for (l_353 = 0; (l_353 <= 0); l_353 += 1)
            {
                int8_t *l_394 = &g_117;
                uint16_t **l_404 = &l_403[0][3];
                uint64_t **l_406 = &g_218;
                uint64_t ***l_407 = &g_217[0];
                const int32_t l_410 = 0x322A0877L;
                int32_t l_414 = 0L;
                int32_t l_415 = (-2L);
                int32_t l_417 = 0L;
                int32_t l_424 = 0x609439A7L;
                int16_t * const l_456 = (void*)0;
                int16_t * const *l_455[2];
                int16_t * const **l_454 = &l_455[1];
                int64_t l_463 = (-7L);
                int16_t *l_499 = &g_15[0];
                int16_t **l_498 = &l_499;
                const uint64_t *l_523[10] = {(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0};
                const uint64_t **l_522 = &l_523[9];
                const uint64_t *l_525 = &g_526;
                const uint32_t *l_528 = (void*)0;
                const uint32_t **l_527 = &l_528;
                uint8_t *l_529 = &l_474;
                int32_t *l_549 = &g_62;
                volatile int32_t * volatile l_551[6];
                int32_t l_569 = 0x7DFEA501L;
                int i;
                for (i = 0; i < 2; i++)
                    l_455[i] = &l_456;
                for (i = 0; i < 6; i++)
                    l_551[i] = &g_197;
            }
            (*g_196) = (*g_550);
            for (l_416 = 0; (l_416 >= 0); l_416 -= 1)
            {
                int8_t l_575 = (-1L);
                int32_t l_581[5][5][9];
                int32_t * const *l_601 = &l_408;
                uint32_t *l_602 = &l_426;
                uint64_t *l_643 = (void*)0;
                int i, j, k;
                for (i = 0; i < 5; i++)
                {
                    for (j = 0; j < 5; j++)
                    {
                        for (k = 0; k < 9; k++)
                            l_581[i][j][k] = 0L;
                    }
                }
                l_582 |= (l_581[3][1][8] |= (safe_mul_func_int64_t_s_s((0x2284L && ((p_52 , ((l_575 == ((l_575 >= (((void*)0 != &l_223[0]) , (0x97L >= (((((+(((safe_rshift_func_int8_t_s_s((0xBBAFL & ((*p_50) <= (*p_50))), g_197)) == (-1L)) && (-1L))) > 0x7B53206B796BEC2ELL) > p_51) >= 252UL) > p_52)))) < 0x1265L)) , l_575)) > 0x49BAD742L)), p_52)));
                (****g_193) &= (p_52 , (l_387[3][0] &= 1L));
                for (l_530 = 0; (l_530 <= 0); l_530 += 1)
                {
                    int32_t * const l_599 = &g_600;
                    int32_t * const *l_598[2];
                    int32_t * const **l_597 = &l_598[0];
                    int i;
                    for (i = 0; i < 2; i++)
                        l_598[i] = &l_599;
                    (****g_193) = (safe_sub_func_int32_t_s_s((p_51 && (safe_lshift_func_int64_t_s_s(0x03CEC6730B2F04F7LL, 49))), ((((**l_252)--) , p_52) , (((safe_lshift_func_int32_t_s_s((((*l_597) = ((l_581[3][1][8] < (!((safe_div_func_uint8_t_u_u((safe_unary_minus_func_uint8_t_u((((0x94L && (0x6FABL == 4L)) , (*l_541)) == (void*)0))), g_124)) && 1UL))) , (void*)0)) == l_601), l_422)) , l_408) == l_602))));
                    for (g_600 = 0; (g_600 <= 0); g_600 += 1)
                    {
                        if (p_52)
                            break;
                        (****g_193) = (-9L);
                    }
                    if (((safe_mul_func_int8_t_s_s(((((*g_370) = (safe_mul_func_int64_t_s_s(p_52, (safe_sub_func_uint32_t_u_u((safe_sub_func_uint8_t_u_u((safe_sub_func_int16_t_s_s(((*g_502) = 0x8860L), (((((void*)0 == p_50) <= 0x39F3L) == (safe_add_func_uint64_t_u_u((safe_div_func_int64_t_s_s(0x9D0033823C9C4A55LL, (~(safe_div_func_int16_t_s_s((**g_311), p_52))))), (((safe_mod_func_uint16_t_u_u(((p_52 > 1L) >= g_600), p_52)) && (*g_218)) , l_622)))) || g_187))), 253UL)), 3L))))) > (*p_50)) > 65534UL), p_52)) , p_52))
                    {
                        int8_t l_636 = 0L;
                        int32_t l_637 = (-2L);
                        int32_t l_638 = (-1L);
                        int32_t l_639[6] = {(-1L),0L,0L,(-1L),0L,0L};
                        uint32_t l_640 = 4UL;
                        int i;
                        (*g_196) = ((*l_599) = (safe_sub_func_uint32_t_u_u(p_51, (((((*g_370) = (1UL == (safe_div_func_int32_t_s_s((*g_196), p_52)))) >= (safe_add_func_uint8_t_u_u(0xDDL, p_52))) < (safe_sub_func_uint32_t_u_u((((((*g_381) == (l_631 = l_541)) <= (((safe_sub_func_uint32_t_u_u(((++g_171) != p_52), p_51)) ^ 0UL) && 18446744073709551615UL)) , l_636) >= 0L), p_52))) & p_52))));
                        --l_640;
                    }
                    else
                    {
                        return l_643;
                    }
                }
            }
        }
    }
    (****g_193) = (-1L);
    return (*l_252);
}







static const uint64_t * func_53(uint64_t * p_54, uint64_t * p_55)
{
    int32_t **l_87 = &g_75[2][0][2];
    int32_t l_96 = 0L;
    uint8_t l_121 = 0x28L;
    int32_t l_188 = 1L;
    int32_t l_203 = 0L;
    int32_t l_205 = 0xD4D6F5E0L;
    int32_t l_206 = (-4L);
    int32_t l_208 = (-1L);
    const uint64_t *l_212 = &g_28[5][1];
    for (g_62 = 0; (g_62 != 5); g_62 = safe_add_func_uint16_t_u_u(g_62, 5))
    {
        uint16_t l_70 = 2UL;
        int32_t *l_73 = &g_62;
        int32_t ***l_93 = &l_87;
        const uint64_t *l_128 = (void*)0;
        uint32_t l_162 = 4294967288UL;
        int32_t ** const *l_164 = &l_87;
        int32_t ** const **l_163 = &l_164;
        int32_t l_199 = (-8L);
        int32_t l_202 = 0L;
        int32_t l_204[6] = {(-1L),(-1L),(-1L),(-1L),(-1L),(-1L)};
        int i;
        for (g_65 = 15; (g_65 <= 0); g_65 = safe_sub_func_int16_t_s_s(g_65, 4))
        {
            int32_t *l_68[10];
            int32_t **l_161 = &g_75[2][1][1];
            int16_t l_207 = 8L;
            int i;
            for (i = 0; i < 10; i++)
                l_68[i] = &g_69;
            --l_70;
            (*g_74) = l_73;
            for (l_70 = 0; (l_70 == 33); l_70++)
            {
                int32_t **l_89[2][5][5] = {{{&g_75[2][0][2],&l_73,&g_75[2][0][2],&g_75[2][0][2],&g_75[2][0][2]},{&l_73,&l_73,&g_75[2][0][2],&l_73,(void*)0},{&g_75[0][1][0],(void*)0,&g_75[2][0][2],&g_75[2][0][2],(void*)0},{(void*)0,&l_73,&g_75[1][0][3],(void*)0,&l_68[1]},{(void*)0,(void*)0,&l_68[2],(void*)0,(void*)0}},{{&g_75[1][0][3],&l_73,&l_73,&l_68[1],&l_73},{(void*)0,&l_73,&l_73,(void*)0,&g_75[2][0][2]},{(void*)0,&l_73,&g_75[2][0][2],&l_73,&l_73},{&g_75[0][1][0],(void*)0,&g_75[0][1][0],&g_75[2][0][2],(void*)0},{&l_73,&l_73,&l_68[1],&l_73,&l_68[1]}}};
                int32_t ***l_88 = &l_89[1][1][2];
                int32_t ****l_94 = (void*)0;
                int32_t ****l_95 = &l_88;
                int i, j, k;
                if ((safe_div_func_uint64_t_u_u(g_15[8], ((safe_add_func_int32_t_s_s((g_69 , (((safe_mul_func_int8_t_s_s(((!(((safe_sub_func_int64_t_s_s((l_87 == l_87), (((*p_55) ^= g_62) || ((((*l_88) = &l_73) == (void*)0) , (safe_rshift_func_uint64_t_u_u(((!g_15[1]) || (((*l_95) = l_93) != &l_87)), 29)))))) || g_5) && g_15[0])) | g_5), (*l_73))) < (*l_73)) , l_96)), (*l_73))) , (*l_73)))))
                {
                    uint64_t l_99[2];
                    uint64_t l_123 = 0UL;
                    int32_t l_125 = 0x6E94FA07L;
                    int i;
                    for (i = 0; i < 2; i++)
                        l_99[i] = 0x01C31E92403636CDLL;
                    if ((*l_73))
                    {
                        uint32_t l_108[10] = {8UL,8UL,18446744073709551615UL,8UL,8UL,18446744073709551615UL,8UL,8UL,18446744073709551615UL,8UL};
                        uint16_t l_116[5] = {9UL,9UL,9UL,9UL,9UL};
                        int32_t l_122[1][7][2] = {{{(-5L),0x25C8CE6CL},{0xD237583FL,0x25C8CE6CL},{(-5L),0xD237583FL},{0x5DA103C1L,0x5DA103C1L},{0x5DA103C1L,0xD237583FL},{(-5L),0x25C8CE6CL},{0xD237583FL,0x25C8CE6CL}}};
                        int i, j, k;
                        l_125 = (safe_sub_func_int64_t_s_s(1L, ((((void*)0 == p_55) > (((g_124 = ((l_99[0] , (safe_mod_func_int32_t_s_s((safe_div_func_uint8_t_u_u((safe_mul_func_int32_t_s_s((l_122[0][5][0] &= ((safe_mod_func_int32_t_s_s(l_108[3], ((safe_add_func_uint8_t_u_u(l_108[6], (((safe_rshift_func_uint8_t_u_u((+(((safe_add_func_uint32_t_u_u((g_117 ^= l_116[0]), ((l_99[0] != (((~((safe_unary_minus_func_int16_t_s(((((*l_73) , g_28[0][4]) <= l_99[1]) , 0xE095L))) >= 0xAD2EC94644A74709LL)) & (*p_54)) | g_120)) , 1UL))) ^ l_99[0]) < l_108[3])), g_69)) >= l_121) & g_62))) , 0xF4D78F30L))) | l_116[0])), g_62)), g_65)), l_123))) , 0xC284L)) >= l_99[0]) | 65529UL)) , 0x3F81597F862FE7C9LL)));
                    }
                    else
                    {
                        uint32_t l_126 = 0x0812B24AL;
                        int32_t l_127 = 0x3A4418BAL;
                        l_127 = l_126;
                    }
                    return l_128;
                }
                else
                {
                    int32_t *l_131 = &g_2;
                    int32_t l_132 = 5L;
                    int32_t ** const *l_160 = &l_89[1][1][2];
                    int32_t ** const ***l_165 = &l_163;
                    uint16_t *l_170 = &g_171;
                    uint16_t *l_186[1][8][6] = {{{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,&l_70,&l_70,&l_70,&l_70},{&l_70,&l_70,(void*)0,(void*)0,&l_70,(void*)0}}};
                    int32_t l_198 = (-2L);
                    int32_t l_200 = 0x830C1190L;
                    int32_t l_201[9];
                    uint8_t l_209 = 0x4FL;
                    int i, j, k;
                    for (i = 0; i < 9; i++)
                        l_201[i] = 0x33DA5EF5L;
                    for (l_96 = 0; (l_96 > 18); ++l_96)
                    {
                        uint64_t l_133 = 0x128F678E9A5F1CA1LL;
                        int32_t l_134[9] = {0L,0L,0L,0L,0L,0L,0L,0L,0L};
                        int i;
                        (*l_87) = (*g_74);
                        l_132 &= (((*g_74) = (**l_93)) != l_131);
                        l_134[1] ^= (l_133 |= (*l_131));
                        l_134[1] = (safe_rshift_func_int8_t_s_u((safe_sub_func_int64_t_s_s(((safe_mul_func_uint8_t_u_u((safe_sub_func_uint32_t_u_u(((((safe_add_func_uint64_t_u_u((g_28[7][7] || (((((safe_lshift_func_uint16_t_u_u(l_133, ((safe_mod_func_uint64_t_u_u(6UL, (((g_62 | ((((safe_rshift_func_uint16_t_u_s((((safe_mod_func_int16_t_s_s((safe_mod_func_int32_t_s_s((safe_unary_minus_func_int32_t_s((safe_div_func_int32_t_s_s((safe_add_func_uint64_t_u_u((g_65 == (l_160 == (void*)0)), (g_28[4][2] == ((l_161 = &l_131) == (void*)0)))), (*l_131))))), (*l_131))), l_133)) && l_134[1]) >= 252UL), 6)) , l_162) >= g_2) != l_134[8])) > g_2) , g_65))) ^ 0xA61F905A64256595LL))) , g_117) != g_28[5][1]) <= 0L) <= l_96)), g_15[1])) != g_62) , l_133) && 0x07FB85FAL), l_133)), g_5)) >= (*l_131)), (*p_55))), 1));
                    }
                    l_188 |= ((((*l_165) = l_163) != ((safe_mod_func_int16_t_s_s((safe_mul_func_uint16_t_u_u(((*l_170) ^= g_15[7]), g_124)), g_65)) , &l_88)) ^ (safe_rshift_func_int64_t_s_u((safe_div_func_int32_t_s_s(((g_120 , (safe_div_func_uint64_t_u_u((((*l_88) = &l_131) == (((((safe_mod_func_int16_t_s_s((safe_lshift_func_uint64_t_u_s((safe_rshift_func_uint16_t_u_u(((safe_mul_func_int8_t_s_s(0x8FL, (((--g_187) , ((safe_add_func_int64_t_s_s(g_28[0][1], 0x19CC642A9DD3993FLL)) , &l_164)) != g_193))) != l_121), g_120)), (*l_73))), g_15[0])) , g_65) , l_96) | 0UL) , (*g_194))), g_62))) <= g_15[0]), 0xC8352F65L)), 55)));
                    l_209--;
                }
                return &g_28[4][4];
            }
        }
        return &g_28[5][1];
    }
    return l_212;
}







static uint16_t func_56(const uint64_t * const p_57, uint64_t * p_58, uint32_t p_59, uint8_t p_60, const uint64_t * p_61)
{
    return g_2;
}





int main (int argc, char* argv[])
{
    int i, j, k;
    int print_hash_value = 0;
    if (argc == 2 && strcmp(argv[1], "1") == 0) print_hash_value = 1;
    platform_main_begin();
    crc32_gentab();
    func_1();
    transparent_crc(g_2, "g_2", print_hash_value);
    transparent_crc(g_5, "g_5", print_hash_value);
    for (i = 0; i < 10; i++)
    {
        transparent_crc(g_15[i], "g_15[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 9; j++)
        {
            transparent_crc(g_28[i][j], "g_28[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_62, "g_62", print_hash_value);
    transparent_crc(g_65, "g_65", print_hash_value);
    transparent_crc(g_69, "g_69", print_hash_value);
    transparent_crc(g_117, "g_117", print_hash_value);
    transparent_crc(g_120, "g_120", print_hash_value);
    transparent_crc(g_124, "g_124", print_hash_value);
    transparent_crc(g_171, "g_171", print_hash_value);
    transparent_crc(g_187, "g_187", print_hash_value);
    transparent_crc(g_197, "g_197", print_hash_value);
    transparent_crc(g_245, "g_245", print_hash_value);
    transparent_crc(g_285, "g_285", print_hash_value);
    transparent_crc(g_313, "g_313", print_hash_value);
    transparent_crc(g_330, "g_330", print_hash_value);
    transparent_crc(g_490, "g_490", print_hash_value);
    transparent_crc(g_524, "g_524", print_hash_value);
    transparent_crc(g_526, "g_526", print_hash_value);
    transparent_crc(g_534, "g_534", print_hash_value);
    transparent_crc(g_554, "g_554", print_hash_value);
    transparent_crc(g_600, "g_600", print_hash_value);
    transparent_crc(g_645, "g_645", print_hash_value);
    transparent_crc(g_715, "g_715", print_hash_value);
    transparent_crc(g_762, "g_762", print_hash_value);
    transparent_crc(g_849, "g_849", print_hash_value);
    for (i = 0; i < 5; i++)
    {
        transparent_crc(g_894[i], "g_894[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_919, "g_919", print_hash_value);
    transparent_crc(g_922, "g_922", print_hash_value);
    for (i = 0; i < 8; i++)
    {
        transparent_crc(g_1012[i], "g_1012[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_1088, "g_1088", print_hash_value);
    transparent_crc(g_1090, "g_1090", print_hash_value);
    transparent_crc(g_1125, "g_1125", print_hash_value);
    transparent_crc(g_1208, "g_1208", print_hash_value);
    transparent_crc(g_1480, "g_1480", print_hash_value);
    transparent_crc(g_1633, "g_1633", print_hash_value);
    transparent_crc(g_1690, "g_1690", print_hash_value);
    for (i = 0; i < 9; i++)
    {
        transparent_crc(g_1823[i], "g_1823[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_1860, "g_1860", print_hash_value);
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 5; j++)
        {
            transparent_crc(g_2019[i][j], "g_2019[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_2175, "g_2175", print_hash_value);
    transparent_crc(g_2268, "g_2268", print_hash_value);
    transparent_crc(g_2446, "g_2446", print_hash_value);
    platform_main_end(crc32_context ^ 0xFFFFFFFFUL, print_hash_value);
    return 0;
}
