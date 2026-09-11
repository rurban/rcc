/* csmith 1105826-fail_refmismatch_156: rcc -O0 miscompiled a chained
 * comparison whose rhs contained a nested outlined-call chain
 * (safe_lshift/safe_add/safe_mul GNU statement-expression helpers).
 * Two independent bugs combined to corrupt g_887's final value:
 *
 * 1. The generic scalar `<`/`<=`/`==`/`!=` codegen path evaluated
 *    `r_lhs = gen(node->lhs)` then `r_rhs = gen(node->rhs)`, trusting
 *    r_lhs's physical register to survive rhs's evaluation. Any
 *    mechanism that silently reuses r_lhs's register during rhs's
 *    evaluation without ever setting the `spilled_regs` bookkeeping
 *    bit (e.g. gen_funcall's argument-staging release, which clears
 *    `used_regs` directly) left the comparison reading a stranger's
 *    value, or -- when r_lhs's register got legitimately recycled for
 *    an unrelated constant later in the same rhs subtree -- comparing
 *    a register against itself (`cmp r15, r15`), which is always true
 *    for `<=`.
 *
 * 2. `%rsi` (one of rcc's 8 allocatable scratch registers) is also the
 *    SysV ABI's 2nd integer argument register. gen_funcall only
 *    protected it from clobbering when THIS call's own argument
 *    targeted rsi; any OTHER call (even a single-argument one) is
 *    still free to clobber rsi internally as ordinary caller-saved
 *    scratch, silently destroying an outer live value parked there.
 *
 * Fixed by (1) staging the comparison's lhs into a dedicated stack
 * slot and freeing its register before evaluating rhs -- only when
 * rhs's subtree can reach gen_funcall (node_may_call), to avoid
 * unconditionally growing every function's stack frame by one slot
 * per comparison -- then reloading into a register distinct from
 * rhs's; and (2) giving %rsi the same unconditional save-before/
 * restore-after treatment around every call that %r10/%r11 already
 * had, rather than only protecting it case-by-case as an argument
 * register.
 *
 * Without the fix, rcc's checksum diverged from gcc -O2's reference
 * checksum 0xB75F145A; asserted directly at the platform_main_end call
 * site below (the checksum print itself is architecture-neutral
 * csmith boilerplate, left untouched). Preprocessed (no csmith.h
 * dependency) so it builds without csmith installed, matching every
 * other test_csmith_*.c regression test. Uses plain printf (not
 * fprintf(stderr, ...)) for the failure message: stderr is a macro
 * expanding to a platform-specific accessor on Darwin/MinGW, not a
 * portable linkable symbol once baked into preprocessed output by a
 * Linux toolchain.
 */
# 0 "/tmp/test156_src_v2.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3
# 0 "<command-line>" 2
# 1 "/tmp/test156_src_v2.c"
# 10 "/tmp/test156_src_v2.c"
# 1 "/usr/local/include/csmith.h" 1 3
# 40 "/usr/local/include/csmith.h" 3
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/float.h" 1 3
# 41 "/usr/local/include/csmith.h" 2 3
# 1 "/usr/include/math.h" 1 3
# 27 "/usr/include/math.h" 3
# 1 "/usr/include/bits/libc-header-start.h" 1 3
# 33 "/usr/include/bits/libc-header-start.h" 3
# 1 "/usr/include/features.h" 1 3
# 431 "/usr/include/features.h" 3
# 1 "/usr/include/features-time64.h" 1 3
# 20 "/usr/include/features-time64.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 21 "/usr/include/features-time64.h" 2 3
# 1 "/usr/include/bits/timesize.h" 1 3
# 19 "/usr/include/bits/timesize.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 20 "/usr/include/bits/timesize.h" 2 3
# 22 "/usr/include/features-time64.h" 2 3
# 432 "/usr/include/features.h" 2 3
# 540 "/usr/include/features.h" 3
# 1 "/usr/include/sys/cdefs.h" 1 3
# 730 "/usr/include/sys/cdefs.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 731 "/usr/include/sys/cdefs.h" 2 3
# 1 "/usr/include/bits/long-double.h" 1 3
# 732 "/usr/include/sys/cdefs.h" 2 3
# 541 "/usr/include/features.h" 2 3
# 564 "/usr/include/features.h" 3
# 1 "/usr/include/gnu/stubs.h" 1 3
# 10 "/usr/include/gnu/stubs.h" 3
# 1 "/usr/include/gnu/stubs-64.h" 1 3
# 11 "/usr/include/gnu/stubs.h" 2 3
# 565 "/usr/include/features.h" 2 3
# 34 "/usr/include/bits/libc-header-start.h" 2 3
# 28 "/usr/include/math.h" 2 3









# 1 "/usr/include/bits/math-vector.h" 1 3
# 25 "/usr/include/bits/math-vector.h" 3
# 1 "/usr/include/bits/libm-simd-decl-stubs.h" 1 3
# 26 "/usr/include/bits/math-vector.h" 2 3
# 38 "/usr/include/math.h" 2 3


# 1 "/usr/include/bits/floatn.h" 1 3
# 131 "/usr/include/bits/floatn.h" 3
# 1 "/usr/include/bits/floatn-common.h" 1 3
# 24 "/usr/include/bits/floatn-common.h" 3
# 1 "/usr/include/bits/long-double.h" 1 3
# 25 "/usr/include/bits/floatn-common.h" 2 3
# 132 "/usr/include/bits/floatn.h" 2 3
# 41 "/usr/include/math.h" 2 3
# 157 "/usr/include/math.h" 3
# 1 "/usr/include/bits/flt-eval-method.h" 1 3
# 158 "/usr/include/math.h" 2 3
# 170 "/usr/include/math.h" 3

# 170 "/usr/include/math.h" 3
typedef float float_t;
typedef double double_t;
# 376 "/usr/include/math.h" 3
# 1 "/usr/include/bits/fp-logb.h" 1 3
# 377 "/usr/include/math.h" 2 3
# 419 "/usr/include/math.h" 3
# 1 "/usr/include/bits/fp-fast.h" 1 3
# 420 "/usr/include/math.h" 2 3



enum
  {
    FP_INT_UPWARD =

      0,
    FP_INT_DOWNWARD =

      1,
    FP_INT_TOWARDZERO =

      2,
    FP_INT_TONEARESTFROMZERO =

      3,
    FP_INT_TONEAREST =

      4,
  };


# 1 "/usr/include/bits/mathcalls-macros.h" 1 3
# 444 "/usr/include/math.h" 2 3





# 1 "/usr/include/bits/mathcalls-helper-functions.h" 1 3
# 20 "/usr/include/bits/mathcalls-helper-functions.h" 3
extern int __fpclassify (double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));


extern int __signbit (double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));



extern int __isinf (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __finite (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __isnan (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __iseqsig (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern int __issignaling (double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
# 450 "/usr/include/math.h" 2 3
# 1 "/usr/include/bits/mathcalls.h" 1 3
# 53 "/usr/include/bits/mathcalls.h" 3
 extern double acos (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __acos (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double asin (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __asin (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double atan (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atan (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double atan2 (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atan2 (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double cos (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cos (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double sin (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sin (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double tan (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __tan (double __x) __attribute__ ((__nothrow__ , __leaf__));



extern double acospi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __acospi (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double acospi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __acospi (double __x) __attribute__ ((__nothrow__ , __leaf__));

extern double asinpi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __asinpi (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double asinpi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __asinpi (double __x) __attribute__ ((__nothrow__ , __leaf__));

extern double atanpi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atanpi (double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double atanpi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atanpi (double __x) __attribute__ ((__nothrow__ , __leaf__));

extern double atan2pi (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atan2pi (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern double atan2pi (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atan2pi (double __y, double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double cospi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cospi (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double sinpi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sinpi (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double tanpi (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __tanpi (double __x) __attribute__ ((__nothrow__ , __leaf__));





 extern double cosh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cosh (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double sinh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sinh (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double tanh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __tanh (double __x) __attribute__ ((__nothrow__ , __leaf__));
# 107 "/usr/include/bits/mathcalls.h" 3
 extern double acosh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __acosh (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double asinh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __asinh (double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern double atanh (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __atanh (double __x) __attribute__ ((__nothrow__ , __leaf__));





 extern double exp (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp (double __x) __attribute__ ((__nothrow__ , __leaf__));


extern double frexp (double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__)); extern double __frexp (double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__));


extern double ldexp (double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__)); extern double __ldexp (double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__));


 extern double log (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double log10 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log10 (double __x) __attribute__ ((__nothrow__ , __leaf__));


extern double modf (double __x, double *__iptr) __attribute__ ((__nothrow__ , __leaf__)); extern double __modf (double __x, double *__iptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



 extern double exp10 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp10 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double exp2m1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp2m1 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double exp10m1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp10m1 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double log2p1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log2p1 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double log10p1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log10p1 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double logp1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __logp1 (double __x) __attribute__ ((__nothrow__ , __leaf__));




 extern double expm1 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __expm1 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double log1p (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log1p (double __x) __attribute__ ((__nothrow__ , __leaf__));


extern double logb (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __logb (double __x) __attribute__ ((__nothrow__ , __leaf__));




 extern double exp2 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __exp2 (double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern double log2 (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __log2 (double __x) __attribute__ ((__nothrow__ , __leaf__));






 extern double pow (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __pow (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double sqrt (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __sqrt (double __x) __attribute__ ((__nothrow__ , __leaf__));



 extern double hypot (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __hypot (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));




 extern double cbrt (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __cbrt (double __x) __attribute__ ((__nothrow__ , __leaf__));




extern double compoundn (double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __compoundn (double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


extern double pown (double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __pown (double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


extern double powr (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __powr (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double rootn (double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __rootn (double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


 extern double rsqrt (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __rsqrt (double __x) __attribute__ ((__nothrow__ , __leaf__));






extern double ceil (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fabs (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double floor (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmod (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __fmod (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
# 231 "/usr/include/bits/mathcalls.h" 3
extern int isinf (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));




extern int finite (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern double drem (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __drem (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));



extern double significand (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __significand (double __x) __attribute__ ((__nothrow__ , __leaf__));






extern double copysign (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern double nan (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern double __nan (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
# 267 "/usr/include/bits/mathcalls.h" 3
extern int isnan (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));





extern double j0 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __j0 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double j1 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __j1 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double jn (int, double) __attribute__ ((__nothrow__ , __leaf__)); extern double __jn (int, double) __attribute__ ((__nothrow__ , __leaf__));
extern double y0 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __y0 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double y1 (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __y1 (double) __attribute__ ((__nothrow__ , __leaf__));
extern double yn (int, double) __attribute__ ((__nothrow__ , __leaf__)); extern double __yn (int, double) __attribute__ ((__nothrow__ , __leaf__));





 extern double erf (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __erf (double) __attribute__ ((__nothrow__ , __leaf__));
 extern double erfc (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __erfc (double) __attribute__ ((__nothrow__ , __leaf__));
extern double lgamma (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __lgamma (double) __attribute__ ((__nothrow__ , __leaf__));




extern double tgamma (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __tgamma (double) __attribute__ ((__nothrow__ , __leaf__));





extern double gamma (double) __attribute__ ((__nothrow__ , __leaf__)); extern double __gamma (double) __attribute__ ((__nothrow__ , __leaf__));







extern double lgamma_r (double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__)); extern double __lgamma_r (double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__));






extern double rint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __rint (double __x) __attribute__ ((__nothrow__ , __leaf__));


extern double nextafter (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __nextafter (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));

extern double nexttoward (double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __nexttoward (double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));




extern double nextdown (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __nextdown (double __x) __attribute__ ((__nothrow__ , __leaf__));

extern double nextup (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __nextup (double __x) __attribute__ ((__nothrow__ , __leaf__));



extern double remainder (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __remainder (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));



extern double scalbn (double __x, int __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalbn (double __x, int __n) __attribute__ ((__nothrow__ , __leaf__));



extern int ilogb (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern int __ilogb (double __x) __attribute__ ((__nothrow__ , __leaf__));




extern long int llogb (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __llogb (double __x) __attribute__ ((__nothrow__ , __leaf__));




extern double scalbln (double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalbln (double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__));



extern double nearbyint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __nearbyint (double __x) __attribute__ ((__nothrow__ , __leaf__));



extern double round (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern double trunc (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern double remquo (double __x, double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__)); extern double __remquo (double __x, double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__));






extern long int lrint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lrint (double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llrint (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llrint (double __x) __attribute__ ((__nothrow__ , __leaf__));



extern long int lround (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lround (double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llround (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llround (double __x) __attribute__ ((__nothrow__ , __leaf__));



extern double fdim (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __fdim (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));



extern double fmax (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmin (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern double fma (double __x, double __y, double __z) __attribute__ ((__nothrow__ , __leaf__)); extern double __fma (double __x, double __y, double __z) __attribute__ ((__nothrow__ , __leaf__));




extern double roundeven (double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern double fromfp (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern double __fromfp (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));



extern double ufromfp (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern double __ufromfp (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));




extern double fromfpx (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern double __fromfpx (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));




extern double ufromfpx (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern double __ufromfpx (double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));


extern int canonicalize (double *__cx, const double *__x) __attribute__ ((__nothrow__ , __leaf__));
# 435 "/usr/include/bits/mathcalls.h" 3
extern double fmaximum (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmaximum_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmaximum_mag (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum_mag (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmaximum_mag_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum_mag_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 485 "/usr/include/bits/mathcalls.h" 3
extern double scalb (double __x, double __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalb (double __x, double __n) __attribute__ ((__nothrow__ , __leaf__));
# 451 "/usr/include/math.h" 2 3
# 466 "/usr/include/math.h" 3
# 1 "/usr/include/bits/mathcalls-helper-functions.h" 1 3
# 20 "/usr/include/bits/mathcalls-helper-functions.h" 3
extern int __fpclassifyf (float __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));


extern int __signbitf (float __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));



extern int __isinff (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __finitef (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __isnanf (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __iseqsigf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));


extern int __issignalingf (float __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
# 467 "/usr/include/math.h" 2 3
# 1 "/usr/include/bits/mathcalls.h" 1 3
# 53 "/usr/include/bits/mathcalls.h" 3
 extern float acosf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __acosf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float asinf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __asinf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float atanf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atanf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float atan2f (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atan2f (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float cosf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __cosf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float sinf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sinf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float tanf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __tanf (float __x) __attribute__ ((__nothrow__ , __leaf__));



extern float acospif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __acospif (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float acospif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __acospif (float __x) __attribute__ ((__nothrow__ , __leaf__));

extern float asinpif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __asinpif (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float asinpif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __asinpif (float __x) __attribute__ ((__nothrow__ , __leaf__));

extern float atanpif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atanpif (float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float atanpif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atanpif (float __x) __attribute__ ((__nothrow__ , __leaf__));

extern float atan2pif (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atan2pif (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__));
 extern float atan2pif (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atan2pif (float __y, float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float cospif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __cospif (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float sinpif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sinpif (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float tanpif (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __tanpif (float __x) __attribute__ ((__nothrow__ , __leaf__));





 extern float coshf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __coshf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float sinhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sinhf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float tanhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __tanhf (float __x) __attribute__ ((__nothrow__ , __leaf__));
# 107 "/usr/include/bits/mathcalls.h" 3
 extern float acoshf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __acoshf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float asinhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __asinhf (float __x) __attribute__ ((__nothrow__ , __leaf__));

 extern float atanhf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __atanhf (float __x) __attribute__ ((__nothrow__ , __leaf__));





 extern float expf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __expf (float __x) __attribute__ ((__nothrow__ , __leaf__));


extern float frexpf (float __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__)); extern float __frexpf (float __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__));


extern float ldexpf (float __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__)); extern float __ldexpf (float __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__));


 extern float logf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __logf (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float log10f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log10f (float __x) __attribute__ ((__nothrow__ , __leaf__));


extern float modff (float __x, float *__iptr) __attribute__ ((__nothrow__ , __leaf__)); extern float __modff (float __x, float *__iptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



 extern float exp10f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __exp10f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float exp2m1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __exp2m1f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float exp10m1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __exp10m1f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float log2p1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log2p1f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float log10p1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log10p1f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float logp1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __logp1f (float __x) __attribute__ ((__nothrow__ , __leaf__));




 extern float expm1f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __expm1f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float log1pf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log1pf (float __x) __attribute__ ((__nothrow__ , __leaf__));


extern float logbf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __logbf (float __x) __attribute__ ((__nothrow__ , __leaf__));




 extern float exp2f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __exp2f (float __x) __attribute__ ((__nothrow__ , __leaf__));


 extern float log2f (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __log2f (float __x) __attribute__ ((__nothrow__ , __leaf__));






 extern float powf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __powf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));


extern float sqrtf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __sqrtf (float __x) __attribute__ ((__nothrow__ , __leaf__));



 extern float hypotf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __hypotf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));




 extern float cbrtf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __cbrtf (float __x) __attribute__ ((__nothrow__ , __leaf__));




extern float compoundnf (float __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __compoundnf (float __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


extern float pownf (float __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __pownf (float __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


extern float powrf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __powrf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));


extern float rootnf (float __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __rootnf (float __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


 extern float rsqrtf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __rsqrtf (float __x) __attribute__ ((__nothrow__ , __leaf__));






extern float ceilf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fabsf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float floorf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmodf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __fmodf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));
# 231 "/usr/include/bits/mathcalls.h" 3
extern int isinff (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));




extern int finitef (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern float dremf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __dremf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));



extern float significandf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __significandf (float __x) __attribute__ ((__nothrow__ , __leaf__));






extern float copysignf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern float nanf (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern float __nanf (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
# 267 "/usr/include/bits/mathcalls.h" 3
extern int isnanf (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));





extern float j0f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __j0f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float j1f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __j1f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float jnf (int, float) __attribute__ ((__nothrow__ , __leaf__)); extern float __jnf (int, float) __attribute__ ((__nothrow__ , __leaf__));
extern float y0f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __y0f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float y1f (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __y1f (float) __attribute__ ((__nothrow__ , __leaf__));
extern float ynf (int, float) __attribute__ ((__nothrow__ , __leaf__)); extern float __ynf (int, float) __attribute__ ((__nothrow__ , __leaf__));





 extern float erff (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __erff (float) __attribute__ ((__nothrow__ , __leaf__));
 extern float erfcf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __erfcf (float) __attribute__ ((__nothrow__ , __leaf__));
extern float lgammaf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __lgammaf (float) __attribute__ ((__nothrow__ , __leaf__));




extern float tgammaf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __tgammaf (float) __attribute__ ((__nothrow__ , __leaf__));





extern float gammaf (float) __attribute__ ((__nothrow__ , __leaf__)); extern float __gammaf (float) __attribute__ ((__nothrow__ , __leaf__));







extern float lgammaf_r (float, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__)); extern float __lgammaf_r (float, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__));






extern float rintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __rintf (float __x) __attribute__ ((__nothrow__ , __leaf__));


extern float nextafterf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __nextafterf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));

extern float nexttowardf (float __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __nexttowardf (float __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));




extern float nextdownf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __nextdownf (float __x) __attribute__ ((__nothrow__ , __leaf__));

extern float nextupf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __nextupf (float __x) __attribute__ ((__nothrow__ , __leaf__));



extern float remainderf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __remainderf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));



extern float scalbnf (float __x, int __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalbnf (float __x, int __n) __attribute__ ((__nothrow__ , __leaf__));



extern int ilogbf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern int __ilogbf (float __x) __attribute__ ((__nothrow__ , __leaf__));




extern long int llogbf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __llogbf (float __x) __attribute__ ((__nothrow__ , __leaf__));




extern float scalblnf (float __x, long int __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalblnf (float __x, long int __n) __attribute__ ((__nothrow__ , __leaf__));



extern float nearbyintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __nearbyintf (float __x) __attribute__ ((__nothrow__ , __leaf__));



extern float roundf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern float truncf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern float remquof (float __x, float __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__)); extern float __remquof (float __x, float __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__));






extern long int lrintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lrintf (float __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llrintf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llrintf (float __x) __attribute__ ((__nothrow__ , __leaf__));



extern long int lroundf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lroundf (float __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llroundf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llroundf (float __x) __attribute__ ((__nothrow__ , __leaf__));



extern float fdimf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __fdimf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));



extern float fmaxf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern float fmaf (float __x, float __y, float __z) __attribute__ ((__nothrow__ , __leaf__)); extern float __fmaf (float __x, float __y, float __z) __attribute__ ((__nothrow__ , __leaf__));




extern float roundevenf (float __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern float fromfpf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern float __fromfpf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));



extern float ufromfpf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern float __ufromfpf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));




extern float fromfpxf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern float __fromfpxf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));




extern float ufromfpxf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern float __ufromfpxf (float __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));


extern int canonicalizef (float *__cx, const float *__x) __attribute__ ((__nothrow__ , __leaf__));
# 435 "/usr/include/bits/mathcalls.h" 3
extern float fmaximumf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimumf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmaximum_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimum_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmaximum_magf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimum_magf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmaximum_mag_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimum_mag_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 485 "/usr/include/bits/mathcalls.h" 3
extern float scalbf (float __x, float __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalbf (float __x, float __n) __attribute__ ((__nothrow__ , __leaf__));
# 468 "/usr/include/math.h" 2 3
# 535 "/usr/include/math.h" 3
# 1 "/usr/include/bits/mathcalls-helper-functions.h" 1 3
# 20 "/usr/include/bits/mathcalls-helper-functions.h" 3
extern int __fpclassifyl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));


extern int __signbitl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));



extern int __isinfl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __finitel (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __isnanl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __iseqsigl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern int __issignalingl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
# 536 "/usr/include/math.h" 2 3
# 1 "/usr/include/bits/mathcalls.h" 1 3
# 53 "/usr/include/bits/mathcalls.h" 3
 extern long double acosl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __acosl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double asinl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __asinl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double atanl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atanl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double atan2l (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atan2l (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double cosl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __cosl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double sinl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sinl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double tanl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tanl (long double __x) __attribute__ ((__nothrow__ , __leaf__));



extern long double acospil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __acospil (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double acospil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __acospil (long double __x) __attribute__ ((__nothrow__ , __leaf__));

extern long double asinpil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __asinpil (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double asinpil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __asinpil (long double __x) __attribute__ ((__nothrow__ , __leaf__));

extern long double atanpil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atanpil (long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double atanpil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atanpil (long double __x) __attribute__ ((__nothrow__ , __leaf__));

extern long double atan2pil (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atan2pil (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__));
 extern long double atan2pil (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atan2pil (long double __y, long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double cospil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __cospil (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double sinpil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sinpil (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double tanpil (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tanpil (long double __x) __attribute__ ((__nothrow__ , __leaf__));





 extern long double coshl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __coshl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double sinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double tanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
# 107 "/usr/include/bits/mathcalls.h" 3
 extern long double acoshl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __acoshl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double asinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __asinhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

 extern long double atanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __atanhl (long double __x) __attribute__ ((__nothrow__ , __leaf__));





 extern long double expl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __expl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern long double frexpl (long double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__)); extern long double __frexpl (long double __x, int *__exponent) __attribute__ ((__nothrow__ , __leaf__));


extern long double ldexpl (long double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__)); extern long double __ldexpl (long double __x, int __exponent) __attribute__ ((__nothrow__ , __leaf__));


 extern long double logl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __logl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double log10l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log10l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern long double modfl (long double __x, long double *__iptr) __attribute__ ((__nothrow__ , __leaf__)); extern long double __modfl (long double __x, long double *__iptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



 extern long double exp10l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __exp10l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double exp2m1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __exp2m1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double exp10m1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __exp10m1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double log2p1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log2p1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double log10p1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log10p1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double logp1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __logp1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));




 extern long double expm1l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __expm1l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double log1pl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log1pl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern long double logbl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __logbl (long double __x) __attribute__ ((__nothrow__ , __leaf__));




 extern long double exp2l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __exp2l (long double __x) __attribute__ ((__nothrow__ , __leaf__));


 extern long double log2l (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __log2l (long double __x) __attribute__ ((__nothrow__ , __leaf__));






 extern long double powl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __powl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern long double sqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __sqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));



 extern long double hypotl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __hypotl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));




 extern long double cbrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __cbrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));




extern long double compoundnl (long double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __compoundnl (long double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


extern long double pownl (long double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __pownl (long double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


extern long double powrl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __powrl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern long double rootnl (long double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __rootnl (long double __x, long long int __y) __attribute__ ((__nothrow__ , __leaf__));


 extern long double rsqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __rsqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));






extern long double ceill (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fabsl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double floorl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmodl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fmodl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
# 231 "/usr/include/bits/mathcalls.h" 3
extern int isinfl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));




extern int finitel (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern long double dreml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __dreml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));



extern long double significandl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __significandl (long double __x) __attribute__ ((__nothrow__ , __leaf__));






extern long double copysignl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern long double nanl (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nanl (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
# 267 "/usr/include/bits/mathcalls.h" 3
extern int isnanl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));





extern long double j0l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __j0l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double j1l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __j1l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double jnl (int, long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __jnl (int, long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double y0l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __y0l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double y1l (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __y1l (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double ynl (int, long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __ynl (int, long double) __attribute__ ((__nothrow__ , __leaf__));





 extern long double erfl (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __erfl (long double) __attribute__ ((__nothrow__ , __leaf__));
 extern long double erfcl (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __erfcl (long double) __attribute__ ((__nothrow__ , __leaf__));
extern long double lgammal (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __lgammal (long double) __attribute__ ((__nothrow__ , __leaf__));




extern long double tgammal (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __tgammal (long double) __attribute__ ((__nothrow__ , __leaf__));





extern long double gammal (long double) __attribute__ ((__nothrow__ , __leaf__)); extern long double __gammal (long double) __attribute__ ((__nothrow__ , __leaf__));







extern long double lgammal_r (long double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__)); extern long double __lgammal_r (long double, int *__signgamp) __attribute__ ((__nothrow__ , __leaf__));






extern long double rintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __rintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern long double nextafterl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nextafterl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));

extern long double nexttowardl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nexttowardl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));




extern long double nextdownl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nextdownl (long double __x) __attribute__ ((__nothrow__ , __leaf__));

extern long double nextupl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nextupl (long double __x) __attribute__ ((__nothrow__ , __leaf__));



extern long double remainderl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __remainderl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));



extern long double scalbnl (long double __x, int __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalbnl (long double __x, int __n) __attribute__ ((__nothrow__ , __leaf__));



extern int ilogbl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern int __ilogbl (long double __x) __attribute__ ((__nothrow__ , __leaf__));




extern long int llogbl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __llogbl (long double __x) __attribute__ ((__nothrow__ , __leaf__));




extern long double scalblnl (long double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalblnl (long double __x, long int __n) __attribute__ ((__nothrow__ , __leaf__));



extern long double nearbyintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nearbyintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));



extern long double roundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern long double truncl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern long double remquol (long double __x, long double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__)); extern long double __remquol (long double __x, long double __y, int *__quo) __attribute__ ((__nothrow__ , __leaf__));






extern long int lrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llrintl (long double __x) __attribute__ ((__nothrow__ , __leaf__));



extern long int lroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long int __lroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__));
__extension__
extern long long int llroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long long int __llroundl (long double __x) __attribute__ ((__nothrow__ , __leaf__));



extern long double fdiml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fdiml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));



extern long double fmaxl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern long double fmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__));




extern long double roundevenl (long double __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern long double fromfpl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fromfpl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));



extern long double ufromfpl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern long double __ufromfpl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));




extern long double fromfpxl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern long double __fromfpxl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));




extern long double ufromfpxl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__)); extern long double __ufromfpxl (long double __x, int __round, unsigned int __width) __attribute__ ((__nothrow__ , __leaf__));


extern int canonicalizel (long double *__cx, const long double *__x) __attribute__ ((__nothrow__ , __leaf__));
# 435 "/usr/include/bits/mathcalls.h" 3
extern long double fmaximuml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimuml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmaximum_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimum_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmaximum_magl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimum_magl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmaximum_mag_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimum_mag_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 485 "/usr/include/bits/mathcalls.h" 3
extern long double scalbl (long double __x, long double __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalbl (long double __x, long double __n) __attribute__ ((__nothrow__ , __leaf__));
# 537 "/usr/include/math.h" 2 3
# 618 "/usr/include/math.h" 3
# 1 "/usr/include/bits/mathcalls-helper-functions.h" 1 3
# 20 "/usr/include/bits/mathcalls-helper-functions.h" 3
extern int __fpclassifyf128 (_Float128 __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));


extern int __signbitf128 (_Float128 __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));



extern int __isinff128 (_Float128 __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __finitef128 (_Float128 __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __isnanf128 (_Float128 __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern int __iseqsigf128 (_Float128 __x, _Float128 __y) __attribute__ ((__nothrow__ , __leaf__));


extern int __issignalingf128 (_Float128 __value) __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__const__));
# 619 "/usr/include/math.h" 2 3
# 703 "/usr/include/math.h" 3
# 1 "/usr/include/bits/mathcalls-narrow.h" 1 3
# 24 "/usr/include/bits/mathcalls-narrow.h" 3
extern float fadd (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fdiv (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float ffma (double __x, double __y, double __z) __attribute__ ((__nothrow__ , __leaf__));


extern float fmul (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fsqrt (double __x) __attribute__ ((__nothrow__ , __leaf__));


extern float fsub (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
# 704 "/usr/include/math.h" 2 3
# 724 "/usr/include/math.h" 3
# 1 "/usr/include/bits/mathcalls-narrow.h" 1 3
# 24 "/usr/include/bits/mathcalls-narrow.h" 3
extern float faddl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fdivl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float ffmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__));


extern float fmull (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fsqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern float fsubl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
# 725 "/usr/include/math.h" 2 3
# 753 "/usr/include/math.h" 3
# 1 "/usr/include/bits/mathcalls-narrow.h" 1 3
# 24 "/usr/include/bits/mathcalls-narrow.h" 3
extern double daddl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double ddivl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double dfmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__));


extern double dmull (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double dsqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern double dsubl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
# 754 "/usr/include/math.h" 2 3
# 991 "/usr/include/math.h" 3
extern int signgam;
# 1071 "/usr/include/math.h" 3
enum
  {
    FP_NAN =

      0,
    FP_INFINITE =

      1,
    FP_ZERO =

      2,
    FP_SUBNORMAL =

      3,
    FP_NORMAL =

      4
  };
# 1192 "/usr/include/math.h" 3
# 1 "/usr/include/bits/iscanonical.h" 1 3
# 23 "/usr/include/bits/iscanonical.h" 3
extern int __iscanonicall (long double __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 1193 "/usr/include/math.h" 2 3
# 1609 "/usr/include/math.h" 3

# 42 "/usr/local/include/csmith.h" 2 3
# 1 "/usr/include/string.h" 1 3
# 26 "/usr/include/string.h" 3
# 1 "/usr/include/bits/libc-header-start.h" 1 3
# 27 "/usr/include/string.h" 2 3


# 37 "/usr/include/string.h" 3
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stddef.h" 1 3
# 229 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stddef.h" 3
typedef long unsigned int size_t;
# 38 "/usr/include/string.h" 2 3
# 47 "/usr/include/string.h" 3
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
       size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void *memmove (void *__dest, const void *__src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern void *memccpy (void *__restrict __dest, const void *__restrict __src,
        int __c, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 4)));




extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern void *memset_explicit (void *__s, int __c, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__write_only__, 1, 3)));



extern int memcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 91 "/usr/include/string.h" 3
extern int __memcmpeq (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 118 "/usr/include/string.h" 3
extern void *memchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 156 "/usr/include/string.h" 3
extern char *strcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strcat (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncat (char *__restrict __dest, const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern int strncmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcoll (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern size_t strxfrm (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 1, 3)));



# 1 "/usr/include/bits/types/locale_t.h" 1 3
# 22 "/usr/include/bits/types/locale_t.h" 3
# 1 "/usr/include/bits/types/__locale_t.h" 1 3
# 27 "/usr/include/bits/types/__locale_t.h" 3
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/bits/types/locale_t.h" 2 3

typedef __locale_t locale_t;
# 188 "/usr/include/string.h" 2 3


extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));


extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
    locale_t __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)))
     __attribute__ ((__access__ (__write_only__, 1, 3)));





extern char *strdup (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));






extern char *strndup (const char *__string, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
# 261 "/usr/include/string.h" 3
extern char *strchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 292 "/usr/include/string.h" 3
extern char *strrchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 309 "/usr/include/string.h" 3
extern char *strchrnul (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));





extern size_t strcspn (const char *__s, const char *__reject)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 346 "/usr/include/string.h" 3
extern char *strpbrk (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 377 "/usr/include/string.h" 3
extern char *strstr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 388 "/usr/include/string.h" 3
extern char *strtok (char *__restrict __s, const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *__strtok_r (char *__restrict __s,
    const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));

extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
# 412 "/usr/include/string.h" 3
extern char *strcasestr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));







extern void *memmem (const void *__haystack, size_t __haystacklen,
       const void *__needle, size_t __needlelen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 3)))
    __attribute__ ((__access__ (__read_only__, 1, 2)))
    __attribute__ ((__access__ (__read_only__, 3, 4)));



extern void *__mempcpy (void *__restrict __dest,
   const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *mempcpy (void *__restrict __dest,
        const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlen (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern size_t strnlen (const char *__string, size_t __maxlen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern char *strerror (int __errnum) __attribute__ ((__nothrow__ , __leaf__));
# 464 "/usr/include/string.h" 3
extern int strerror_r (int __errnum, char *__buf, size_t __buflen) __asm__ ("" "__xpg_strerror_r") __attribute__ ((__nothrow__ , __leaf__))

                        __attribute__ ((__nonnull__ (2)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
# 490 "/usr/include/string.h" 3
extern char *strerror_l (int __errnum, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/strings.h" 1 3
# 23 "/usr/include/strings.h" 3
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stddef.h" 1 3
# 24 "/usr/include/strings.h" 2 3










extern int bcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bcopy (const void *__src, void *__dest, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 68 "/usr/include/strings.h" 3
extern char *index (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 96 "/usr/include/strings.h" 3
extern char *rindex (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));






extern int ffs (int __i) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));





extern int ffsl (long int __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__ extern int ffsll (long long int __ll)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern int strcasecmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strncasecmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int strcasecmp_l (const char *__s1, const char *__s2, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));



extern int strncasecmp_l (const char *__s1, const char *__s2,
     size_t __n, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 4)));



# 495 "/usr/include/string.h" 2 3



extern void explicit_bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));



extern char *strsep (char **__restrict __stringp,
       const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strsignal (int __sig) __attribute__ ((__nothrow__ , __leaf__));
# 521 "/usr/include/string.h" 3
extern char *__stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern char *__stpncpy (char *__restrict __dest,
   const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlcpy (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 3)));



extern size_t strlcat (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__read_write__, 1, 3)));
# 584 "/usr/include/string.h" 3

# 43 "/usr/local/include/csmith.h" 2 3


# 1 "/usr/local/include/random_inc.h" 1 3
# 50 "/usr/local/include/random_inc.h" 3
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/limits.h" 1 3
# 34 "/usr/lib/gcc/x86_64-redhat-linux/16/include/limits.h" 3
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/syslimits.h" 1 3






 
# 7 "/usr/lib/gcc/x86_64-redhat-linux/16/include/syslimits.h" 3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/limits.h" 1 3
# 212 "/usr/lib/gcc/x86_64-redhat-linux/16/include/limits.h" 3
# 1 "/usr/include/limits.h" 1 3
# 26 "/usr/include/limits.h" 3
# 1 "/usr/include/bits/libc-header-start.h" 1 3
# 27 "/usr/include/limits.h" 2 3
# 198 "/usr/include/limits.h" 3
# 1 "/usr/include/bits/posix1_lim.h" 1 3
# 27 "/usr/include/bits/posix1_lim.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 28 "/usr/include/bits/posix1_lim.h" 2 3
# 161 "/usr/include/bits/posix1_lim.h" 3
# 1 "/usr/include/bits/local_lim.h" 1 3
# 38 "/usr/include/bits/local_lim.h" 3
# 1 "/usr/include/linux/limits.h" 1 3
# 39 "/usr/include/bits/local_lim.h" 2 3
# 81 "/usr/include/bits/local_lim.h" 3
# 1 "/usr/include/bits/pthread_stack_min-dynamic.h" 1 3
# 29 "/usr/include/bits/pthread_stack_min-dynamic.h" 3
# 1 "/usr/include/bits/pthread_stack_min.h" 1 3
# 30 "/usr/include/bits/pthread_stack_min-dynamic.h" 2 3
# 82 "/usr/include/bits/local_lim.h" 2 3
# 162 "/usr/include/bits/posix1_lim.h" 2 3
# 199 "/usr/include/limits.h" 2 3



# 1 "/usr/include/bits/posix2_lim.h" 1 3
# 203 "/usr/include/limits.h" 2 3
# 213 "/usr/lib/gcc/x86_64-redhat-linux/16/include/limits.h" 2 3
# 10 "/usr/lib/gcc/x86_64-redhat-linux/16/include/syslimits.h" 2 3
#pragma GCC diagnostic pop
# 35 "/usr/lib/gcc/x86_64-redhat-linux/16/include/limits.h" 2 3
# 51 "/usr/local/include/random_inc.h" 2 3



# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stdint.h" 1 3
# 9 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stdint.h" 3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
# 1 "/usr/include/stdint.h" 1 3
# 26 "/usr/include/stdint.h" 3
# 1 "/usr/include/bits/libc-header-start.h" 1 3
# 27 "/usr/include/stdint.h" 2 3
# 1 "/usr/include/bits/types.h" 1 3
# 27 "/usr/include/bits/types.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 28 "/usr/include/bits/types.h" 2 3
# 1 "/usr/include/bits/timesize.h" 1 3
# 19 "/usr/include/bits/timesize.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 20 "/usr/include/bits/timesize.h" 2 3
# 29 "/usr/include/bits/types.h" 2 3


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/bits/types.h" 3
# 1 "/usr/include/bits/typesizes.h" 1 3
# 142 "/usr/include/bits/types.h" 2 3
# 1 "/usr/include/bits/time64.h" 1 3
# 143 "/usr/include/bits/types.h" 2 3


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 28 "/usr/include/stdint.h" 2 3
# 1 "/usr/include/bits/wchar.h" 1 3
# 29 "/usr/include/stdint.h" 2 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 30 "/usr/include/stdint.h" 2 3
# 38 "/usr/include/stdint.h" 3
# 1 "/usr/include/bits/stdint-intn.h" 1 3
# 24 "/usr/include/bits/stdint-intn.h" 3
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 39 "/usr/include/stdint.h" 2 3


# 1 "/usr/include/bits/stdint-uintn.h" 1 3
# 24 "/usr/include/bits/stdint-uintn.h" 3
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
# 42 "/usr/include/stdint.h" 2 3



# 1 "/usr/include/bits/stdint-least.h" 1 3
# 25 "/usr/include/bits/stdint-least.h" 3
typedef __int_least8_t int_least8_t;
typedef __int_least16_t int_least16_t;
typedef __int_least32_t int_least32_t;
typedef __int_least64_t int_least64_t;


typedef __uint_least8_t uint_least8_t;
typedef __uint_least16_t uint_least16_t;
typedef __uint_least32_t uint_least32_t;
typedef __uint_least64_t uint_least64_t;
# 46 "/usr/include/stdint.h" 2 3





typedef signed char int_fast8_t;

typedef long int int_fast16_t;
typedef long int int_fast32_t;
typedef long int int_fast64_t;
# 64 "/usr/include/stdint.h" 3
typedef unsigned char uint_fast8_t;

typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
typedef unsigned long int uint_fast64_t;
# 80 "/usr/include/stdint.h" 3
typedef long int intptr_t;


typedef unsigned long int uintptr_t;
# 94 "/usr/include/stdint.h" 3
typedef __intmax_t intmax_t;
typedef __uintmax_t uintmax_t;
# 12 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stdint.h" 2 3
#pragma GCC diagnostic pop
# 55 "/usr/local/include/random_inc.h" 2 3



# 1 "/usr/include/assert.h" 1 3
# 92 "/usr/include/assert.h" 3



extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));


extern void __assert_perror_fail (int __errnum, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));




extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));






extern _Bool __assert_single_arg (_Bool);



# 59 "/usr/local/include/random_inc.h" 2 3
# 88 "/usr/local/include/random_inc.h" 3
# 1 "/usr/local/include/platform_generic.h" 1 3
# 39 "/usr/local/include/platform_generic.h" 3
# 1 "/usr/include/stdio.h" 1 3
# 28 "/usr/include/stdio.h" 3
# 1 "/usr/include/bits/libc-header-start.h" 1 3
# 29 "/usr/include/stdio.h" 2 3









# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stddef.h" 1 3
# 39 "/usr/include/stdio.h" 2 3


# 1 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stdarg.h" 1 3
# 40 "/usr/lib/gcc/x86_64-redhat-linux/16/include/stdarg.h" 3
typedef __builtin_va_list __gnuc_va_list;
# 42 "/usr/include/stdio.h" 2 3


# 1 "/usr/include/bits/types/__fpos_t.h" 1 3




# 1 "/usr/include/bits/types/__mbstate_t.h" 1 3
# 13 "/usr/include/bits/types/__mbstate_t.h" 3
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 6 "/usr/include/bits/types/__fpos_t.h" 2 3




typedef struct _G_fpos_t
{
  __off_t __pos;
  __mbstate_t __state;
} __fpos_t;
# 45 "/usr/include/stdio.h" 2 3
# 1 "/usr/include/bits/types/__fpos64_t.h" 1 3
# 10 "/usr/include/bits/types/__fpos64_t.h" 3
typedef struct _G_fpos64_t
{
  __off64_t __pos;
  __mbstate_t __state;
} __fpos64_t;
# 46 "/usr/include/stdio.h" 2 3
# 1 "/usr/include/bits/types/__FILE.h" 1 3



struct _IO_FILE;
typedef struct _IO_FILE __FILE;
# 47 "/usr/include/stdio.h" 2 3
# 1 "/usr/include/bits/types/FILE.h" 1 3



struct _IO_FILE;


typedef struct _IO_FILE FILE;
# 48 "/usr/include/stdio.h" 2 3
# 1 "/usr/include/bits/types/struct_FILE.h" 1 3
# 35 "/usr/include/bits/types/struct_FILE.h" 3
# 1 "/usr/include/bits/wordsize.h" 1 3
# 36 "/usr/include/bits/types/struct_FILE.h" 2 3

struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;




typedef void _IO_lock_t;





struct _IO_FILE
{
  int _flags;


  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;


  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _flags2:24;

  char _short_backupbuf[1];
  __off_t _old_offset;


  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  _IO_lock_t *_lock;







  __off64_t _offset;

  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  struct _IO_FILE **_prevchain;
  int _mode;

  int _unused3;

  __uint64_t _total_written;




  char _unused2[12 * sizeof (int) - 5 * sizeof (void *)];
};
# 49 "/usr/include/stdio.h" 2 3


# 1 "/usr/include/bits/types/cookie_io_functions_t.h" 1 3
# 27 "/usr/include/bits/types/cookie_io_functions_t.h" 3
typedef __ssize_t cookie_read_function_t (void *__cookie, char *__buf,
                                          size_t __nbytes);







typedef __ssize_t cookie_write_function_t (void *__cookie, const char *__buf,
                                           size_t __nbytes);







typedef int cookie_seek_function_t (void *__cookie, __off64_t *__pos, int __w);


typedef int cookie_close_function_t (void *__cookie);






typedef struct _IO_cookie_io_functions_t
{
  cookie_read_function_t *read;
  cookie_write_function_t *write;
  cookie_seek_function_t *seek;
  cookie_close_function_t *close;
} cookie_io_functions_t;
# 52 "/usr/include/stdio.h" 2 3





typedef __gnuc_va_list va_list;
# 68 "/usr/include/stdio.h" 3
typedef __off_t off_t;
# 82 "/usr/include/stdio.h" 3
typedef __ssize_t ssize_t;






typedef __fpos_t fpos_t;
# 133 "/usr/include/stdio.h" 3
# 1 "/usr/include/bits/stdio_lim.h" 1 3
# 134 "/usr/include/stdio.h" 2 3
# 153 "/usr/include/stdio.h" 3
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;






extern int remove (const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (const char *__old, const char *__new) __attribute__ ((__nothrow__ , __leaf__));



extern int renameat (int __oldfd, const char *__old, int __newfd,
       const char *__new) __attribute__ ((__nothrow__ , __leaf__));
# 191 "/usr/include/stdio.h" 3
extern int fclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 201 "/usr/include/stdio.h" 3
extern FILE *tmpfile (void)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 218 "/usr/include/stdio.h" 3
extern char *tmpnam (char[20]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern char *tmpnam_r (char __s[20]) __attribute__ ((__nothrow__ , __leaf__)) ;
# 235 "/usr/include/stdio.h" 3
extern char *tempnam (const char *__dir, const char *__pfx)
   __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (__builtin_free, 1)));






extern int fflush (FILE *__stream);
# 252 "/usr/include/stdio.h" 3
extern int fflush_unlocked (FILE *__stream);
# 271 "/usr/include/stdio.h" 3
extern FILE *fopen (const char *__restrict __filename,
      const char *__restrict __modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *freopen (const char *__restrict __filename,
        const char *__restrict __modes,
        FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));
# 306 "/usr/include/stdio.h" 3
extern FILE *fdopen (int __fd, const char *__modes) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;





extern FILE *fopencookie (void *__restrict __magic_cookie,
     const char *__restrict __modes,
     cookie_io_functions_t __io_funcs) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *fmemopen (void *__s, size_t __len, const char *__modes)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *open_memstream (char **__bufloc, size_t *__sizeloc) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 341 "/usr/include/stdio.h" 3
extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__nonnull__ (1)));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int fprintf (FILE *__restrict __stream,
      const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int printf (const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nonnull__ (1)));




extern int vprintf (const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));



extern int snprintf (char *__restrict __s, size_t __maxlen,
       const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));





extern int vasprintf (char **__restrict __ptr, const char *__restrict __f,
        __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0))) ;
extern int __asprintf (char **__restrict __ptr,
         const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;
extern int asprintf (char **__restrict __ptr,
       const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;




extern int vdprintf (int __fd, const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));







extern int fscanf (FILE *__restrict __stream,
     const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int scanf (const char *__restrict __format, ...) ;

extern int sscanf (const char *__restrict __s,
     const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));
# 449 "/usr/include/stdio.h" 3
extern int fscanf (FILE *__restrict __stream, const char *__restrict __format, ...) __asm__ ("" "__isoc23_fscanf")

                                __attribute__ ((__nonnull__ (1)));
extern int scanf (const char *__restrict __format, ...) __asm__ ("" "__isoc23_scanf")
                              ;
extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...) __asm__ ("" "__isoc23_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 497 "/usr/include/stdio.h" 3
extern int vfscanf (FILE *__restrict __s, const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));





extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (const char *__restrict __s,
      const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));






extern int vfscanf (FILE *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));
extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) ;
extern int vsscanf (const char *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vsscanf") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__format__ (__scanf__, 2, 0)));
# 582 "/usr/include/stdio.h" 3
extern int fgetc (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getc (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern int getchar (void);






extern int getc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getchar_unlocked (void);
# 607 "/usr/include/stdio.h" 3
extern int fgetc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));







extern int fputc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));





extern int putchar (int __c);
# 631 "/usr/include/stdio.h" 3
extern int fputc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern int putc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream) __attribute__ ((__nonnull__ (1)));


extern int putw (int __w, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 693 "/usr/include/stdio.h" 3
extern __ssize_t __getdelim (char **__restrict __lineptr,
                             size_t *__restrict __n, int __delimiter,
                             FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));
extern __ssize_t getdelim (char **__restrict __lineptr,
                           size_t *__restrict __n, int __delimiter,
                           FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));


extern __ssize_t getline (char **__restrict __lineptr,
                          size_t *__restrict __n,
                          FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));







extern int fputs (const char *__restrict __s, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (2)));





extern int puts (const char *__s);






extern int ungetc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));




extern size_t fwrite (const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) __attribute__ ((__nonnull__ (4)));
# 760 "/usr/include/stdio.h" 3
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));
extern size_t fwrite_unlocked (const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));







extern int fseek (FILE *__stream, long int __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern long int ftell (FILE *__stream) __attribute__ ((__nonnull__ (1)));




extern void rewind (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 797 "/usr/include/stdio.h" 3
extern int fseeko (FILE *__stream, __off_t __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern __off_t ftello (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 823 "/usr/include/stdio.h" 3
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos)
  __attribute__ ((__nonnull__ (1)));




extern int fsetpos (FILE *__stream, const fpos_t *__pos) __attribute__ ((__nonnull__ (1)));
# 854 "/usr/include/stdio.h" 3
extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern void perror (const char *__s) __attribute__ ((__cold__));




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 891 "/usr/include/stdio.h" 3
extern int pclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern FILE *popen (const char *__command, const char *__modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (pclose, 1))) ;






extern char *ctermid (char *__s) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1)));
# 935 "/usr/include/stdio.h" 3
extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 953 "/usr/include/stdio.h" 3
extern int __uflow (FILE *);
extern int __overflow (FILE *, int);
# 977 "/usr/include/stdio.h" 3

# 40 "/usr/local/include/platform_generic.h" 2 3


static void platform_main_begin(void) { }

static void platform_main_end(uint32_t crc, int flag) {





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
static uint32_t crc32_context = 0xFFFFFFFFUL;

static void crc32_gentab(void) {
  uint32_t crc;
  const uint32_t poly = 0xEDB88320UL;
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
           crc32_context ^ 0xFFFFFFFFUL);
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
           crc32_context ^ 0xFFFFFFFFUL);
  }
}
# 11 "/tmp/test156_src_v2.c" 2



# 13 "/tmp/test156_src_v2.c"
static long __undefined;



static volatile int32_t g_2 = (-7L);
static uint8_t g_9 = 0x3EL;
static volatile int32_t g_12 = 0xD0107893L;
static const volatile int32_t * const g_11 = &g_12;
static int32_t g_22 = 1L;
static int16_t g_61 = 0x156BL;
static uint64_t g_97 = 0xFA32FBB2F22DDF81LL;
static uint8_t g_102 = 0x1EL;
static uint16_t g_109[3] = {65535UL,65535UL,65535UL};
static uint32_t g_125 = 0x2FC28BF8L;
static int64_t g_127 = 0xE070DDB654FA892FLL;
static int16_t g_131 = 0x0A5BL;
static uint32_t g_150 = 5UL;
static uint64_t g_161 = 0xB0BCF91A75F86E74LL;
static int16_t *g_164 = (void*)0;
static int16_t * volatile *g_163 = &g_164;
static int16_t * volatile **g_162 = &g_163;
static uint32_t *g_169 = &g_125;
static uint32_t * volatile *g_168 = &g_169;
static uint32_t g_193 = 0x2F403320L;
static int32_t g_195 = 0x238D2D0EL;
static int16_t g_200 = 0x97C1L;
static int32_t g_201 = 0L;
static uint16_t g_202 = 0x83BDL;
static int32_t *g_255 = (void*)0;
static int32_t **g_254 = &g_255;
static uint64_t g_295 = 0x56C0A20D6E4A1601LL;
static int16_t g_299 = (-3L);
static int16_t g_300 = 0x450EL;
static int16_t **g_309 = &g_164;
static int16_t ***g_308 = &g_309;
static int16_t ****g_307 = &g_308;
static int64_t g_315 = 0xD4B90360C1850530LL;
static int8_t g_316 = 0x8CL;
static int64_t g_319 = (-1L);
static uint32_t g_320 = 4294967295UL;
static uint32_t * volatile **g_350 = (void*)0;
static uint32_t * volatile *** const g_349 = &g_350;
static const uint16_t g_387 = 0UL;
static const uint16_t g_389 = 1UL;
static int32_t g_429 = 0xC70196E3L;
static const int16_t ***g_474 = (void*)0;
static const int16_t ****g_473 = &g_474;
static int32_t ** volatile *g_498[5][1] = {{&g_254},{&g_254},{&g_254},{&g_254},{&g_254}};
static int32_t ** volatile * volatile *g_497 = &g_498[3][0];
static int64_t *g_518 = &g_315;
static int64_t **g_517 = &g_518;
static int32_t g_552 = 0x85EEB956L;
static uint32_t **g_588 = &g_169;
static uint32_t ***g_587[3] = {&g_588,&g_588,&g_588};
static uint32_t ***g_589[6][9][4] = {{{(void*)0,&g_588,(void*)0,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,&g_588,(void*)0,(void*)0},{&g_588,(void*)0,&g_588,&g_588},{(void*)0,&g_588,&g_588,&g_588},{&g_588,(void*)0,(void*)0,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,&g_588,(void*)0,(void*)0},{(void*)0,&g_588,&g_588,&g_588}},{{&g_588,(void*)0,&g_588,&g_588},{&g_588,&g_588,&g_588,(void*)0},{&g_588,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,(void*)0},{&g_588,(void*)0,(void*)0,&g_588},{(void*)0,&g_588,&g_588,&g_588},{(void*)0,&g_588,(void*)0,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,&g_588,&g_588,&g_588}},{{&g_588,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,&g_588},{&g_588,(void*)0,&g_588,&g_588},{(void*)0,&g_588,&g_588,&g_588},{(void*)0,&g_588,(void*)0,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,&g_588,&g_588,&g_588},{(void*)0,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,(void*)0}},{{&g_588,(void*)0,(void*)0,&g_588},{(void*)0,&g_588,&g_588,(void*)0},{(void*)0,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,&g_588},{&g_588,&g_588,&g_588,(void*)0},{&g_588,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,(void*)0},{&g_588,(void*)0,(void*)0,&g_588},{(void*)0,&g_588,&g_588,&g_588}},{{(void*)0,&g_588,(void*)0,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,&g_588},{&g_588,(void*)0,&g_588,&g_588},{(void*)0,&g_588,&g_588,&g_588},{(void*)0,&g_588,(void*)0,&g_588},{&g_588,&g_588,&g_588,&g_588}},{{&g_588,&g_588,&g_588,&g_588},{(void*)0,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,(void*)0},{&g_588,(void*)0,(void*)0,&g_588},{(void*)0,&g_588,&g_588,(void*)0},{(void*)0,&g_588,&g_588,&g_588},{&g_588,(void*)0,&g_588,&g_588},{&g_588,&g_588,&g_588,(void*)0},{&g_588,&g_588,&g_588,&g_588}}};
static int16_t *** const *g_642 = &g_308;
static int16_t *** const **g_641 = &g_642;
static const int16_t * const g_716 = (void*)0;
static const int16_t * const *g_715[7] = {&g_716,(void*)0,(void*)0,&g_716,(void*)0,(void*)0,&g_716};
static const int16_t * const **g_714 = &g_715[3];
static const int16_t * const ***g_713 = &g_714;
static const int16_t * const ****g_712 = &g_713;
static int64_t ***g_777 = &g_517;
static uint8_t g_887 = 0x94L;
static uint16_t **g_928 = (void*)0;
static uint16_t ***g_927 = &g_928;
static uint16_t * const g_942 = (void*)0;
static uint16_t * const *g_941[2] = {&g_942,&g_942};
static uint16_t * const **g_940 = &g_941[1];
static uint16_t g_984[7] = {0UL,0UL,0UL,0UL,0UL,0UL,0UL};
static uint32_t *****g_1002 = (void*)0;
static int64_t ** const **g_1091 = (void*)0;
static int64_t g_1222 = 0L;
static int16_t * const * const g_1307 = &g_164;
static int16_t * const * const *g_1306 = &g_1307;
static int64_t g_1365 = (-3L);
static const uint16_t g_1449 = 65529UL;
static const uint16_t *g_1448 = &g_1449;
static int8_t g_1581 = 0L;
static uint32_t g_1650 = 4294967295UL;
static int32_t ***g_1674 = &g_254;
static int32_t ****g_1673[1] = {&g_1674};
static int32_t g_1803 = 0x14E3635CL;
static int8_t g_1909 = 0x24L;
static uint32_t ** const *g_1943[1] = {(void*)0};
static uint32_t ** const * const *g_1942[7] = {&g_1943[0],&g_1943[0],&g_1943[0],&g_1943[0],&g_1943[0],&g_1943[0],&g_1943[0]};
static uint32_t ** const * const * const *g_1941 = &g_1942[1];
static int8_t *g_1987 = (void*)0;
static int8_t **g_1986 = &g_1987;
static const uint64_t g_2080 = 0xF78A5A361827B233LL;
static uint8_t g_2103 = 5UL;
static const uint32_t *g_2122 = &g_1650;
static const uint32_t **g_2121 = &g_2122;
static int32_t *g_2153 = (void*)0;
static int32_t **g_2152 = &g_2153;
static uint8_t *g_2179 = &g_887;
static uint8_t **g_2178 = &g_2179;
static volatile int32_t g_2221[5] = {0xE9B6D850L,0xE9B6D850L,0xE9B6D850L,0xE9B6D850L,0xE9B6D850L};
static volatile int32_t *g_2220 = &g_2221[1];
static int32_t g_2284[3][9] = {{0xE115D679L,0L,0L,0xE115D679L,0xF8AB076BL,0xE115D679L,0L,0L,0xE115D679L},{0L,0L,1L,0L,0L,0L,0L,1L,0L},{0L,0xF8AB076BL,1L,1L,0xF8AB076BL,0L,0xF8AB076BL,1L,1L}};
static uint64_t g_2344[8] = {18446744073709551606UL,18446744073709551606UL,18446744073709551606UL,18446744073709551606UL,18446744073709551606UL,18446744073709551606UL,18446744073709551606UL,18446744073709551606UL};
static int32_t g_2403[9] = {0x3649E8EEL,0x3649E8EEL,0x8A9CBEBAL,0x3649E8EEL,0x3649E8EEL,0x8A9CBEBAL,0x3649E8EEL,0x3649E8EEL,0x8A9CBEBAL};
static int8_t ***g_2410[7][6][3] = {{{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986}},{{&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986},{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986}},{{&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986}},{{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986}},{{&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986},{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986}},{{&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986}},{{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986}}};
static int8_t ****g_2409[2][3] = {{&g_2410[3][1][0],&g_2410[3][1][0],&g_2410[3][1][0]},{&g_2410[3][1][0],&g_2410[3][1][0],&g_2410[3][1][0]}};
static int32_t ***g_2466 = &g_2152;
static int32_t **** volatile g_2465 = &g_2466;
static int16_t ** volatile * const volatile g_2489 = &g_309;
static uint16_t g_2520 = 2UL;
static uint64_t *g_2532 = (void*)0;
static uint64_t ** volatile g_2531 = &g_2532;
static int16_t ** const g_2546 = (void*)0;
static int16_t ** const *g_2545 = &g_2546;
static int16_t ** const **g_2544 = &g_2545;
static int16_t ** const ***g_2543 = &g_2544;
static int64_t ** const ***g_2722 = &g_1091;
static uint16_t *g_2785 = (void*)0;
static uint16_t * volatile * const volatile g_2784 = &g_2785;
static uint16_t * volatile * const volatile * volatile g_2783[8] = {&g_2784,&g_2784,(void*)0,&g_2784,&g_2784,(void*)0,&g_2784,&g_2784};
static uint16_t * volatile * const volatile * volatile *g_2782[4][9] = {{(void*)0,&g_2783[6],&g_2783[6],&g_2783[6],&g_2783[6],&g_2783[6],&g_2783[6],(void*)0,&g_2783[2]},{&g_2783[6],&g_2783[6],&g_2783[7],(void*)0,&g_2783[6],&g_2783[6],&g_2783[6],&g_2783[6],(void*)0},{&g_2783[6],&g_2783[6],&g_2783[6],(void*)0,&g_2783[6],&g_2783[6],&g_2783[6],&g_2783[2],&g_2783[2]},{&g_2783[6],&g_2783[6],&g_2783[2],&g_2783[6],&g_2783[2],&g_2783[6],&g_2783[6],&g_2783[6],&g_2783[7]}};
static uint16_t * volatile * const volatile * volatile **g_2781[1][8] = {{&g_2782[2][5],&g_2782[2][5],&g_2782[2][5],&g_2782[2][5],&g_2782[2][5],&g_2782[2][5],&g_2782[2][5],&g_2782[2][5]}};
static const uint8_t g_2826 = 0x18L;
static uint32_t g_2960 = 4294967294UL;
static int64_t * const ****g_2970 = (void*)0;
static int32_t ***** volatile g_2992 = (void*)0;
static uint16_t g_3000 = 0xEE19L;
static int32_t **** volatile *g_3121 = &g_2465;
static uint16_t g_3171 = 0x38C7L;
static uint32_t g_3261[8] = {18446744073709551610UL,18446744073709551610UL,18446744073709551610UL,18446744073709551610UL,18446744073709551610UL,18446744073709551610UL,18446744073709551610UL,18446744073709551610UL};
static volatile uint64_t g_3365 = 18446744073709551615UL;
static uint32_t * const *g_3409 = &g_169;
static uint32_t * const **g_3408 = &g_3409;
static uint32_t * const ***g_3407 = &g_3408;
static int32_t g_3415 = 0x015B4F93L;
static int32_t *g_3434 = &g_3415;
static int32_t ** volatile g_3433 = &g_3434;
static const int32_t g_3443[6][8][2] = {{{7L,(-8L)},{3L,(-1L)},{0xF1548BA0L,0x702257ECL},{(-8L),(-8L)},{(-1L),(-2L)},{0xEEA87B35L,0xCDD00C2EL},{0x777463FBL,0x9411A40DL},{0x702257ECL,0x777463FBL}},{{1L,0x0A1F1895L},{1L,0x777463FBL},{0x702257ECL,0x9411A40DL},{0x777463FBL,0xCDD00C2EL},{0xEEA87B35L,(-2L)},{(-1L),(-8L)},{(-8L),0x702257ECL},{0xF1548BA0L,(-1L)}},{{3L,(-8L)},{7L,1L},{0xEEA87B35L,0L},{0x65D59874L,0x9411A40DL},{(-1L),0x65D59874L},{1L,(-10L)},{0L,0x777463FBL},{(-1L),0xE00F23F2L}},{{0x777463FBL,0L},{0x49B6954FL,(-2L)},{7L,3L},{(-8L),(-1L)},{0x210E2E69L,(-1L)},{(-8L),3L},{7L,(-2L)},{0x49B6954FL,0L}},{{0x777463FBL,0xE00F23F2L},{(-1L),0x777463FBL},{0L,(-10L)},{1L,0x65D59874L},{(-1L),0x9411A40DL},{0x65D59874L,0L},{0xEEA87B35L,1L},{7L,(-8L)}},{{3L,(-1L)},{0xF1548BA0L,0x702257ECL},{(-8L),(-8L)},{(-1L),(-2L)},{0xEEA87B35L,0xCDD00C2EL},{0x777463FBL,0x9411A40DL},{0x702257ECL,0x777463FBL},{1L,0x0A1F1895L}}};



static uint32_t func_1(void);
static int32_t * func_3(int64_t p_4, uint32_t p_5, int32_t * p_6);
static int32_t * func_13(uint32_t p_14);
static uint8_t func_25(int16_t p_26);
static uint8_t func_36(int32_t * p_37);
static int32_t * func_38(uint16_t p_39);
static uint8_t func_44(uint32_t p_45, int32_t * p_46, uint32_t p_47);
static int16_t func_56(int8_t p_57);
static int32_t * func_64(const int64_t p_65, int32_t p_66, int16_t * p_67);
static int64_t func_68(const uint64_t p_69, uint32_t p_70, int32_t p_71);
# 170 "/tmp/test156_src_v2.c"
static uint32_t func_1(void)
{
    uint32_t l_10 = 0xCB61D225L;
    uint64_t l_2894 = 0xA634D3C826A29B52LL;
    const int8_t l_2917 = 0xE6L;
    uint8_t l_2918 = 249UL;
    int32_t *l_2927 = &g_1803;
    int64_t l_2959 = 0L;
    int32_t l_2984 = 0xAE2EA8EDL;
    int32_t l_2999[1];
    int16_t l_3013 = 7L;
    int32_t l_3052 = 0x55C2E9DEL;
    int64_t l_3094 = 0x0AAD04AC55E533BBLL;
    int64_t l_3120 = 8L;
    uint8_t l_3169 = 7UL;
    int32_t l_3224 = 0x19860080L;
    uint32_t l_3225 = 4UL;
    int32_t l_3226 = 0xA328975BL;
    int32_t **l_3271 = (void*)0;
    uint32_t l_3283[8] = {0xD56BEA88L,0xD56BEA88L,0xD56BEA88L,0xD56BEA88L,0xD56BEA88L,0xD56BEA88L,0xD56BEA88L,0xD56BEA88L};
    int8_t l_3292 = 0L;
    int8_t ***l_3338 = &g_1986;
    const uint32_t **l_3352 = &g_2122;
    uint32_t l_3362 = 0x3BE68635L;
    uint8_t l_3380[6][6][7] = {{{0xC1L,0x79L,3UL,0UL,255UL,0UL,3UL},{0x7AL,0x7AL,0x06L,255UL,0xB3L,0x0CL,255UL},{0xD6L,255UL,255UL,0x5CL,248UL,0x71L,0x83L},{0xABL,249UL,1UL,255UL,1UL,1UL,0x63L},{0xAAL,1UL,0x5CL,0x8FL,0x33L,0x1DL,0UL},{0x12L,255UL,0UL,1UL,5UL,0x06L,0x5BL}},{{255UL,1UL,0x70L,0x5CL,255UL,4UL,1UL},{0xC8L,1UL,0x0CL,254UL,255UL,0x38L,0xD6L},{0x52L,255UL,0xE8L,255UL,0x60L,0x48L,0x0FL},{252UL,251UL,0UL,255UL,0UL,251UL,252UL},{1UL,0xCBL,1UL,255UL,0UL,1UL,1UL},{0UL,3UL,255UL,0xABL,0UL,252UL,253UL}},{{255UL,255UL,1UL,0UL,0x8FL,255UL,1UL},{3UL,255UL,0UL,253UL,0xFCL,6UL,0xE6L},{1UL,0x8FL,0xE8L,250UL,0xC1L,0x60L,0x70L},{254UL,0x38L,0x0CL,0x7AL,255UL,5UL,1UL},{3UL,0x0FL,0x70L,0x50L,0x79L,255UL,0x60L},{0x5BL,255UL,0UL,0x0DL,0xB3L,0xC9L,0xC9L}},{{3UL,1UL,0x5CL,1UL,3UL,0x50L,0UL},{5UL,0UL,1UL,0xF9L,255UL,255UL,0x0CL},{0x71L,0x48L,0x33L,255UL,4UL,0xF3L,0x47L},{5UL,0xF9L,0xA5L,0xFCL,0x4DL,0UL,255UL},{3UL,0xCFL,1UL,246UL,0x7CL,0x67L,1UL},{0x5BL,0xA7L,0xF9L,0x0CL,253UL,1UL,2UL}},{{3UL,255UL,252UL,0x7CL,0x48L,0x8FL,248UL},{254UL,0UL,9UL,5UL,2UL,0xA7L,255UL},{1UL,1UL,0xF3L,1UL,0UL,0UL,1UL},{3UL,255UL,3UL,0x62L,0x00L,0x36L,0xABL},{255UL,0UL,246UL,0xD6L,1UL,251UL,0x92L},{0UL,9UL,0x06L,0xB2L,9UL,0x36L,249UL}},{{1UL,0UL,0x78L,0x83L,255UL,1UL,1UL},{0xECL,0xABL,1UL,255UL,252UL,1UL,0x4DL},{0x1DL,3UL,0xCFL,255UL,1UL,0x67L,255UL},{253UL,9UL,0x0CL,0xA5L,0UL,252UL,0xE6L},{248UL,0x5CL,255UL,255UL,0xD6L,0xCFL,255UL},{5UL,249UL,0x02L,0UL,0UL,0x02L,249UL}}};
    uint32_t l_3385 = 1UL;
    int16_t *l_3440 = &l_3013;
    int i, j, k;
    for (i = 0; i < 1; i++)
        l_2999[i] = 0x1196B8D3L;
    if ((g_2 , 0x6CE125D1L))
    {
        int16_t l_18[4];
        int32_t *l_2893[2];
        uint16_t *l_2909 = &g_2520;
        uint8_t l_2919 = 0x94L;
        int8_t l_2920 = 3L;
        uint64_t l_2951[6];
        uint64_t l_2985 = 0xCFBD471508EF5E45LL;
        uint32_t l_3031 = 0xC829C5C4L;
        uint32_t l_3095 = 0x7B7E7231L;
        const int32_t l_3193 = 0x8530F2E2L;
        int32_t l_3202 = (-1L);
        int64_t ****l_3219[7];
        int64_t l_3327[5];
        uint8_t l_3333 = 255UL;
        const int32_t *l_3438 = &g_3415;
        int i;
        for (i = 0; i < 4; i++)
            l_18[i] = 1L;
        for (i = 0; i < 2; i++)
            l_2893[i] = &g_2284[1][8];
        for (i = 0; i < 6; i++)
            l_2951[i] = 2UL;
        for (i = 0; i < 7; i++)
            l_3219[i] = &g_777;
        for (i = 0; i < 5; i++)
            l_3327[i] = 3L;
lbl_2989:
        l_2893[1] = ((*g_254) = func_3(g_2, ((safe_sub_func_uint32_t_u_u((g_9 || l_10), ((void*)0 != g_11))) , 4294967286UL), func_13((safe_add_func_int64_t_s_s((!((l_18[0] , l_18[1]) != 0x939F5FA3187110E6LL)), l_18[3])))));
        if ((((l_2894 , l_10) || ((safe_mul_func_uint8_t_u_u((**g_2178), (((safe_sub_func_uint16_t_u_u(((safe_sub_func_uint32_t_u_u(l_10, (safe_add_func_uint8_t_u_u((((((((safe_mul_func_uint8_t_u_u((safe_add_func_int32_t_s_s(((((void*)0 != &g_2532) | ((**g_168)++)) == (++(*l_2909))), (safe_unary_minus_func_int8_t_s(((safe_div_func_int16_t_s_s((safe_mod_func_uint64_t_u_u((l_2917 <= l_2918), ((l_2917 != l_10) , l_2917))), 2L)) && (**g_2178)))))), l_2917)) < l_2917) , (*g_2179)) < l_2919) <= l_2918) , l_2894) < l_10), 0xFBL)))) , l_2920), l_2917)) , &g_2531) == &g_2531))) , (*g_2220))) < l_2918))
        {
            uint16_t l_2934 = 0xE0F8L;
            int32_t l_2958[2];
            int32_t l_2961 = 8L;
            const int32_t *l_3018 = &g_195;
            const int32_t **l_3017 = &l_3018;
            const int32_t ***l_3016 = &l_3017;
            const int32_t ****l_3015 = &l_3016;
            int32_t *****l_3030 = &g_1673[0];
            uint32_t l_3059 = 0x62A98EA7L;
            int32_t l_3061 = 7L;
            int i;
            for (i = 0; i < 2; i++)
                l_2958[i] = (-9L);
            for (g_316 = 0; (g_316 < 10); g_316 = safe_add_func_uint8_t_u_u(g_316, 5))
            {
                uint32_t l_2956 = 0UL;
                uint16_t *l_2957 = &g_202;
                int64_t ****l_2972 = &g_777;
                int64_t *****l_2971 = &l_2972;
                int16_t *l_2983 = &l_18[0];
                uint64_t *l_2986 = &g_2344[6];
                int32_t l_2996 = 4L;
                int16_t l_2998 = 0x2651L;
                int32_t *****l_3029 = (void*)0;
                const int64_t l_3051[6] = {9L,9L,9L,9L,9L,9L};
                uint8_t l_3122 = 0x40L;
                int i;
                l_2961 |= ((((safe_lshift_func_uint16_t_u_u((((((safe_mul_func_int8_t_s_s((l_2927 != (*g_254)), ((((safe_rshift_func_uint16_t_u_s(((safe_lshift_func_int8_t_s_u(((safe_rshift_func_uint16_t_u_s((l_2934 > l_2934), (safe_mul_func_uint16_t_u_u(l_2934, (((((***g_777) = (~(safe_rshift_func_int16_t_s_s((&l_2934 == ((safe_rshift_func_uint8_t_u_u(((**g_2178) = (((((*l_2927) = ((safe_lshift_func_uint16_t_u_u(((*l_2957) = (safe_add_func_uint64_t_u_u((safe_mul_func_uint8_t_u_u((safe_rshift_func_uint16_t_u_u((+(l_2951[4]--)), 6)), (safe_lshift_func_uint16_t_u_s(((*l_2909) ^= l_2956), 13)))), (-5L)))), l_2958[0])) >= (*l_2927))) & l_2956) && l_2959) >= 0x20L)), 0)) , (void*)0)), l_2958[0])))) > 0x389902F9F1ED3449LL) & 0xF781L) ^ 0UL))))) , l_2958[0]), l_2958[0])) <= l_2956), l_2918)) <= l_2956) >= 0L) && (*l_2927)))) > g_125) , l_2958[0]) ^ 0x57AE191B6FA7DF3ALL) == l_2958[0]), l_2956)) && l_2934) <= g_2960) <= (-1L));
                if ((safe_mod_func_uint64_t_u_u((safe_lshift_func_uint16_t_u_u(((safe_add_func_uint64_t_u_u(((*l_2986) = (((0x6BA866DFL == ((*g_11) | ((safe_add_func_int8_t_s_s(l_2956, (((g_2970 = g_2970) == l_2971) <= (l_2956 | ((safe_lshift_func_int8_t_s_u((safe_rshift_func_int16_t_s_s((l_2984 &= (((safe_lshift_func_int16_t_s_u(((*l_2983) |= (((((*l_2927) = (!g_295)) <= (safe_add_func_int8_t_s_s(0L, ((+l_2959) == l_2958[0])))) , 8L) < (**g_2178))), 1)) || l_2956) ^ (**g_2178))), 1)), 1)) & l_2956))))) , (*l_2927)))) > l_2985) == l_10)), 2L)) , l_2956), l_2934)), 1UL)))
                {
                    int32_t l_2995 = 0x340D4978L;
                    int32_t l_2997 = 0L;
                    for (g_131 = 2; (g_131 <= 10); g_131++)
                    {
                        return (*l_2927);
                    }
                    if (l_2920)
                        goto lbl_2989;
                    for (g_195 = 10; (g_195 == 21); g_195 = safe_add_func_uint64_t_u_u(g_195, 8))
                    {
                        int32_t ****l_2994 = &g_2466;
                        int32_t *****l_2993 = &l_2994;
                        (*l_2993) = &g_2466;
                        return g_2826;
                    }
                    g_3000++;
                }
                else
                {
                    const int64_t l_3005 = 0L;
                    int32_t l_3006[8][6][5] = {{{(-1L),0x2E62EF46L,0xD48111B5L,0x9097DCECL,0x1B4D9216L},{0x348CEC48L,0x7BB600CDL,0x977F9664L,9L,0L},{(-5L),0xD48111B5L,0x1B4D9216L,0x2E62EF46L,0x1B4D9216L},{9L,9L,0L,0x977F9664L,2L},{0x1B4D9216L,0L,1L,0xE2C0D9F1L,(-10L)},{8L,1L,0L,0xB674216FL,7L}},{{(-1L),0L,0L,(-1L),7L},{0x7BB600CDL,9L,2L,2L,5L},{1L,0xD48111B5L,0L,(-5L),0L},{1L,0x7BB600CDL,2L,2L,0x7BB600CDL},{(-10L),0x2E62EF46L,0x1ABD91C7L,(-1L),(-1L)},{8L,7L,9L,0xB674216FL,2L}},{{0x2E62EF46L,1L,(-5L),0xE2C0D9F1L,0xE2C0D9F1L},{8L,0L,8L,0x977F9664L,0xABEA4E0DL},{(-10L),0x1ABD91C7L,(-5L),0x2E62EF46L,0x9097DCECL},{1L,0xB674216FL,5L,9L,7L},{1L,0x9097DCECL,(-5L),0x9097DCECL,1L},{0x7BB600CDL,0xC3E8EB99L,8L,7L,9L}},{{(-1L),(-1L),(-5L),0x1B4D9216L,0L},{8L,8L,9L,0xC3E8EB99L,9L},{0x1B4D9216L,0x1B4D9216L,0x1ABD91C7L,(-5L),1L},{9L,2L,2L,5L,7L},{(-5L),0xCB82F9DFL,0L,1L,0x9097DCECL},{0x348CEC48L,2L,2L,0x348CEC48L,0xABEA4E0DL}},{{(-1L),0x1B4D9216L,0L,1L,0xE2C0D9F1L},{2L,8L,0L,8L,2L},{0xCB82F9DFL,(-1L),1L,1L,(-1L)},{7L,0xC3E8EB99L,0L,0x348CEC48L,0x7BB600CDL},{0xD48111B5L,0x9097DCECL,0x1B4D9216L,1L,0L},{0x348CEC48L,1L,0x7BB600CDL,2L,2L}},{{7L,1L,7L,(-1L),0x1ABD91C7L},{0x977F9664L,0xB674216FL,0xC3E8EB99L,0x348CEC48L,9L},{0x9097DCECL,0xCB82F9DFL,1L,(-5L),(-5L)},{8L,9L,0xC3E8EB99L,9L,8L},{0L,(-1L),7L,0x1B4D9216L,(-5L)},{7L,2L,0x7BB600CDL,8L,5L}},{{0x2E62EF46L,7L,(-5L),(-1L),(-5L)},{8L,8L,0xB674216FL,0x7BB600CDL,8L},{(-5L),0L,0xD48111B5L,1L,(-5L)},{0xC3E8EB99L,7L,5L,1L,9L},{(-10L),0L,0L,(-10L),0x1ABD91C7L},{2L,8L,0L,8L,2L}},{{0xD48111B5L,7L,0xE2C0D9F1L,0x2E62EF46L,0L},{7L,2L,8L,8L,2L},{(-5L),(-1L),1L,(-10L),0L},{0xABEA4E0DL,9L,8L,1L,0L},{(-1L),0xCB82F9DFL,(-1L),1L,1L},{0xABEA4E0DL,0xB674216FL,0xABEA4E0DL,0x7BB600CDL,0L}}};
                    int32_t ****l_3014[5][9] = {{&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466},{&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466},{&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466},{&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466},{&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466,&g_2466}};
                    uint64_t l_3035 = 0x1677C99ACAE21BCALL;
                    int32_t l_3036 = (-1L);
                    uint64_t l_3091 = 18446744073709551615UL;
                    uint32_t *l_3096 = (void*)0;
                    uint8_t l_3097 = 0x97L;
                    int8_t l_3117 = 0xE4L;
                    int i, j, k;
                    (*l_2927) = ((*l_2927) <= (((safe_mod_func_uint8_t_u_u(((*l_2927) , (l_3006[2][2][4] = (l_3005 == (*g_11)))), 3UL)) <= (((safe_lshift_func_int8_t_s_u((((safe_add_func_uint16_t_u_u(l_2956, 0x599EL)) , l_2961) | ((***g_777) = (safe_div_func_uint16_t_u_u((l_3013 , l_2961), 0xE145L)))), (**g_2178))) , l_3005) >= l_3005)) , (*l_2927)));
                    if (((l_3014[4][3] == l_3015) <= (((l_3006[2][2][4] != (*g_11)) || (((*l_2909) = 0x2694L) == (safe_mul_func_int16_t_s_s(l_3006[7][4][3], ((safe_div_func_int64_t_s_s((l_2956 && 0UL), (safe_add_func_uint32_t_u_u((safe_add_func_uint32_t_u_u((((l_3030 = ((safe_div_func_uint16_t_u_u(1UL, 0xCF89L)) , l_3029)) == &g_1673[0]) >= l_3031), l_2934)), (**g_168))))) , l_2958[0]))))) > 0x7F2086D5L)))
                    {
                        int16_t l_3034 = 0x3DC4L;
                        int32_t l_3060[8] = {0x6206FE42L,0x6206FE42L,0x6206FE42L,0x6206FE42L,0x6206FE42L,0x6206FE42L,0x6206FE42L,0x6206FE42L};
                        int32_t l_3062 = 1L;
                        int8_t *l_3072 = &g_1581;
                        int i;
                        l_3062 &= ((+((+((l_3034 >= (l_3035 != ((*g_169)--))) && (l_3060[2] = ((safe_rshift_func_uint16_t_u_u(l_3006[2][2][4], (((((safe_lshift_func_uint16_t_u_s(((safe_div_func_int16_t_s_s(((safe_sub_func_uint64_t_u_u((safe_lshift_func_uint8_t_u_u((safe_div_func_uint64_t_u_u(l_3051[4], l_3035)), 6)), l_3052)) != (*l_2927)), (safe_rshift_func_uint16_t_u_u((*l_2927), (safe_sub_func_int32_t_s_s(((safe_mod_func_int64_t_s_s((**g_517), l_3059)) ^ (**g_517)), 4294967295UL)))))) && (*g_2179)), 15)) , l_2893[1]) != (void*)0) < (***g_777)) | l_3006[5][0][1]))) & 0x2CL)))) ^ (*g_2122))) || l_3061);
                        (*g_2220) = 0x79C192D5L;
                        (*g_2220) = ((safe_sub_func_int32_t_s_s(2L, ((*g_169) &= (*l_2927)))) && (safe_add_func_uint8_t_u_u((safe_mod_func_uint16_t_u_u((safe_div_func_uint32_t_u_u((((*l_3072) |= (!l_3006[5][2][0])) >= (((*l_2983) = (safe_mul_func_int16_t_s_s((l_3062 = (l_3059 || (*l_2927))), l_3051[4]))) != (safe_mul_func_uint16_t_u_u(((safe_add_func_int32_t_s_s(((**g_168) , (*l_2927)), l_3006[7][0][3])) <= l_3006[1][2][2]), l_3006[2][0][4])))), l_3006[2][2][4])), (*g_1448))), (-6L))));
                    }
                    else
                    {
                        uint32_t l_3114 = 18446744073709551611UL;
                        int8_t *l_3118 = &l_3117;
                        int8_t l_3119 = 0xDEL;
                        l_3097 &= ((g_12 , ((((safe_sub_func_int64_t_s_s((((((*l_2927) != (safe_mod_func_int64_t_s_s((safe_mul_func_int8_t_s_s((((*l_2909) |= ((safe_mod_func_int16_t_s_s((safe_mul_func_int8_t_s_s((((safe_div_func_uint8_t_u_u((((*g_2178) != (l_3091 , &l_2918)) < 0x93L), l_3006[2][2][4])) , (**g_517)) , (safe_add_func_uint32_t_u_u(0xC381B216L, l_3091))), 0UL)), 0x7252L)) && (-1L))) > l_3006[3][5][4]), 0UL)), 0xDA164009ECDB52DELL))) ^ 1UL) , (**g_168)) > 0x03DBFEC6L), l_3094)) , l_3095) , 0x9FC5L) , l_3096)) != &l_3095);
                        (*g_2220) = (safe_mul_func_int8_t_s_s((-1L), (+((safe_rshift_func_int16_t_s_s((-1L), 14)) ^ ((((*l_3118) = ((safe_div_func_int8_t_s_s((*l_2927), (((~l_2958[0]) || ((*l_2927) != (safe_mul_func_uint16_t_u_u(((safe_lshift_func_int16_t_s_s(0x5C6FL, ((*l_2983) = l_3036))) <= ((safe_rshift_func_int16_t_s_s(((safe_mod_func_int8_t_s_s((l_3114 || (safe_add_func_uint16_t_u_u(l_3117, l_3097))), 9UL)) || (-4L)), l_3097)) == 4294967295UL)), (*l_2927))))) | (**g_517)))) & l_3006[2][2][4])) ^ l_3119) , l_3120)))));
                    }
                }
                g_3121 = &g_2465;
                l_3122--;
            }
        }
        else
        {
            int8_t l_3128 = 1L;
            int8_t *l_3129 = &g_1909;
            uint32_t *l_3147 = &g_2960;
            int32_t l_3172 = (-1L);
            uint16_t **l_3212 = (void*)0;
            uint8_t l_3230[4] = {0x42L,0x42L,0x42L,0x42L};
            uint32_t l_3282 = 0UL;
            uint32_t l_3293 = 18446744073709551615UL;
            int8_t ** const *l_3339 = &g_1986;
            int32_t l_3383 = 0L;
            int32_t l_3384 = 1L;
            uint32_t * const ** const *l_3406 = (void*)0;
            int i;
lbl_3347:
            (*g_254) = l_2893[1];
            for (g_125 = 11; (g_125 >= 45); g_125 = safe_add_func_int64_t_s_s(g_125, 1))
            {
                uint32_t l_3158 = 0UL;
                int32_t l_3167 = 0x1341709DL;
                int32_t l_3170 = 0x243A4EF9L;
                uint64_t *l_3196[3][7] = {{&l_2894,&g_2344[4],&l_2894,&l_2894,&g_2344[4],&l_2894,&l_2894},{&g_2344[2],&g_2344[2],&l_2951[4],&g_2344[2],&g_2344[2],&l_2951[4],&g_2344[2]},{&g_2344[4],&l_2894,&l_2894,&g_2344[4],&l_2894,&l_2894,&g_2344[4]}};
                uint16_t **l_3213 = &l_2909;
                int32_t l_3255 = 8L;
                int16_t **l_3294 = &g_164;
                int64_t l_3328 = 1L;
                int i, j;
            }
            for (l_2959 = 2; (l_2959 >= 0); l_2959 -= 1)
            {
                int32_t *l_3349 = &l_2984;
                uint8_t ***l_3355 = &g_2178;
                int32_t l_3379 = 8L;
                int32_t l_3392 = 0L;
                int16_t l_3432 = (-1L);
                int i;
                if (g_109[l_2959])
                    break;
                for (l_3128 = 2; (l_3128 >= 0); l_3128 -= 1)
                {
                    if (g_1449)
                        goto lbl_3347;
                    return g_109[0];
                }
                for (l_3202 = 0; (l_3202 <= 2); l_3202 += 1)
                {
                    const uint8_t l_3372 = 0UL;
                    uint32_t ****l_3378 = &g_587[2];
                    uint32_t *****l_3377 = &l_3378;
                    int32_t l_3382 = 0L;
                    uint32_t * const ****l_3410 = &g_3407;
                    int16_t *l_3418 = &g_61;
                    uint64_t **l_3425 = (void*)0;
                    uint64_t *l_3427[7] = {&g_2344[0],&g_2344[0],&g_2344[0],&g_2344[0],&g_2344[0],&g_2344[0],&g_2344[0]};
                    uint64_t **l_3426 = &l_3427[6];
                    int i;
                    for (g_3171 = 0; (g_3171 <= 2); g_3171 += 1)
                    {
                        int32_t l_3348 = 0xD311B2D7L;
                        int16_t l_3358 = 0x053FL;
                        uint32_t l_3359 = 18446744073709551615UL;
                        int32_t ****l_3366 = (void*)0;
                        int32_t l_3381 = 5L;
                        int i;
                        (**g_1674) = func_3(l_3282, (l_3172 = ((*g_169) = (l_3348 = (*g_169)))), l_3349);
                        (*g_2220) &= (8L >= (((**g_2178) || (safe_mul_func_int16_t_s_s(((void*)0 == l_3352), ((((((safe_add_func_uint64_t_u_u(0x7BCD4C478189869ALL, (&g_2178 != l_3355))) < (l_3230[3] && (((((safe_lshift_func_uint16_t_u_s(l_3358, l_3348)) , 255UL) & l_3358) | 0L) >= l_3359))) || l_3230[2]) >= 1UL) , (*g_712)) == (void*)0)))) < 0x4A627569C5935151LL));
                        (*l_3349) = (((safe_div_func_int32_t_s_s((l_3362 <= (safe_add_func_int32_t_s_s(((g_3365 , l_3366) == ((safe_lshift_func_int8_t_s_u((+(((((((*l_3129) = (safe_add_func_int16_t_s_s(l_3372, (l_3352 == (void*)0)))) & ((*l_3349) & (safe_mod_func_int32_t_s_s((l_3379 = ((*l_2927) = (safe_mul_func_int8_t_s_s(l_3372, ((void*)0 == l_3377))))), l_3230[2])))) && l_3372) <= 0xD0L) & (*l_3349)) , (*l_3349))), 2)) , (void*)0)), 0xD3141DE6L))), l_3282)) | (**g_2178)) | l_3380[4][3][4]);
                        l_3385++;
                    }
                    if (g_2080)
                        goto lbl_3347;
                    (*l_3349) = (safe_sub_func_int8_t_s_s((safe_lshift_func_int8_t_s_s((l_3392 > ((safe_unary_minus_func_uint64_t_u((l_2951[l_3202]++))) , (((**g_3121) = (*g_2465)) != (void*)0))), (safe_lshift_func_uint16_t_u_u(65535UL, (safe_add_func_int64_t_s_s(((((safe_div_func_uint16_t_u_u(65535UL, (safe_lshift_func_int8_t_s_s(((((**g_517) = (*l_2927)) , l_3406) != ((*l_3410) = g_3407)), 4)))) < ((safe_add_func_int8_t_s_s((safe_add_func_uint64_t_u_u(1UL, 18446744073709551615UL)), l_3382)) >= (-10L))) ^ 0x89L) , 0x37D3720AB66515D9LL), l_3382)))))), g_3415));
                    (*g_3433) = ((*g_254) = ((((*l_3349) == (safe_div_func_int16_t_s_s(((*l_3418) = l_3383), (safe_rshift_func_uint16_t_u_s(((safe_lshift_func_uint8_t_u_u((**g_2178), 5)) != (safe_div_func_uint32_t_u_u((((((*g_2179) , ((*g_2531) == ((*l_3426) = &l_2951[4]))) , ((safe_mod_func_int16_t_s_s((safe_add_func_int64_t_s_s((*l_3349), (l_3432 ^ 0xD353L))), (*l_3349))) , (****g_3407))) & (***g_3408)) , l_3128), (*g_169)))), 13))))) > (*l_2927)) , (void*)0));
                }
            }
            for (l_3013 = (-14); (l_3013 == 20); l_3013 = safe_add_func_int64_t_s_s(l_3013, 2))
            {
                int8_t l_3437 = 1L;
                if (l_3437)
                    break;
                l_3438 = &l_3193;
            }
        }
        for (g_3415 = 0; (g_3415 <= 3); g_3415 += 1)
        {
            int i;
            return g_984[(g_3415 + 3)];
        }
    }
    else
    {
        const int32_t *l_3442 = &g_3443[4][5][0];
        const int32_t **l_3441 = &l_3442;
        const int32_t *l_3444 = (void*)0;
        l_3444 = ((*l_3441) = func_64((+((*l_2927) == ((*g_1448) || 0x85FEL))), (*l_2927), l_3440));
        return g_12;
    }
    return (*l_2927);
}







static int32_t * func_3(int64_t p_4, uint32_t p_5, int32_t * p_6)
{
    const int64_t *l_2239 = &g_315;
    const int64_t **l_2238 = &l_2239;
    const int64_t *** const l_2237 = &l_2238;
    int32_t l_2242 = 0xB1F4958DL;
    int32_t l_2243 = 0L;
    int16_t ***l_2250 = &g_309;
    int32_t l_2251 = 0xE872FFC6L;
    uint64_t *l_2252[8][3][7] = {{{&g_161,&g_97,&g_97,&g_295,&g_161,&g_97,&g_161},{(void*)0,(void*)0,(void*)0,&g_295,&g_295,&g_97,&g_97},{&g_161,&g_161,&g_161,&g_161,&g_295,&g_295,&g_161}},{{&g_97,&g_295,&g_97,(void*)0,&g_161,&g_161,&g_97},{(void*)0,&g_295,&g_161,(void*)0,(void*)0,&g_161,&g_161},{&g_97,&g_161,&g_97,&g_295,&g_97,&g_97,&g_161}},{{(void*)0,&g_295,(void*)0,&g_295,&g_161,&g_295,(void*)0},{(void*)0,&g_97,&g_295,(void*)0,&g_161,&g_161,&g_161},{&g_97,(void*)0,(void*)0,(void*)0,&g_161,&g_161,&g_295}},{{(void*)0,&g_97,&g_97,(void*)0,&g_97,&g_295,&g_97},{&g_295,&g_161,(void*)0,(void*)0,&g_295,&g_97,(void*)0},{&g_161,(void*)0,&g_97,&g_161,(void*)0,&g_161,&g_97}},{{&g_161,&g_161,&g_97,&g_97,&g_161,&g_161,&g_161},{(void*)0,&g_295,(void*)0,&g_161,&g_97,&g_295,(void*)0},{&g_161,&g_97,&g_161,&g_295,&g_161,&g_97,&g_295}},{{&g_295,(void*)0,&g_295,&g_97,(void*)0,&g_97,&g_97},{(void*)0,&g_295,&g_295,&g_295,&g_295,&g_161,(void*)0},{&g_295,&g_161,&g_295,(void*)0,&g_97,&g_97,&g_161}},{{(void*)0,&g_161,&g_161,&g_295,&g_161,&g_161,&g_295},{&g_295,&g_161,&g_161,&g_295,&g_161,&g_161,(void*)0},{&g_97,&g_97,&g_161,(void*)0,&g_161,&g_97,&g_161}},{{&g_97,&g_97,&g_161,&g_295,&g_97,&g_295,&g_161},{&g_161,&g_97,(void*)0,&g_97,(void*)0,&g_97,&g_161},{&g_161,(void*)0,&g_295,&g_295,&g_161,&g_97,(void*)0}}};
    int32_t l_2253 = 0x626CD460L;
    int32_t l_2254 = (-1L);
    int32_t l_2255 = (-5L);
    int32_t l_2256 = (-2L);
    uint32_t l_2321 = 1UL;
    int32_t l_2386 = 6L;
    int32_t l_2389 = 0xE351A609L;
    int32_t l_2391[8] = {(-10L),(-10L),0x12C9317AL,(-10L),(-10L),0x12C9317AL,(-10L),(-10L)};
    uint64_t l_2404 = 0xAC298B4A01C31E92LL;
    uint16_t *l_2430 = &g_109[2];
    uint16_t **l_2429 = &l_2430;
    int16_t l_2457[6][8] = {{0x4E75L,(-10L),(-10L),0x4E75L,0x52D1L,(-7L),(-7L),0x52D1L},{0x4E75L,(-10L),(-10L),0x4E75L,0x52D1L,(-7L),(-7L),0x52D1L},{0x4E75L,(-10L),(-10L),0x4E75L,0x52D1L,(-7L),(-7L),0x52D1L},{0x4E75L,(-10L),(-10L),0x4E75L,0x52D1L,(-7L),(-7L),0x52D1L},{0x4E75L,(-10L),(-10L),0x4E75L,0x52D1L,(-7L),(-7L),0x52D1L},{0x4E75L,(-10L),(-10L),0x4E75L,0x52D1L,(-7L),(-7L),0x52D1L}};
    int64_t l_2458 = 0xCD46132A9E99FB6ELL;
    int32_t l_2459 = 0xE7C5B995L;
    uint16_t l_2460[7] = {65528UL,65528UL,65528UL,65528UL,65528UL,65528UL,65528UL};
    int16_t ** const ***l_2542 = (void*)0;
    int8_t ***l_2569[7][9][4] = {{{&g_1986,(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986,(void*)0},{&g_1986,(void*)0,&g_1986,(void*)0},{(void*)0,(void*)0,&g_1986,(void*)0},{&g_1986,(void*)0,&g_1986,(void*)0},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986,(void*)0},{&g_1986,&g_1986,(void*)0,(void*)0},{&g_1986,&g_1986,&g_1986,&g_1986}},{{&g_1986,&g_1986,&g_1986,(void*)0},{(void*)0,(void*)0,&g_1986,&g_1986},{(void*)0,&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986,(void*)0},{(void*)0,&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,(void*)0,(void*)0},{&g_1986,&g_1986,&g_1986,(void*)0}},{{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,(void*)0,(void*)0},{(void*)0,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,(void*)0,&g_1986},{(void*)0,&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986,&g_1986}},{{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,(void*)0,&g_1986}},{{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,(void*)0},{(void*)0,&g_1986,(void*)0,&g_1986},{&g_1986,&g_1986,(void*)0,&g_1986},{(void*)0,&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986,(void*)0}},{{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{(void*)0,&g_1986,&g_1986,&g_1986},{(void*)0,(void*)0,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,(void*)0},{(void*)0,&g_1986,(void*)0,(void*)0},{&g_1986,&g_1986,(void*)0,&g_1986}},{{(void*)0,&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,(void*)0,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,(void*)0},{&g_1986,&g_1986,&g_1986,(void*)0},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,&g_1986,&g_1986,&g_1986},{&g_1986,(void*)0,&g_1986,&g_1986}}};
    int16_t l_2657 = 0xA9A7L;
    uint8_t l_2688 = 0xC9L;
    uint16_t l_2692 = 65534UL;
    uint32_t l_2730 = 0x658F1977L;
    int8_t l_2841 = 0x1DL;
    uint32_t **l_2850 = &g_169;
    int64_t ***l_2865 = &g_517;
    uint32_t l_2890 = 5UL;
    int i, j, k;
    if (((*p_6) = ((l_2237 != ((l_2243 ^= (safe_lshift_func_int16_t_s_s(l_2242, 4))) , &l_2238)) != ((safe_mod_func_int16_t_s_s(((safe_rshift_func_uint8_t_u_u(((**g_2178) = ((g_127 , g_61) , (65535UL && (safe_mod_func_uint64_t_u_u((((void*)0 == l_2250) , (--g_295)), (+(((((safe_mod_func_int64_t_s_s(((***g_777) = ((safe_sub_func_uint8_t_u_u(0xA0L, (**g_2178))) & p_5)), l_2256)) , 0xE7835CA0ACCE9912LL) <= g_1449) <= p_4) , l_2256))))))), l_2255)) < p_5), l_2255)) ^ p_4))))
    {
        int32_t *l_2268 = &g_552;
        uint16_t ****l_2270 = &g_927;
        int32_t l_2285 = (-3L);
        const int16_t * const ***l_2300[4] = {&g_714,&g_714,&g_714,&g_714};
        int64_t * const ***l_2325[2];
        int64_t * const ****l_2324 = &l_2325[1];
        int16_t l_2337 = 0x2DE2L;
        int32_t **l_2346 = (void*)0;
        int32_t l_2392 = (-4L);
        int32_t l_2393[6][1][5] = {{{0x9D05BA77L,0L,0L,0x9D05BA77L,0L}},{{0x9D05BA77L,0x2AB60E70L,0x2FF40FC0L,0x9D05BA77L,0x77B568BEL}},{{0x72A72759L,0x2AB60E70L,0L,0x72A72759L,0x77B568BEL}},{{0x9D05BA77L,0L,0L,0x9D05BA77L,0L}},{{0x9D05BA77L,0x2AB60E70L,0x2FF40FC0L,0x9D05BA77L,0x77B568BEL}},{{0x72A72759L,0x2AB60E70L,0L,0x72A72759L,0x77B568BEL}}};
        int64_t l_2396[4];
        int32_t l_2456 = (-1L);
        uint64_t **l_2533 = &l_2252[5][0][2];
        uint64_t * const *l_2556 = &l_2252[3][1][2];
        int8_t ***l_2572[3][5];
        uint32_t l_2583 = 0x483BA15FL;
        const int32_t l_2598[5][1] = {{0x27922238L},{(-4L)},{0x27922238L},{(-4L)},{0x27922238L}};
        int i, j, k;
        for (i = 0; i < 2; i++)
            l_2325[i] = (void*)0;
        for (i = 0; i < 4; i++)
            l_2396[i] = 0xF47B5881F6C987F9LL;
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 5; j++)
                l_2572[i][j] = (void*)0;
        }
lbl_2506:
        for (g_887 = 0; (g_887 >= 22); g_887 = safe_add_func_uint64_t_u_u(g_887, 7))
        {
            int8_t l_2278 = (-1L);
            int16_t **l_2318[9][9][1] = {{{(void*)0},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{(void*)0}},{{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164}},{{&g_164},{(void*)0},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164}},{{(void*)0},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164}},{{&g_164},{&g_164},{(void*)0},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164}},{{&g_164},{(void*)0},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164}},{{&g_164},{&g_164},{&g_164},{(void*)0},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164}},{{&g_164},{&g_164},{&g_164},{&g_164},{(void*)0},{&g_164},{(void*)0},{&g_164},{&g_164}},{{&g_164},{(void*)0},{&g_164},{(void*)0},{&g_164},{&g_164},{&g_164},{&g_164},{&g_164}}};
            int32_t l_2319[1][10][5] = {{{2L,0x68873666L,2L,0x28232BB5L,0L},{8L,0L,8L,0xBA41B96AL,0xBA41B96AL},{2L,0x68873666L,2L,0x28232BB5L,0L},{8L,0L,8L,0xBA41B96AL,0xBA41B96AL},{2L,0x68873666L,2L,0x28232BB5L,0L},{8L,0L,8L,0xBA41B96AL,0xBA41B96AL},{2L,0x68873666L,2L,0x28232BB5L,0L},{8L,0L,8L,0xBA41B96AL,0xBA41B96AL},{2L,0x68873666L,2L,0x28232BB5L,0L},{8L,0L,8L,0xBA41B96AL,0xBA41B96AL}}};
            uint32_t l_2320 = 0UL;
            uint16_t ***l_2326 = &g_928;
            int64_t *****l_2327 = (void*)0;
            uint8_t *l_2330 = &g_102;
            int i, j, k;
            for (g_1909 = 0; (g_1909 <= 2); g_1909 += 1)
            {
                int16_t l_2269 = 0xC268L;
                uint16_t *****l_2271 = &l_2270;
                int64_t l_2295 = (-1L);
                int32_t l_2309 = 0xF75F4625L;
                for (g_2103 = 0; (g_2103 <= 2); g_2103 += 1)
                {
                    (*g_2220) |= ((*l_2237) == (void*)0);
                    return p_6;
                }
                (*p_6) = (safe_rshift_func_int8_t_s_s((((g_319 || (l_2268 != (*g_2152))) == (((***g_777) &= p_5) & (l_2269 , (((*l_2271) = l_2270) != (p_4 , &g_927))))) && ((*g_2179) < 5L)), 0));
                for (p_5 = 0; (p_5 <= 2); p_5 += 1)
                {
                    int32_t *l_2283 = &l_2253;
                    int16_t ****l_2286 = &g_308;
                    int i, j, k;
                    if ((*p_6))
                        break;
                    if ((*p_6))
                        break;
                    if ((p_4 ^ ((safe_lshift_func_int16_t_s_u(((((l_2252[g_1909][p_5][(p_5 + 2)] == (void*)0) > l_2256) , ((((*g_11) <= (safe_lshift_func_uint8_t_u_s(5UL, (safe_add_func_int16_t_s_s(l_2278, ((0UL && (safe_sub_func_uint16_t_u_u((safe_add_func_int8_t_s_s((((void*)0 != l_2283) && l_2269), g_2284[0][2])), l_2285))) > 0x3F27L)))))) < l_2278) , (void*)0)) != l_2286), p_4)) >= (*g_2179))))
                    {
                        uint64_t l_2296[7][10][3] = {{{0UL,0x0BB225C034ABF163LL,1UL},{3UL,0x1D491610A2B6D948LL,0x5386C2725B4B3BA6LL},{0x0BB225C034ABF163LL,0x226C84D3C679C94ELL,0xB5251C1EB6A17344LL},{3UL,1UL,3UL},{0UL,0xDF8D13F9C42D3E92LL,9UL},{0xAA83B88555568408LL,0xD4EC7D637E063711LL,0xD3C37E97B524FA21LL},{5UL,5UL,0xA0B813982E64856DLL},{9UL,18446744073709551615UL,0UL},{0xB5251C1EB6A17344LL,1UL,18446744073709551615UL},{0xD3C37E97B524FA21LL,0x5386C2725B4B3BA6LL,0x7BAE760B8982D9F0LL}},{{0xDF8D13F9C42D3E92LL,0xB5251C1EB6A17344LL,18446744073709551615UL},{0x9638E2715D3885E4LL,3UL,0UL},{9UL,9UL,0xA0B813982E64856DLL},{0x844281520D70AE15LL,0xD3C37E97B524FA21LL,0xD3C37E97B524FA21LL},{0UL,0xA0B813982E64856DLL,9UL},{0x93688AD9142E168CLL,0UL,3UL},{9UL,18446744073709551615UL,0xB5251C1EB6A17344LL},{18446744073709551615UL,0x7BAE760B8982D9F0LL,0x5386C2725B4B3BA6LL},{18446744073709551607UL,18446744073709551615UL,1UL},{0x1D491610A2B6D948LL,0UL,18446744073709551615UL}},{{0UL,0xA0B813982E64856DLL,5UL},{0xD4EC7D637E063711LL,0xD3C37E97B524FA21LL,0xD4EC7D637E063711LL},{18446744073709551615UL,9UL,0xDF8D13F9C42D3E92LL},{0xD34E25E69BA1521BLL,3UL,1UL},{0x6A6C25ABB7A6DEF2LL,0xB5251C1EB6A17344LL,0x226C84D3C679C94ELL},{3UL,0x5386C2725B4B3BA6LL,0x1D491610A2B6D948LL},{0x6A6C25ABB7A6DEF2LL,1UL,0x0BB225C034ABF163LL},{0xD34E25E69BA1521BLL,18446744073709551615UL,0x9638E2715D3885E4LL},{18446744073709551615UL,5UL,18446744073709551607UL},{0xD4EC7D637E063711LL,0xD4EC7D637E063711LL,0xECBDDDEDE32752D9LL}},{{0UL,0xDF8D13F9C42D3E92LL,0x64B0CF33FDCB6CBALL},{0x1D491610A2B6D948LL,1UL,0x7EA3DD9D15629D56LL},{18446744073709551607UL,0x226C84D3C679C94ELL,0x33E7975BE20BFC58LL},{18446744073709551615UL,0x1D491610A2B6D948LL,0x7EA3DD9D15629D56LL},{9UL,0x0BB225C034ABF163LL,0x64B0CF33FDCB6CBALL},{0x93688AD9142E168CLL,0x9638E2715D3885E4LL,0xECBDDDEDE32752D9LL},{0UL,18446744073709551607UL,18446744073709551607UL},{0x844281520D70AE15LL,0xECBDDDEDE32752D9LL,0x9638E2715D3885E4LL},{9UL,0x64B0CF33FDCB6CBALL,0x0BB225C034ABF163LL},{0x9638E2715D3885E4LL,0x7EA3DD9D15629D56LL,0x1D491610A2B6D948LL}},{{18446744073709551615UL,0x6A6C25ABB7A6DEF2LL,0xA0B813982E64856DLL},{0UL,9UL,0x1D491610A2B6D948LL},{0UL,0x226C84D3C679C94ELL,18446744073709551615UL},{0xD4EC7D637E063711LL,0x7BAE760B8982D9F0LL,0x8D54F20F9EB08C7FLL},{4UL,0xDEEF6D83E7BEA65DLL,4UL},{3UL,0xD3C37E97B524FA21LL,0xAA83B88555568408LL},{0xDF8D13F9C42D3E92LL,0x64B0CF33FDCB6CBALL,0xB5251C1EB6A17344LL},{0x6A76BA008BEDEBC0LL,0xD34E25E69BA1521BLL,0xECBDDDEDE32752D9LL},{0x64B0CF33FDCB6CBALL,0xA0B813982E64856DLL,0UL},{0x6A76BA008BEDEBC0LL,0x1D491610A2B6D948LL,0UL}},{{0xDF8D13F9C42D3E92LL,18446744073709551615UL,18446744073709551607UL},{3UL,0x8D54F20F9EB08C7FLL,0UL},{4UL,4UL,0x33E7975BE20BFC58LL},{0xD4EC7D637E063711LL,0xAA83B88555568408LL,0x5386C2725B4B3BA6LL},{0UL,0xB5251C1EB6A17344LL,0UL},{0UL,0xECBDDDEDE32752D9LL,3UL},{18446744073709551615UL,0UL,0UL},{0xD3C37E97B524FA21LL,0UL,0x5386C2725B4B3BA6LL},{1UL,18446744073709551607UL,0x33E7975BE20BFC58LL},{0UL,0UL,0UL}},{{7UL,0x33E7975BE20BFC58LL,18446744073709551607UL},{1UL,0x5386C2725B4B3BA6LL,0UL},{18446744073709551607UL,0UL,0UL},{0xAA83B88555568408LL,3UL,0xECBDDDEDE32752D9LL},{0xDEEF6D83E7BEA65DLL,0UL,0xB5251C1EB6A17344LL},{0xD34E25E69BA1521BLL,0x5386C2725B4B3BA6LL,0xAA83B88555568408LL},{5UL,0x33E7975BE20BFC58LL,4UL},{0x8D54F20F9EB08C7FLL,0UL,0x8D54F20F9EB08C7FLL},{0x0BB225C034ABF163LL,18446744073709551607UL,18446744073709551615UL},{18446744073709551615UL,0UL,0x1D491610A2B6D948LL}}};
                        int i, j, k;
                        (*l_2283) = (((((g_161--) != ((l_2296[4][0][0] &= (l_2251 && ((safe_add_func_uint8_t_u_u(254UL, p_5)) , (safe_sub_func_int32_t_s_s((((((0x0F2CL == (safe_lshift_func_uint8_t_u_s(p_4, ((*l_2283) < (*p_6))))) , ((((l_2295 >= p_4) != 0x3CL) , p_5) < 0x2722L)) < (*l_2283)) , (*p_6)) & 1UL), (*g_2220)))))) , 0x68CC14B4DFC25B25LL)) , 4294967293UL) > l_2285) , l_2296[5][6][2]);
                        return p_6;
                    }
                    else
                    {
                        int16_t l_2301 = 8L;
                        int8_t *l_2304 = &l_2278;
                        l_2320 &= (((((l_2254 && (l_2301 ^= (!(safe_mod_func_uint64_t_u_u(((*g_641) == ((*g_712) = l_2300[2])), g_315))))) == (safe_rshift_func_int8_t_s_u(((*l_2304) ^= l_2285), (safe_add_func_uint8_t_u_u((safe_mul_func_uint16_t_u_u(((l_2309 = 0x7634L) & (safe_div_func_int64_t_s_s((safe_div_func_int32_t_s_s((safe_mul_func_int16_t_s_s(((safe_lshift_func_int16_t_s_u((((**l_2286) = l_2318[0][4][0]) == (void*)0), 2)) , 0xEDF0L), 0x669DL)), (-3L))), p_5))), l_2319[0][5][2])), p_5))))) | 0UL) ^ p_5) , (-10L));
                        (*g_2220) |= 0xDD517F73L;
                    }
                }
            }
            if (l_2321)
                continue;
            (*p_6) &= (safe_sub_func_uint64_t_u_u(g_9, (((((l_2324 == (((*l_2270) == l_2326) , l_2327)) < (safe_div_func_uint8_t_u_u(3UL, ((*l_2330)++)))) | 255UL) , ((l_2319[0][6][1] |= l_2285) >= (&l_2300[3] == &g_307))) || (*g_2179))));
            for (g_1365 = 6; (g_1365 >= 0); g_1365 -= 1)
            {
                int8_t l_2343 = 1L;
                uint16_t *l_2347 = &g_984[6];
                uint16_t *****l_2348 = &l_2270;
                int8_t *l_2349 = &g_316;
                (*p_6) |= (safe_mul_func_int16_t_s_s((safe_mod_func_uint64_t_u_u(((p_4 < l_2337) , (+((((((**g_2178) && ((((safe_div_func_uint32_t_u_u(p_5, 0x984F1C52L)) <= ((0xE7B775F704A020DBLL >= p_4) <= 1L)) <= (safe_lshift_func_uint16_t_u_u(l_2343, 5))) || p_5)) && l_2319[0][6][3]) ^ g_2344[2]) > p_5) > g_984[4]))), p_5)), p_5));
                if ((*p_6))
                    continue;
                for (g_315 = 0; (g_315 <= 6); g_315 += 1)
                {
                    for (l_2321 = 0; (l_2321 <= 6); l_2321 += 1)
                    {
                        if ((*p_6))
                            break;
                    }
                }
                (*p_6) |= ((p_5 == p_5) < (+((*l_2349) = ((((*l_2347) &= (&p_6 == (l_2346 = (*g_1674)))) , &g_927) != ((*l_2348) = l_2270)))));
            }
        }
        if ((safe_unary_minus_func_uint64_t_u((safe_rshift_func_int16_t_s_u(0x9F39L, 6)))))
        {
            int8_t *l_2356 = &g_1909;
            int32_t l_2369 = 0xA1FC4495L;
            int32_t l_2373 = 0x043BD500L;
            int32_t l_2375[2];
            int32_t l_2381 = 0L;
            uint8_t l_2397 = 255UL;
            uint16_t * const ***l_2426 = &g_940;
            uint16_t * const ****l_2425 = &l_2426;
            int i;
            for (i = 0; i < 2; i++)
                l_2375[i] = 0xA766F846L;
            if ((+((safe_rshift_func_int8_t_s_s(((*l_2356) &= p_4), 3)) , ((-1L) && 0xA6F4366DL))))
            {
                return p_6;
            }
            else
            {
                int64_t l_2371[9][7][3] = {{{0x0F098E5198806958LL,(-6L),0xA2775DC10C0D2622LL},{1L,0xCD066DB2B897772DLL,0x7E870CA3AF23499DLL},{0xE50BEA9A93B1B2E5LL,1L,1L},{0xA53595B5772F0EFDLL,(-5L),0L},{0x6B0CFA901585D8F7LL,0x6017EA9FDFACA04BLL,0x0D3FEAD34521F568LL},{0x0D3FEAD34521F568LL,0x236E81BCBA838B83LL,0x32E3A54803CB30FELL},{(-1L),0x32E3A54803CB30FELL,0xB510C3F9097D7D09LL}},{{0xED43BE8382A59CCFLL,0xED43BE8382A59CCFLL,1L},{1L,1L,0x05EC9659D33A2138LL},{(-1L),0x8F7E5465EBEDDFA2LL,(-3L)},{0x051CE5EBC2EB5A7FLL,4L,0xE50BEA9A93B1B2E5LL},{0x05EC9659D33A2138LL,(-1L),(-3L)},{0L,1L,0x05EC9659D33A2138LL},{(-7L),0xBA587772598384B4LL,1L}},{{0L,(-10L),0xB510C3F9097D7D09LL},{0L,7L,0x32E3A54803CB30FELL},{0x19B6AE770095879CLL,0xE15F541AD25BCE2ALL,0x0D3FEAD34521F568LL},{1L,(-1L),0L},{(-1L),0x0F098E5198806958LL,1L},{0x1651D7F4B3B0804FLL,0x660FE58BBC29291DLL,0x7E870CA3AF23499DLL},{0xE15F541AD25BCE2ALL,0x1651D7F4B3B0804FLL,0xA2775DC10C0D2622LL}},{{0xCC834AD8AF5114FFLL,0x9D9378BFB88C5A36LL,0L},{(-1L),0x4836B3017EE8B7DFLL,0x0F098E5198806958LL},{(-1L),0x1D1C8AAE5062B6D9LL,1L},{(-1L),0x1D1C8AAE5062B6D9LL,1L},{0x32E3A54803CB30FELL,0x4836B3017EE8B7DFLL,0xCD066DB2B897772DLL},{0x7E870CA3AF23499DLL,0x9D9378BFB88C5A36LL,0xA97615B60DDF2466LL},{(-5L),0x1651D7F4B3B0804FLL,(-1L)}},{{0x9D9378BFB88C5A36LL,0x660FE58BBC29291DLL,0x8F7E5465EBEDDFA2LL},{0xB46062BC0F7EB124LL,0x0F098E5198806958LL,(-1L)},{1L,(-1L),(-1L)},{0L,0xE15F541AD25BCE2ALL,(-10L)},{1L,7L,0xA53595B5772F0EFDLL},{0x660FE58BBC29291DLL,(-10L),0xCD066DB2B897772DLL},{(-5L),0x9A035AF4ADBF3550LL,0x1D1C8AAE5062B6D9LL}},{{0x660FE58BBC29291DLL,0xA2775DC10C0D2622LL,(-1L)},{(-1L),(-1L),0x5E505636DCE5CDB6LL},{0L,1L,0xDF08908FB209B285LL},{(-1L),0x6017EA9FDFACA04BLL,0x19B6AE770095879CLL},{0x660FE58BBC29291DLL,(-1L),(-1L)},{(-5L),1L,6L},{0xCD066DB2B897772DLL,0x4836B3017EE8B7DFLL,0x32E3A54803CB30FELL}},{{0x0AB13053C536DCB3LL,1L,1L},{0x8F7E5465EBEDDFA2LL,0L,(-5L)},{0xA2775DC10C0D2622LL,(-1L),(-7L)},{0x0D3FEAD34521F568LL,0x1D1C8AAE5062B6D9LL,(-1L)},{0L,0xE50BEA9A93B1B2E5LL,1L},{(-1L),(-10L),1L},{1L,0L,(-6L)}},{{0x4836B3017EE8B7DFLL,0x0F098E5198806958LL,0L},{(-7L),1L,0L},{0xA53595B5772F0EFDLL,(-8L),(-6L)},{0x0F99EC48DBFCD857LL,0xBAD16F7DB91C9C52LL,1L},{0xED43BE8382A59CCFLL,0x48C620F8A16535DELL,1L},{0xB46062BC0F7EB124LL,(-1L),(-1L)},{4L,0xE15F541AD25BCE2ALL,(-7L)}},{{0xF5125C2EFCDF376ELL,0x0AB13053C536DCB3LL,(-5L)},{0x5E505636DCE5CDB6LL,(-1L),1L},{0x1B69ABF6DA135C8ALL,(-1L),0x32E3A54803CB30FELL},{0x236E81BCBA838B83LL,6L,6L},{(-1L),0x0D3FEAD34521F568LL,(-1L)},{(-5L),(-1L),0x19B6AE770095879CLL},{1L,7L,0xDF08908FB209B285LL}}};
                int32_t l_2374 = 0xE493147BL;
                int32_t l_2376 = 0xCD63E56AL;
                int32_t l_2378 = 0xD9CD782DL;
                int32_t l_2379 = 5L;
                int32_t l_2382 = 0x68D087E5L;
                int32_t l_2384 = (-8L);
                int32_t l_2385 = 0L;
                int32_t l_2387 = 0x638B7639L;
                int32_t l_2388 = 0x83FDA2B8L;
                int32_t l_2390 = 0x72BD30B1L;
                int32_t l_2394 = 3L;
                int32_t l_2395[10] = {(-1L),(-1L),(-1L),(-1L),(-1L),(-1L),(-1L),(-1L),(-1L),(-1L)};
                int i, j, k;
                for (g_161 = (-15); (g_161 >= 2); g_161++)
                {
                    int32_t l_2377 = 0x3E325C4CL;
                    int32_t l_2380 = 0L;
                    int32_t l_2383[8] = {0xFA35823BL,0xFA35823BL,0x623D45BFL,0xFA35823BL,0xFA35823BL,0x623D45BFL,0xFA35823BL,0xFA35823BL};
                    int32_t *l_2400 = &l_2375[0];
                    int32_t *l_2401 = &l_2389;
                    int32_t *l_2402[1][7] = {{&l_2379,&l_2379,&l_2379,&l_2379,&l_2379,&l_2379,&l_2379}};
                    int8_t ***l_2408 = &g_1986;
                    int8_t ****l_2407 = &l_2408;
                    int i, j;
                    for (g_315 = 0; (g_315 >= (-16)); --g_315)
                    {
                        int32_t *l_2361 = &l_2253;
                        int32_t *l_2362 = &l_2285;
                        int32_t *l_2363 = (void*)0;
                        int32_t *l_2364 = &l_2256;
                        int32_t *l_2365 = &l_2243;
                        int32_t *l_2366 = &l_2243;
                        int32_t *l_2367 = &l_2256;
                        int32_t *l_2368 = &l_2256;
                        int32_t *l_2370[4] = {&g_2284[2][8],&g_2284[2][8],&g_2284[2][8],&g_2284[2][8]};
                        int16_t l_2372[10][4] = {{0xF42AL,4L,(-9L),0L},{4L,0L,(-9L),(-9L)},{0xF42AL,0xF42AL,0x2CA6L,0x5F3EL},{1L,9L,(-10L),0L},{(-10L),0L,4L,(-10L)},{0xF42AL,0L,0x01BDL,0L},{0L,9L,(-9L),0x5F3EL},{0x1CFAL,0xF42AL,4L,(-9L)},{1L,0L,8L,0L},{1L,4L,4L,1L}};
                        int i, j;
                        ++l_2397;
                        (*g_254) = func_13(p_4);
                        return p_6;
                    }
                    l_2404--;
                    for (g_2103 = 0; (g_2103 <= 0); g_2103 += 1)
                    {
                        g_2409[0][2] = l_2407;
                    }
                    for (l_2392 = 3; (l_2392 >= 0); l_2392 -= 1)
                    {
                        int32_t l_2437[1];
                        int i;
                        for (i = 0; i < 1; i++)
                            l_2437[i] = 0xD53B0E2DL;
                        g_2221[(l_2392 + 1)] = (((safe_mul_func_int16_t_s_s(g_2221[(l_2392 + 1)], (((safe_lshift_func_int8_t_s_u((((safe_add_func_uint32_t_u_u((safe_mul_func_uint16_t_u_u((safe_rshift_func_uint8_t_u_s((safe_div_func_int8_t_s_s((safe_div_func_uint64_t_u_u(((p_4 || (((l_2425 != (g_200 , &l_2426)) >= ((safe_add_func_uint8_t_u_u(((((*g_927) = l_2429) == (**l_2426)) && ((safe_add_func_int16_t_s_s((safe_lshift_func_int8_t_s_s(p_5, 6)), ((safe_mul_func_int8_t_s_s((l_2437[0] ^= ((*l_2356) &= 6L)), p_4)) && l_2437[0]))) , 0xC5L)), 0x5CL)) ^ (*p_6))) != g_61)) == 254UL), 0x2D237583F02998B4LL)), p_4)), p_5)), 0x7457L)), 0xE873F636L)) || (-5L)) | p_4), l_2375[0])) < p_4) < p_5))) , (-6L)) || 5UL);
                    }
                }
                return (**g_1674);
            }
        }
        else
        {
            int32_t l_2438 = 0xE7C9D89BL;
            int32_t *l_2439 = &l_2389;
            int32_t *l_2440 = &g_201;
            int32_t *l_2441 = &g_1803;
            int32_t *l_2442 = &l_2389;
            int32_t *l_2443 = &l_2254;
            int32_t *l_2444 = (void*)0;
            int32_t *l_2445 = &g_1803;
            int32_t *l_2446 = &g_22;
            int32_t *l_2447 = &l_2389;
            int32_t *l_2448 = &g_2284[0][2];
            int32_t *l_2449 = (void*)0;
            int32_t *l_2450 = &l_2253;
            int32_t *l_2451 = &l_2255;
            int32_t l_2452 = 1L;
            int32_t *l_2453 = (void*)0;
            int32_t *l_2454 = &l_2255;
            int32_t *l_2455[6][5] = {{&l_2243,&l_2243,&l_2243,&l_2243,&l_2243},{&g_22,&g_22,&g_22,&g_22,&g_22},{&l_2243,&l_2243,&l_2243,&l_2243,&l_2243},{&g_22,&g_22,&g_22,&g_22,&g_22},{&l_2243,&l_2243,&l_2243,&l_2243,&l_2243},{&g_22,&g_22,&g_22,&g_22,&g_22}};
            uint32_t **l_2472 = &g_169;
            int i, j;
            ++l_2460[1];
            for (l_2438 = 0; (l_2438 <= 3); l_2438 += 1)
            {
                int32_t *l_2463 = (void*)0;
                uint16_t *l_2484 = &g_109[1];
                if ((*p_6))
                    break;
                for (g_200 = 1; (g_200 <= 4); g_200 += 1)
                {
                    int i, j;
                    if ((*p_6))
                    {
                        return (*g_254);
                    }
                    else
                    {
                        if ((*p_6))
                            break;
                        return l_2463;
                    }
                }
                for (g_102 = 0; (g_102 <= 2); g_102 += 1)
                {
                    int32_t ***l_2464 = &g_2152;
                    uint16_t *l_2483[8][7] = {{&l_2460[6],&g_109[2],&l_2460[6],(void*)0,&l_2460[6],(void*)0,(void*)0},{&l_2460[6],&l_2460[6],&g_109[2],&l_2460[6],&l_2460[6],&g_109[2],&l_2460[6]},{&l_2460[6],(void*)0,(void*)0,&l_2460[6],(void*)0,(void*)0,&l_2460[6]},{(void*)0,&l_2460[6],(void*)0,(void*)0,&l_2460[6],(void*)0,(void*)0},{&l_2460[6],&l_2460[6],&g_109[2],&l_2460[6],&l_2460[6],&g_109[2],&l_2460[6]},{&l_2460[6],(void*)0,(void*)0,&l_2460[6],(void*)0,(void*)0,&l_2460[6]},{(void*)0,&l_2460[6],(void*)0,(void*)0,&l_2460[6],(void*)0,(void*)0},{&l_2460[6],&l_2460[6],&g_109[2],&l_2460[6],&l_2460[6],&g_109[2],&l_2460[6]}};
                    int32_t l_2490 = 0x533DA5EFL;
                    int8_t *l_2493 = &g_316;
                    int64_t l_2494 = 0xE663B1B9816CB2FFLL;
                    uint32_t ***l_2497 = &l_2472;
                    int i, j;
                }
            }
            g_2520++;
        }
        if ((+(*g_2179)))
        {
            uint16_t ****l_2525 = (void*)0;
            int32_t l_2530[2][8] = {{0xC8EE47FBL,(-1L),(-1L),0xC8EE47FBL,0x78AA5749L,0L,0x78AA5749L,0xC8EE47FBL},{(-1L),0x78AA5749L,(-1L),1L,7L,7L,1L,(-1L)}};
            uint8_t **l_2536 = &g_2179;
            uint64_t ***l_2555 = &l_2533;
            int i, j;
            for (g_193 = 0; (g_193 <= 2); g_193 += 1)
            {
                uint16_t *****l_2524[10][3][4] = {{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}},{{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270},{&l_2270,&l_2270,&l_2270,&l_2270}}};
                int32_t l_2537[3];
                int i, j, k;
                for (i = 0; i < 3; i++)
                    l_2537[i] = (-5L);
                (***g_497) = func_38(g_109[g_193]);
                l_2392 |= ((l_2525 = l_2270) == (((l_2537[0] |= (l_2530[0][6] = ((safe_rshift_func_uint8_t_u_u((g_109[g_193] ^ (l_2256 ^= ((((l_2530[0][6] || (l_2393[2][0][3] = l_2530[0][3])) , g_2531) == l_2533) <= (((((p_4 < (((**g_588) <= (safe_div_func_int8_t_s_s((p_4 , 0x37L), 0x4DL))) == 0x3EC22CC2L)) , &g_2179) == l_2536) , (void*)0) == (*g_712))))), 4)) != p_4))) , (*g_2179)) , (void*)0));
                (*g_2220) = (*g_2220);
            }
            (*p_6) = ((safe_div_func_uint16_t_u_u((((*l_2430) = 0xDB38L) || ((p_5 , ((safe_sub_func_uint8_t_u_u((p_5 == (l_2386 == (((((g_2543 = l_2542) == &g_473) , (safe_mod_func_int8_t_s_s(((*p_6) < (safe_sub_func_uint32_t_u_u((safe_add_func_int32_t_s_s((safe_sub_func_int8_t_s_s((((((*l_2555) = &l_2252[4][0][1]) == l_2556) <= p_5) < 1UL), p_4)), p_5)), l_2530[1][4]))), p_5))) > p_4) && l_2386))), 0xBFL)) != p_4)) == l_2530[0][6])), 0xE18CL)) ^ p_4);
        }
        else
        {
            uint64_t **l_2561 = &g_2532;
            uint8_t l_2568 = 0xD9L;
            int8_t ****l_2570 = &l_2569[5][8][3];
            int8_t ***l_2571 = (void*)0;
            int8_t ****l_2573 = &g_2410[3][5][1];
            uint8_t *l_2580 = &g_2103;
            const int32_t l_2581 = 0L;
            uint8_t *l_2582[2];
            int64_t ***l_2632 = (void*)0;
            int32_t l_2651 = (-1L);
            int32_t l_2652 = (-1L);
            int32_t l_2656 = 0x27D985C5L;
            int32_t l_2659 = 0L;
            int32_t l_2660 = 0xDBB45C83L;
            uint64_t l_2661 = 0x035D422A79D004C0LL;
            int32_t *l_2674 = &g_2284[0][2];
            int32_t *** const *l_2682 = &g_1674;
            int i;
            for (i = 0; i < 2; i++)
                l_2582[i] = &g_9;
            (*p_6) = (safe_sub_func_uint32_t_u_u((safe_rshift_func_uint8_t_u_u((**g_2178), 1)), ((((*l_2561) = ((*l_2533) = (*g_2531))) == (void*)0) >= (safe_sub_func_uint8_t_u_u(p_4, ((((((safe_sub_func_uint8_t_u_u((g_9 = ((safe_sub_func_uint8_t_u_u(((l_2568 = 0x0C1FL) >= ((((*l_2570) = l_2569[5][8][3]) == ((*l_2573) = (l_2572[2][3] = l_2571))) != ((((safe_mul_func_uint8_t_u_u(((safe_add_func_uint8_t_u_u((safe_sub_func_uint8_t_u_u(((*l_2580) = l_2254), (p_5 | p_4))), l_2581)) < 0x615E9AC7L), p_5)) , 4294967287UL) && l_2321) , 65535UL))), 7UL)) | p_4)), l_2581)) == l_2581) == l_2583) && l_2459) | 0UL) , 251UL))))));
            for (g_429 = 6; (g_429 >= 0); g_429 -= 1)
            {
                int16_t ***l_2605 = &g_309;
                int32_t l_2607 = 0x236270ADL;
                int32_t l_2653 = 0x2A1E5884L;
                int32_t l_2654 = 0L;
                int32_t l_2655 = (-1L);
                int32_t l_2658[9][9] = {{0x7CF134C2L,0x24AFC16FL,4L,0x39D56451L,0xDEE0BED1L,(-8L),0x2512C9CCL,0x7CF134C2L,(-8L)},{(-8L),1L,4L,0x337297B0L,1L,0x701A0D4FL,0x735E1940L,0x235FB332L,0x39D56451L},{0x337297B0L,1L,(-8L),0x39D56451L,1L,(-1L),0x337297B0L,0xDEE0BED1L,(-6L)},{0x2512C9CCL,0L,0x337297B0L,0x701A0D4FL,0xDEE0BED1L,0x701A0D4FL,0x337297B0L,0L,0x2512C9CCL},{(-1L),0xACBA701CL,0x2512C9CCL,0x2E18E0E9L,0L,(-8L),0x735E1940L,0xDEE0BED1L,0x2512C9CCL},{0x735E1940L,0L,(-1L),4L,0L,0x39D56451L,0x2512C9CCL,0x235FB332L,(-6L)},{(-1L),0L,0x735E1940L,(-1L),1L,(-6L),0x39D56451L,0x7CF134C2L,0x39D56451L},{0x2512C9CCL,0xACBA701CL,(-1L),(-1L),0xACBA701CL,0x2512C9CCL,0x2E18E0E9L,0L,(-8L)},{0x337297B0L,0L,0x2512C9CCL,4L,1L,0x2512C9CCL,(-6L),1L,0x701A0D4FL}};
                int32_t *l_2691 = &l_2389;
                int i, j;
                for (p_4 = 0; (p_4 <= 2); p_4 += 1)
                {
                    const int32_t *l_2603 = &g_552;
                    const int32_t **l_2602[2][5] = {{&l_2603,&l_2603,&l_2603,&l_2603,&l_2603},{&l_2603,&l_2603,&l_2603,&l_2603,&l_2603}};
                    const int32_t ***l_2601 = &l_2602[0][1];
                    int32_t l_2604 = 0x64053738L;
                    uint32_t l_2606 = 0xF8AD5CD2L;
                    int i, j;
                    (*g_2220) = (g_2344[g_429] > (g_2344[(g_429 + 1)] && (safe_mul_func_int16_t_s_s(p_5, (-1L)))));
                    (*g_2220) ^= (((safe_lshift_func_int16_t_s_s(3L, 5)) != 0xDD53A873L) && ((*l_2430) = (safe_lshift_func_int16_t_s_u((safe_add_func_int8_t_s_s((((safe_mod_func_int16_t_s_s((((((p_5--) , (safe_sub_func_uint8_t_u_u((*g_2179), ((l_2598[4][0] & (((0x2E8BD18FL & (l_2601 != (void*)0)) || ((l_2604 = p_4) <= (l_2606 ^= (((void*)0 == l_2605) && 0x1F90226CL)))) , (**g_517))) | l_2607)))) & l_2459) >= 0x21C33C2831B363DELL) > l_2242), 0xB376L)) <= l_2581) , l_2460[4]), g_2344[(g_429 + 1)])), 14))));
                }
                for (l_2337 = 0; (l_2337 <= 0); l_2337 += 1)
                {
                    for (g_125 = 0; (g_125 <= 2); g_125 += 1)
                    {
                        if (l_2392)
                            goto lbl_2506;
                    }
                    for (p_5 = 0; (p_5 <= 2); p_5 += 1)
                    {
                        return p_6;
                    }
                }
                for (g_125 = 0; (g_125 <= 2); g_125 += 1)
                {
                    int32_t *l_2645 = &g_2403[6];
                    int32_t *l_2646 = (void*)0;
                    int32_t *l_2647 = &g_2284[0][2];
                    int32_t *l_2648 = &l_2243;
                    int32_t *l_2649 = &l_2386;
                    int32_t *l_2650[9][2] = {{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253},{&l_2253,&l_2253}};
                    int64_t l_2675[5] = {0x730379DDA993EE35LL,0x730379DDA993EE35LL,0x730379DDA993EE35LL,0x730379DDA993EE35LL,0x730379DDA993EE35LL};
                    uint8_t **l_2683 = &g_2179;
                    int i, j;
                    ++l_2661;
                    (*l_2647) = ((safe_add_func_int32_t_s_s((safe_add_func_int64_t_s_s(((((safe_mod_func_int8_t_s_s(((*g_2220) > (safe_div_func_int64_t_s_s((((((**g_517) , (254UL == p_5)) , g_2080) == (*l_2647)) & (((((safe_mod_func_uint64_t_u_u(18446744073709551613UL, (l_2568 && 65535UL))) < l_2583) , l_2658[0][4]) >= p_5) ^ l_2607)), l_2391[5]))), l_2659)) || 0x919EL) , (*l_2645)) && (*p_6)), 0x2315FBFECCAF806ELL)), 0x49770B23L)) == (-1L));
                    for (l_2255 = 0; (l_2255 <= 0); l_2255 += 1)
                    {
                        uint8_t l_2686 = 1UL;
                        l_2674 = p_6;
                        (***g_497) = func_13(((l_2675[0] & ((*g_518) &= 3L)) , l_2657));
                        (***g_1674) = ((safe_mul_func_int8_t_s_s(((safe_add_func_int16_t_s_s((safe_div_func_uint32_t_u_u((((void*)0 == l_2682) <= 0x8E17L), 0xEF8428E9L)), (l_2683 == (void*)0))) == (l_2457[0][6] == p_5)), (safe_sub_func_uint8_t_u_u(0x67L, 0L)))) , (**g_254));
                        (****g_497) &= ((*l_2648) &= l_2686);
                    }
                }
                for (l_2404 = 0; (l_2404 <= 2); l_2404 += 1)
                {
                    int32_t *l_2687[2][5][5] = {{{&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0]},{&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0]},{&l_2393[2][0][0],&l_2393[2][0][0],&l_2660,&l_2393[2][0][0],&l_2393[2][0][0]},{&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0]},{&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0]}},{{&l_2393[2][0][0],&l_2393[2][0][0],&l_2660,&l_2393[2][0][0],&l_2393[2][0][0]},{&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0]},{&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0]},{&l_2393[2][0][0],&l_2393[2][0][0],&l_2660,&l_2393[2][0][0],&l_2393[2][0][0]},{&l_2393[0][0][0],&l_2393[2][0][0],&l_2393[0][0][0],&l_2393[0][0][0],&l_2393[2][0][0]}}};
                    int i, j, k;
                    (***g_497) = (void*)0;
                    --l_2688;
                    for (l_2661 = 0; (l_2661 <= 0); l_2661 += 1)
                    {
                        int i, j;
                        (*g_254) = func_13(p_5);
                        l_2691 = ((**g_1674) = p_6);
                        l_2692--;
                    }
                    return (**g_1674);
                }
            }
        }
    }
    else
    {
        int16_t **l_2695 = &g_164;
        int32_t l_2707 = 0x026F677FL;
        int32_t l_2708[6] = {0x5A059E88L,0xCFEDF92EL,0xCFEDF92EL,0x5A059E88L,0xCFEDF92EL,0xCFEDF92EL};
        int64_t l_2728[3][3][10] = {{{1L,0xA97A724F08A8B31ELL,0xE5B0C7E112AFEF8ALL,0x02ECA2E8FD7EA17FLL,8L,0x02ECA2E8FD7EA17FLL,0xE5B0C7E112AFEF8ALL,0xA97A724F08A8B31ELL,1L,0x6F5BABD9F51E27F2LL},{0x599C64521CD60EA3LL,1L,0xE5B0C7E112AFEF8ALL,8L,0xA97A724F08A8B31ELL,0xA97A724F08A8B31ELL,8L,0xE5B0C7E112AFEF8ALL,1L,0x599C64521CD60EA3LL},{1L,0x02ECA2E8FD7EA17FLL,1L,0xA97A724F08A8B31ELL,0x37663B5B553AC31ELL,1L,0x37663B5B553AC31ELL,0xA97A724F08A8B31ELL,0x6F5BABD9F51E27F2LL,1L}},{{0x02ECA2E8FD7EA17FLL,0x0BEFE5C6EB1831B8LL,1L,1L,0x37663B5B553AC31ELL,0xE5B0C7E112AFEF8ALL,0xE5B0C7E112AFEF8ALL,0x37663B5B553AC31ELL,1L,1L},{0x37663B5B553AC31ELL,0x37663B5B553AC31ELL,7L,0x02ECA2E8FD7EA17FLL,0x599C64521CD60EA3LL,0xE5B0C7E112AFEF8ALL,0x6F5BABD9F51E27F2LL,0xE5B0C7E112AFEF8ALL,0x599C64521CD60EA3LL,0x02ECA2E8FD7EA17FLL},{0x02ECA2E8FD7EA17FLL,8L,0x02ECA2E8FD7EA17FLL,0xE5B0C7E112AFEF8ALL,0xA97A724F08A8B31ELL,1L,0x6F5BABD9F51E27F2LL,0x6F5BABD9F51E27F2LL,1L,0xA97A724F08A8B31ELL}},{{7L,0x37663B5B553AC31ELL,0x37663B5B553AC31ELL,7L,0x02ECA2E8FD7EA17FLL,0x599C64521CD60EA3LL,0xE5B0C7E112AFEF8ALL,0x6F5BABD9F51E27F2LL,0xE5B0C7E112AFEF8ALL,0x599C64521CD60EA3LL},{1L,0x0BEFE5C6EB1831B8LL,0x02ECA2E8FD7EA17FLL,0x0BEFE5C6EB1831B8LL,1L,1L,0x37663B5B553AC31ELL,0xE5B0C7E112AFEF8ALL,0xE5B0C7E112AFEF8ALL,0x37663B5B553AC31ELL},{0x6F5BABD9F51E27F2LL,1L,7L,7L,1L,0x6F5BABD9F51E27F2LL,0xA97A724F08A8B31ELL,0x37663B5B553AC31ELL,1L,0x37663B5B553AC31ELL}}};
        const int32_t *l_2779 = &l_2708[0];
        const int32_t **l_2778[8][10] = {{&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779},{&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779},{&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779},{(void*)0,&l_2779,&l_2779,&l_2779,(void*)0,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779},{&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779},{&l_2779,&l_2779,&l_2779,&l_2779,(void*)0,&l_2779,&l_2779,&l_2779,(void*)0,&l_2779},{&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779},{&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779,&l_2779}};
        const int32_t ***l_2777 = &l_2778[5][6];
        uint8_t *l_2812 = &g_9;
        uint32_t **l_2849 = &g_169;
        int8_t l_2888 = 3L;
        int i, j, k;
lbl_2788:
        (*p_6) = (((*l_2250) = (**g_307)) != l_2695);
        if ((*g_2220))
        {
            int32_t *l_2696 = &l_2386;
            int32_t *l_2697 = &g_22;
            int32_t *l_2698 = (void*)0;
            int32_t *l_2699 = (void*)0;
            int32_t *l_2700 = &l_2386;
            int32_t *l_2701 = (void*)0;
            int32_t *l_2702 = &l_2386;
            int32_t *l_2703 = &g_2284[0][2];
            int32_t *l_2704 = &l_2256;
            int32_t *l_2705 = (void*)0;
            int32_t *l_2706[7][8] = {{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256},{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256},{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256},{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256},{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256},{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256},{&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256,&l_2256}};
            uint32_t l_2709[4] = {1UL,1UL,1UL,1UL};
            uint32_t l_2738 = 1UL;
            int32_t ***l_2756 = &g_2152;
            uint16_t *l_2827 = &l_2460[5];
            int32_t ****l_2840 = &g_1674;
            uint32_t **l_2847 = &g_169;
            uint32_t ***l_2853[2];
            int i, j;
            for (i = 0; i < 2; i++)
                l_2853[i] = &g_588;
            l_2709[2]++;
        }
        else
        {
            (*g_254) = &l_2254;
            if (l_2255)
                goto lbl_2788;
        }
        for (g_1222 = (-17); (g_1222 <= 13); g_1222 = safe_add_func_uint8_t_u_u(g_1222, 8))
        {
            int32_t l_2886 = 0x6EE1246CL;
            int32_t l_2887 = 0x63E1B052L;
            int32_t *l_2889 = &l_2389;
            --l_2890;
        }
    }
    return p_6;
}







static int32_t * func_13(uint32_t p_14)
{
    int16_t *l_60[1];
    int32_t l_73 = 0xEE34D1CDL;
    int32_t l_74 = 4L;
    uint64_t *l_577 = &g_97;
    int8_t l_578 = 0x44L;
    int32_t *l_579 = &g_22;
    uint32_t l_580 = 1UL;
    int8_t *l_581 = (void*)0;
    int8_t *l_582 = &g_316;
    int8_t l_1915 = 0xB7L;
    uint32_t ** const * const l_1946 = (void*)0;
    uint32_t ** const * const * const l_1945[4] = {&l_1946,&l_1946,&l_1946,&l_1946};
    uint32_t ** const * const * const *l_1944 = &l_1945[1];
    uint8_t *l_1956 = &g_887;
    uint8_t *l_1974 = &g_102;
    int16_t l_1975 = 0x4F39L;
    int16_t ***l_1976[9][6] = {{&g_309,&g_309,&g_309,&g_309,&g_309,&g_309},{&g_309,&g_309,&g_309,&g_309,&g_309,&g_309},{&g_309,(void*)0,(void*)0,(void*)0,&g_309,&g_309},{&g_309,&g_309,(void*)0,&g_309,&g_309,&g_309},{&g_309,&g_309,&g_309,&g_309,(void*)0,&g_309},{&g_309,&g_309,(void*)0,&g_309,&g_309,&g_309},{(void*)0,&g_309,(void*)0,&g_309,&g_309,&g_309},{(void*)0,&g_309,&g_309,(void*)0,(void*)0,&g_309},{&g_309,&g_309,&g_309,(void*)0,&g_309,&g_309}};
    int32_t * const *l_1980[2][7][2] = {{{&g_255,&l_579},{&g_255,&g_255},{&g_255,&g_255},{&l_579,&g_255},{&l_579,&g_255},{&g_255,&g_255},{&g_255,&l_579}},{{&g_255,&l_579},{&g_255,&g_255},{&g_255,&g_255},{&l_579,&g_255},{&l_579,&g_255},{&g_255,&g_255},{&g_255,&l_579}}};
    int32_t * const **l_1979 = &l_1980[0][5][1];
    int32_t *l_1985 = &g_22;
    int64_t ****l_2005[3];
    int8_t l_2027[5][8] = {{(-1L),0x89L,0xF5L,0x99L,1L,0L,0xE6L,0xE6L},{0xF5L,(-1L),0xFBL,0xFBL,(-1L),0xF5L,0xE6L,(-3L)},{(-6L),0xFBL,0xF5L,0L,0x99L,0x9FL,0x99L,0L},{0x99L,0x9FL,0x99L,0L,0xF5L,0xFBL,(-6L),(-3L)},{0xE6L,0xF5L,(-1L),0xFBL,0xFBL,(-1L),0xF5L,0xE6L}};
    uint16_t ****l_2050 = &g_927;
    uint32_t l_2055 = 4294967295UL;
    int32_t **l_2155 = (void*)0;
    int32_t *l_2236 = &g_201;
    int i, j, k;
    for (i = 0; i < 1; i++)
        l_60[i] = &g_61;
    for (i = 0; i < 3; i++)
        l_2005[i] = &g_777;
    for (p_14 = (-13); (p_14 > 47); p_14 = safe_add_func_uint32_t_u_u(p_14, 6))
    {
        int32_t *l_21 = &g_22;
        return l_21;
    }
    return l_2236;
}







static uint8_t func_25(int16_t p_26)
{
    uint32_t l_1918 = 0UL;
    uint8_t *l_1919 = &g_887;
    int16_t *****l_1920 = &g_307;
    int32_t *l_1921 = (void*)0;
    int32_t *l_1922[2][5][3] = {{{&g_201,&g_22,(void*)0},{(void*)0,(void*)0,&g_201},{&g_201,&g_201,&g_22},{&g_22,&g_201,&g_201},{&g_201,(void*)0,(void*)0}},{{(void*)0,&g_22,&g_201},{&g_22,&g_201,&g_22},{&g_22,&g_201,&g_201},{(void*)0,(void*)0,(void*)0},{&g_201,&g_201,&g_22}}};
    uint16_t l_1923 = 0UL;
    int i, j, k;
    l_1923 = (safe_div_func_uint16_t_u_u((((*l_1919) = (l_1918 <= 2UL)) , (&g_713 == l_1920)), l_1918));
    return g_1365;
}







static uint8_t func_36(int32_t * p_37)
{
    int32_t l_1739 = 0xD2A5A1C5L;
    int16_t ****l_1767 = &g_308;
    uint32_t l_1768 = 0x4AA3D09CL;
    uint8_t *l_1769[6][7] = {{(void*)0,(void*)0,(void*)0,(void*)0,&g_887,&g_9,&g_102},{&g_887,&g_9,&g_887,(void*)0,(void*)0,&g_887,&g_9},{&g_102,&g_9,&g_887,&g_887,&g_9,&g_9,&g_102},{&g_887,&g_102,&g_9,(void*)0,(void*)0,&g_102,&g_9},{&g_887,&g_887,(void*)0,(void*)0,&g_887,&g_887,&g_887},{&g_9,&g_9,(void*)0,&g_9,&g_9,&g_102,&g_887}};
    int32_t l_1770 = 0xA3E4DBFAL;
    int32_t l_1771[5];
    int32_t l_1772[3][1][9] = {{{0L,0x79FD5DBFL,5L,0x79FD5DBFL,0L,5L,0x1D0A66B4L,0x1D0A66B4L,5L}},{{0L,0x79FD5DBFL,5L,0x79FD5DBFL,0L,5L,0x1D0A66B4L,0x1D0A66B4L,5L}},{{0L,0x79FD5DBFL,5L,0x79FD5DBFL,0L,5L,0x1D0A66B4L,0x1D0A66B4L,5L}}};
    int32_t *l_1773 = &l_1771[3];
    uint64_t l_1776 = 0x0A32804917C2F8E1LL;
    uint32_t **l_1783[3][7][1] = {{{&g_169},{&g_169},{&g_169},{&g_169},{&g_169},{&g_169},{&g_169}},{{&g_169},{&g_169},{&g_169},{&g_169},{&g_169},{&g_169},{&g_169}},{{&g_169},{&g_169},{&g_169},{&g_169},{&g_169},{&g_169},{&g_169}}};
    int16_t l_1801 = 0x09FBL;
    int32_t l_1804 = 0x6D8BE528L;
    uint32_t **** const l_1812 = &g_587[1];
    uint32_t l_1822 = 0xFE0AA3ABL;
    int32_t l_1911 = 7L;
    int i, j, k;
    for (i = 0; i < 5; i++)
        l_1771[i] = 0L;
    l_1772[0][0][6] = (l_1771[3] = (!(safe_add_func_uint8_t_u_u((l_1770 &= ((((safe_mod_func_uint8_t_u_u(l_1739, (safe_div_func_uint32_t_u_u(((safe_mod_func_uint32_t_u_u((safe_lshift_func_int16_t_s_u((safe_rshift_func_uint16_t_u_s((safe_rshift_func_int8_t_s_u((l_1739 > (((**g_168) = (((safe_lshift_func_uint8_t_u_u(l_1739, (safe_unary_minus_func_int8_t_s(0x2BL)))) && (safe_rshift_func_int8_t_s_s((safe_mod_func_int8_t_s_s((((safe_sub_func_int32_t_s_s(((g_300 , l_1739) & ((safe_lshift_func_int8_t_s_s((l_1739 >= ((~(safe_unary_minus_func_uint64_t_u((((safe_add_func_uint8_t_u_u((((*g_641) == ((safe_mod_func_uint16_t_u_u(l_1739, l_1739)) , l_1767)) == l_1739), (-1L))) > 0xB638C773L) != l_1739)))) , l_1768)), g_1222)) , 0x4A51B229L)), l_1768)) > l_1768) , g_1449), g_389)), 3))) , l_1739)) && l_1768)), l_1768)), 10)), l_1739)), l_1739)) && l_1739), 0xB44CA8FDL)))) == l_1739) >= 0xDEL) < 0x77F0L)), 249UL))));
    l_1773 = &l_1772[0][0][4];
lbl_1848:
    if (((*l_1773) | (0xA486F033L || (safe_rshift_func_uint16_t_u_s(((l_1776 , (safe_mod_func_int16_t_s_s((-1L), (g_97 , (safe_div_func_int8_t_s_s((safe_add_func_int16_t_s_s(((void*)0 == l_1783[2][6][0]), 0xD4CCL)), (safe_div_func_uint8_t_u_u((+(safe_sub_func_uint8_t_u_u(0x42L, g_9))), g_387)))))))) & (*l_1773)), 2)))))
    {
        uint32_t l_1791 = 9UL;
        for (g_161 = 22; (g_161 < 39); g_161 = safe_add_func_uint32_t_u_u(g_161, 2))
        {
            (*g_254) = &l_1771[3];
            return l_1791;
        }
    }
    else
    {
        int32_t l_1792[6][1] = {{(-10L)},{0x7EC81EA0L},{(-10L)},{0x7EC81EA0L},{(-10L)},{0x7EC81EA0L}};
        int32_t l_1793 = 0xC89C8AC3L;
        int32_t *l_1794 = &g_201;
        int32_t *l_1795 = &g_22;
        int32_t l_1796[8] = {0L,5L,0L,5L,0L,5L,0L,5L};
        int32_t *l_1797 = &l_1772[0][0][6];
        int32_t *l_1798 = &l_1772[1][0][6];
        int32_t *l_1799 = &l_1793;
        int32_t *l_1800[8][1][3] = {{{&l_1772[1][0][2],&l_1771[3],&l_1793}},{{&l_1772[1][0][2],&l_1796[7],&l_1772[1][0][2]}},{{&l_1772[1][0][2],&g_22,&l_1796[7]}},{{&l_1772[1][0][2],&l_1771[3],&l_1793}},{{&l_1772[1][0][2],&l_1796[7],&l_1772[1][0][2]}},{{&l_1772[1][0][2],&g_22,&l_1796[7]}},{{&l_1772[1][0][2],&l_1771[3],&l_1793}},{{&l_1772[1][0][2],&l_1796[7],&l_1772[1][0][2]}}};
        int32_t l_1802 = 0x3B5E9283L;
        uint64_t l_1805 = 18446744073709551615UL;
        int i, j, k;
        (*l_1773) = l_1792[2][0];
        ++l_1805;
    }
    if ((safe_mul_func_uint16_t_u_u(65535UL, (safe_lshift_func_uint16_t_u_u(((l_1812 == l_1812) | ((safe_sub_func_int16_t_s_s(((~(safe_add_func_int64_t_s_s((*l_1773), (safe_add_func_int16_t_s_s((l_1804 = (*l_1773)), (safe_mod_func_int8_t_s_s((l_1822 , (((safe_lshift_func_uint8_t_u_s((safe_div_func_uint32_t_u_u((*l_1773), ((((**g_517) = (0x40A7B691L && (safe_add_func_uint32_t_u_u((!(safe_mul_func_int8_t_s_s(0x36L, 0x56L))), (*l_1773))))) <= 0x6A2E9B9FCE432310LL) , (*l_1773)))), (*l_1773))) && 0xE8L) <= g_109[0])), (*l_1773)))))))) < 5UL), 0L)) >= 1UL)), (*l_1773))))))
    {
        const uint32_t *l_1834 = &g_125;
        const uint32_t **l_1833 = &l_1834;
        const uint32_t ***l_1832 = &l_1833;
        int8_t l_1840 = 0xA6L;
        uint32_t ***l_1846 = &l_1783[2][6][0];
        int32_t *l_1849 = &g_201;
        uint8_t *l_1862 = &g_9;
        uint32_t ** const *l_1865 = &l_1783[2][6][0];
        uint32_t ** const ** const l_1864[3] = {&l_1865,&l_1865,&l_1865};
        uint32_t ** const ** const *l_1863 = &l_1864[0];
        uint16_t ****l_1885 = &g_927;
        int32_t l_1893 = 1L;
        int32_t l_1895 = 4L;
        int32_t l_1898[6];
        int i;
        for (i = 0; i < 6; i++)
            l_1898[i] = 8L;
        (*l_1773) ^= ((*g_349) == l_1832);
        for (g_150 = 0; (g_150 <= 57); g_150 = safe_add_func_int64_t_s_s(g_150, 9))
        {
            uint32_t l_1837[3][6][7] = {{{0UL,0x3825896EL,4294967295UL,9UL,1UL,0x89E478BEL,0UL},{4294967291UL,0x3825896EL,0x79FB6D42L,0UL,1UL,4294967290UL,4294967291UL},{4294967291UL,0x89E478BEL,4294967295UL,0UL,4294967295UL,0x89E478BEL,4294967291UL},{0UL,0x3825896EL,4294967295UL,9UL,1UL,0x89E478BEL,0UL},{4294967291UL,0x3825896EL,0x79FB6D42L,0UL,1UL,4294967290UL,4294967291UL},{4294967291UL,0x89E478BEL,4294967295UL,0UL,4294967295UL,0x89E478BEL,4294967291UL}},{{0UL,0x3825896EL,4294967295UL,9UL,1UL,0x89E478BEL,0UL},{4294967291UL,0x3825896EL,0x79FB6D42L,0UL,1UL,4294967290UL,4294967291UL},{4294967291UL,0x89E478BEL,4294967295UL,0UL,4294967295UL,0x89E478BEL,4294967291UL},{0UL,0x3825896EL,4294967295UL,9UL,1UL,0x89E478BEL,0UL},{4294967291UL,0x3825896EL,0x79FB6D42L,0UL,1UL,4294967290UL,4294967291UL},{4294967291UL,0UL,0xDD6DA315L,0xC51B2364L,0xDD6DA315L,0UL,9UL}},{{0x888A6C65L,0UL,0xDD6DA315L,0x52D7A50CL,4294967295UL,0UL,0x888A6C65L},{9UL,0UL,0x94F5D15CL,0xC51B2364L,4294967295UL,4294967291UL,9UL},{9UL,0UL,0xDD6DA315L,0xC51B2364L,0xDD6DA315L,0UL,9UL},{0x888A6C65L,0UL,0xDD6DA315L,0x52D7A50CL,4294967295UL,0UL,0x888A6C65L},{9UL,0UL,0x94F5D15CL,0xC51B2364L,4294967295UL,4294967291UL,9UL},{9UL,0UL,0xDD6DA315L,0xC51B2364L,0xDD6DA315L,0UL,9UL}}};
            int8_t *l_1845[2][5] = {{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0},{&g_316,(void*)0,&g_316,(void*)0,&g_316}};
            int16_t *l_1857 = &g_61;
            int i, j, k;
            if (l_1837[0][2][2])
            {
                uint8_t l_1847 = 255UL;
                if (((safe_mod_func_int8_t_s_s((l_1840 && ((safe_div_func_int64_t_s_s((safe_add_func_int32_t_s_s(((*l_1773) = ((((((g_984[0] , func_38(l_1837[0][2][2])) != &g_195) , (l_1845[0][1] == &l_1840)) & ((*g_349) != ((*l_1812) = l_1846))) | l_1847) & 0L)), l_1847)), 0xAD14E4E538020216LL)) <= g_387)), l_1847)) != 0x136CL))
                {
                    if (g_1222)
                        goto lbl_1848;
                    return l_1837[0][2][0];
                }
                else
                {
                    l_1849 = p_37;
                }
            }
            else
            {
                (***g_497) = &l_1804;
                for (g_161 = 0; (g_161 <= 4); g_161 += 1)
                {
                    int i;
                    return l_1771[g_161];
                }
            }
            (*l_1773) = ((+(safe_sub_func_int16_t_s_s((safe_mul_func_int8_t_s_s(l_1837[0][0][1], ((safe_mul_func_int16_t_s_s(((((*l_1857) = (*l_1773)) & (safe_rshift_func_uint16_t_u_u(((*l_1773) , (safe_sub_func_int64_t_s_s(((l_1837[0][2][2] , (((((l_1862 != &g_9) , g_315) , l_1863) == ((safe_mod_func_int16_t_s_s((safe_sub_func_uint8_t_u_u((safe_mod_func_uint32_t_u_u((l_1837[1][5][5] , (*l_1773)), (*l_1773))), l_1837[0][2][2])), l_1837[0][2][2])) , (void*)0)) && l_1837[0][2][2])) == l_1837[1][5][6]), g_320))), (*l_1773)))) , (*l_1773)), 0x454AL)) >= (-5L)))), 0x9F3FL))) == 0x062EL);
            if (l_1837[0][2][2])
                continue;
            return g_131;
        }
        if ((0xFA83F3E4A44F795FLL > 1L))
        {
            int64_t l_1872 = 0xA75D8EB064F4838ELL;
            (*l_1773) = l_1872;
            for (g_61 = (-18); (g_61 >= 4); g_61 = safe_add_func_int8_t_s_s(g_61, 9))
            {
                uint64_t l_1882 = 18446744073709551614UL;
                const int16_t **l_1889 = (void*)0;
                const int16_t ** const *l_1888[8] = {&l_1889,&l_1889,&l_1889,&l_1889,&l_1889,&l_1889,&l_1889,&l_1889};
                const int16_t ** const **l_1887 = &l_1888[1];
                const int16_t ** const ***l_1886 = &l_1887;
                const int16_t *****l_1890 = &g_473;
                int i;
                for (g_125 = 0; (g_125 > 6); g_125 = safe_add_func_int8_t_s_s(g_125, 1))
                {
                    int8_t l_1879[3][6][8] = {{{0x88L,(-5L),0xE9L,0L,0L,0x33L,0xEEL,0x0DL},{0x2AL,0x6EL,0x9BL,(-9L),(-4L),7L,7L,0L},{0x0DL,1L,0x2AL,(-4L),0L,(-4L),1L,0L},{1L,1L,0L,(-1L),(-6L),(-1L),0xEFL,0x48L},{(-4L),0x81L,0x0AL,(-4L),1L,0x88L,0L,1L},{0L,5L,(-5L),0L,(-4L),0L,0x81L,0x17L}},{{0x5AL,5L,0L,0xB8L,0x33L,0x8BL,0x9BL,0xFCL},{1L,0x17L,(-9L),0xA7L,(-9L),0x17L,1L,(-1L)},{1L,0x33L,0x1EL,0x1BL,7L,0x35L,(-4L),(-9L)},{1L,(-6L),(-9L),0L,7L,0x5AL,0xAEL,0xA7L},{1L,0L,0L,(-9L),(-9L),0xFEL,0x0AL,5L},{1L,0L,0xB8L,(-4L),0x33L,(-1L),0x88L,0x5AL}},{{0x5AL,0L,0xFEL,(-3L),(-4L),1L,2L,0x6EL},{0L,0xE9L,1L,(-1L),1L,0xEFL,(-4L),(-1L)},{(-4L),(-1L),0x17L,0x88L,(-6L),(-1L),1L,0x56L},{1L,0x5AL,5L,1L,0L,0x6EL,0xFEL,0xFEL},{0xEFL,0x07L,2L,2L,0x07L,0xEFL,9L,0L},{0x90L,(-9L),0x8BL,2L,0xA7L,0x2AL,(-1L),(-4L)}}};
                    int32_t *l_1894[5][10] = {{&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803},{&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803},{&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803},{&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803},{&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803,&g_1803}};
                    int i, j, k;
                    l_1895 = ((l_1893 = ((((*l_1773) = ((safe_div_func_int32_t_s_s(((l_1879[0][0][3] , ((safe_rshift_func_uint8_t_u_s((g_887 |= (((l_1882 > l_1872) , ((safe_lshift_func_uint8_t_u_u(((void*)0 == l_1885), 1)) , l_1886)) != (l_1890 = &g_473))), (safe_rshift_func_int8_t_s_u(g_295, ((g_387 , l_1879[0][0][3]) , (*l_1773)))))) , l_1879[1][1][2])) != (*l_1773)), 4294967295UL)) | l_1872)) >= l_1879[0][0][3]) || (*l_1773))) == (*g_169));
                }
            }
        }
        else
        {
            int32_t *l_1896 = (void*)0;
            int32_t *l_1897[3][1][6];
            uint64_t l_1899 = 0UL;
            int32_t *l_1905 = &g_552;
            int32_t **l_1904 = &l_1905;
            int8_t *l_1908[5][10] = {{&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0,&g_1581},{(void*)0,&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0},{&g_1581,&g_1581,&g_1581,&g_1581,&g_1581,&g_1581,&g_1581,&g_1581,&g_1581,&g_1581},{&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0,&g_1581},{(void*)0,&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0,&g_1581,(void*)0,(void*)0}};
            uint16_t *l_1910[3][9] = {{&g_984[2],&g_984[2],(void*)0,(void*)0,&g_984[2],&g_984[2],(void*)0,(void*)0,&g_984[2]},{(void*)0,&g_984[1],(void*)0,&g_984[1],(void*)0,&g_984[1],(void*)0,&g_984[1],(void*)0},{&g_984[2],(void*)0,(void*)0,&g_984[2],&g_984[2],(void*)0,(void*)0,&g_984[2],&g_984[2]}};
            int i, j, k;
            for (i = 0; i < 3; i++)
            {
                for (j = 0; j < 1; j++)
                {
                    for (k = 0; k < 6; k++)
                        l_1897[i][j][k] = &l_1771[4];
                }
            }
            --l_1899;
            l_1770 = (((l_1911 = ((l_1898[1] = ((((safe_mul_func_uint8_t_u_u((((*l_1904) = p_37) == &l_1739), (l_1804 ^= ((+((*l_1773) = (-2L))) , (+(*l_1773)))))) <= 0x56D0254EC05DAFF2LL) , ((*l_1773) , g_161)) & ((**g_588) && 0x3E1D0B39L))) < g_1909)) && l_1898[0]) <= 0x46B4L);
        }
        for (g_195 = 0; (g_195 <= 7); g_195++)
        {
            uint8_t l_1914[10] = {0x19L,0x19L,0x67L,0x19L,0x19L,0x67L,0x19L,0x19L,0x67L,0x19L};
            int i;
            return l_1914[7];
        }
    }
    else
    {
        return g_389;
    }
    return g_9;
}







static int32_t * func_38(uint16_t p_39)
{
    int64_t l_895 = 0xF2342CA9C71401A9LL;
    int32_t l_932 = 0x33BB0510L;
    int32_t *l_933[4];
    uint16_t l_963 = 1UL;
    uint32_t ****l_1004[7];
    uint32_t *****l_1003 = &l_1004[4];
    int64_t **l_1049 = &g_518;
    int32_t l_1121 = 1L;
    uint32_t l_1128 = 0x68EADACAL;
    int64_t l_1146 = 8L;
    uint32_t l_1148 = 0xCCE59BBEL;
    int32_t l_1165 = (-1L);
    int32_t *l_1269[5] = {(void*)0,(void*)0,(void*)0,(void*)0,(void*)0};
    int32_t ***l_1395 = (void*)0;
    uint32_t l_1420 = 0xF32A3A6AL;
    uint16_t l_1523 = 2UL;
    int16_t ***l_1535 = (void*)0;
    int16_t l_1584[10] = {0x7669L,0x7669L,0x7669L,0x7669L,0x7669L,0x7669L,0x7669L,0x7669L,0x7669L,0x7669L};
    int32_t l_1612 = 0x11B9102AL;
    uint32_t l_1637 = 8UL;
    uint16_t l_1697[2];
    int32_t l_1710 = 0x6026AB48L;
    uint64_t l_1723 = 0x0E51D6C749DC4C34LL;
    int i;
    for (i = 0; i < 4; i++)
        l_933[i] = &g_201;
    for (i = 0; i < 7; i++)
        l_1004[i] = &g_589[4][3][0];
    for (i = 0; i < 2; i++)
        l_1697[i] = 65526UL;
    for (g_320 = (-6); (g_320 >= 1); g_320 = safe_add_func_uint64_t_u_u(g_320, 1))
    {
        int32_t *l_894[2][4];
        int16_t *l_896 = &g_299;
        uint8_t l_903 = 1UL;
        uint16_t *l_904 = (void*)0;
        uint16_t *l_905[1];
        int32_t *l_908 = (void*)0;
        int16_t *l_960 = &g_200;
        uint16_t l_962 = 0x31F3L;
        uint16_t l_983 = 9UL;
        int32_t l_1017 = 1L;
        uint32_t **l_1038 = &g_169;
        int16_t ***l_1145 = &g_309;
        int32_t l_1147 = (-4L);
        int i, j;
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 4; j++)
                l_894[i][j] = &g_22;
        }
        for (i = 0; i < 1; i++)
            l_905[i] = &g_109[0];
    }
    --l_1148;
    for (g_429 = 0; (g_429 <= 1); g_429 += 1)
    {
        int32_t l_1153 = (-2L);
        int32_t l_1154[3];
        uint32_t l_1166 = 0x5565F603L;
        int32_t l_1181 = 0x660569AFL;
        int64_t **l_1182 = (void*)0;
        int8_t l_1185 = 1L;
        int16_t ***l_1186 = &g_309;
        uint64_t l_1331 = 0xEE8980F9AD58649ELL;
        uint32_t **l_1383 = &g_169;
        int32_t ***l_1394 = &g_254;
        int32_t l_1414 = 1L;
        uint32_t l_1426 = 0xEF42DB04L;
        uint64_t l_1442 = 0x93FDE79B4C9AC12BLL;
        int64_t *l_1452 = &g_1365;
        uint32_t l_1472[2][8] = {{0x8FB288D0L,0x8FB288D0L,0x6289482DL,0x6289482DL,0x8FB288D0L,0x8FB288D0L,0x6289482DL,0x6289482DL},{0x8FB288D0L,0x8FB288D0L,0x6289482DL,0x6289482DL,0x8FB288D0L,0x8FB288D0L,0x6289482DL,0x6289482DL}};
        uint32_t l_1493 = 18446744073709551615UL;
        uint32_t l_1586 = 0x1AC36E7FL;
        int8_t l_1590 = (-1L);
        int32_t l_1601 = (-6L);
        const uint32_t *l_1670 = &g_320;
        const uint32_t **l_1669 = &l_1670;
        const uint32_t ***l_1668[6][10][4] = {{{&l_1669,&l_1669,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,(void*)0},{(void*)0,&l_1669,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,(void*)0,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,(void*)0,&l_1669}},{{(void*)0,&l_1669,&l_1669,&l_1669},{&l_1669,(void*)0,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,(void*)0},{&l_1669,(void*)0,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,(void*)0},{&l_1669,&l_1669,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669}},{{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{(void*)0,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,(void*)0,(void*)0},{&l_1669,&l_1669,&l_1669,&l_1669},{(void*)0,(void*)0,(void*)0,(void*)0},{&l_1669,&l_1669,&l_1669,&l_1669},{(void*)0,(void*)0,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669}},{{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{(void*)0,&l_1669,&l_1669,&l_1669},{(void*)0,(void*)0,(void*)0,&l_1669},{(void*)0,&l_1669,&l_1669,&l_1669},{(void*)0,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,(void*)0},{&l_1669,&l_1669,(void*)0,&l_1669},{(void*)0,&l_1669,&l_1669,&l_1669}},{{&l_1669,&l_1669,(void*)0,&l_1669},{(void*)0,&l_1669,&l_1669,(void*)0},{&l_1669,(void*)0,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{(void*)0,&l_1669,&l_1669,(void*)0},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,(void*)0,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,(void*)0,&l_1669,&l_1669},{&l_1669,(void*)0,(void*)0,(void*)0}},{{&l_1669,(void*)0,(void*)0,&l_1669},{&l_1669,&l_1669,&l_1669,(void*)0},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,(void*)0},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,(void*)0,&l_1669,(void*)0},{(void*)0,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,&l_1669,&l_1669,&l_1669},{&l_1669,(void*)0,&l_1669,&l_1669}}};
        const uint32_t ****l_1667 = &l_1668[5][9][3];
        uint8_t *l_1676 = &g_887;
        uint32_t l_1680 = 0xD283EA79L;
        uint32_t l_1681[7][7] = {{0xB8B25C75L,0x5783B027L,0xB8B25C75L,0UL,0x5783B027L,0x2D59EFDCL,0x2D59EFDCL},{18446744073709551609UL,0x5804CAF6L,0x8A0D8ADDL,0x5804CAF6L,18446744073709551609UL,0x8A0D8ADDL,0x505B464FL},{1UL,0x2D59EFDCL,0UL,1UL,0UL,0x2D59EFDCL,1UL},{1UL,0x505B464FL,0xC5DAACCBL,1UL,0x505B464FL,1UL,0xC5DAACCBL},{1UL,1UL,0x0EFD0A57L,0x5783B027L,1UL,0x0EFD0A57L,1UL},{18446744073709551609UL,0xC5DAACCBL,0xC5DAACCBL,18446744073709551609UL,1UL,1UL,18446744073709551609UL},{0xB8B25C75L,1UL,0UL,0UL,1UL,0xB8B25C75L,0x2D59EFDCL}};
        uint64_t l_1696 = 0UL;
        int i, j, k;
        for (i = 0; i < 3; i++)
            l_1154[i] = 0x2435E69CL;
        for (g_125 = 0; (g_125 <= 3); g_125 += 1)
        {
            int32_t l_1152 = (-1L);
            int32_t l_1155 = 0xD54267D0L;
            int32_t l_1158 = (-1L);
            int32_t l_1160 = (-8L);
            int32_t l_1162[8] = {0x94EAC19EL,0x94EAC19EL,0x94EAC19EL,0x94EAC19EL,0x94EAC19EL,0x94EAC19EL,0x94EAC19EL,0x94EAC19EL};
            int32_t l_1164[5];
            int i;
            for (i = 0; i < 5; i++)
                l_1164[i] = 0x0F57C40CL;
            for (g_299 = 0; (g_299 <= 3); g_299 += 1)
            {
                int32_t l_1151 = 0x5AA9C608L;
                int32_t l_1156 = 0xFCE1B525L;
                int32_t l_1157 = (-1L);
                int32_t l_1159 = 8L;
                int32_t l_1161 = 5L;
                int32_t l_1163[5][4][3] = {{{0x749615BBL,6L,(-1L)},{0x71911DABL,(-3L),0xFC104854L},{0x749615BBL,0xDF3BA726L,0x749615BBL},{0x71911DABL,0x27EACF68L,0x63E426A6L}},{{0x749615BBL,0x87EA5D25L,1L},{0x71911DABL,0x8C23D4D3L,0x71911DABL},{0x749615BBL,6L,(-1L)},{0x71911DABL,(-3L),0xFC104854L}},{{0x749615BBL,0xDF3BA726L,0x749615BBL},{0x71911DABL,0x27EACF68L,0x63E426A6L},{0x749615BBL,0x87EA5D25L,1L},{0x71911DABL,0x8C23D4D3L,0x71911DABL}},{{0x749615BBL,6L,(-1L)},{0x71911DABL,(-3L),0xFC104854L},{0x749615BBL,0xDF3BA726L,0x749615BBL},{0x71911DABL,0x27EACF68L,0x63E426A6L}},{{0x749615BBL,0x87EA5D25L,1L},{0x71911DABL,0x8C23D4D3L,0x71911DABL},{0x749615BBL,6L,(-1L)},{0x71911DABL,(-3L),0xFC104854L}}};
                uint16_t l_1169[8] = {65535UL,0x51BEL,65535UL,0x51BEL,65535UL,0x51BEL,65535UL,0x51BEL};
                int i, j, k;
                ++l_1166;
                --l_1169[7];
            }
            for (l_1160 = 0; (l_1160 <= 1); l_1160 += 1)
            {
                int32_t *l_1180 = &l_1164[0];
                int i, j, k;
                l_1154[0] ^= ((-9L) ^ p_39);
                l_1181 = ((safe_unary_minus_func_int8_t_s((((7UL > ((l_1166 > 0L) || (p_39 != (safe_rshift_func_int16_t_s_s((l_1154[2] ^= ((safe_mul_func_int8_t_s_s((((*l_1180) = ((p_39 & 1L) & (+(g_127 == 0x2AB98CE89349FB09LL)))) , l_1162[3]), g_150)) <= p_39)), 7))))) , 0x5595CE57L) < 1L))) | 0xC85E4693L);
            }
        }
    }
    return (*g_254);
}







static uint8_t func_44(uint32_t p_45, int32_t * p_46, uint32_t p_47)
{
    uint32_t ***l_585 = (void*)0;
    uint16_t *l_600 = &g_109[2];
    uint16_t **l_599 = &l_600;
    int32_t l_602 = 3L;
    uint32_t l_622 = 0UL;
    uint32_t *l_637 = &g_150;
    uint32_t ** const l_636 = &l_637;
    uint32_t ** const *l_635 = &l_636;
    uint32_t ** const **l_634 = &l_635;
    int16_t *** const **l_643 = &g_642;
    const int16_t * const ****l_717 = &g_713;
    uint8_t *l_731 = &g_102;
    int32_t ***l_772 = &g_254;
    int32_t ****l_771 = &l_772;
    int16_t l_773[10];
    int32_t *l_778[7] = {&l_602,&l_602,&l_602,&l_602,&l_602,&l_602,&l_602};
    uint32_t l_779 = 0x0D3517C4L;
    int32_t l_784 = 0x5A11DEE8L;
    int64_t ***l_795 = &g_517;
    int8_t l_804[4];
    int16_t *l_805 = &g_61;
    int64_t l_806 = (-3L);
    int16_t *l_807 = (void*)0;
    int16_t *l_808 = &g_131;
    uint32_t l_813 = 4294967295UL;
    uint32_t l_814[2];
    uint8_t l_833 = 0x4AL;
    int64_t l_834[5];
    uint64_t *l_837 = &g_161;
    uint32_t l_847 = 6UL;
    int32_t l_853 = 0x9AFB550FL;
    int16_t l_856[10];
    const uint16_t l_885 = 0x111AL;
    int i;
    for (i = 0; i < 10; i++)
        l_773[i] = 7L;
    for (i = 0; i < 4; i++)
        l_804[i] = 0x91L;
    for (i = 0; i < 2; i++)
        l_814[i] = 0x9B0D1D50L;
    for (i = 0; i < 5; i++)
        l_834[i] = 2L;
    for (i = 0; i < 10; i++)
        l_856[i] = 4L;
    for (p_45 = 0; (p_45 > 22); ++p_45)
    {
        uint32_t ****l_586[9][9] = {{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585},{&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585,&l_585}};
        uint16_t *l_598 = &g_109[0];
        uint16_t **l_597 = &l_598;
        int32_t l_601 = 9L;
        int32_t ** const *l_612 = &g_254;
        int32_t ** const **l_611[7][1] = {{&l_612},{&l_612},{&l_612},{&l_612},{&l_612},{&l_612},{&l_612}};
        uint32_t ** const *l_632 = &g_588;
        uint32_t ** const **l_631 = &l_632;
        int16_t *****l_645[4][9] = {{&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307},{&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307},{&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307},{&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307,&g_307}};
        uint8_t *l_730 = &g_102;
        int i, j;
        g_589[4][3][0] = (g_587[2] = l_585);
        for (g_202 = 0; (g_202 >= 10); g_202 = safe_add_func_int64_t_s_s(g_202, 6))
        {
            uint32_t l_596 = 0x94A52266L;
            int32_t l_623 = (-1L);
            uint8_t l_628 = 0x54L;
            int16_t *** const **l_644 = &g_642;
            uint16_t ** const *l_655 = &l_599;
            int64_t **l_686[5][9][5] = {{{&g_518,(void*)0,&g_518,&g_518,(void*)0},{(void*)0,(void*)0,&g_518,(void*)0,&g_518},{(void*)0,&g_518,&g_518,(void*)0,&g_518},{&g_518,(void*)0,&g_518,&g_518,(void*)0},{(void*)0,(void*)0,(void*)0,&g_518,&g_518},{(void*)0,(void*)0,&g_518,&g_518,&g_518},{&g_518,&g_518,(void*)0,(void*)0,(void*)0},{&g_518,(void*)0,&g_518,(void*)0,(void*)0},{&g_518,(void*)0,&g_518,&g_518,(void*)0}},{{(void*)0,(void*)0,&g_518,(void*)0,&g_518},{(void*)0,&g_518,&g_518,(void*)0,&g_518},{&g_518,(void*)0,&g_518,&g_518,(void*)0},{(void*)0,(void*)0,(void*)0,&g_518,&g_518},{(void*)0,(void*)0,&g_518,&g_518,&g_518},{&g_518,&g_518,(void*)0,(void*)0,(void*)0},{&g_518,(void*)0,&g_518,(void*)0,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,(void*)0,&g_518,&g_518,(void*)0}},{{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,(void*)0,&g_518,(void*)0,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,&g_518,&g_518,(void*)0,(void*)0},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,(void*)0,&g_518,&g_518,(void*)0},{&g_518,&g_518,&g_518,&g_518,&g_518}},{{&g_518,(void*)0,&g_518,(void*)0,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,&g_518,&g_518,(void*)0,(void*)0},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,(void*)0,&g_518,&g_518,(void*)0},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,(void*)0,&g_518,(void*)0,&g_518}},{{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,&g_518,&g_518,(void*)0,(void*)0},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518},{(void*)0,(void*)0,&g_518,&g_518,(void*)0},{&g_518,&g_518,&g_518,&g_518,&g_518},{&g_518,(void*)0,&g_518,(void*)0,&g_518},{&g_518,(void*)0,&g_518,&g_518,&g_518}}};
            uint16_t **l_743 = &l_598;
            int i, j, k;
            if ((safe_rshift_func_int8_t_s_s((safe_mod_func_uint64_t_u_u(l_596, (((l_597 == l_599) && (p_45 < (l_602 = l_601))) , (l_602 = p_47)))), p_47)))
            {
                const int64_t *l_619 = (void*)0;
                const int64_t **l_618 = &l_619;
                int16_t *l_620 = &g_131;
                int32_t l_621 = (-10L);
                if ((l_623 = ((safe_lshift_func_int8_t_s_u((safe_rshift_func_int8_t_s_u((((safe_add_func_int8_t_s_s((0xBDL ^ (((l_601 > ((((l_611[2][0] == (void*)0) == (-9L)) || g_202) < (safe_rshift_func_int8_t_s_u(g_387, 1)))) == (+p_47)) && (((g_109[1] ^= ((((((*l_620) = ((safe_mul_func_int16_t_s_s((((*l_618) = (*g_517)) == (void*)0), p_47)) <= l_596)) , g_193) || p_47) != p_47) , 0x07F2L)) != l_621) >= p_47))), l_622)) <= p_47) != l_621), 5)), 3)) < (-1L))))
                {
                    uint32_t l_624 = 0x77A1AFB0L;
                    ++l_624;
                    if (l_621)
                        continue;
                    if (l_602)
                        continue;
                    l_628 = (~g_387);
                }
                else
                {
                    uint32_t l_638 = 4294967295UL;
                    for (g_299 = 13; (g_299 <= (-13)); g_299 = safe_sub_func_uint8_t_u_u(g_299, 3))
                    {
                        uint32_t ** const ***l_633[1][3][7] = {{{&l_631,(void*)0,&l_631,(void*)0,&l_631,(void*)0,&l_631},{&l_631,&l_631,&l_631,&l_631,&l_631,&l_631,&l_631},{&l_631,(void*)0,&l_631,(void*)0,&l_631,(void*)0,&l_631}}};
                        int i, j, k;
                        l_638 = ((l_634 = l_631) == (void*)0);
                        return l_596;
                    }
                    (**l_612) = (void*)0;
                    l_602 ^= (p_45 , l_638);
                    (*g_254) = ((safe_lshift_func_int16_t_s_s(((l_644 = (l_643 = g_641)) != l_645[2][5]), (l_621 == g_61))) , p_46);
                }
            }
            else
            {
                const uint8_t l_676[7] = {0x3CL,0x3CL,0x3CL,0x3CL,0x3CL,0x3CL,0x3CL};
                int32_t l_677[9][6][2];
                int64_t **l_685 = (void*)0;
                uint32_t ** const ***l_690 = &l_634;
                int32_t l_774 = (-1L);
                int64_t ***l_776 = &l_686[3][1][1];
                int64_t ****l_775[6][10][3] = {{{(void*)0,&l_776,&l_776},{(void*)0,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,(void*)0,&l_776},{&l_776,&l_776,&l_776},{(void*)0,&l_776,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776}},{{&l_776,&l_776,&l_776},{&l_776,(void*)0,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,(void*)0},{&l_776,&l_776,(void*)0}},{{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,(void*)0,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776}},{{&l_776,&l_776,&l_776},{&l_776,&l_776,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,(void*)0,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776}},{{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776}},{{&l_776,(void*)0,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,&l_776},{&l_776,&l_776,(void*)0},{&l_776,&l_776,(void*)0},{&l_776,&l_776,&l_776}}};
                int i, j, k;
                for (i = 0; i < 9; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 2; k++)
                            l_677[i][j][k] = 0x10F0023BL;
                    }
                }
                for (l_623 = 2; (l_623 >= 0); l_623 -= 1)
                {
                    int i, j, k;
                    if (((safe_unary_minus_func_int16_t_s(((safe_lshift_func_int16_t_s_s((((safe_lshift_func_int8_t_s_u(g_109[l_623], (safe_add_func_int16_t_s_s(g_109[l_623], (((safe_rshift_func_int16_t_s_s(0xA15EL, l_602)) < ((**g_588) &= (((p_45 ^ 4294967289UL) , 3UL) < ((*g_518) < 18446744073709551615UL)))) | g_109[l_623]))))) , (void*)0) == l_655), 11)) || g_22))) >= 1UL))
                    {
                        uint64_t *l_660 = &g_161;
                        int32_t l_671 = 0xF0C1FCFBL;
                        l_677[5][3][1] |= ((--g_295) == ((6UL && g_125) & ((((((safe_sub_func_uint16_t_u_u(7UL, 0x7530L)) <= ((*l_660) = 18446744073709551615UL)) && (safe_lshift_func_uint8_t_u_u(g_125, 6))) > 255UL) , (((safe_mul_func_int16_t_s_s((safe_add_func_uint8_t_u_u((safe_lshift_func_int16_t_s_u((safe_rshift_func_uint16_t_u_s(((l_671 <= ((safe_lshift_func_int16_t_s_s((((*l_637) &= ((safe_mul_func_int16_t_s_s((-1L), p_47)) ^ (**g_588))) != (**g_588)), p_47)) == 0x3E5ED29FL)) , p_47), 6)), g_387)), p_47)), 7L)) != p_45) , l_676[4])) != l_623)));
                    }
                    else
                    {
                        int16_t l_678 = 0x8151L;
                        l_678 &= p_47;
                        p_46 = p_46;
                    }
                }
                if ((((safe_mul_func_int64_t_s_s((safe_sub_func_uint16_t_u_u((safe_mul_func_int16_t_s_s(l_602, l_676[4])), ((void*)0 == l_685))), (g_295 = (((void*)0 != l_686[3][1][1]) <= (((~(l_676[4] == 0x432916C0L)) , (safe_lshift_func_uint8_t_u_u((g_22 > 0x30L), 7))) || 1L))))) == g_127) && p_45))
                {
                    const int64_t l_692 = 0L;
                    int16_t *****l_706 = &g_307;
                    int32_t l_721 = 0x0FADB3EBL;
                    uint8_t l_729 = 0xF9L;
                    if ((((0x89954DA600E4C763LL ^ (0x28D72A514633C68ELL >= p_45)) , &l_634) == l_690))
                    {
                        uint8_t l_691 = 0xADL;
                        const int16_t * const l_711 = &g_300;
                        const int16_t * const *l_710 = &l_711;
                        const int16_t * const **l_709 = &l_710;
                        const int16_t * const ***l_708[3][6] = {{&l_709,&l_709,&l_709,&l_709,&l_709,&l_709},{&l_709,&l_709,&l_709,&l_709,&l_709,&l_709},{&l_709,&l_709,&l_709,&l_709,&l_709,&l_709}};
                        const int16_t * const ****l_707 = &l_708[0][0];
                        const int8_t *l_720 = &g_316;
                        int i, j;
                        l_602 = l_691;
                        if (l_692)
                            break;
                        l_602 = ((safe_mod_func_uint32_t_u_u((safe_lshift_func_int8_t_s_s((safe_add_func_uint8_t_u_u(((l_721 = (safe_mod_func_int64_t_s_s(0x0A0F2D54EEFEB5C5LL, (safe_unary_minus_func_uint8_t_u((((0x7EL && (safe_rshift_func_uint8_t_u_s((p_45 ^ (l_676[1] || (safe_lshift_func_uint8_t_u_u(((l_706 != (l_717 = (g_712 = l_707))) >= ((safe_div_func_uint64_t_u_u(((l_720 != (void*)0) && (1L == p_45)), (**g_517))) , p_47)), g_319)))), 6))) && p_45) < 0x64C179C4L)))))) & 0xAC779F7514E4D693LL), 1L)), g_552)), l_676[4])) >= 0xDFL);
                        if (l_721)
                            break;
                    }
                    else
                    {
                        int8_t l_722 = 0xD5L;
                        l_722 = l_623;
                    }
                    l_677[5][3][1] = (l_677[7][1][1] ^ ((safe_mod_func_uint8_t_u_u((p_45 > 0x3369L), ((*l_730) = ((safe_add_func_uint16_t_u_u(((**l_597) = ((-6L) != (safe_add_func_int64_t_s_s((**g_517), (l_729 && ((l_731 = l_730) == &l_729)))))), ((safe_mod_func_int8_t_s_s(((l_602 != g_552) , g_200), l_677[2][1][1])) , p_47))) ^ 18446744073709551609UL)))) , p_47));
                }
                else
                {
                    uint16_t ***l_744 = &l_597;
                    int32_t *l_745 = &l_677[8][0][1];
                    (*l_745) ^= (+(safe_mul_func_uint16_t_u_u(((((g_97 <= p_45) == ((*l_730) = (1UL < (0x59L > (safe_mul_func_int16_t_s_s((((safe_mul_func_int8_t_s_s((((p_47 | ((((*l_655) != ((*l_744) = l_743)) , &g_350) == ((l_745 == &l_677[2][1][0]) , (void*)0))) , p_46) == (void*)0), 3UL)) & 0x7CEAD608L) != 4294967288UL), g_319)))))) < p_45) ^ l_602), l_622)));
                }
                l_774 &= ((safe_mul_func_int16_t_s_s((l_623 = (((void*)0 != (***l_644)) > (((safe_sub_func_int32_t_s_s((((**l_743) &= (((safe_div_func_uint32_t_u_u(((*****l_690) = (safe_add_func_uint64_t_u_u((((safe_add_func_int32_t_s_s((!(p_45 != g_316)), (((safe_div_func_int64_t_s_s((~(safe_mod_func_uint16_t_u_u((safe_mod_func_int32_t_s_s(l_622, (safe_sub_func_uint16_t_u_u((safe_lshift_func_uint16_t_u_s(p_47, (safe_rshift_func_uint16_t_u_s((~(((*l_731) |= (l_677[5][3][1] > ((void*)0 == l_771))) >= p_45)), 10)))), 65533UL)))), p_47))), g_97)) , 0x8E3AL) != p_47))) > p_45) >= (-1L)), p_45))), 4294967293UL)) , p_47) ^ 0x73L)) <= l_773[7]), l_623)) , (*g_349)) != (*l_634)))), g_315)) ^ l_677[2][2][1]);
                g_777 = &l_686[4][7][2];
            }
        }
        if (p_45)
            continue;
    }
    ++l_779;
    l_784 = (safe_div_func_uint64_t_u_u(0x8B2525D5E1277629LL, g_387));
    if ((safe_lshift_func_int16_t_s_u(((*l_808) = ((*l_634) != (((((***l_795) = (safe_mul_func_int8_t_s_s((safe_rshift_func_uint8_t_u_s((((*l_805) = (safe_rshift_func_uint8_t_u_u(p_47, ((((safe_rshift_func_int8_t_s_u((l_795 != ((((void*)0 != (*l_771)) != (p_47 != (((g_102 |= ((((safe_mul_func_uint16_t_u_u(((+(p_45 == (((~(safe_sub_func_int16_t_s_s((0x0F38L ^ (safe_div_func_uint16_t_u_u(0x5A98L, p_47))), 0xFF76L))) == l_804[1]) , (**g_168)))) , g_552), p_45)) || (**g_517)) || 0xDAL) == g_109[0])) | 0x3DL) > p_47))) , (void*)0)), g_387)) & g_195) > 0x12728498L) ^ p_47)))) != l_806), p_47)), g_127))) , g_61) >= 0x0973L) , (void*)0))), 15)))
    {
        int32_t l_809 = 0x2E61A1FEL;
        int32_t *l_810[4][3];
        int i, j;
        for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 3; j++)
                l_810[i][j] = &l_809;
        }
        l_814[1] = (((g_195 = l_809) , l_809) == (safe_mod_func_uint64_t_u_u(0x9FC5461BB2F503B9LL, l_813)));
    }
    else
    {
        int64_t l_815 = 8L;
        int32_t l_826 = 3L;
        uint64_t *l_838 = &g_97;
        int32_t l_849 = (-7L);
        p_46 = (***g_497);
        if ((l_815 , ((((*l_808) = (p_45 <= ((safe_mod_func_uint8_t_u_u(249UL, (safe_rshift_func_int8_t_s_s(((safe_sub_func_uint64_t_u_u(((safe_lshift_func_uint8_t_u_s(((((safe_add_func_int32_t_s_s(p_47, (l_826 = (-3L)))) , (safe_mul_func_uint16_t_u_u(((*g_518) != ((safe_add_func_int32_t_s_s(((safe_lshift_func_uint16_t_u_s((0xEDL | ((g_195 == l_833) , 0xC9L)), 0)) <= p_45), 0x360FC2C0L)) > 1UL)), 8L))) <= l_815) != l_834[1]), 7)) ^ 65535UL), 0L)) | l_815), 3)))) && 65528UL))) & p_45) > 0xACFE029DL)))
        {
            return p_45;
        }
        else
        {
            int16_t l_843 = 0x2AFDL;
            int8_t *l_846 = (void*)0;
            int32_t ****l_848 = &l_772;
            uint32_t ****l_873 = &l_585;
            int32_t l_884 = 0xB20F18D3L;
            int32_t l_886 = 0x9ABAE0BCL;
            if ((((((((safe_lshift_func_int16_t_s_s(p_45, ((p_47 != (l_837 != l_838)) >= p_45))) | ((*l_808) |= ((safe_mul_func_uint8_t_u_u(((safe_add_func_uint8_t_u_u(((((((void*)0 == &l_833) , (l_843 , ((safe_rshift_func_int8_t_s_u((g_316 ^= ((&g_109[2] == (void*)0) <= p_47)), 6)) || p_45))) != 0x14E3C857L) , l_847) == 0x14L), 0L)) <= 0x08CC0DF21B449090LL), g_389)) , 0xA12DL))) , l_846) != (void*)0) , l_848) == l_848) >= (*g_518)))
            {
                uint16_t l_850[10][8] = {{65534UL,0xE5B2L,0xE5B2L,65534UL,65527UL,0xC54FL,65527UL,65534UL},{0xE5B2L,65527UL,0xC54FL,0xD728L,65527UL,65527UL,0xD728L,0xC54FL},{65533UL,65533UL,65527UL,7UL,65534UL,7UL,65527UL,65533UL},{65533UL,0xC54FL,0xD728L,65527UL,65527UL,0xD728L,0xC54FL,65533UL},{0xC54FL,0xE5B2L,65533UL,7UL,65533UL,0xE5B2L,0xC54FL,0xC54FL},{0xE5B2L,7UL,0xD728L,0xD728L,7UL,0xE5B2L,65527UL,0xE5B2L},{7UL,0xE5B2L,65527UL,0xE5B2L,7UL,0xD728L,0xD728L,7UL},{0xE5B2L,0xC54FL,0xC54FL,0xE5B2L,65533UL,7UL,65533UL,0xE5B2L},{0xC54FL,65533UL,0xC54FL,0xD728L,65527UL,65527UL,0xD728L,0xC54FL},{65533UL,65533UL,65527UL,7UL,65534UL,7UL,65527UL,65533UL}};
                uint8_t *l_855 = &l_833;
                int i, j;
                (***l_771) = (*g_254);
                l_849 &= l_826;
                ++l_850[3][2];
                l_856[4] = (l_853 == ((*l_855) ^= ((*l_731) = (safe_unary_minus_func_uint8_t_u(0x21L)))));
            }
            else
            {
                uint32_t *****l_874 = &l_873;
                int32_t l_883 = 0x33909A30L;
                l_886 |= (((0x6554B81845445E67LL >= (l_883 = ((safe_sub_func_int8_t_s_s((g_316 = (safe_sub_func_uint8_t_u_u(((safe_add_func_int16_t_s_s(((l_815 > ((safe_mod_func_uint16_t_u_u((safe_add_func_int8_t_s_s(((safe_mod_func_int8_t_s_s(l_826, (safe_lshift_func_uint16_t_u_u((safe_rshift_func_int16_t_s_s(((l_849 = ((((*l_874) = l_873) == (void*)0) ^ ((safe_mod_func_uint16_t_u_u((0xC8EDL == (l_884 = ((safe_mod_func_int8_t_s_s((p_47 | (((l_815 || ((safe_sub_func_int64_t_s_s(((*g_518) = (safe_add_func_int16_t_s_s((l_883 < 1UL), 0xA3FEL))), 0xAD88A929EB51D2F4LL)) > 0x5141L)) | p_45) , 0xCE1778669004628FLL)), l_883)) ^ p_47))), 0xE262L)) > p_47))) == p_45), 10)), p_45)))) >= (-1L)), 0x3AL)), 0xC08DL)) == p_45)) && l_885), (-3L))) <= p_45), p_45))), l_883)) || l_883))) >= p_45) && p_45);
                return g_295;
            }
        }
        l_849 = g_887;
        l_826 = l_849;
    }
    return g_319;
}







static int16_t func_56(int8_t p_57)
{
    uint64_t l_58[6] = {0xF88592840CC74BF7LL,0xF88592840CC74BF7LL,0xB1C5ADF851F08A1BLL,0xF88592840CC74BF7LL,0xF88592840CC74BF7LL,0xB1C5ADF851F08A1BLL};
    int32_t *l_59 = &g_22;
    int i;
    (*l_59) |= l_58[4];
    return p_57;
}







static int32_t * func_64(const int64_t p_65, int32_t p_66, int16_t * p_67)
{
    uint32_t l_78[3];
    int32_t l_84[4] = {0x5E53F01EL,0x5E53F01EL,0x5E53F01EL,0x5E53F01EL};
    int32_t *l_95 = &g_22;
    int32_t **l_94[1][3][6];
    int64_t l_160 = 0xC7289F41D23F08B2LL;
    int8_t l_190 = 2L;
    uint8_t *l_231 = &g_102;
    uint32_t l_246[10][10][2] = {{{4294967295UL,0x3050E478L},{0x9274EEE2L,0xC5E54166L},{0x06E931A3L,0x9274EEE2L},{0x8B98E25FL,1UL},{0x8B98E25FL,0x9274EEE2L},{0x06E931A3L,0xC5E54166L},{0x9274EEE2L,0x3050E478L},{4294967295UL,0x8B98E25FL},{0x6CDAD958L,0xC5E54166L},{0xC5E54166L,0x6CDAD958L}},{{0x8B98E25FL,4294967295UL},{0x3050E478L,0x9274EEE2L},{0xC5E54166L,0x06E931A3L},{0x9274EEE2L,0x8B98E25FL},{1UL,0x8B98E25FL},{0x9274EEE2L,0x06E931A3L},{0xC5E54166L,0x9274EEE2L},{0x3050E478L,4294967295UL},{0x8B98E25FL,0x6CDAD958L},{0xC5E54166L,0xC5E54166L}},{{0x6CDAD958L,0x8B98E25FL},{4294967295UL,0x3050E478L},{0x9274EEE2L,0xC5E54166L},{0x06E931A3L,0x9274EEE2L},{0x8B98E25FL,1UL},{0x8B98E25FL,0x9274EEE2L},{0x06E931A3L,0xC5E54166L},{0x9274EEE2L,0x3050E478L},{4294967295UL,0x8B98E25FL},{0x6CDAD958L,0xC5E54166L}},{{0xC5E54166L,0x6CDAD958L},{0x8B98E25FL,4294967295UL},{0x3050E478L,0x9274EEE2L},{0xC5E54166L,0x06E931A3L},{0x9274EEE2L,0x8B98E25FL},{1UL,0x8B98E25FL},{0x9274EEE2L,0x06E931A3L},{0xC5E54166L,0x9274EEE2L},{0x3050E478L,4294967295UL},{0x8B98E25FL,0x6CDAD958L}},{{0xC5E54166L,0xC5E54166L},{0x6CDAD958L,0x8B98E25FL},{4294967295UL,0x3050E478L},{0x9274EEE2L,0xC5E54166L},{0x06E931A3L,0x9274EEE2L},{0x8B98E25FL,1UL},{0x8B98E25FL,0x9274EEE2L},{0x06E931A3L,0xC5E54166L},{0x9274EEE2L,0x3050E478L},{4294967295UL,0x8B98E25FL}},{{0x6CDAD958L,0xC5E54166L},{0xC5E54166L,0x6CDAD958L},{0x8B98E25FL,4294967295UL},{0x3050E478L,0x9274EEE2L},{0xC5E54166L,0x06E931A3L},{1UL,0x6CDAD958L},{7UL,0x6CDAD958L},{1UL,0xC6AF0C3EL},{1UL,1UL},{0x569A2F9DL,0xD4A470ADL}},{{0x6CDAD958L,0x82A31A93L},{1UL,1UL},{0x82A31A93L,0x6CDAD958L},{0xD4A470ADL,0x569A2F9DL},{1UL,1UL},{0xC6AF0C3EL,1UL},{0x6CDAD958L,7UL},{0x6CDAD958L,1UL},{0xC6AF0C3EL,1UL},{1UL,0x569A2F9DL}},{{0xD4A470ADL,0x6CDAD958L},{0x82A31A93L,1UL},{1UL,0x82A31A93L},{0x6CDAD958L,0xD4A470ADL},{0x569A2F9DL,1UL},{1UL,0xC6AF0C3EL},{1UL,0x6CDAD958L},{7UL,0x6CDAD958L},{1UL,0xC6AF0C3EL},{1UL,1UL}},{{0x569A2F9DL,0xD4A470ADL},{0x6CDAD958L,0x82A31A93L},{1UL,1UL},{0x82A31A93L,0x6CDAD958L},{0xD4A470ADL,0x569A2F9DL},{1UL,1UL},{0xC6AF0C3EL,1UL},{0x6CDAD958L,7UL},{0x6CDAD958L,1UL},{0xC6AF0C3EL,1UL}},{{1UL,0x569A2F9DL},{0xD4A470ADL,0x6CDAD958L},{0x82A31A93L,1UL},{1UL,0x82A31A93L},{0x6CDAD958L,0xD4A470ADL},{0x569A2F9DL,1UL},{1UL,0xC6AF0C3EL},{1UL,0x6CDAD958L},{7UL,0x6CDAD958L},{1UL,0xC6AF0C3EL}}};
    uint16_t *l_251[7] = {&g_109[1],&g_109[1],&g_109[1],&g_109[1],&g_109[1],&g_109[1],&g_109[1]};
    const uint32_t *l_292 = &l_246[6][4][0];
    const uint32_t **l_291[9] = {&l_292,&l_292,&l_292,&l_292,&l_292,&l_292,&l_292,&l_292,&l_292};
    const uint32_t ***l_290 = &l_291[4];
    int8_t l_338 = 0xECL;
    uint32_t l_362 = 0xBF90950AL;
    int16_t ****l_430 = &g_308;
    int32_t l_452 = (-1L);
    int64_t l_480 = 0x7F720654CC578BD2LL;
    int32_t l_571 = 0xB2CFCE5EL;
    uint32_t l_572 = 0x415FF63AL;
    int32_t *l_576[6][7][2] = {{{&g_201,&g_201},{&l_84[1],&l_84[0]},{&g_22,&l_84[0]},{(void*)0,(void*)0},{(void*)0,(void*)0},{&g_22,&g_22},{&g_22,(void*)0}},{{(void*)0,(void*)0},{(void*)0,&l_84[0]},{&g_22,&l_84[0]},{&l_84[1],&g_201},{&g_201,(void*)0},{&l_84[0],(void*)0},{&g_201,&g_201}},{{&l_84[1],&l_84[0]},{&g_22,&l_84[0]},{(void*)0,(void*)0},{(void*)0,(void*)0},{&g_22,&g_22},{&g_22,(void*)0},{(void*)0,(void*)0}},{{(void*)0,&l_84[0]},{&g_22,&l_84[0]},{&l_84[1],&g_201},{&g_201,(void*)0},{&l_84[0],(void*)0},{&g_201,&g_201},{&l_84[1],&l_84[0]}},{{&g_22,&l_84[0]},{(void*)0,(void*)0},{(void*)0,(void*)0},{&g_22,&g_22},{&g_22,(void*)0},{(void*)0,(void*)0},{(void*)0,&l_84[0]}},{{&g_22,&l_84[0]},{&l_84[1],&g_201},{&g_201,(void*)0},{&l_84[0],(void*)0},{&g_201,&g_201},{&l_84[1],&l_84[0]},{&g_22,&l_84[0]}}};
    int i, j, k;
    for (i = 0; i < 3; i++)
        l_78[i] = 0UL;
    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < 3; j++)
        {
            for (k = 0; k < 6; k++)
                l_94[i][j][k] = &l_95;
        }
    }
    for (p_66 = 0; (p_66 <= 2); p_66 += 1)
    {
        int32_t *l_82 = &g_22;
        int32_t **l_81 = &l_82;
        int32_t l_106[1];
        int16_t *l_130 = &g_61;
        uint32_t l_196[2];
        uint64_t *l_236 = &g_97;
        uint8_t *l_243 = &g_102;
        int16_t ** const **l_244 = (void*)0;
        uint16_t *l_245 = &g_202;
        int i;
        for (i = 0; i < 1; i++)
            l_106[i] = 0x5C014297L;
        for (i = 0; i < 2; i++)
            l_196[i] = 0xED5CDA24L;
        if ((9L & l_78[p_66]))
        {
            (*l_82) |= (safe_add_func_int16_t_s_s((l_81 != &g_11), ((*p_67) = 0x1AC6L)));
        }
        else
        {
            uint64_t *l_83[3];
            int16_t *l_93 = &g_61;
            int16_t **l_92 = &l_93;
            int32_t ***l_96[8] = {&l_94[0][1][3],&l_94[0][1][3],&l_94[0][1][3],&l_94[0][1][3],&l_94[0][1][3],&l_94[0][1][3],&l_94[0][1][3],&l_94[0][1][3]};
            uint8_t *l_101 = &g_102;
            int i;
            for (i = 0; i < 3; i++)
                l_83[i] = (void*)0;
            if ((*l_82))
                break;
            g_97 &= (((((l_78[0] , ((((((l_78[2] <= p_66) > ((((l_84[0] = p_66) | ((!(safe_lshift_func_uint16_t_u_s((safe_sub_func_uint8_t_u_u((safe_div_func_int16_t_s_s((((*l_92) = p_67) == (((l_94[0][0][2] = l_94[0][1][3]) == &g_11) , &g_61)), (((p_66 & p_66) , g_61) | g_22))), (*l_95))), 10))) , (*l_82))) , (*l_81)) == l_95)) < 18446744073709551612UL) && g_22) <= (*l_95)) > g_22)) >= (*l_95)) > p_66) < 0UL) >= p_66);
            (**l_81) = (safe_unary_minus_func_int16_t_s(0L));
            if ((safe_mul_func_int8_t_s_s(p_65, ((*l_101)--))))
            {
                int32_t l_105 = 0xBF6A774BL;
                uint64_t l_128 = 0xD84A95BEA926FAA8LL;
                int32_t **l_151 = &l_82;
                for (g_61 = 2; (g_61 >= 0); g_61 -= 1)
                {
                    int16_t l_107 = 0x2F57L;
                    int32_t l_108[5];
                    uint32_t *l_124 = &g_125;
                    int64_t *l_126[6][7][5] = {{{&g_127,(void*)0,&g_127,&g_127,&g_127},{&g_127,&g_127,(void*)0,(void*)0,&g_127},{&g_127,&g_127,(void*)0,&g_127,(void*)0},{&g_127,&g_127,&g_127,&g_127,&g_127},{&g_127,&g_127,&g_127,&g_127,&g_127},{(void*)0,&g_127,&g_127,&g_127,(void*)0},{(void*)0,(void*)0,&g_127,(void*)0,&g_127}},{{&g_127,&g_127,(void*)0,&g_127,&g_127},{&g_127,&g_127,&g_127,&g_127,&g_127},{(void*)0,&g_127,&g_127,&g_127,&g_127},{&g_127,&g_127,&g_127,&g_127,(void*)0},{(void*)0,&g_127,(void*)0,&g_127,&g_127},{&g_127,&g_127,(void*)0,&g_127,&g_127},{(void*)0,(void*)0,(void*)0,&g_127,&g_127}},{{&g_127,&g_127,&g_127,(void*)0,&g_127},{&g_127,&g_127,&g_127,&g_127,(void*)0},{&g_127,&g_127,(void*)0,&g_127,&g_127},{&g_127,(void*)0,&g_127,&g_127,&g_127},{&g_127,&g_127,&g_127,&g_127,&g_127},{&g_127,&g_127,&g_127,(void*)0,&g_127},{&g_127,&g_127,&g_127,&g_127,&g_127}},{{&g_127,(void*)0,&g_127,&g_127,&g_127},{(void*)0,&g_127,&g_127,&g_127,(void*)0},{&g_127,&g_127,(void*)0,&g_127,&g_127},{&g_127,&g_127,&g_127,(void*)0,&g_127},{&g_127,&g_127,&g_127,(void*)0,&g_127},{&g_127,(void*)0,(void*)0,&g_127,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,&g_127}},{{(void*)0,&g_127,(void*)0,&g_127,&g_127},{&g_127,(void*)0,&g_127,&g_127,(void*)0},{&g_127,&g_127,&g_127,&g_127,&g_127},{&g_127,(void*)0,&g_127,&g_127,&g_127},{&g_127,(void*)0,(void*)0,&g_127,(void*)0},{&g_127,&g_127,&g_127,&g_127,&g_127},{&g_127,&g_127,&g_127,(void*)0,&g_127}},{{(void*)0,&g_127,&g_127,&g_127,&g_127},{(void*)0,&g_127,&g_127,&g_127,(void*)0},{&g_127,(void*)0,&g_127,&g_127,&g_127},{&g_127,(void*)0,(void*)0,&g_127,&g_127},{&g_127,&g_127,&g_127,&g_127,&g_127},{&g_127,&g_127,(void*)0,(void*)0,&g_127},{(void*)0,&g_127,&g_127,&g_127,&g_127}}};
                    int32_t l_129[7] = {0x78518540L,0x78518540L,0x71FE85D9L,0x78518540L,0x78518540L,0x71FE85D9L,0x78518540L};
                    int i, j, k;
                    for (i = 0; i < 5; i++)
                        l_108[i] = 5L;
                    g_109[0]++;
                    for (l_105 = 0; (l_105 <= 2); l_105 += 1)
                    {
                        if ((**l_81))
                            break;
                        return &g_22;
                    }
                    if (g_61)
                        break;
                    if (((l_108[1] == (+(safe_div_func_int64_t_s_s((l_129[3] |= ((g_127 ^= (p_66 >= ((safe_add_func_uint64_t_u_u((+l_105), 0x8F66F91BAA844380LL)) <= ((((0x87F353C2L <= (safe_add_func_uint32_t_u_u(g_9, ((*l_124) = ((safe_add_func_uint32_t_u_u(((l_78[p_66] = ((*l_95) >= p_66)) , (((((safe_add_func_uint64_t_u_u(0x71F4E5A6583FDF71LL, 0xB637DDF4F03A57AALL)) , &g_61) == p_67) > g_9) , g_109[1])), (**l_81))) , l_105))))) ^ p_66) < 0x8AE84C8BE40C264FLL) , 0x603581D61F6B5038LL)))) <= l_128)), p_66)))) <= g_109[0]))
                    {
                        uint16_t l_132 = 3UL;
                        uint32_t *l_149 = &g_150;
                        g_131 = ((*l_82) &= (l_130 != p_67));
                        (*l_81) = &g_22;
                        --l_132;
                        (**l_151) = (((safe_mod_func_uint8_t_u_u(((*l_101) = p_66), (((l_129[5] = (p_65 & (safe_lshift_func_int8_t_s_s(((safe_div_func_uint8_t_u_u(((((p_65 > ((safe_rshift_func_uint8_t_u_u(0x08L, (safe_sub_func_uint32_t_u_u((((((*l_149) |= (++(*l_124))) , l_151) != (void*)0) & (g_161 = (safe_lshift_func_int8_t_s_u(((safe_lshift_func_uint8_t_u_s((safe_add_func_int8_t_s_s(g_109[2], (safe_unary_minus_func_uint16_t_u(g_109[0])))), (safe_unary_minus_func_int64_t_s((g_97 >= (*l_95)))))) || l_160), (**l_151))))), l_129[3])))) == p_66)) != 4L) || 5L) , 0UL), g_131)) && 0x8C0CL), 6)))) <= g_127) , 6UL))) < 4294967291UL) < p_66);
                    }
                    else
                    {
                        int16_t * volatile ***l_165 = &g_162;
                        (*l_165) = g_162;
                        return &g_22;
                    }
                }
                for (g_131 = 0; (g_131 >= 5); g_131++)
                {
                    uint32_t * volatile **l_170 = &g_168;
                    int32_t *l_173 = &g_22;
                    (*l_170) = g_168;
                    if (p_66)
                        continue;
                    for (g_125 = 0; (g_125 >= 48); ++g_125)
                    {
                        return l_173;
                    }
                }
            }
            else
            {
                int64_t *l_178 = &g_127;
                int32_t l_191 = 0xA52435B8L;
                uint8_t *l_192[9] = {&g_9,&g_9,&g_9,&g_9,&g_9,&g_9,&g_9,&g_9,&g_9};
                int32_t l_194 = 9L;
                int i;
                l_194 |= ((*l_82) = ((safe_mul_func_int16_t_s_s((((*l_178) = (((g_102 != p_66) && (**l_81)) != (g_22 || (safe_lshift_func_int16_t_s_u(0x5DF2L, 0))))) , (((65535UL > ((safe_mul_func_uint16_t_u_u((safe_lshift_func_int16_t_s_s((!1L), (((**l_81) <= (safe_add_func_uint8_t_u_u((g_193 = (((safe_lshift_func_uint16_t_u_s(((((*l_101) = ((((safe_mod_func_int64_t_s_s(p_66, l_190)) , p_65) , 8L) | p_65)) >= p_65) > l_191), (*p_67))) , &l_81) != (void*)0)), p_65))) != g_97))), g_127)) , (*l_82))) > 1L) && (*l_82))), g_97)) , g_131));
                for (l_194 = 2; (l_194 >= 0); l_194 -= 1)
                {
                    int32_t l_199 = 0x688CC613L;
                    for (g_150 = 0; (g_150 <= 2); g_150 += 1)
                    {
                        --l_196[1];
                        l_199 ^= g_109[0];
                        if (g_61)
                            continue;
                        if (l_199)
                            break;
                    }
                }
            }
        }
        g_202--;
        l_106[0] ^= ((safe_div_func_uint64_t_u_u((safe_div_func_int64_t_s_s((safe_mod_func_int64_t_s_s((((*l_231) = (0x508AL && ((*l_245) ^= (safe_rshift_func_uint16_t_u_s((safe_add_func_int8_t_s_s((((((safe_add_func_int64_t_s_s(((((safe_div_func_uint64_t_u_u((safe_mul_func_int16_t_s_s((safe_add_func_uint8_t_u_u((p_65 < ((safe_mod_func_uint64_t_u_u((((safe_add_func_uint32_t_u_u((((safe_rshift_func_uint16_t_u_u((safe_add_func_int16_t_s_s((l_231 != ((safe_mul_func_uint8_t_u_u((0xF8L && 0L), (((safe_sub_func_uint32_t_u_u((**l_81), (0xF3L == (((*l_236) = p_66) , ((safe_rshift_func_uint16_t_u_u((safe_lshift_func_uint16_t_u_u((((safe_mod_func_int32_t_s_s(((void*)0 == l_243), p_66)) , l_244) != (void*)0), 8)), p_65)) & p_66))))) & g_109[2]) > g_9))) , l_231)), (*p_67))), 7)) , (*l_95)) ^ (**g_168)), (**l_81))) != (-1L)) >= 0xB12810C4A4D08B0DLL), (**l_81))) == (-1L))), (-10L))), (*p_67))), g_127)) < p_65) ^ g_125) , 0xACACBF73657E3DCALL), g_127)) || (-7L)) , (*g_169)) & (**l_81)) , g_97), 0x94L)), 10))))) , p_65), g_61)), p_65)), 3L)) > 4294967295UL);
        if (g_131)
            continue;
    }
    (*l_95) ^= (-5L);
    l_246[4][9][0]++;
    if ((safe_mul_func_int16_t_s_s(((g_109[0]--) | ((*l_95) = (g_202 = g_202))), (*p_67))))
    {
        int32_t **l_256 = &g_255;
        int32_t ***l_257 = &l_94[0][1][3];
        uint32_t **l_287[6] = {&g_169,&g_169,&g_169,&g_169,&g_169,&g_169};
        uint32_t ** const *l_286 = &l_287[2];
        const int16_t *l_297 = &g_61;
        int16_t **l_306 = &g_164;
        int16_t ***l_305 = &l_306;
        int16_t ****l_304 = &l_305;
        int32_t l_312 = 1L;
        uint64_t l_337 = 0UL;
        const uint16_t *l_388 = &g_389;
        int32_t l_538 = 0L;
        int32_t l_544 = 0x4D653010L;
        int32_t l_548 = 0x5ECFA799L;
        int32_t l_553 = 0xF05929FFL;
        int32_t l_554 = (-1L);
        int32_t l_559 = 3L;
        int32_t l_561 = 0x32CB0566L;
        int32_t l_564[5][9][5] = {{{0x7A22D02CL,1L,(-1L),0xCC19B883L,1L},{0x405B2C34L,0xD364A1C2L,0L,0x7A0144DFL,0x1F9B467DL},{0xCC19B883L,0L,(-1L),1L,(-1L)},{0x9641C11CL,0x9641C11CL,7L,(-1L),7L},{0xF190E59DL,0xDF14EF85L,0x1BE29D33L,0xDFE70636L,0x98304C7DL},{9L,0x314F7B55L,0x9641C11CL,5L,7L},{0xF80D90F1L,0xDF14EF85L,0xC9FF1E84L,0xC9FF1E84L,0xDF14EF85L},{0x1F9B467DL,0x9641C11CL,0x7A0144DFL,0x405B2C34L,0xE0FDFBF5L},{0xD99F85ADL,0L,0L,0xF80D90F1L,0xDFE70636L}},{{(-1L),0xD364A1C2L,7L,0x314F7B55L,1L},{0xD99F85ADL,1L,0xF190E59DL,1L,0xD99F85ADL},{0x1F9B467DL,1L,1L,7L,0x9641C11CL},{0xF80D90F1L,0x7A22D02CL,1L,0L,0xFE348CBDL},{9L,0L,1L,1L,0x9641C11CL},{0xF190E59DL,0L,0L,0xF190E59DL,0xD99F85ADL},{0x9641C11CL,0x7A0144DFL,0x405B2C34L,0xE0FDFBF5L,1L},{0xCC19B883L,0xD99F85ADL,0x0A139A13L,(-1L),0xDFE70636L},{0x405B2C34L,7L,0xD364A1C2L,0xE0FDFBF5L,0xE0FDFBF5L}},{{0x7A22D02CL,0xCC19B883L,0x7A22D02CL,0xF190E59DL,0xDF14EF85L},{7L,(-1L),7L,1L,7L},{0xC9FF1E84L,0x1BE29D33L,0xD99F85ADL,0L,0x98304C7DL},{1L,0xE0FDFBF5L,7L,7L,7L},{0x0A139A13L,0x0A139A13L,0x7A22D02CL,1L,(-1L)},{1L,0x955292A2L,0xD364A1C2L,0x314F7B55L,0x1F9B467DL},{0xFE348CBDL,0xDFE70636L,0x0A139A13L,0xF80D90F1L,1L},{5L,0x955292A2L,0x405B2C34L,0x405B2C34L,0x955292A2L},{0x98304C7DL,0x0A139A13L,0L,0xC9FF1E84L,0L}},{{0L,0xE0FDFBF5L,1L,5L,0x314F7B55L},{1L,0x1BE29D33L,0xC9FF1E84L,0L,0x7A22D02CL},{(-1L),0x7A0144DFL,7L,0x7A0144DFL,(-1L)},{0L,0x1BE29D33L,0x7A22D02CL,0xC9FF1E84L,0xFE348CBDL},{7L,0x1F9B467DL,0x405B2C34L,0x9641C11CL,0xEFA37957L},{0xD051C94AL,1L,0xCC19B883L,0x1BE29D33L,0xFE348CBDL},{7L,0x9641C11CL,0x9641C11CL,7L,(-1L)},{0xFE348CBDL,0x0A139A13L,0xF190E59DL,(-1L),0x7A22D02CL},{0xD364A1C2L,(-1L),9L,0L,1L}},{{0xF190E59DL,0x98304C7DL,0xF80D90F1L,(-1L),(-1L)},{0x1F9B467DL,0xD364A1C2L,0x1F9B467DL,7L,0x314F7B55L},{0x98304C7DL,0L,0xD99F85ADL,0x1BE29D33L,0xC9FF1E84L},{1L,5L,(-1L),0x9641C11CL,0xE0FDFBF5L},{0x1BE29D33L,(-1L),0xD99F85ADL,0xC9FF1E84L,0xD99F85ADL},{9L,9L,0x1F9B467DL,0x7A0144DFL,0L},{0x7A22D02CL,0xDFE70636L,0xF80D90F1L,0L,0L},{0xEFA37957L,1L,9L,7L,0x405B2C34L},{1L,0xDFE70636L,0xF190E59DL,0xF190E59DL,0xDFE70636L}}};
        int i, j, k;
        g_201 |= (*l_95);
        (*l_257) = (l_256 = g_254);
        for (g_195 = 0; (g_195 <= 6); g_195 += 1)
        {
            uint64_t *l_263 = &g_161;
            uint32_t ** const **l_288 = (void*)0;
            uint32_t ** const **l_289 = &l_286;
            int32_t l_293 = (-10L);
            int8_t *l_294[3];
            uint16_t l_339 = 0xE576L;
            int64_t l_365 = (-3L);
            int32_t **l_394 = &g_255;
            int16_t ****l_468[9][4][6] = {{{(void*)0,&g_308,&g_308,&g_308,&l_305,(void*)0},{&g_308,(void*)0,&g_308,&g_308,&g_308,&g_308},{(void*)0,&g_308,(void*)0,(void*)0,(void*)0,&g_308},{(void*)0,(void*)0,&g_308,(void*)0,&g_308,(void*)0}},{{&g_308,&g_308,(void*)0,&g_308,&g_308,&l_305},{&g_308,&g_308,&g_308,(void*)0,&g_308,&g_308},{(void*)0,(void*)0,&g_308,&g_308,(void*)0,(void*)0},{(void*)0,&g_308,&g_308,&g_308,&g_308,(void*)0}},{{&l_305,(void*)0,(void*)0,&g_308,&l_305,&g_308},{&l_305,&g_308,&g_308,&g_308,&g_308,&l_305},{(void*)0,&g_308,&g_308,&g_308,(void*)0,(void*)0},{(void*)0,&g_308,&l_305,(void*)0,&l_305,&g_308}},{{&g_308,(void*)0,&l_305,&g_308,(void*)0,&g_308},{&g_308,&g_308,&l_305,(void*)0,(void*)0,(void*)0},{(void*)0,&g_308,&g_308,(void*)0,&g_308,&l_305},{(void*)0,&l_305,&g_308,&g_308,&g_308,&g_308}},{{&g_308,&l_305,(void*)0,&g_308,&g_308,(void*)0},{(void*)0,&l_305,&g_308,(void*)0,&g_308,(void*)0},{&g_308,(void*)0,&g_308,&g_308,(void*)0,&g_308},{&g_308,&l_305,(void*)0,&l_305,&g_308,(void*)0}},{{&l_305,(void*)0,&g_308,&l_305,(void*)0,&g_308},{&g_308,(void*)0,&g_308,&g_308,&g_308,&g_308},{&g_308,&g_308,&l_305,&g_308,(void*)0,&l_305},{&g_308,(void*)0,(void*)0,(void*)0,&g_308,&l_305}},{{(void*)0,&g_308,(void*)0,&g_308,&g_308,&l_305},{&g_308,&g_308,&l_305,&g_308,&l_305,&g_308},{&g_308,&l_305,&g_308,&g_308,&l_305,&g_308},{&g_308,(void*)0,&g_308,(void*)0,(void*)0,(void*)0}},{{(void*)0,(void*)0,(void*)0,&g_308,&l_305,&g_308},{&g_308,&l_305,&g_308,&g_308,&l_305,&g_308},{&g_308,&g_308,(void*)0,&g_308,&g_308,&g_308},{&l_305,&g_308,(void*)0,&l_305,&g_308,(void*)0}},{{&l_305,(void*)0,&l_305,&g_308,(void*)0,&g_308},{&g_308,&g_308,(void*)0,&g_308,&g_308,(void*)0},{&g_308,(void*)0,(void*)0,&g_308,(void*)0,&g_308},{(void*)0,(void*)0,&l_305,(void*)0,&g_308,&g_308}}};
            int32_t l_536 = 0xB34C7463L;
            int32_t l_539 = 0x631EA953L;
            int32_t l_543 = 0L;
            int32_t l_556 = 0L;
            int16_t l_557 = 0x04C7L;
            int32_t l_558 = 0x94FDDD6BL;
            int32_t l_562 = 8L;
            int32_t l_566 = 0x62BA1176L;
            int32_t l_568[8][10][3] = {{{0x714A1D24L,0x714A1D24L,(-1L)},{1L,0x6275EE18L,0x6275EE18L},{(-1L),0x46150AF1L,0xBBD3EDEFL},{1L,0x579E50E8L,1L},{0x714A1D24L,(-1L),0xBBD3EDEFL},{0xD98F4494L,0xD98F4494L,0x6275EE18L},{1L,(-1L),(-1L)},{0x6275EE18L,0x579E50E8L,9L},{1L,0x46150AF1L,1L},{0xD98F4494L,0x6275EE18L,9L}},{{0x714A1D24L,0x714A1D24L,(-1L)},{1L,0x6275EE18L,0x6275EE18L},{(-1L),0x46150AF1L,0xBBD3EDEFL},{1L,0x579E50E8L,1L},{0x714A1D24L,(-1L),0xBBD3EDEFL},{0xD98F4494L,0xD98F4494L,0x6275EE18L},{1L,(-1L),(-1L)},{0x6275EE18L,0x579E50E8L,9L},{1L,0x46150AF1L,1L},{0xD98F4494L,0x6275EE18L,9L}},{{0x714A1D24L,0x714A1D24L,(-1L)},{1L,0x6275EE18L,0x6275EE18L},{(-1L),0x46150AF1L,0xBBD3EDEFL},{1L,0x579E50E8L,1L},{0x714A1D24L,(-1L),0xBBD3EDEFL},{0xD98F4494L,0xD98F4494L,0x6275EE18L},{1L,(-1L),(-1L)},{0x6275EE18L,0xD98F4494L,0x579E50E8L},{0xBBD3EDEFL,0x714A1D24L,0xBBD3EDEFL},{0x6275EE18L,1L,0x579E50E8L}},{{(-1L),(-1L),1L},{9L,1L,1L},{1L,0x714A1D24L,0x46150AF1L},{9L,0xD98F4494L,9L},{(-1L),1L,0x46150AF1L},{0x6275EE18L,0x6275EE18L,1L},{0xBBD3EDEFL,1L,1L},{1L,0xD98F4494L,0x579E50E8L},{0xBBD3EDEFL,0x714A1D24L,0xBBD3EDEFL},{0x6275EE18L,1L,0x579E50E8L}},{{(-1L),(-1L),1L},{9L,1L,1L},{1L,0x714A1D24L,0x46150AF1L},{9L,0xD98F4494L,9L},{(-1L),1L,0x46150AF1L},{0x6275EE18L,0x6275EE18L,1L},{0xBBD3EDEFL,1L,1L},{1L,0xD98F4494L,0x579E50E8L},{0xBBD3EDEFL,0x714A1D24L,0xBBD3EDEFL},{0x6275EE18L,1L,0x579E50E8L}},{{(-1L),(-1L),1L},{9L,1L,1L},{1L,0x714A1D24L,0x46150AF1L},{9L,0xD98F4494L,9L},{(-1L),1L,0x46150AF1L},{0x6275EE18L,0x6275EE18L,1L},{0xBBD3EDEFL,1L,1L},{1L,0xD98F4494L,0x579E50E8L},{0xBBD3EDEFL,0x714A1D24L,0xBBD3EDEFL},{0x6275EE18L,1L,0x579E50E8L}},{{(-1L),(-1L),1L},{9L,1L,1L},{1L,0x714A1D24L,0x46150AF1L},{9L,0xD98F4494L,9L},{(-1L),1L,0x46150AF1L},{0x6275EE18L,0x6275EE18L,1L},{0xBBD3EDEFL,1L,1L},{1L,0xD98F4494L,0x579E50E8L},{0xBBD3EDEFL,0x714A1D24L,0xBBD3EDEFL},{0x6275EE18L,1L,0x579E50E8L}},{{(-1L),(-1L),1L},{9L,1L,1L},{1L,0x714A1D24L,0x46150AF1L},{9L,0xD98F4494L,9L},{(-1L),1L,0x46150AF1L},{0x6275EE18L,0x6275EE18L,1L},{0xBBD3EDEFL,1L,1L},{1L,0xD98F4494L,0x579E50E8L},{0xBBD3EDEFL,0x714A1D24L,0xBBD3EDEFL},{0x6275EE18L,1L,0x579E50E8L}}};
            int i, j, k;
            for (i = 0; i < 3; i++)
                l_294[i] = &l_190;
        }
    }
    else
    {
        int32_t *l_575 = &g_201;
        return l_575;
    }
    return (***g_497);
}







static int64_t func_68(const uint64_t p_69, uint32_t p_70, int32_t p_71)
{
    int32_t *l_76 = &g_22;
    int32_t **l_75 = &l_76;
    int64_t l_77 = 0L;
    (*l_75) = (void*)0;
    return l_77;
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
    transparent_crc(g_9, "g_9", print_hash_value);
    transparent_crc(g_12, "g_12", print_hash_value);
    transparent_crc(g_22, "g_22", print_hash_value);
    transparent_crc(g_61, "g_61", print_hash_value);
    transparent_crc(g_97, "g_97", print_hash_value);
    transparent_crc(g_102, "g_102", print_hash_value);
    for (i = 0; i < 3; i++)
    {
        transparent_crc(g_109[i], "g_109[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_125, "g_125", print_hash_value);
    transparent_crc(g_127, "g_127", print_hash_value);
    transparent_crc(g_131, "g_131", print_hash_value);
    transparent_crc(g_150, "g_150", print_hash_value);
    transparent_crc(g_161, "g_161", print_hash_value);
    transparent_crc(g_193, "g_193", print_hash_value);
    transparent_crc(g_195, "g_195", print_hash_value);
    transparent_crc(g_200, "g_200", print_hash_value);
    transparent_crc(g_201, "g_201", print_hash_value);
    transparent_crc(g_202, "g_202", print_hash_value);
    transparent_crc(g_295, "g_295", print_hash_value);
    transparent_crc(g_299, "g_299", print_hash_value);
    transparent_crc(g_300, "g_300", print_hash_value);
    transparent_crc(g_315, "g_315", print_hash_value);
    transparent_crc(g_316, "g_316", print_hash_value);
    transparent_crc(g_319, "g_319", print_hash_value);
    transparent_crc(g_320, "g_320", print_hash_value);
    transparent_crc(g_387, "g_387", print_hash_value);
    transparent_crc(g_389, "g_389", print_hash_value);
    transparent_crc(g_429, "g_429", print_hash_value);
    transparent_crc(g_552, "g_552", print_hash_value);
    transparent_crc(g_887, "g_887", print_hash_value);
    for (i = 0; i < 7; i++)
    {
        transparent_crc(g_984[i], "g_984[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_1222, "g_1222", print_hash_value);
    transparent_crc(g_1365, "g_1365", print_hash_value);
    transparent_crc(g_1449, "g_1449", print_hash_value);
    transparent_crc(g_1581, "g_1581", print_hash_value);
    transparent_crc(g_1650, "g_1650", print_hash_value);
    transparent_crc(g_1803, "g_1803", print_hash_value);
    transparent_crc(g_1909, "g_1909", print_hash_value);
    transparent_crc(g_2080, "g_2080", print_hash_value);
    transparent_crc(g_2103, "g_2103", print_hash_value);
    for (i = 0; i < 5; i++)
    {
        transparent_crc(g_2221[i], "g_2221[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 9; j++)
        {
            transparent_crc(g_2284[i][j], "g_2284[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    for (i = 0; i < 8; i++)
    {
        transparent_crc(g_2344[i], "g_2344[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    for (i = 0; i < 9; i++)
    {
        transparent_crc(g_2403[i], "g_2403[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_2520, "g_2520", print_hash_value);
    transparent_crc(g_2826, "g_2826", print_hash_value);
    transparent_crc(g_2960, "g_2960", print_hash_value);
    transparent_crc(g_3000, "g_3000", print_hash_value);
    transparent_crc(g_3171, "g_3171", print_hash_value);
    for (i = 0; i < 8; i++)
    {
        transparent_crc(g_3261[i], "g_3261[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_3365, "g_3365", print_hash_value);
    transparent_crc(g_3415, "g_3415", print_hash_value);
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 8; j++)
        {
            for (k = 0; k < 2; k++)
            {
                transparent_crc(g_3443[i][j][k], "g_3443[i][j][k]", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    if ((crc32_context ^ 0xFFFFFFFFUL) != 0xB75F145AUL) {
        printf("FAIL: got checksum = %X, expected B75F145A\n", crc32_context ^ 0xFFFFFFFFUL);
        exit(1);
    }
    platform_main_end(crc32_context ^ 0xFFFFFFFFUL, print_hash_value);
    return 0;
}
