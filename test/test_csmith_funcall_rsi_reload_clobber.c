/* csmith 1105826-reduced_refmismatch_O0_413: rcc -O0 segfaulted deep inside
 * func_31's condition, which calls itself with 5 arguments where the 2nd
 * (a pointer, ABI register %rsi) is computed through 3 levels of nested
 * calls: func_31(l_17, func_37(func_39(func_45(...))), l_17, (*g_499),
 * g_2759).
 *
 * gen_funcall's argument-staging path (active whenever register pressure
 * -- live_now + arg count + per-arg worst-case pressure -- meets the 8
 * allocatable x86-64 registers) evaluates each argument in turn, stages
 * it to a private stack slot, then reloads all staged values into their
 * SysV ABI registers (%rdi/%rsi/%rdx/%rcx/%r8/%r9) in a single final
 * pass. The reload for each argument gets its scratch register from the
 * SAME general allocator (`alloc_reg()`) used everywhere else -- and
 * %rsi (reg64[7]) is BOTH the 8th allocatable scratch register AND the
 * 2nd argument's real ABI destination. Once the 2nd argument's staged
 * value is reloaded into %rsi (its own correct final home, so no
 * separate move is needed), nothing stopped a LATER argument's reload
 * from also being handed %rsi as ITS OWN scratch register -- silently
 * overwriting the 2nd argument's already-placed pointer with a later
 * argument's raw integer value before the call ever executed. The 2nd
 * argument arrived as garbage (a tiny integer reinterpreted as a
 * pointer), and dereferencing it segfaulted.
 *
 * Fixed in gen_funcall's staging placement loop (codegen.c): once an
 * argument claims %rsi as part of its ABI destination (gp_idx 0 or 1),
 * `alloc_reg_avoid2` excludes VReg 7 from every subsequent argument's
 * reload for the rest of this call's placement.
 *
 * Manual minimization lost the exact register-pressure trigger (like
 * the other test_csmith_*_spill.c cases), so this keeps the full
 * reduced csmith source with the checksum printout replaced by an
 * assertion against the gcc -O2 reference checksum.
 */
# 0 "test/csmith/136073-0.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "test/csmith/136073-0.c"
# 10 "test/csmith/136073-0.c"
# 1 "/usr/local/include/csmith.h" 1 3
# 41 "/usr/local/include/csmith.h" 2 3
# 1 "/usr/include/math.h" 1 3 4
# 27 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4
# 33 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 431 "/usr/include/features.h" 3 4
# 1 "/usr/include/features-time64.h" 1 3 4
# 732 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 540 "/usr/include/features.h" 2 3 4
# 563 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4
# 564 "/usr/include/features.h" 2 3 4
# 34 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 2 3 4
# 28 "/usr/include/math.h" 2 3 4









# 1 "/usr/include/x86_64-linux-gnu/bits/math-vector.h" 1 3 4
# 25 "/usr/include/x86_64-linux-gnu/bits/math-vector.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libm-simd-decl-stubs.h" 1 3 4
# 26 "/usr/include/x86_64-linux-gnu/bits/math-vector.h" 2 3 4
# 38 "/usr/include/math.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/floatn.h" 1 3 4
# 131 "/usr/include/x86_64-linux-gnu/bits/floatn.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/floatn-common.h" 1 3 4
# 41 "/usr/include/math.h" 2 3 4
# 157 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/flt-eval-method.h" 1 3 4
# 158 "/usr/include/math.h" 2 3 4
# 170 "/usr/include/math.h" 3 4

# 170 "/usr/include/math.h" 3 4
typedef float float_t;
typedef double double_t;
# 376 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/fp-logb.h" 1 3 4



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
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 1 3 4
# 53 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 107 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 231 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern int isinf (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));




extern int finite (double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern double drem (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)); extern double __drem (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));



extern double significand (double __x) __attribute__ ((__nothrow__ , __leaf__)); extern double __significand (double __x) __attribute__ ((__nothrow__ , __leaf__));






extern double copysign (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern double nan (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern double __nan (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
# 267 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 435 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern double fmaximum (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmaximum_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmaximum_mag (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum_mag (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fmaximum_mag_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern double fminimum_mag_num (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 485 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern double scalb (double __x, double __n) __attribute__ ((__nothrow__ , __leaf__)); extern double __scalb (double __x, double __n) __attribute__ ((__nothrow__ , __leaf__));
# 451 "/usr/include/math.h" 2 3 4
# 466 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h" 1 3 4
# 20 "/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h" 3 4
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
# 467 "/usr/include/math.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 1 3 4
# 53 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 107 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 231 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern int isinff (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));




extern int finitef (float __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern float dremf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)); extern float __dremf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__));



extern float significandf (float __x) __attribute__ ((__nothrow__ , __leaf__)); extern float __significandf (float __x) __attribute__ ((__nothrow__ , __leaf__));






extern float copysignf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern float nanf (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern float __nanf (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
# 267 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 435 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern float fmaximumf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimumf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmaximum_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimum_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmaximum_magf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimum_magf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fmaximum_mag_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern float fminimum_mag_numf (float __x, float __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 485 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern float scalbf (float __x, float __n) __attribute__ ((__nothrow__ , __leaf__)); extern float __scalbf (float __x, float __n) __attribute__ ((__nothrow__ , __leaf__));
# 468 "/usr/include/math.h" 2 3 4
# 535 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h" 1 3 4
# 20 "/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h" 3 4
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
# 536 "/usr/include/math.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 1 3 4
# 53 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 107 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 231 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern int isinfl (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));




extern int finitel (long double __value) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__const__));


extern long double dreml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)); extern long double __dreml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));



extern long double significandl (long double __x) __attribute__ ((__nothrow__ , __leaf__)); extern long double __significandl (long double __x) __attribute__ ((__nothrow__ , __leaf__));






extern long double copysignl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern long double nanl (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__)); extern long double __nanl (const char *__tagb) __attribute__ ((__nothrow__ , __leaf__));
# 267 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
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
# 435 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern long double fmaximuml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimuml (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmaximum_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimum_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmaximum_magl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimum_magl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fmaximum_mag_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));


extern long double fminimum_mag_numl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 485 "/usr/include/x86_64-linux-gnu/bits/mathcalls.h" 3 4
extern long double scalbl (long double __x, long double __n) __attribute__ ((__nothrow__ , __leaf__)); extern long double __scalbl (long double __x, long double __n) __attribute__ ((__nothrow__ , __leaf__));
# 537 "/usr/include/math.h" 2 3 4
# 618 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h" 1 3 4
# 20 "/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h" 3 4
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
# 619 "/usr/include/math.h" 2 3 4
# 703 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls-narrow.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/mathcalls-narrow.h" 3 4
extern float fadd (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fdiv (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float ffma (double __x, double __y, double __z) __attribute__ ((__nothrow__ , __leaf__));


extern float fmul (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fsqrt (double __x) __attribute__ ((__nothrow__ , __leaf__));


extern float fsub (double __x, double __y) __attribute__ ((__nothrow__ , __leaf__));
# 704 "/usr/include/math.h" 2 3 4
# 724 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/mathcalls-narrow.h" 1 3 4
extern float faddl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fdivl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float ffmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__));


extern float fmull (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern float fsqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern float fsubl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
# 24 "/usr/include/x86_64-linux-gnu/bits/mathcalls-narrow.h" 3 4
extern double daddl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double ddivl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double dfmal (long double __x, long double __y, long double __z) __attribute__ ((__nothrow__ , __leaf__));


extern double dmull (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));


extern double dsqrtl (long double __x) __attribute__ ((__nothrow__ , __leaf__));


extern double dsubl (long double __x, long double __y) __attribute__ ((__nothrow__ , __leaf__));
# 754 "/usr/include/math.h" 2 3 4
# 991 "/usr/include/math.h" 3 4
extern int signgam;
# 1071 "/usr/include/math.h" 3 4
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
# 1192 "/usr/include/math.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/iscanonical.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/iscanonical.h" 3 4
extern int __iscanonicall (long double __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 1193 "/usr/include/math.h" 2 3 4
# 1609 "/usr/include/math.h" 3 4

# 42 "/usr/local/include/csmith.h" 2 3
# 1 "/usr/include/string.h" 1 3 4
# 26 "/usr/include/string.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/string.h" 2 3 4


# 37 "/usr/include/string.h" 3 4
# 1 "/usr/lib/gcc/x86_64-linux-gnu/15/include/stddef.h" 1 3 4
# 229 "/usr/lib/gcc/x86_64-linux-gnu/15/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 38 "/usr/include/string.h" 2 3 4
# 47 "/usr/include/string.h" 3 4
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
# 91 "/usr/include/string.h" 3 4
extern int __memcmpeq (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 118 "/usr/include/string.h" 3 4
extern void *memchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 156 "/usr/include/string.h" 3 4
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



# 1 "/usr/include/x86_64-linux-gnu/bits/types/locale_t.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/bits/types/locale_t.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/__locale_t.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/types/__locale_t.h" 3 4
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/x86_64-linux-gnu/bits/types/locale_t.h" 2 3 4

typedef __locale_t locale_t;
# 188 "/usr/include/string.h" 2 3 4


extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));


extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
    locale_t __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)))
     __attribute__ ((__access__ (__write_only__, 1, 3)));





extern char *strdup (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));






extern char *strndup (const char *__string, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
# 261 "/usr/include/string.h" 3 4
extern char *strchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 292 "/usr/include/string.h" 3 4
extern char *strrchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 309 "/usr/include/string.h" 3 4
extern char *strchrnul (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));





extern size_t strcspn (const char *__s, const char *__reject)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 346 "/usr/include/string.h" 3 4
extern char *strpbrk (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 377 "/usr/include/string.h" 3 4
extern char *strstr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 388 "/usr/include/string.h" 3 4
extern char *strtok (char *__restrict __s, const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *__strtok_r (char *__restrict __s,
    const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));

extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
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
extern int strerror_r (int __errnum, char *__buf, size_t __buflen) __asm__ ("" "__xpg_strerror_r") __attribute__ ((__nothrow__ , __leaf__))

                        __attribute__ ((__nonnull__ (2)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
extern char *strerror_l (int __errnum, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));













extern int bcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bcopy (const void *__src, void *__dest, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern char *index (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
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






extern void explicit_bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));



extern char *strsep (char **__restrict __stringp,
       const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strsignal (int __sig) __attribute__ ((__nothrow__ , __leaf__));
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









 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
# 1 "/usr/lib/gcc/x86_64-linux-gnu/15/include/limits.h" 1 3 4
# 210 "/usr/lib/gcc/x86_64-linux-gnu/15/include/limits.h" 3 4
# 1 "/usr/include/limits.h" 1 3 4
# 26 "/usr/include/limits.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/limits.h" 2 3 4
# 198 "/usr/include/limits.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 2 3 4
# 161 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 1 3 4
# 38 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 39 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 2 3 4
# 81 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/pthread_stack_min-dynamic.h" 1 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/pthread_stack_min-dynamic.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/pthread_stack_min.h" 1 3 4
# 30 "/usr/include/x86_64-linux-gnu/bits/pthread_stack_min-dynamic.h" 2 3 4
# 82 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 2 3 4



# 211 "/usr/lib/gcc/x86_64-linux-gnu/15/include/limits.h" 2 3 4
# 10 "/usr/lib/gcc/x86_64-linux-gnu/15/include/syslimits.h" 2 3 4
#pragma GCC diagnostic pop
# 35 "/usr/lib/gcc/x86_64-linux-gnu/15/include/limits.h" 2 3 4
# 51 "/usr/local/include/random_inc.h" 2 3



# 1 "/usr/lib/gcc/x86_64-linux-gnu/15/include/stdint.h" 1 3 4
# 9 "/usr/lib/gcc/x86_64-linux-gnu/15/include/stdint.h" 3 4
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
# 1 "/usr/include/stdint.h" 1 3 4
# 26 "/usr/include/stdint.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/stdint.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4


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
# 24 "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h" 3 4
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 39 "/usr/include/stdint.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/stdint-uintn.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/stdint-uintn.h" 3 4
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;



typedef __int_least8_t int_least8_t;
typedef __int_least16_t int_least16_t;
typedef __int_least32_t int_least32_t;
typedef __int_least64_t int_least64_t;


typedef __uint_least8_t uint_least8_t;
typedef __uint_least16_t uint_least16_t;
typedef __uint_least32_t uint_least32_t;
typedef __uint_least64_t uint_least64_t;





typedef signed char int_fast8_t;

typedef long int int_fast16_t;
typedef long int int_fast32_t;
typedef long int int_fast64_t;
# 64 "/usr/include/stdint.h" 3 4
typedef unsigned char uint_fast8_t;

typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
typedef unsigned long int uint_fast64_t;
# 80 "/usr/include/stdint.h" 3 4
typedef long int intptr_t;


typedef unsigned long int uintptr_t;
# 94 "/usr/include/stdint.h" 3 4
typedef __intmax_t intmax_t;
typedef __uintmax_t uintmax_t;
# 12 "/usr/lib/gcc/x86_64-linux-gnu/15/include/stdint.h" 2 3 4
#pragma GCC diagnostic pop
# 55 "/usr/local/include/random_inc.h" 2 3



# 1 "/usr/include/assert.h" 1 3 4



extern void __assert_fail (const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));


extern void __assert_perror_fail (int __errnum, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));




extern void __assert (const char *__assertion, const char *__file, int __line)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));






extern _Bool __assert_single_arg (_Bool);



# 88 "/usr/local/include/random_inc.h" 3
# 1 "/usr/local/include/platform_generic.h" 1 3
# 39 "/usr/local/include/platform_generic.h" 3
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h" 1 3 4











typedef __builtin_va_list __gnuc_va_list;


# 1 "/usr/include/x86_64-linux-gnu/bits/types/__fpos_t.h" 1 3 4




# 1 "/usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h" 1 3 4
# 13 "/usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h" 3 4
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 6 "/usr/include/x86_64-linux-gnu/bits/types/__fpos_t.h" 2 3 4




typedef struct _G_fpos_t
{
  __off_t __pos;
  __mbstate_t __state;
} __fpos_t;
# 45 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/__fpos64_t.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/bits/types/__fpos64_t.h" 3 4
typedef struct _G_fpos64_t
{
  __off64_t __pos;
  __mbstate_t __state;
} __fpos64_t;
# 46 "/usr/include/stdio.h" 2 3 4



struct _IO_FILE;
typedef struct _IO_FILE __FILE;



struct _IO_FILE;


typedef struct _IO_FILE FILE;
# 1 "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h" 1 3 4
# 35 "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 36 "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h" 2 3 4

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





typedef __gnuc_va_list va_list;
# 68 "/usr/include/stdio.h" 3 4
typedef __off_t off_t;
# 82 "/usr/include/stdio.h" 3 4
typedef __ssize_t ssize_t;






typedef __fpos_t fpos_t;
# 133 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdio_lim.h" 1 3 4
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;






extern int remove (const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (const char *__old, const char *__new) __attribute__ ((__nothrow__ , __leaf__));



extern int renameat (int __oldfd, const char *__old, int __newfd,
       const char *__new) __attribute__ ((__nothrow__ , __leaf__));
# 191 "/usr/include/stdio.h" 3 4
extern int fclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 201 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile (void)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 218 "/usr/include/stdio.h" 3 4
extern char *tmpnam (char[20]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern char *tmpnam_r (char __s[20]) __attribute__ ((__nothrow__ , __leaf__)) ;
# 235 "/usr/include/stdio.h" 3 4
extern char *tempnam (const char *__dir, const char *__pfx)
   __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (__builtin_free, 1)));






extern int fflush (FILE *__stream);
# 252 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 271 "/usr/include/stdio.h" 3 4
extern FILE *fopen (const char *__restrict __filename,
      const char *__restrict __modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *freopen (const char *__restrict __filename,
        const char *__restrict __modes,
        FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));
# 306 "/usr/include/stdio.h" 3 4
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
# 341 "/usr/include/stdio.h" 3 4
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
extern void exit(int status);

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
extern int fscanf (FILE *__restrict __stream, const char *__restrict __format, ...) __asm__ ("" "__isoc23_fscanf")

                                __attribute__ ((__nonnull__ (1)));
extern int scanf (const char *__restrict __format, ...) __asm__ ("" "__isoc23_scanf")
                              ;
extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...) __asm__ ("" "__isoc23_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
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
# 582 "/usr/include/stdio.h" 3 4
extern int fgetc (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getc (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern int getchar (void);






extern int getc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getchar_unlocked (void);
# 607 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));







extern int fputc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));





extern int putchar (int __c);
# 631 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern int putc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream) __attribute__ ((__nonnull__ (1)));


extern int putw (int __w, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 693 "/usr/include/stdio.h" 3 4
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
# 760 "/usr/include/stdio.h" 3 4
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
# 797 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off_t __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern __off_t ftello (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 823 "/usr/include/stdio.h" 3 4
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos)
  __attribute__ ((__nonnull__ (1)));




extern int fsetpos (FILE *__stream, const fpos_t *__pos) __attribute__ ((__nonnull__ (1)));
# 854 "/usr/include/stdio.h" 3 4
extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern void perror (const char *__s) __attribute__ ((__cold__));




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int pclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern FILE *popen (const char *__command, const char *__modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (pclose, 1))) ;






extern char *ctermid (char *__s) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1)));
extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 953 "/usr/include/stdio.h" 3 4
extern int __uflow (FILE *);
extern int __overflow (FILE *, int);
# 977 "/usr/include/stdio.h" 3 4



static void platform_main_begin(void) { }

static void platform_main_end(uint32_t crc, int flag) {
  uint32_t expected = 0x7374B5C3;
  if (crc != expected) {
      printf("FAIL: got checksum = %X, expected %X\n", crc, expected);
    exit(1);
  }
  printf("checksum = %X\n", crc);
}
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
static int32_t
(safe_convert_func_float_to_int32_t)(float sf1 )
{
 
  return

    ((sf1 <= (-2147483647-1)) || (sf1 >= (2147483647))) ?
    ((2147483647)) :

    ((int32_t)(sf1));
}

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



static long __undefined;


struct S0 {
   const volatile signed f0 : 23;
   volatile signed f1 : 16;
   const unsigned f2 : 18;
   unsigned f3 : 22;
   volatile signed f4 : 13;
   unsigned f5 : 16;
   signed f6 : 14;
   volatile signed f7 : 11;
};

struct S1 {
   signed f0 : 18;
   unsigned f1 : 25;
   signed f2 : 16;
   signed f3 : 8;
   signed : 0;
   signed f4 : 23;
};

struct S2 {
   unsigned f0 : 21;
   unsigned f1 : 22;
   signed f2 : 16;
   unsigned f3 : 18;
   volatile int32_t f4;
   volatile uint64_t f5;
};

union U3 {
   int8_t * f0;
};


static volatile uint32_t g_13 = 0x9C2D621EL;
static uint8_t g_16 = 0xAFL;
static union U3 g_49 = {0};
static int8_t g_53 = 5L;
static int8_t *g_52 = &g_53;
static int8_t g_58 = (-9L);
static uint32_t g_63 = 7UL;
static int32_t g_82 = 0x6DCEBFDCL;
static uint8_t g_85[5][8][6] = {{{0xB6L,0x58L,0UL,2UL,7UL,0xACL},{0x38L,0UL,0x17L,0UL,0x38L,8UL},{0x66L,0x38L,0x38L,0x80L,0xD1L,255UL},{7UL,0x38L,0x96L,0x38L,1UL,255UL},{0UL,0xCFL,0x38L,0xACL,0x96L,8UL},{1UL,0xD8L,0x17L,0x66L,0xACL,0xACL},{255UL,0UL,0UL,255UL,8UL,0x96L},{0UL,254UL,8UL,0xAAL,0xACL,0xD8L}},{{0x17L,0x58L,255UL,254UL,0xACL,0x7BL},{0x7BL,0x38L,0x88L,9UL,0UL,0x66L},{255UL,7UL,0xB6L,0xCFL,0x96L,0xCFL},{0x38L,2UL,0x38L,255UL,254UL,1UL},{0UL,9UL,0x96L,7UL,0xD8L,0xB6L},{1UL,0x66L,8UL,7UL,0x7BL,255UL},{0UL,0xD1L,0x58L,255UL,0x66L,0xAAL},{0x38L,255UL,0x6CL,0xCFL,0xCFL,0x6CL}},{{255UL,255UL,0x38L,9UL,1UL,255UL},{0x7BL,0x6CL,249UL,254UL,0xB6L,0x38L},{0x17L,0x7BL,249UL,8UL,255UL,255UL},{7UL,8UL,0x38L,249UL,0xAAL,0x6CL},{249UL,0xAAL,0x6CL,0x88L,0x6CL,0xAAL},{0xD8L,254UL,0x58L,0x96L,255UL,255UL},{255UL,0x17L,8UL,0xD1L,0x38L,0xB6L},{0xCFL,0x17L,0x96L,0UL,255UL,1UL}},{{0x88L,254UL,0x38L,255UL,0x6CL,0xCFL},{0x66L,0xAAL,0xB6L,0xB6L,0xAAL,0x66L},{0x80L,8UL,0x88L,0xACL,255UL,0x7BL},{0x38L,0x7BL,255UL,255UL,0xB6L,0xD8L},{0x38L,0x6CL,255UL,0xACL,1UL,254UL},{0x80L,255UL,7UL,0xB6L,0xCFL,0x96L},{0x66L,255UL,0xAAL,255UL,0x66L,0UL},{0x88L,0xD1L,0x66L,0UL,0x7BL,0xACL}},{{0xCFL,0x66L,254UL,0xD1L,0xD8L,0xACL},{255UL,9UL,0x66L,0x96L,254UL,0UL},{0xD8L,2UL,0xAAL,0x88L,0x96L,0x96L},{249UL,7UL,7UL,249UL,0UL,254UL},{7UL,0x38L,255UL,8UL,0xACL,0xD8L},{0x17L,0x58L,255UL,254UL,0xACL,0x7BL},{0x7BL,0x38L,0x88L,9UL,0UL,0x66L},{255UL,7UL,0xB6L,9UL,254UL,9UL}}};
static volatile struct S0 g_98 = {-1638,-62,502,468,-10,38,71,-14};
static volatile struct S0 *g_97 = &g_98;
static int32_t *g_100 = &g_82;
static uint16_t g_107 = 0x6369L;
static int16_t g_109[10][9] = {{0x363FL,1L,1L,0x363FL,0x363FL,1L,1L,0x363FL,0x363FL},{0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L},{0x363FL,0x363FL,1L,1L,0x363FL,0x363FL,1L,1L,0x363FL},{8L,0xDD6FL,8L,0xDD6FL,8L,0xDD6FL,8L,0xDD6FL,8L},{0x363FL,1L,1L,0x363FL,0x363FL,1L,1L,0x363FL,0x363FL},{0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L},{0x363FL,0x363FL,1L,1L,0x363FL,0x363FL,1L,1L,0x363FL},{8L,0xDD6FL,8L,0xDD6FL,8L,0xDD6FL,8L,0xDD6FL,8L},{0x363FL,1L,1L,0x363FL,0x363FL,1L,1L,0x363FL,0x363FL},{0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L,0xDD6FL,0x1705L}};
static struct S1 g_114 = {-194,5466,99,-2,-413};
static int32_t g_117 = 0xC4065A8CL;
static uint32_t g_121 = 4294967290UL;
static struct S0 g_134 = {1782,-210,189,669,-24,57,-66,-4};
static struct S0 g_154 = {1621,-35,135,1294,56,96,-103,-10};
static const struct S0 g_155 = {-2738,114,378,289,-69,250,-90,13};
static uint32_t g_177 = 0x661776EDL;
static uint64_t g_188 = 4UL;
static int64_t g_236[3][2] = {{0x64BD90867E643A7ALL,0x9747AEA97A0ADC2BLL},{0x9747AEA97A0ADC2BLL,0x64BD90867E643A7ALL},{0x9747AEA97A0ADC2BLL,0x9747AEA97A0ADC2BLL}};
static uint32_t g_239 = 1UL;
static int16_t *g_250 = &g_109[7][8];
static int16_t **g_249 = &g_250;
static int16_t ***g_248 = &g_249;
static struct S1 *g_258 = &g_114;
static int32_t g_266[5][8] = {{1L,0L,0xD286BB95L,6L,6L,0xD286BB95L,0L,1L},{1L,0L,0xD286BB95L,6L,6L,0xD286BB95L,0L,1L},{1L,0L,0xD286BB95L,6L,6L,0xD286BB95L,0L,1L},{1L,0L,0xD286BB95L,6L,6L,0xD286BB95L,0L,1L},{1L,0L,0xD286BB95L,6L,6L,0xD286BB95L,0L,1L}};
static const int32_t g_291[4][2][10] = {{{0x9EF02342L,7L,0x6C76AD62L,0x392D92FFL,0x392D92FFL,0x6C76AD62L,7L,0x9EF02342L,(-4L),3L},{(-1L),0x392D92FFL,7L,(-1L),0x43C9B887L,0x88A92D1EL,0x43C9B887L,(-1L),7L,0x392D92FFL}},{{0x6C76AD62L,(-3L),7L,0x43C9B887L,(-9L),(-1L),0x9EF02342L,0x9EF02342L,(-1L),(-9L)},{3L,0x6C76AD62L,0x6C76AD62L,3L,(-5L),(-1L),(-1L),7L,(-9L),7L}},{{0x6C76AD62L,0x88A92D1EL,(-4L),7L,(-4L),0x88A92D1EL,0x6C76AD62L,(-1L),(-9L),(-3L)},{(-1L),(-3L),0x88A92D1EL,7L,3L,3L,7L,0x88A92D1EL,(-3L),0x43C9B887L}},{{(-1L),(-3L),(-4L),0x392D92FFL,(-5L),(-9L),3L,(-9L),(-5L),0x392D92FFL},{0x392D92FFL,0x9EF02342L,0x392D92FFL,(-3L),(-5L),0x6C76AD62L,0x43C9B887L,(-1L),(-1L),0x43C9B887L}}};
static struct S0 g_322 = {31,-191,81,874,39,93,-115,-32};
static const int32_t g_378 = (-4L);
static const int32_t g_382 = 5L;
static const int32_t *g_383 = (void*)0;
static struct S0 g_396 = {-1730,248,327,1422,-59,255,20,-41};
static const uint32_t g_430 = 0x6F6A9006L;
static union U3 *g_458 = &g_49;
static uint32_t g_469[6] = {18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL};
static uint8_t *g_499 = &g_85[4][3][3];
static volatile struct S1 *g_511 = (void*)0;
static volatile struct S1 * volatile *g_510 = &g_511;
static volatile struct S1 * volatile **g_509 = &g_510;
static uint16_t * volatile g_566 = &g_107;
static uint16_t * volatile *g_565 = &g_566;
static const volatile struct S2 g_719 = {1172,1640,-241,241,0x5F8314A7L,0x77F96647E5636A90LL};
static const volatile struct S2 *g_718 = &g_719;
static uint64_t g_858 = 0x10EFDD568CE22E61LL;
static struct S1 g_892 = {-297,2281,-189,8,1115};
static uint16_t **g_933 = (void*)0;
static uint16_t ***g_932 = &g_933;
static uint32_t g_978[2][10][10] = {{{0xC7C3DF63L,0x38E6CCD9L,1UL,0x38E6CCD9L,0xC7C3DF63L,0UL,1UL,18446744073709551610UL,0x04795C6CL,0xD6B5B9DBL},{0x5FE714F1L,0UL,1UL,18446744073709551611UL,1UL,1UL,0UL,0x04795C6CL,2UL,0xD6B5B9DBL},{0UL,18446744073709551611UL,0xC9D1DDA3L,0x4BA4D34CL,0xC7C3DF63L,0x42AF3C6FL,18446744073709551610UL,1UL,1UL,18446744073709551610UL},{18446744073709551611UL,1UL,18446744073709551615UL,18446744073709551615UL,1UL,18446744073709551611UL,0UL,0UL,6UL,0xDE609416L},{6UL,1UL,18446744073709551611UL,0UL,0x38E6CCD9L,1UL,1UL,0xD6B5B9DBL,0x5FE714F1L,0x42AF3C6FL},{6UL,0x4BA4D34CL,1UL,0xD9DA2DA6L,18446744073709551611UL,18446744073709551611UL,0x42AF3C6FL,2UL,18446744073709551610UL,2UL},{18446744073709551611UL,0x42AF3C6FL,2UL,18446744073709551610UL,2UL,0x42AF3C6FL,18446744073709551611UL,18446744073709551611UL,0xD9DA2DA6L,1UL},{0UL,0xC9D1DDA3L,0x42AF3C6FL,0x5FE714F1L,0xD6B5B9DBL,1UL,1UL,0x38E6CCD9L,0UL,18446744073709551611UL},{0x5FE714F1L,0xC9D1DDA3L,0xDE609416L,6UL,0UL,0UL,18446744073709551611UL,1UL,18446744073709551615UL,18446744073709551615UL},{0xC7C3DF63L,0x42AF3C6FL,18446744073709551610UL,1UL,1UL,18446744073709551610UL,0x42AF3C6FL,0xC7C3DF63L,0x4BA4D34CL,0xC9D1DDA3L}},{{0xDE609416L,0x4BA4D34CL,0xD6B5B9DBL,2UL,0x04795C6CL,0UL,1UL,1UL,18446744073709551611UL,1UL},{0xD9DA2DA6L,1UL,0xD6B5B9DBL,0x04795C6CL,18446744073709551610UL,1UL,0UL,0xC7C3DF63L,0x38E6CCD9L,1UL},{0x7F6C8433L,1UL,18446744073709551610UL,0xC9D1DDA3L,1UL,0xC9D1DDA3L,18446744073709551610UL,1UL,0x7F6C8433L,1UL},{1UL,18446744073709551611UL,0xDE609416L,1UL,18446744073709551608UL,0xC7C3DF63L,0UL,0x38E6CCD9L,0xD6B5B9DBL,0x7F6C8433L},{0x38E6CCD9L,0UL,0x42AF3C6FL,1UL,18446744073709551611UL,18446744073709551608UL,1UL,18446744073709551611UL,0x7F6C8433L,0xC7C3DF63L},{0UL,0x38E6CCD9L,2UL,0xC9D1DDA3L,0UL,0UL,0UL,18446744073709551608UL,1UL,0x5FE714F1L},{0xC9D1DDA3L,18446744073709551608UL,18446744073709551615UL,18446744073709551611UL,18446744073709551615UL,1UL,1UL,1UL,0x38E6CCD9L,0xD9DA2DA6L},{0xB5670868L,18446744073709551610UL,0xDE609416L,18446744073709551608UL,18446744073709551615UL,1UL,0xC9D1DDA3L,1UL,18446744073709551611UL,0x5FE714F1L},{18446744073709551615UL,1UL,2UL,1UL,0xD6B5B9DBL,0UL,0xD6B5B9DBL,1UL,2UL,1UL},{1UL,0UL,0UL,18446744073709551610UL,0x38E6CCD9L,18446744073709551611UL,1UL,18446744073709551611UL,0xD6B5B9DBL,0xC7C3DF63L}}};
static int64_t g_1047 = 0L;
static volatile int32_t g_1094 = 0xCED4261FL;
static volatile int32_t *g_1093 = &g_1094;
static volatile int32_t **g_1092 = &g_1093;
static volatile int32_t ***g_1091 = &g_1092;
static volatile int32_t g_1144 = 0xDC524F05L;
static struct S0 g_1163 = {2714,-94,275,1484,-44,255,-20,3};
static struct S0 g_1164 = {1509,169,449,1676,-21,35,-33,-36};
static int32_t g_1204 = 5L;
static uint8_t g_1232[5] = {0xE5L,0xE5L,0xE5L,0xE5L,0xE5L};
static const struct S0 g_1264 = {842,-79,36,790,79,209,71,-10};
static volatile union U3 g_1301 = {0};
static const volatile union U3 *g_1300 = &g_1301;
static const volatile union U3 **g_1299 = &g_1300;
static const volatile union U3 ** volatile *g_1298[7][9] = {{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299},{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299},{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299},{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299},{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299},{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299},{&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299,&g_1299}};
static uint32_t *g_1356 = &g_121;
static uint32_t * volatile *g_1355[6][10][1] = {{{&g_1356},{&g_1356},{(void*)0},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{(void*)0},{&g_1356}},{{&g_1356},{&g_1356},{&g_1356},{&g_1356},{(void*)0},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{&g_1356}},{{(void*)0},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{(void*)0},{&g_1356},{&g_1356},{&g_1356}},{{&g_1356},{&g_1356},{(void*)0},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{(void*)0},{&g_1356}},{{&g_1356},{&g_1356},{&g_1356},{&g_1356},{(void*)0},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{&g_1356}},{{(void*)0},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{&g_1356},{(void*)0},{&g_1356},{&g_1356},{&g_1356}}};
static union U3 g_1368 = {0};
static struct S2 g_1382 = {1073,332,-96,285,1L,1UL};
static uint64_t g_1401 = 0x4225263275512E98LL;
static const uint32_t *g_1407 = (void*)0;
static struct S0 g_1469 = {-2785,-160,91,744,-45,124,104,-41};
static struct S0 g_1471 = {860,-131,468,1879,-52,188,16,-44};
static struct S0 *g_1470 = &g_1471;
static struct S2 g_1480 = {1353,1341,-114,281,0L,0x5AF6A94BF7CB1D88LL};
static uint32_t g_1525 = 0x1BA9F9FFL;
static int16_t g_1553 = 0x12B2L;
static struct S0 g_1561 = {-2130,-152,307,47,-46,249,52,9};
static struct S0 g_1562 = {-2866,241,30,1127,34,111,-70,-39};
static volatile uint16_t g_1585 = 65535UL;
static volatile uint16_t * volatile g_1584[3] = {&g_1585,&g_1585,&g_1585};
static volatile uint16_t * volatile *g_1583 = &g_1584[0];
static const struct S2 g_1603 = {699,1134,-192,176,-4L,18446744073709551610UL};
static uint64_t *g_1617 = &g_188;
static uint64_t **g_1616 = &g_1617;
static struct S2 g_1687 = {586,59,46,129,0x2C599904L,18446744073709551612UL};
static struct S2 g_1690[4] = {{84,755,-173,299,0x42A4C68EL,0xE765A1B1A166C5E6LL},{84,755,-173,299,0x42A4C68EL,0xE765A1B1A166C5E6LL},{84,755,-173,299,0x42A4C68EL,0xE765A1B1A166C5E6LL},{84,755,-173,299,0x42A4C68EL,0xE765A1B1A166C5E6LL}};
static struct S0 g_1779 = {-2806,119,29,558,-83,219,-26,-17};
static const struct S0 g_1780[7] = {{2221,13,93,1802,12,222,30,-30},{1763,235,371,1387,-0,165,70,-0},{1763,235,371,1387,-0,165,70,-0},{2221,13,93,1802,12,222,30,-30},{1763,235,371,1387,-0,165,70,-0},{1763,235,371,1387,-0,165,70,-0},{2221,13,93,1802,12,222,30,-30}};
static const struct S0 g_1781 = {1514,95,425,771,-44,114,45,-5};
static const struct S0 g_1782 = {-1085,44,264,1965,11,164,19,38};
static const struct S0 g_1783 = {-868,60,429,784,-60,202,-20,36};
static struct S0 g_1784[1][7][10] = {{{{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24}},{{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13}},{{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13}},{{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24}},{{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13},{1933,131,282,2043,15,228,-61,13},{1417,100,102,1399,-75,213,-54,-29},{1933,131,282,2043,15,228,-61,13}},{{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13}},{{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24},{1933,131,282,2043,15,228,-61,13},{-1431,-234,114,570,6,9,51,24},{-1431,-234,114,570,6,9,51,24}}}};
static uint8_t g_1824 = 255UL;
static int64_t *g_1835 = &g_1047;
static const int64_t g_1840 = (-1L);
static int32_t g_1853 = (-1L);
static struct S2 g_1904[10][8] = {{{1001,1878,-64,148,0x47F635F3L,5UL},{826,648,220,394,0xE253174BL,18446744073709551607UL},{1014,1001,-86,197,4L,0x46D4069A54DD79D4LL},{826,648,220,394,0xE253174BL,18446744073709551607UL},{1001,1878,-64,148,0x47F635F3L,5UL},{500,587,221,380,8L,9UL},{138,1719,84,27,4L,0xF0DEC04472065FB0LL},{1001,1878,-64,148,0x47F635F3L,5UL}},{{770,654,-149,355,7L,18446744073709551611UL},{725,642,118,412,-1L,1UL},{1392,439,239,372,3L,0x287D0026F10BE231LL},{770,654,-149,355,7L,18446744073709551611UL},{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL},{196,379,91,87,0x575D11F9L,0UL},{783,836,177,143,0L,18446744073709551615UL},{826,648,220,394,0xE253174BL,18446744073709551607UL}},{{725,642,118,412,-1L,1UL},{783,836,177,143,0L,18446744073709551615UL},{1392,439,239,372,3L,0x287D0026F10BE231LL},{138,1719,84,27,4L,0xF0DEC04472065FB0LL},{1065,2009,169,478,0L,7UL},{1065,2009,169,478,0L,7UL},{138,1719,84,27,4L,0xF0DEC04472065FB0LL},{1392,439,239,372,3L,0x287D0026F10BE231LL}},{{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL},{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL},{1014,1001,-86,197,4L,0x46D4069A54DD79D4LL},{1001,1878,-64,148,0x47F635F3L,5UL},{1406,516,-192,228,0x96DB0ED9L,0x9CBA377970D7A5D8LL},{1261,226,-157,0,0L,18446744073709551607UL},{460,944,-145,312,0L,0xE6D1746F5B52AEBDLL},{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL}},{{783,836,177,143,0L,18446744073709551615UL},{725,642,118,412,-1L,1UL},{923,1469,60,246,2L,0UL},{1065,2009,169,478,0L,7UL},{460,944,-145,312,0L,0xE6D1746F5B52AEBDLL},{923,1469,60,246,2L,0UL},{1392,439,239,372,3L,0x287D0026F10BE231LL},{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL}},{{725,642,118,412,-1L,1UL},{770,654,-149,355,7L,18446744073709551611UL},{783,836,177,143,0L,18446744073709551615UL},{1001,1878,-64,148,0x47F635F3L,5UL},{783,836,177,143,0L,18446744073709551615UL},{770,654,-149,355,7L,18446744073709551611UL},{725,642,118,412,-1L,1UL},{1392,439,239,372,3L,0x287D0026F10BE231LL}},{{826,648,220,394,0xE253174BL,18446744073709551607UL},{1001,1878,-64,148,0x47F635F3L,5UL},{500,587,221,380,8L,9UL},{138,1719,84,27,4L,0xF0DEC04472065FB0LL},{1001,1878,-64,148,0x47F635F3L,5UL},{1049,1706,-110,327,-2L,18446744073709551615UL},{460,944,-145,312,0L,0xE6D1746F5B52AEBDLL},{826,648,220,394,0xE253174BL,18446744073709551607UL}},{{196,379,91,87,0x575D11F9L,0UL},{826,648,220,394,0xE253174BL,18446744073709551607UL},{1065,2009,169,478,0L,7UL},{770,654,-149,355,7L,18446744073709551611UL},{1001,1878,-64,148,0x47F635F3L,5UL},{923,1469,60,246,2L,0UL},{923,1469,60,246,2L,0UL},{1001,1878,-64,148,0x47F635F3L,5UL}},{{826,648,220,394,0xE253174BL,18446744073709551607UL},{1392,439,239,372,3L,0x287D0026F10BE231LL},{1392,439,239,372,3L,0x287D0026F10BE231LL},{826,648,220,394,0xE253174BL,18446744073709551607UL},{783,836,177,143,0L,18446744073709551615UL},{196,379,91,87,0x575D11F9L,0UL},{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL},{770,654,-149,355,7L,18446744073709551611UL}},{{725,642,118,412,-1L,1UL},{1173,1014,-240,173,-1L,0x25CAA5B95DA15581LL},{1369,1575,109,21,0L,0xBE21D728B9A44FFBLL},{138,1719,84,27,4L,0xF0DEC04472065FB0LL},{460,944,-145,312,0L,0xE6D1746F5B52AEBDLL},{1014,1001,-86,197,4L,0x46D4069A54DD79D4LL},{138,1719,84,27,4L,0xF0DEC04472065FB0LL},{725,642,118,412,-1L,1UL}}};
static uint32_t *g_1929 = (void*)0;
static uint32_t **g_1928[6] = {&g_1929,&g_1929,&g_1929,&g_1929,&g_1929,&g_1929};
static uint32_t ***g_1927 = &g_1928[2];
static uint32_t *g_1962[5][7][7] = {{{&g_239,&g_1525,&g_239,&g_239,&g_1525,&g_1525,&g_239},{&g_121,&g_239,&g_121,(void*)0,&g_121,&g_1525,&g_121},{&g_1525,&g_1525,&g_1525,&g_1525,&g_121,&g_121,&g_239},{&g_121,&g_239,(void*)0,&g_121,&g_1525,&g_1525,&g_1525},{&g_1525,&g_239,&g_121,&g_1525,&g_239,&g_1525,&g_1525},{&g_1525,&g_239,(void*)0,&g_1525,(void*)0,&g_239,&g_1525},{&g_1525,(void*)0,&g_1525,&g_121,&g_121,&g_121,&g_239}},{{&g_1525,&g_121,&g_239,&g_1525,&g_1525,&g_239,(void*)0},{&g_239,(void*)0,&g_1525,&g_239,(void*)0,&g_1525,&g_239},{(void*)0,&g_239,&g_121,&g_1525,&g_239,&g_1525,(void*)0},{&g_1525,&g_239,&g_1525,&g_239,&g_1525,&g_1525,(void*)0},{&g_1525,&g_239,&g_1525,&g_239,&g_239,&g_239,&g_121},{(void*)0,&g_121,&g_1525,&g_239,&g_239,&g_1525,&g_239},{&g_239,(void*)0,&g_1525,(void*)0,&g_1525,&g_1525,&g_121}},{{&g_1525,(void*)0,&g_121,(void*)0,(void*)0,&g_239,&g_1525},{&g_1525,&g_1525,&g_121,(void*)0,&g_239,&g_239,(void*)0},{&g_239,&g_1525,&g_121,&g_1525,&g_121,&g_1525,&g_239},{&g_239,&g_121,&g_1525,&g_239,&g_121,&g_239,&g_239},{&g_1525,&g_121,&g_121,&g_1525,&g_1525,&g_1525,&g_121},{&g_1525,&g_239,&g_1525,(void*)0,&g_1525,(void*)0,&g_1525},{(void*)0,&g_239,&g_121,(void*)0,&g_1525,&g_239,&g_121}},{{&g_239,&g_1525,&g_121,&g_1525,(void*)0,&g_1525,&g_239},{&g_121,&g_239,&g_121,&g_239,&g_1525,&g_239,&g_1525},{&g_239,&g_239,&g_1525,&g_239,(void*)0,&g_239,&g_239},{&g_239,&g_239,&g_1525,&g_121,&g_1525,&g_121,(void*)0},{(void*)0,&g_1525,&g_1525,&g_121,&g_239,(void*)0,&g_121},{&g_1525,(void*)0,&g_1525,(void*)0,&g_239,(void*)0,&g_239},{&g_1525,&g_239,&g_121,&g_239,&g_1525,&g_121,&g_239}},{{(void*)0,&g_1525,&g_1525,&g_239,(void*)0,&g_1525,&g_239},{&g_121,&g_1525,&g_239,&g_121,&g_1525,&g_1525,&g_1525},{&g_239,&g_239,&g_1525,&g_1525,(void*)0,&g_1525,(void*)0},{&g_239,&g_1525,(void*)0,&g_1525,&g_1525,&g_239,&g_1525},{&g_121,&g_239,&g_239,&g_121,&g_1525,(void*)0,&g_239},{&g_1525,(void*)0,&g_1525,&g_1525,&g_1525,&g_1525,&g_239},{(void*)0,&g_1525,&g_239,&g_239,(void*)0,&g_1525,&g_239}}};
static int64_t g_2069 = (-1L);
static int32_t g_2077 = 3L;
static uint8_t g_2099 = 0x25L;
static const uint32_t *g_2110[4][7][1] = {{{&g_430},{&g_430},{&g_469[1]},{&g_469[4]},{&g_469[1]},{&g_978[1][4][0]},{(void*)0}},{{&g_978[0][8][8]},{(void*)0},{&g_978[1][4][0]},{&g_469[1]},{&g_469[4]},{&g_469[1]},{&g_430}},{{&g_430},{&g_469[1]},{&g_469[4]},{&g_469[1]},{&g_978[1][4][0]},{(void*)0},{&g_978[0][8][8]}},{{(void*)0},{&g_978[1][4][0]},{&g_469[1]},{&g_469[4]},{&g_469[1]},{&g_430},{&g_430}}};
static const uint32_t **g_2109[9][4] = {{(void*)0,&g_2110[3][6][0],&g_2110[0][4][0],&g_2110[0][4][0]},{&g_2110[2][6][0],&g_2110[2][6][0],&g_2110[0][4][0],&g_2110[2][1][0]},{(void*)0,&g_2110[0][4][0],&g_2110[0][4][0],&g_2110[3][6][0]},{&g_2110[0][4][0],&g_2110[0][4][0],&g_2110[2][1][0],&g_2110[0][4][0]},{&g_2110[2][1][0],&g_2110[0][4][0],&g_2110[0][4][0],&g_2110[3][6][0]},{&g_2110[0][4][0],&g_2110[0][4][0],(void*)0,&g_2110[2][1][0]},{&g_2110[0][4][0],&g_2110[2][6][0],&g_2110[2][6][0],&g_2110[0][4][0]},{&g_2110[0][4][0],&g_2110[3][6][0],(void*)0,&g_2110[0][4][0]},{&g_2110[0][4][0],&g_2110[0][4][0],&g_2110[0][4][0],&g_2110[0][5][0]}};
static const uint32_t ***g_2108[8] = {&g_2109[8][0],&g_2109[2][3],&g_2109[8][0],&g_2109[8][0],&g_2109[2][3],&g_2109[8][0],&g_2109[8][0],&g_2109[2][3]};
static const uint8_t *g_2197[2] = {&g_16,&g_16};
static const uint8_t **g_2196 = &g_2197[0];
static int8_t g_2209 = 1L;
static uint64_t g_2234[6] = {0x80D268F8804C8695LL,0x80D268F8804C8695LL,0x80D268F8804C8695LL,0x80D268F8804C8695LL,0x80D268F8804C8695LL,0x80D268F8804C8695LL};
static int16_t ****g_2283 = &g_248;
static uint32_t g_2353 = 0x96F7087DL;
static struct S1 **g_2380 = &g_258;
static struct S1 ***g_2379 = &g_2380;
static struct S1 ****g_2378 = &g_2379;
static struct S2 g_2411 = {963,684,228,63,-1L,18446744073709551609UL};
static uint16_t g_2427 = 65535UL;
static uint16_t g_2450 = 0x2942L;
static uint16_t g_2453 = 8UL;
static int32_t **g_2477[4][1] = {{(void*)0},{(void*)0},{(void*)0},{(void*)0}};
static int32_t ***g_2476 = &g_2477[3][0];
static int32_t ** const volatile *g_2484[8] = {&g_2477[3][0],(void*)0,&g_2477[3][0],(void*)0,&g_2477[3][0],(void*)0,&g_2477[3][0],(void*)0};
static const struct S0 g_2487[7][6][2] = {{{{-2660,-250,262,615,-57,198,-70,11},{750,-120,423,1120,-70,139,78,11}},{{-2660,-250,262,615,-57,198,-70,11},{353,19,180,637,76,113,-103,-9}},{{1353,195,111,700,7,168,21,13},{2678,-225,387,1955,-28,176,-40,-41}},{{353,19,180,637,76,113,-103,-9},{748,51,325,162,56,178,26,-38}},{{392,190,303,910,-76,202,13,16},{1353,195,111,700,7,168,21,13}},{{-1491,170,270,793,71,255,-69,-27},{1412,154,122,131,-27,200,-78,32}}},{{{1412,154,122,131,-27,200,-78,32},{1519,-40,250,1271,-76,171,105,12}},{{-2514,119,356,280,30,37,-10,-35},{521,104,198,370,-39,74,55,-6}},{{141,113,205,193,-70,71,30,-2},{392,190,303,910,-76,202,13,16}},{{-1898,153,453,1562,-62,235,72,0},{-1386,-164,223,1991,47,85,-61,8}},{{748,51,325,162,56,178,26,-38},{-1386,-164,223,1991,47,85,-61,8}},{{-1898,153,453,1562,-62,235,72,0},{392,190,303,910,-76,202,13,16}}},{{{141,113,205,193,-70,71,30,-2},{521,104,198,370,-39,74,55,-6}},{{-2514,119,356,280,30,37,-10,-35},{1519,-40,250,1271,-76,171,105,12}},{{1412,154,122,131,-27,200,-78,32},{1412,154,122,131,-27,200,-78,32}},{{-1491,170,270,793,71,255,-69,-27},{1353,195,111,700,7,168,21,13}},{{392,190,303,910,-76,202,13,16},{748,51,325,162,56,178,26,-38}},{{353,19,180,637,76,113,-103,-9},{2678,-225,387,1955,-28,176,-40,-41}}},{{{1353,195,111,700,7,168,21,13},{353,19,180,637,76,113,-103,-9}},{{-2660,-250,262,615,-57,198,-70,11},{750,-120,423,1120,-70,139,78,11}},{{-2660,-250,262,615,-57,198,-70,11},{353,19,180,637,76,113,-103,-9}},{{1353,195,111,700,7,168,21,13},{2678,-225,387,1955,-28,176,-40,-41}},{{353,19,180,637,76,113,-103,-9},{748,51,325,162,56,178,26,-38}},{{392,190,303,910,-76,202,13,16},{1353,195,111,700,7,168,21,13}}},{{{-1491,170,270,793,71,255,-69,-27},{1412,154,122,131,-27,200,-78,32}},{{1412,154,122,131,-27,200,-78,32},{1519,-40,250,1271,-76,171,105,12}},{{-2514,119,356,280,30,37,-10,-35},{521,104,198,370,-39,74,55,-6}},{{141,113,205,193,-70,71,30,-2},{392,190,303,910,-76,202,13,16}},{{-1898,153,453,1562,-62,235,72,0},{-1386,-164,223,1991,47,85,-61,8}},{{748,51,325,162,56,178,26,-38},{-1386,-164,223,1991,47,85,-61,8}}},{{{-1898,153,453,1562,-62,235,72,0},{392,190,303,910,-76,202,13,16}},{{141,113,205,193,-70,71,30,-2},{521,104,198,370,-39,74,55,-6}},{{-2514,119,356,280,30,37,-10,-35},{1519,-40,250,1271,-76,171,105,12}},{{1412,154,122,131,-27,200,-78,32},{1412,154,122,131,-27,200,-78,32}},{{-1491,170,270,793,71,255,-69,-27},{1353,195,111,700,7,168,21,13}},{{392,190,303,910,-76,202,13,16},{748,51,325,162,56,178,26,-38}}},{{{353,19,180,637,76,113,-103,-9},{2678,-225,387,1955,-28,176,-40,-41}},{{1353,195,111,700,7,168,21,13},{353,19,180,637,76,113,-103,-9}},{{-2660,-250,262,615,-57,198,-70,11},{750,-120,423,1120,-70,139,78,11}},{{-2660,-250,262,615,-57,198,-70,11},{353,19,180,637,76,113,-103,-9}},{{1353,195,111,700,7,168,21,13},{2678,-225,387,1955,-28,176,-40,-41}},{{353,19,180,637,76,113,-103,-9},{748,51,325,162,56,178,26,-38}}}};
static const struct S0 g_2490 = {-2704,-160,174,1294,-44,106,-119,23};
static int32_t g_2568 = 0x9B4070ABL;
static uint16_t g_2605 = 65531UL;
static struct S2 g_2633[9][5] = {{{481,1666,2,273,0x57A2623CL,1UL},{210,650,54,330,1L,9UL},{783,1850,149,441,1L,4UL},{306,1806,57,366,1L,9UL},{35,1833,115,203,-1L,0xB5BB4ED7F40CEBB8LL}},{{298,855,254,357,9L,0x5E1D27DB71687CD6LL},{1361,211,181,103,-3L,0xBEAF83B400B40810LL},{1223,715,172,49,-1L,6UL},{239,1260,225,34,5L,18446744073709551615UL},{828,90,103,307,8L,18446744073709551607UL}},{{210,650,54,330,1L,9UL},{239,1260,225,34,5L,18446744073709551615UL},{975,1772,-142,197,0x23354F1EL,0UL},{756,243,244,297,-9L,0UL},{756,243,244,297,-9L,0UL}},{{298,855,254,357,9L,0x5E1D27DB71687CD6LL},{783,1850,149,441,1L,4UL},{298,855,254,357,9L,0x5E1D27DB71687CD6LL},{975,1772,-142,197,0x23354F1EL,0UL},{1446,1229,170,439,-4L,18446744073709551615UL}},{{481,1666,2,273,0x57A2623CL,1UL},{783,1850,149,441,1L,4UL},{222,700,116,144,0x5A7A6D8CL,18446744073709551615UL},{210,650,54,330,1L,9UL},{1361,211,181,103,-3L,0xBEAF83B400B40810LL}},{{398,538,213,433,-1L,0x7B497B96C3CB3ADALL},{239,1260,225,34,5L,18446744073709551615UL},{756,243,244,297,-9L,0UL},{1223,715,172,49,-1L,6UL},{481,1666,2,273,0x57A2623CL,1UL}},{{507,1885,-147,462,-9L,0x3852396483332986LL},{1361,211,181,103,-3L,0xBEAF83B400B40810LL},{222,700,116,144,0x5A7A6D8CL,18446744073709551615UL},{1361,211,181,103,-3L,0xBEAF83B400B40810LL},{507,1885,-147,462,-9L,0x3852396483332986LL}},{{35,1833,115,203,-1L,0xB5BB4ED7F40CEBB8LL},{210,650,54,330,1L,9UL},{298,855,254,357,9L,0x5E1D27DB71687CD6LL},{1361,211,181,103,-3L,0xBEAF83B400B40810LL},{1223,715,172,49,-1L,6UL}},{{306,1806,57,366,1L,9UL},{35,1833,115,203,-1L,0xB5BB4ED7F40CEBB8LL},{975,1772,-142,197,0x23354F1EL,0UL},{1223,715,172,49,-1L,6UL},{276,1174,-149,452,-1L,4UL}}};
static volatile int32_t *** volatile *g_2738 = &g_1091;
static volatile int32_t *** volatile **g_2737 = &g_2738;
static struct S2 g_2745 = {689,1967,-116,8,-9L,1UL};
static struct S2 g_2748[1][9] = {{{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL},{67,774,-200,508,-2L,18446744073709551615UL}}};
static struct S2 *g_2747 = &g_2748[0][5];
static struct S2 **g_2746 = &g_2747;
static struct S2 g_2752 = {450,1105,-180,130,1L,0xC9D5272FC57CF39ELL};
static struct S2 g_2755 = {1059,1952,12,34,-3L,2UL};
static const int32_t g_2759 = 8L;
static struct S0 **g_2787 = (void*)0;
static volatile int32_t g_2838 = 0x948A2E85L;
static volatile int32_t * const g_2837 = &g_2838;
static volatile int32_t * const *g_2836 = &g_2837;
static uint8_t g_2856[8][8][4] = {{{250UL,8UL,248UL,0xDFL},{0x3BL,255UL,248UL,0x5FL},{255UL,0xC9L,0UL,0x06L},{255UL,0x9CL,1UL,0xE7L},{0xC8L,0UL,255UL,248UL},{8UL,1UL,0x15L,6UL},{0xDFL,248UL,251UL,0x83L},{0x3CL,0x69L,1UL,0x15L}},{{2UL,0x1DL,255UL,0x60L},{0x20L,0xEFL,0x5FL,0xDEL},{251UL,248UL,0xC9L,255UL},{0xBAL,0xFBL,0xDEL,0xA7L},{0xF9L,0x8EL,250UL,0x0EL},{0UL,0xE4L,6UL,248UL},{0x7CL,3UL,0xC5L,0x3FL},{0UL,0x06L,0x08L,0x3CL}},{{0x1DL,0xC8L,0xDFL,255UL},{254UL,0UL,0x6AL,255UL},{3UL,0UL,3UL,251UL},{0x45L,7UL,0xC3L,0xB8L},{0x69L,250UL,0x3AL,7UL},{255UL,0UL,0x3AL,2UL},{0x69L,0x60L,0xC3L,251UL},{0x45L,248UL,3UL,250UL}},{{3UL,250UL,0x6AL,0x26L},{254UL,0x0CL,0xDFL,2UL},{0x1DL,0x88L,0x08L,0xAFL},{0UL,0UL,0xC5L,0xC9L},{0x7CL,254UL,6UL,0xCAL},{0UL,5UL,250UL,0x20L},{0xF9L,0x7CL,0xDEL,251UL},{0xBAL,0x83L,0xC9L,5UL}},{{251UL,2UL,0x5FL,8UL},{0x20L,0x90L,255UL,0UL},{2UL,6UL,0x45L,0x1DL},{0x6AL,0UL,0x0CL,0x3AL},{0x02L,0x6AL,0xE7L,1UL},{250UL,0x3BL,0x20L,0x60L},{1UL,248UL,0xAAL,0xAFL},{0UL,250UL,0x90L,0UL}},{{248UL,0xA7L,255UL,248UL},{7UL,0x15L,0x8EL,0UL},{255UL,0x0CL,5UL,1UL},{255UL,0x1DL,0x1DL,255UL},{0xC5L,251UL,0x48L,0x3BL},{0x88L,7UL,0x83L,0x3CL},{0x8EL,254UL,2UL,0x3CL},{8UL,7UL,248UL,0x3BL}},{{0x20L,251UL,255UL,255UL},{6UL,0x1DL,0UL,1UL},{8UL,0x0CL,0UL,0UL},{0xC9L,0x15L,0xC5L,248UL},{255UL,0xA7L,1UL,0UL},{255UL,250UL,0x12L,0xAFL},{0xBAL,248UL,255UL,0x60L},{0x60L,0x3BL,7UL,1UL}},{{0x5FL,0x6AL,0UL,0x3AL},{0x45L,0UL,0x20L,0x1DL},{0xCAL,6UL,254UL,0x7CL},{0xEFL,1UL,0x88L,250UL},{0xAAL,0xC9L,0xB8L,7UL},{0xC3L,0x5FL,252UL,0x0CL},{0UL,0x12L,254UL,0UL},{2UL,7UL,0xC9L,6UL}}};
static struct S0 g_2876[6] = {{-631,-117,297,2040,59,139,-68,-10},{-631,-117,297,2040,59,139,-68,-10},{-631,-117,297,2040,59,139,-68,-10},{-631,-117,297,2040,59,139,-68,-10},{-631,-117,297,2040,59,139,-68,-10},{-631,-117,297,2040,59,139,-68,-10}};
static struct S1 g_2981 = {139,2347,230,-6,2053};
static struct S0 g_3015 = {116,70,128,1581,-79,228,44,14};
static int8_t g_3055 = 0x13L;
static volatile uint64_t g_3084 = 0x134D9B25154C4A66LL;
static volatile uint64_t g_3085 = 1UL;
static volatile uint64_t g_3086 = 0x0FD9C5441619997DLL;
static volatile uint64_t g_3087 = 0xDCAE036D9806C9C0LL;
static volatile uint64_t *g_3083[9][4] = {{&g_3084,&g_3087,&g_3087,&g_3084},{&g_3086,&g_3087,(void*)0,&g_3087},{&g_3087,&g_3085,(void*)0,(void*)0},{&g_3086,&g_3086,&g_3087,(void*)0},{&g_3084,&g_3085,&g_3084,&g_3087},{&g_3084,&g_3087,&g_3087,&g_3084},{&g_3086,&g_3087,(void*)0,&g_3087},{&g_3087,&g_3085,(void*)0,(void*)0},{&g_3086,&g_3086,&g_3087,(void*)0}};
static volatile uint64_t * volatile *g_3082[5][10] = {{&g_3083[8][0],(void*)0,&g_3083[8][0],&g_3083[0][1],&g_3083[0][3],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[0][3],&g_3083[0][1]},{&g_3083[8][0],(void*)0,&g_3083[8][0],&g_3083[0][1],&g_3083[0][3],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[0][3],&g_3083[0][1]},{&g_3083[8][0],(void*)0,&g_3083[8][0],&g_3083[0][1],&g_3083[0][3],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[0][3],&g_3083[4][2]},{&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[8][0],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[8][0],&g_3083[4][2]},{&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[8][0],&g_3083[4][2],&g_3083[4][2],&g_3083[4][2],&g_3083[8][0],&g_3083[4][2]}};
static volatile uint64_t * volatile **g_3081[3][5][6] = {{{&g_3082[1][0],&g_3082[2][5],&g_3082[2][5],&g_3082[3][1],&g_3082[2][5],&g_3082[2][5]},{&g_3082[3][1],&g_3082[2][5],&g_3082[2][5],&g_3082[3][4],&g_3082[0][7],&g_3082[4][4]},{&g_3082[4][9],(void*)0,(void*)0,(void*)0,&g_3082[4][9],&g_3082[0][5]},{&g_3082[4][9],&g_3082[3][1],&g_3082[2][5],&g_3082[3][4],&g_3082[3][9],&g_3082[2][5]},{&g_3082[3][1],&g_3082[0][7],&g_3082[2][5],&g_3082[3][1],&g_3082[4][8],&g_3082[2][5]}},{{&g_3082[1][0],&g_3082[3][9],&g_3082[2][5],&g_3082[2][5],&g_3082[3][4],&g_3082[0][5]},{&g_3082[4][8],&g_3082[1][8],(void*)0,&g_3082[2][2],&g_3082[3][4],&g_3082[4][4]},{(void*)0,&g_3082[3][9],&g_3082[2][5],&g_3082[4][8],&g_3082[4][8],&g_3082[2][5]},{&g_3082[0][7],&g_3082[0][7],&g_3082[2][5],&g_3082[4][8],&g_3082[3][9],&g_3082[4][0]},{(void*)0,&g_3082[3][1],&g_3082[4][4],&g_3082[2][2],&g_3082[4][9],&g_3082[2][5]}},{{&g_3082[4][8],(void*)0,&g_3082[4][4],&g_3082[2][5],&g_3082[0][7],&g_3082[4][0]},{&g_3082[1][0],&g_3082[2][5],&g_3082[2][5],&g_3082[3][1],&g_3082[2][5],&g_3082[2][5]},{&g_3082[3][1],&g_3082[2][5],&g_3082[2][5],&g_3082[3][4],&g_3082[0][7],&g_3082[4][4]},{&g_3082[4][9],(void*)0,(void*)0,(void*)0,&g_3082[4][9],&g_3082[0][5]},{&g_3082[4][9],&g_3082[3][1],&g_3082[2][5],&g_3082[3][4],&g_3082[3][9],&g_3082[2][5]}}};
static volatile uint64_t * volatile ***g_3080 = &g_3081[1][2][0];
static volatile uint64_t * volatile ****g_3079 = &g_3080;
static uint8_t g_3092 = 0x38L;
static const int16_t g_3099 = 1L;
static uint8_t g_3108 = 0xA3L;
static struct S0 g_3140 = {-1867,16,67,154,33,31,-116,-27};
static struct S0 * volatile *g_3172 = &g_1470;
static struct S0 * volatile **g_3171 = &g_3172;
static const uint32_t g_3401 = 0x45150D6FL;
static const struct S2 g_3434 = {354,1494,-116,308,0L,9UL};
static struct S2 g_3468 = {372,947,-217,132,-9L,0xA9541826232FEB2ALL};
static uint16_t ****g_3527 = &g_932;
static uint16_t *****g_3526 = &g_3527;
static int8_t g_3562[3] = {3L,3L,3L};
static int64_t g_3636 = 0xE10F20840E163A5ALL;
static int32_t g_3715 = 3L;
static uint32_t **g_3754[10][8][2] = {{{&g_1962[2][2][3],(void*)0},{&g_1962[4][3][6],(void*)0},{(void*)0,&g_1962[4][3][6]},{&g_1962[3][5][0],&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1356},{&g_1356,&g_1962[4][3][6]},{(void*)0,&g_1356},{(void*)0,&g_1356}},{{&g_1356,&g_1962[2][4][1]},{&g_1962[3][5][0],&g_1962[4][3][6]},{&g_1356,(void*)0},{&g_1356,&g_1356},{&g_1962[2][2][3],&g_1962[4][1][1]},{&g_1962[4][3][6],(void*)0},{&g_1356,&g_1962[4][3][6]},{(void*)0,&g_1962[4][3][6]}},{{&g_1962[4][1][3],&g_1962[4][3][6]},{&g_1356,(void*)0},{&g_1356,&g_1356},{&g_1962[4][6][6],&g_1356},{&g_1356,(void*)0},{&g_1356,&g_1962[4][3][6]},{&g_1962[4][1][3],&g_1962[4][3][6]},{(void*)0,&g_1962[4][3][6]}},{{&g_1356,(void*)0},{&g_1962[4][3][6],&g_1962[4][1][1]},{&g_1962[2][2][3],&g_1356},{&g_1356,(void*)0},{&g_1356,&g_1962[4][3][6]},{&g_1962[3][5][0],&g_1962[2][4][1]},{&g_1356,&g_1356},{(void*)0,&g_1356}},{{(void*)0,&g_1962[4][3][6]},{&g_1356,&g_1356},{&g_1962[4][3][6],&g_1962[4][3][6]},{&g_1962[3][5][0],&g_1962[4][3][6]},{(void*)0,(void*)0},{&g_1962[4][3][6],(void*)0},{&g_1962[2][2][3],&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1962[4][3][6]}},{{(void*)0,&g_1962[1][1][5]},{&g_1962[4][3][6],&g_1356},{&g_1962[4][3][6],&g_1962[4][3][6]},{&g_1962[3][3][3],&g_1962[1][5][2]},{&g_1962[1][5][2],(void*)0},{(void*)0,&g_1962[4][6][6]},{(void*)0,&g_1962[1][5][2]},{&g_1962[2][4][1],&g_1962[4][3][6]}},{{&g_1962[4][3][6],&g_1962[4][3][6]},{&g_1356,&g_1962[1][1][5]},{&g_1962[4][6][6],&g_1356},{&g_1962[4][3][6],&g_1356},{&g_1962[4][3][6],&g_1962[4][3][6]},{&g_1962[4][6][6],&g_1962[2][5][2]},{&g_1962[4][3][6],&g_1962[4][3][6]},{(void*)0,&g_1962[4][3][6]}},{{&g_1962[2][4][1],(void*)0},{&g_1962[1][5][2],&g_1962[4][6][6]},{&g_1356,&g_1962[4][6][6]},{&g_1962[1][5][2],(void*)0},{&g_1962[2][4][1],&g_1962[4][3][6]},{(void*)0,&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1962[2][5][2]},{&g_1962[4][6][6],&g_1962[4][3][6]}},{{&g_1962[4][3][6],&g_1356},{&g_1962[4][3][6],&g_1356},{&g_1962[4][6][6],&g_1962[1][1][5]},{&g_1356,&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1962[4][3][6]},{&g_1962[2][4][1],&g_1962[1][5][2]},{(void*)0,&g_1962[4][6][6]},{(void*)0,(void*)0}},{{&g_1962[1][5][2],&g_1962[1][5][2]},{&g_1962[3][3][3],&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1356},{&g_1962[4][3][6],&g_1962[1][1][5]},{(void*)0,&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1962[4][3][6]},{&g_1962[4][3][6],&g_1962[4][3][6]},{(void*)0,&g_1962[1][1][5]}}};
static uint32_t ***g_3753 = &g_3754[6][0][0];
static uint16_t g_3768 = 0x6E04L;
static struct S0 g_3784 = {-2257,-208,261,362,85,6,74,31};
static struct S2 g_3787 = {460,1485,-108,36,0xC2479780L,0xCE48CA78AE746765LL};
static int32_t g_3844 = (-3L);
static int16_t g_3865 = 0x9298L;
static int16_t g_3900 = (-1L);
static int8_t **g_3959 = &g_52;
static int8_t ***g_3958 = &g_3959;
static struct S0 g_3985 = {-1646,-102,334,1968,36,41,-117,-33};
static const uint16_t *g_4054[3] = {&g_107,&g_107,&g_107};
static const uint16_t **g_4053[7][8] = {{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]},{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]},{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]},{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]},{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]},{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]},{&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2],&g_4054[2]}};
static const uint16_t ***g_4052[9] = {&g_4053[6][7],&g_4053[6][7],&g_4053[6][7],&g_4053[6][7],&g_4053[6][7],&g_4053[6][7],&g_4053[6][7],&g_4053[6][7],&g_4053[6][7]};
static volatile struct S0 g_4070 = {-308,-117,46,1654,-37,241,-54,11};
static volatile uint32_t * const ** volatile g_4248 = (void*)0;
static volatile uint32_t * const ** volatile * volatile g_4247 = &g_4248;
static volatile uint32_t * const ** volatile * volatile *g_4246[1][7][1] = {{{(void*)0},{&g_4247},{(void*)0},{&g_4247},{(void*)0},{&g_4247},{(void*)0}}};
static struct S0 g_4322 = {-1036,-90,396,359,85,137,87,37};
static struct S0 g_4551 = {-2831,-87,422,1258,33,113,26,-15};
static uint64_t *g_4556 = &g_188;
static struct S2 g_4712[6] = {{427,998,133,141,0xC486C4A9L,1UL},{427,998,133,141,0xC486C4A9L,1UL},{427,998,133,141,0xC486C4A9L,1UL},{427,998,133,141,0xC486C4A9L,1UL},{427,998,133,141,0xC486C4A9L,1UL},{427,998,133,141,0xC486C4A9L,1UL}};
static struct S2 g_4716 = {1133,1379,-67,486,0L,3UL};
static struct S2 g_4719 = {225,894,-109,415,0xAB7D075FL,0UL};
static struct S2 g_4810[2] = {{908,1908,160,310,7L,2UL},{908,1908,160,310,7L,2UL}};
static const struct S2 g_4813 = {584,976,-245,359,0x340D2327L,0x1A898D8051090CCFLL};
static uint64_t * const *g_4950 = &g_4556;
static uint64_t * const * const *g_4949 = &g_4950;
static struct S1 * const g_4985[1] = {(void*)0};
static struct S1 * const *g_4984 = &g_4985[0];
static struct S1 *g_4999[8][8][3] = {{{(void*)0,&g_114,(void*)0},{(void*)0,&g_2981,&g_2981},{&g_892,&g_892,&g_892},{&g_892,(void*)0,(void*)0},{&g_2981,(void*)0,(void*)0},{&g_892,(void*)0,&g_114},{&g_892,&g_2981,(void*)0},{&g_2981,&g_114,(void*)0}},{{&g_114,(void*)0,&g_114},{&g_2981,&g_892,(void*)0},{(void*)0,&g_114,&g_114},{&g_2981,&g_892,&g_892},{&g_114,(void*)0,&g_892},{&g_892,(void*)0,&g_114},{(void*)0,(void*)0,&g_2981},{&g_2981,(void*)0,&g_892}},{{&g_2981,(void*)0,&g_2981},{&g_892,(void*)0,&g_892},{&g_114,(void*)0,&g_892},{&g_114,&g_892,&g_892},{&g_114,&g_114,&g_892},{(void*)0,&g_892,(void*)0},{&g_2981,(void*)0,&g_114},{&g_892,&g_114,&g_114}},{{&g_114,&g_892,(void*)0},{&g_2981,&g_892,&g_892},{&g_114,&g_114,(void*)0},{&g_892,&g_2981,(void*)0},{&g_2981,&g_892,&g_114},{(void*)0,&g_892,&g_114},{&g_114,(void*)0,&g_114},{&g_114,&g_114,(void*)0}},{{&g_114,&g_2981,&g_114},{&g_892,&g_114,&g_2981},{&g_2981,&g_114,(void*)0},{&g_2981,&g_892,&g_2981},{(void*)0,&g_892,&g_114},{&g_892,(void*)0,(void*)0},{&g_114,&g_892,&g_114},{&g_2981,(void*)0,&g_114}},{{(void*)0,&g_114,&g_114},{&g_2981,&g_114,(void*)0},{&g_114,&g_892,(void*)0},{&g_892,&g_2981,&g_892},{(void*)0,&g_892,(void*)0},{&g_114,&g_2981,&g_114},{&g_892,&g_892,&g_114},{&g_892,&g_114,(void*)0}},{{&g_892,&g_114,&g_892},{&g_892,(void*)0,&g_892},{&g_892,&g_892,&g_892},{&g_114,(void*)0,&g_892},{&g_114,&g_892,&g_2981},{(void*)0,&g_892,&g_892},{&g_2981,&g_114,&g_2981},{(void*)0,&g_114,&g_114}},{{&g_114,&g_2981,&g_892},{&g_114,&g_114,&g_892},{&g_892,(void*)0,&g_114},{&g_892,&g_892,(void*)0},{&g_892,&g_892,&g_114},{&g_892,&g_2981,&g_892},{&g_892,&g_114,(void*)0},{&g_114,&g_892,&g_114}}};
static int8_t g_5112 = 0x0FL;
static union U3 **g_5139 = &g_458;
static const struct S0 g_5226 = {-584,4,153,891,-56,89,28,-9};
static uint64_t g_5248[7][5][7] = {{{0xAB34370E4C779722LL,0UL,0UL,0xAB34370E4C779722LL,18446744073709551615UL,6UL,0x74069CF58698BCD8LL},{18446744073709551609UL,8UL,0x45AF96DD0659B782LL,0x709DAFEB0B675F66LL,0x174C57963E9981ABLL,0x1DA870909C77CDC2LL,0x3113216C9310B39ALL},{0x8AEDD1AA9A4260A5LL,0x03D0280771CF221BLL,0UL,18446744073709551609UL,0x857DCAA381C8F5D8LL,6UL,0x74069CF58698BCD8LL},{6UL,0x62ACA54E510CF1E6LL,1UL,0x562558BBDAB04BF0LL,0x74069CF58698BCD8LL,0UL,0x76472E060BB2647ALL},{0x5990EADCDB3DEFE1LL,18446744073709551611UL,0UL,0x62ACA54E510CF1E6LL,0x341BDB15C69936C1LL,0x49B91D2FD8FB5E1ALL,0xC0CF07FB516732BCLL}},{{0xAB92444F8A95A822LL,1UL,0x174C57963E9981ABLL,0xC0CF07FB516732BCLL,0x562558BBDAB04BF0LL,0x40046957FA0EF4A4LL,1UL},{1UL,18446744073709551615UL,0xEE9955FA4AA8CD7FLL,18446744073709551607UL,18446744073709551615UL,0x89E0B21FDC237BBELL,18446744073709551609UL},{0xEE9955FA4AA8CD7FLL,0UL,2UL,0x174C57963E9981ABLL,1UL,0x03D0280771CF221BLL,0x5746048510856058LL},{0UL,0xF40CA121B14E3056LL,0UL,1UL,0UL,0UL,0xEE9955FA4AA8CD7FLL},{18446744073709551615UL,0UL,0UL,1UL,0x62ACA54E510CF1E6LL,0x74069CF58698BCD8LL,0x4AF4D7FA64A54C35LL}},{{18446744073709551615UL,18446744073709551609UL,2UL,0x5AF281BB228FFF9ALL,3UL,3UL,1UL},{0UL,18446744073709551615UL,0xEE9955FA4AA8CD7FLL,0xAB92444F8A95A822LL,0x1DA870909C77CDC2LL,0x8AEDD1AA9A4260A5LL,0UL},{0x3113216C9310B39ALL,0x1DA870909C77CDC2LL,0x174C57963E9981ABLL,0x709DAFEB0B675F66LL,0x45AF96DD0659B782LL,8UL,18446744073709551609UL},{18446744073709551615UL,0UL,0UL,3UL,6UL,6UL,3UL},{1UL,0x7F8DB7BCB37F5F0CLL,1UL,18446744073709551608UL,0xEDB40AC4C0599F0ELL,0x5746048510856058LL,0x45AF96DD0659B782LL}},{{0x421CAE066FE8E22ALL,0xF294153149A8FF5ALL,18446744073709551608UL,1UL,18446744073709551615UL,7UL,0x03D0280771CF221BLL},{0UL,6UL,0x03D0280771CF221BLL,0x3113216C9310B39ALL,0xEE9955FA4AA8CD7FLL,0x5746048510856058LL,18446744073709551607UL},{8UL,0xD3AF4604B9A3EA8ALL,0x583CBD23FBB0EFEFLL,18446744073709551615UL,18446744073709551609UL,6UL,0x1C976CFB9890A5E3LL},{5UL,18446744073709551615UL,0x5746048510856058LL,0UL,9UL,8UL,1UL},{18446744073709551606UL,0xC0CF07FB516732BCLL,18446744073709551609UL,1UL,18446744073709551608UL,0x8AEDD1AA9A4260A5LL,1UL}},{{1UL,0UL,0x8DCDFDBB7279FE0DLL,0x4AF4D7FA64A54C35LL,0UL,3UL,0x3113216C9310B39ALL},{9UL,0xAB34370E4C779722LL,18446744073709551612UL,0x89E0B21FDC237BBELL,0x5990EADCDB3DEFE1LL,0x74069CF58698BCD8LL,1UL},{0x562558BBDAB04BF0LL,0x9137960363790183LL,18446744073709551607UL,1UL,0x404258D049209762LL,0UL,0xF294153149A8FF5ALL},{0x23B82B7CEFCA058FLL,0x9137960363790183LL,0x49B91D2FD8FB5E1ALL,18446744073709551615UL,0xF40CA121B14E3056LL,0x03D0280771CF221BLL,0xAB34370E4C779722LL},{1UL,0xAB34370E4C779722LL,0xF644DF5E99E7822CLL,18446744073709551613UL,0x03D0280771CF221BLL,0x89E0B21FDC237BBELL,8UL}},{{0UL,0UL,18446744073709551609UL,0xF294153149A8FF5ALL,18446744073709551613UL,0x40046957FA0EF4A4LL,18446744073709551612UL},{0x3AB6E05F994975EBLL,0xC0CF07FB516732BCLL,0UL,0x583CBD23FBB0EFEFLL,0x6CC4793E5FBBD3CFLL,0x49B91D2FD8FB5E1ALL,0UL},{2UL,18446744073709551615UL,0x9137960363790183LL,0UL,0x8DCDFDBB7279FE0DLL,0UL,0x9137960363790183LL},{0xD3AF4604B9A3EA8ALL,0xD3AF4604B9A3EA8ALL,7UL,0x8AEDD1AA9A4260A5LL,18446744073709551615UL,0x1DA870909C77CDC2LL,0xF4C45F88C79BE197LL},{0x709DAFEB0B675F66LL,6UL,18446744073709551615UL,18446744073709551615UL,0xAB92444F8A95A822LL,1UL,0xEDB40AC4C0599F0ELL}},{{0x9137960363790183LL,0xF294153149A8FF5ALL,0x341BDB15C69936C1LL,18446744073709551615UL,18446744073709551612UL,0xE2B09EE347D61D4DLL,18446744073709551615UL},{1UL,0xAB34370E4C779722LL,18446744073709551612UL,0x9137960363790183LL,0xEDB40AC4C0599F0ELL,0xD3AF4604B9A3EA8ALL,0x3DD00DAF5AB7F9FFLL},{0x03D0280771CF221BLL,18446744073709551615UL,0x23B82B7CEFCA058FLL,0x74069CF58698BCD8LL,0xF40CA121B14E3056LL,3UL,0x583CBD23FBB0EFEFLL},{18446744073709551612UL,1UL,0UL,0xF4C45F88C79BE197LL,0x341BDB15C69936C1LL,1UL,1UL},{18446744073709551608UL,0x49B91D2FD8FB5E1ALL,0x5990EADCDB3DEFE1LL,18446744073709551607UL,0x421CAE066FE8E22ALL,7UL,0xEE9955FA4AA8CD7FLL}}};
static int16_t g_5281 = 0x8208L;
static int64_t * volatile *g_5296[1][1][7] = {{{&g_1835,&g_1835,&g_1835,&g_1835,&g_1835,&g_1835,&g_1835}}};
static int64_t * volatile * volatile *g_5295 = &g_5296[0][0][3];
static const struct S2 g_5302 = {833,587,23,452,0x0A271218L,5UL};
static struct S2 g_5305 = {596,968,-1,239,-10L,0x4E8CAB65DD7C8B18LL};
static uint32_t g_5357[9][10] = {{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL},{3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL,3UL}};
static struct S0 g_5411[9] = {{2137,114,440,116,49,55,-29,12},{1983,126,296,1746,-44,247,-13,28},{1983,126,296,1746,-44,247,-13,28},{2137,114,440,116,49,55,-29,12},{1983,126,296,1746,-44,247,-13,28},{1983,126,296,1746,-44,247,-13,28},{2137,114,440,116,49,55,-29,12},{1983,126,296,1746,-44,247,-13,28},{1983,126,296,1746,-44,247,-13,28}};
static struct S0 ** volatile *g_5520 = &g_2787;
static struct S0 ** volatile ** volatile g_5519 = &g_5520;
static struct S0 ** volatile ** volatile * volatile g_5518 = &g_5519;
static volatile struct S0 * const * volatile * const *g_5522 = (void*)0;
static volatile struct S0 * const * volatile * const **g_5521 = &g_5522;
static int32_t ** volatile g_5539 = &g_100;
static struct S1 ** const **g_5559 = (void*)0;
static volatile int32_t * volatile g_5580 = &g_4719.f4;
static volatile int32_t * volatile * volatile g_5581[1][2][2] = {{{&g_5580,&g_5580},{&g_5580,&g_5580}}};
static volatile int32_t * volatile * volatile g_5582[5] = {(void*)0,(void*)0,(void*)0,(void*)0,(void*)0};
static volatile int32_t * volatile * volatile g_5583 = &g_1093;
static const int32_t g_5604 = 0x1AF150FEL;



static uint32_t func_1(void);
static int16_t func_6(int32_t p_7, int8_t * const p_8, int32_t p_9, int8_t * p_10);
static uint64_t func_14(uint16_t p_15);
static uint32_t func_20(int8_t * p_21);
static uint16_t func_24(uint64_t p_25, int32_t p_26);
static uint64_t func_27(uint32_t p_28, const int8_t p_29, const struct S1 p_30);
static struct S1 func_31(int32_t p_32, int8_t * p_33, uint8_t p_34, uint8_t p_35, const int32_t p_36);
static int8_t * func_37(int8_t * p_38);
static int8_t * func_39(const int8_t * p_40, int8_t * p_41, int8_t p_42, int32_t p_43, int8_t * p_44);
static const int8_t * func_45(union U3 p_46, int32_t p_47, int64_t p_48);
static uint32_t func_1(void)
{
    int32_t l_17 = 8L;
    union U3 *l_50 = (void*)0;
    union U3 *l_51 = &g_49;
    int8_t *l_2010 = &g_58;
    int32_t l_4850 = 0xDFBEA050L;
    int32_t *l_5307 = &g_266[2][7];
    int32_t l_5313 = 0L;
    int32_t l_5315[6] = {(-1L),0xD23A5350L,0xD23A5350L,(-1L),0xD23A5350L,0xD23A5350L};
    int16_t l_5319 = 0x2CE6L;
    uint8_t l_5320 = 0x67L;
    uint16_t l_5381 = 1UL;
    int64_t *l_5458 = &g_2069;
    uint64_t l_5495 = 1UL;
    int16_t l_5532 = 0L;
    struct S0 *l_5564 = &g_3140;
    int8_t l_5565 = 0x01L;
    uint8_t l_5588 = 0x2EL;
    struct S1 l_5592 = {-181,3828,-187,0,-73};
    int i;
    if (((*l_5307) = (safe_add_func_int16_t_s_s((safe_sub_func_uint32_t_u_u((func_6((((safe_rshift_func_uint8_t_u_s(g_13, (func_14(((g_16 != (l_17 , ((((safe_lshift_func_uint8_t_u_s(g_16, (func_20((((safe_unary_minus_func_int8_t_s(((!func_24(g_16, (((*g_1356) = (func_27((g_16 & ((func_31(l_17, func_37(func_39(func_45(((*l_51) = g_49), ((g_52 = g_52) != &g_53), g_53), l_2010, l_17, (*g_100), l_2010)), l_17, (*g_499), g_2759) , (**g_2196)) || l_17)), l_17, (***g_2379)) >= 0x177667F2AC80E136LL)) && l_17))) , (*g_52)))) > l_17) , l_2010)) , 0x63L))) , 0x51L) >= l_4850) < 0xC1DDC947L))) , 1UL)) , (***g_3958)))) || l_4850) , (**g_2836)), l_2010, g_4810[0].f1, &g_2209) > 65527UL), l_17)), l_17))))
    {
        struct S0 *****l_5308 = (void*)0;
        const struct S0 ***l_5311 = (void*)0;
        const struct S0 ****l_5310 = &l_5311;
        const struct S0 *****l_5309 = &l_5310;
        int32_t l_5312 = 0L;
        int32_t *l_5314 = &g_1853;
        int32_t *l_5316 = &g_266[2][4];
        int32_t *l_5317 = &l_5315[3];
        int32_t *l_5318[8] = {(void*)0,&g_3715,&g_3715,(void*)0,&g_3715,&g_3715,(void*)0,&g_3715};
        int i;
        (*l_5309) = (void*)0;
        (*l_5307) ^= 0x5F0DA6AAL;
        for (g_2099 = 0; (g_2099 <= 0); g_2099 += 1)
        {
            return l_5312;
        }
        --l_5320;
    }
    else
    {
        uint32_t l_5329 = 18446744073709551614UL;
        int32_t l_5354 = 0xFC83E333L;
        int32_t l_5358[4] = {0x62B9AA79L,0x62B9AA79L,0x62B9AA79L,0x62B9AA79L};
        int16_t l_5405 = 0x7195L;
        struct S1 *****l_5416 = &g_2378;
        int32_t **l_5441 = (void*)0;
        const struct S1 l_5494 = {366,2456,-243,0,16};
        int64_t l_5496[3];
        uint32_t l_5498 = 0xD07D1D2CL;
        int32_t l_5534 = 5L;
        int i;
        for (i = 0; i < 3; i++)
            l_5496[i] = 0x2C76D46E95725950LL;
        for (g_2209 = 4; (g_2209 >= 0); g_2209 -= 1)
        {
            int32_t *l_5323 = &g_82;
            int32_t *l_5324 = (void*)0;
            int32_t *l_5325 = &l_5315[3];
            int32_t *l_5326 = &l_5315[3];
            int32_t *l_5327 = (void*)0;
            int32_t *l_5328[6] = {&g_3715,&g_3844,&g_3844,&g_3715,&g_3844,&g_3844};
            union U3 l_5349 = {0};
            uint32_t *l_5355 = &g_239;
            int16_t *l_5356 = (void*)0;
            uint32_t l_5359 = 4294967295UL;
            uint64_t l_5382 = 0x043DF06C8D8C068DLL;
            struct S0 *l_5408 = &g_134;
            int i;
            l_5329++;
            if (((safe_mod_func_int32_t_s_s(((safe_mod_func_int64_t_s_s((*l_5326), (((safe_mul_func_uint8_t_u_u((0UL || (g_5357[0][2] ^= ((safe_mod_func_uint16_t_u_u(((safe_lshift_func_int8_t_s_s(0x77L, 6)) == (((((safe_add_func_int8_t_s_s(3L, ((*l_5307) != (safe_unary_minus_func_uint32_t_u(((*l_5355) |= ((*g_1356) = (safe_mul_func_uint8_t_u_u((*g_499), (safe_add_func_uint16_t_u_u((l_5354 = (((**g_249) , l_5349) , (safe_mod_func_uint32_t_u_u((((safe_mul_func_uint16_t_u_u((*l_5307), 0UL)) == (***g_5295)) != (*l_5307)), (*g_1356))))), 0x786EL))))))))))) | (*g_250)) , (void*)0) == (void*)0) < (-10L))), (*g_250))) == 7L))), 7UL)) != l_5329) && 0xEBC9351222500177LL))) && 1UL), l_5329)) != 0x0C4FL))
            {
                l_5359 ^= ((-3L) >= l_5358[1]);
                return (*l_5307);
            }
            else
            {
                int32_t ****l_5367 = &g_2476;
                int32_t l_5380[9][10][2] = {{{1L,1L},{4L,0xD8E915E9L},{0xD8E915E9L,(-1L)},{4L,0xB99AA8FBL},{1L,(-3L)},{0x1A739C35L,4L},{0x55530D28L,5L},{1L,0xABC03553L},{1L,5L},{0x55530D28L,4L}},{{0xABC03553L,0x5FFB53E0L},{1L,4L},{0xD8E915E9L,0L},{0xC3E0B94AL,0xC3E0B94AL},{0xD8E915E9L,0x21B598BCL},{1L,0x55530D28L},{0xABC03553L,(-1L)},{1L,0xABC03553L},{0xB99AA8FBL,0xEA05F1CDL},{0xB99AA8FBL,0xABC03553L}},{{1L,(-1L)},{0xABC03553L,0x55530D28L},{1L,0x21B598BCL},{0xD8E915E9L,0xC3E0B94AL},{0xC3E0B94AL,0L},{0xD8E915E9L,4L},{1L,0x5FFB53E0L},{0xABC03553L,0xD8E915E9L},{1L,0x7C249C29L},{0xB99AA8FBL,0L}},{{0xB99AA8FBL,0x7C249C29L},{1L,0xD8E915E9L},{0xABC03553L,0x5FFB53E0L},{1L,4L},{0xD8E915E9L,0L},{0xC3E0B94AL,0xC3E0B94AL},{0xD8E915E9L,0x21B598BCL},{1L,0x55530D28L},{0xABC03553L,(-1L)},{1L,0xABC03553L}},{{0xB99AA8FBL,0xEA05F1CDL},{0xB99AA8FBL,0xABC03553L},{1L,(-1L)},{0xABC03553L,0x55530D28L},{1L,0x21B598BCL},{0xD8E915E9L,0xC3E0B94AL},{0xC3E0B94AL,0L},{0xD8E915E9L,4L},{1L,0x5FFB53E0L},{0xABC03553L,0xD8E915E9L}},{{1L,0x7C249C29L},{0xB99AA8FBL,0L},{0xB99AA8FBL,0x7C249C29L},{1L,0xD8E915E9L},{0xABC03553L,0x5FFB53E0L},{1L,4L},{0xD8E915E9L,0L},{0xC3E0B94AL,0xC3E0B94AL},{0xD8E915E9L,0x21B598BCL},{1L,0x55530D28L}},{{0xABC03553L,(-1L)},{1L,0xABC03553L},{0xB99AA8FBL,0xEA05F1CDL},{0xB99AA8FBL,0xABC03553L},{1L,(-1L)},{0xABC03553L,0x55530D28L},{1L,0x21B598BCL},{0xD8E915E9L,0xC3E0B94AL},{0xC3E0B94AL,0L},{0xD8E915E9L,4L}},{{1L,0x5FFB53E0L},{0xABC03553L,0xD8E915E9L},{1L,0x7C249C29L},{0xB99AA8FBL,0L},{0xB99AA8FBL,0x7C249C29L},{1L,0xD8E915E9L},{0xABC03553L,0x5FFB53E0L},{1L,4L},{0xD8E915E9L,0L},{0xC3E0B94AL,0xC3E0B94AL}},{{0xD8E915E9L,0x21B598BCL},{1L,0x55530D28L},{0xABC03553L,(-1L)},{1L,0xABC03553L},{0xB99AA8FBL,0xEA05F1CDL},{0xB99AA8FBL,0xABC03553L},{1L,(-1L)},{0xABC03553L,0x55530D28L},{1L,0x21B598BCL},{0xD8E915E9L,0xC3E0B94AL}}};
                int32_t **l_5383 = (void*)0;
                int32_t **l_5384 = &l_5325;
                int i, j, k;
                l_5313 ^= (safe_sub_func_int16_t_s_s(((((safe_mod_func_int8_t_s_s((&l_5355 == ((*g_3753) = (*g_3753))), (*g_52))) ^ (((safe_unary_minus_func_int8_t_s((***g_3958))) && (safe_sub_func_int64_t_s_s((l_5367 != (void*)0), ((*g_1835) = ((safe_add_func_int8_t_s_s((safe_mod_func_uint16_t_u_u((((***g_4949) = ((safe_mod_func_uint8_t_u_u((*g_499), (((safe_add_func_uint8_t_u_u(((*g_1835) & ((safe_add_func_uint16_t_u_u((*l_5326), 0xB579L)) <= l_5354)), (*g_499))) < l_5380[5][2][0]) , (***g_3958)))) && (**g_3959))) | l_5381), 0x606DL)), (***g_3958))) | (*l_5307)))))) , (*l_5323))) <= 0xAD2C8DE3C7F23434LL) <= l_5382), (**g_1583)));
                (*l_5384) = &l_5354;
            }
            for (g_2353 = 1; (g_2353 <= 4); g_2353 += 1)
            {
                uint8_t **l_5392 = &g_499;
                int32_t l_5399 = 1L;
                int8_t l_5404 = (-1L);
                struct S0 *l_5410 = &g_5411[5];
                int64_t l_5446 = (-5L);
                int32_t l_5447 = 0xF3FCE4E5L;
                int32_t l_5448[1];
                int32_t l_5450 = 0xC29389B8L;
                uint32_t l_5451 = 0x3340D1B1L;
                int i, j;
                for (i = 0; i < 1; i++)
                    l_5448[i] = 0L;
                (*l_5323) |= ((!(safe_add_func_uint32_t_u_u((safe_lshift_func_int8_t_s_u((safe_mul_func_uint16_t_u_u(((*l_5307) , ((void*)0 == l_5392)), (((safe_rshift_func_int16_t_s_u((safe_rshift_func_int8_t_s_u((*l_5307), (safe_mod_func_int32_t_s_s(l_5399, (l_5399 ^ ((&l_5329 == ((**g_1927) = (void*)0)) & ((safe_rshift_func_int16_t_s_s(((safe_add_func_uint32_t_u_u(((*l_5355) = (l_5404 , (*g_1356))), l_5404)) == (*l_5307)), l_5405)) == l_5329))))))), 12)) >= 0xA664A08E73B29F8CLL) | 0x47264078L))), 2)), (-7L)))) && (*l_5307));
                for (g_3636 = 13; (g_3636 >= 26); ++g_3636)
                {
                    struct S0 **l_5409 = (void*)0;
                    int32_t l_5442 = 0L;
                    union U3 l_5444 = {0};
                    int32_t l_5445[10];
                    int32_t l_5449[1];
                    int i;
                    for (i = 0; i < 10; i++)
                        l_5445[i] = 0xB8B8C069L;
                    for (i = 0; i < 1; i++)
                        l_5449[i] = (-1L);
                    (*g_3172) = (l_5410 = l_5408);
                    if (((void*)0 != &g_5139))
                    {
                        return l_5354;
                    }
                    else
                    {
                        int64_t l_5433 = 5L;
                        uint32_t l_5443 = 1UL;
                        (*l_5307) = (safe_mod_func_int8_t_s_s(((((safe_add_func_uint8_t_u_u(((l_5416 == (void*)0) > ((*g_499)--)), ((**g_3959) |= l_5404))) < 0x308ACC81F7CD38D4LL) ^ (((safe_sub_func_uint32_t_u_u((safe_div_func_int64_t_s_s((safe_lshift_func_uint8_t_u_u(6UL, ((((safe_add_func_uint16_t_u_u((safe_add_func_int32_t_s_s(((safe_rshift_func_int8_t_s_u((safe_rshift_func_int8_t_s_s((l_5433 & (((safe_lshift_func_uint8_t_u_s((safe_mul_func_int16_t_s_s((((*g_1356) == 0xA9B53C1AL) & (safe_unary_minus_func_uint32_t_u(((safe_add_func_int16_t_s_s((((***g_5295) = (l_5441 != (void*)0)) >= l_5442), 6L)) != (*l_5307))))), (*g_566))), 4)) == 0x71EBL) , 18446744073709551608UL)), l_5433)), 5)) == l_5443), l_5443)), (*l_5307))) , (*g_2747)) , (*g_4556)) == (*g_4556)))), 0x73E5F8D752CF49A8LL)), (*l_5323))) != l_5442) < 0x8513L)) , l_5404), l_5399));
                        if (l_5404)
                            break;
                        (*l_5307) = (l_5444 , (-1L));
                    }
                    --l_5451;
                }
            }
        }
        for (l_4850 = (-18); (l_4850 == (-22)); l_4850--)
        {
            uint16_t l_5456[10];
            int64_t **l_5457[4] = {&g_1835,&g_1835,&g_1835,&g_1835};
            union U3 l_5459 = {0};
            int32_t l_5472 = 0xC7A918FCL;
            uint64_t l_5473 = 1UL;
            int32_t l_5497 = 0x4C4DD460L;
            int32_t l_5516 = 0L;
            int16_t *l_5517[1];
            int i;
            for (i = 0; i < 10; i++)
                l_5456[i] = 0UL;
            for (i = 0; i < 1; i++)
                l_5517[i] = &g_3900;
            l_5358[1] ^= ((((***g_248) >= l_5456[1]) | (((-8L) == ((l_5458 = &g_2069) != (**g_5295))) , ((***g_5295) == (((l_5459 , (((!l_5354) >= ((safe_rshift_func_uint16_t_u_s(0x605BL, (***g_248))) >= 0xC7L)) , 0x33L)) & (*l_5307)) | l_5456[5])))) & (*l_5307));
            for (g_2605 = 0; (g_2605 <= 1); g_2605 += 1)
            {
                uint8_t l_5467 = 0UL;
                int32_t l_5470 = (-1L);
                int32_t l_5471 = 4L;
                uint16_t l_5523 = 1UL;
                int32_t l_5530 = 5L;
                int8_t l_5531[6][1] = {{(-1L)},{0L},{0L},{(-1L)},{0L},{0L}};
                int32_t l_5533[10];
                int i, j;
                for (i = 0; i < 10; i++)
                    l_5533[i] = 0x677DF63BL;
                if ((!((void*)0 == &g_3080)))
                {
                    int32_t *l_5464 = &l_5358[3];
                    int32_t *l_5465 = &g_3844;
                    int32_t *l_5466[5] = {&l_5315[3],&l_5315[3],&l_5315[3],&l_5315[3],&l_5315[3]};
                    int i;
                    --l_5467;
                    --l_5473;
                    if (l_5471)
                        break;
                }
                else
                {
                    int32_t l_5493 = 0L;
                    int32_t *l_5524 = (void*)0;
                    int32_t *l_5525 = &g_3715;
                    int32_t *l_5526 = &l_5493;
                    int32_t *l_5527 = &l_5313;
                    int32_t *l_5528 = &l_5470;
                    int32_t *l_5529[4];
                    uint32_t l_5535 = 1UL;
                    int i, j;
                    for (i = 0; i < 4; i++)
                        l_5529[i] = &g_1853;
                    l_5497 |= (safe_lshift_func_int8_t_s_s(((safe_rshift_func_int8_t_s_u(((((*l_5307) < (safe_lshift_func_int8_t_s_u(((**g_3959) = l_5470), 2))) | ((l_5329 && l_5467) | ((*g_1617) = ((safe_add_func_int8_t_s_s(l_5456[8], (safe_rshift_func_int16_t_s_u((((*g_499) = ((safe_mul_func_int16_t_s_s((((safe_sub_func_int64_t_s_s(((((*g_97) , ((safe_mul_func_int16_t_s_s((+0xCAA770B861471AB4LL), (l_5467 && (l_5493 != 1UL)))) | l_5456[8])) , l_5494) , 0L), l_5495)) == l_5496[2]) , l_5471), (****g_2283))) > (*l_5307))) ^ l_5493), 8)))) <= (***g_5295))))) > (***g_5295)), 0)) <= (*l_5307)), l_5472));
                    if (l_5498)
                        break;
                    (*l_5307) = (safe_rshift_func_int16_t_s_u((safe_mul_func_uint16_t_u_u((((**g_249) == (safe_mul_func_uint8_t_u_u(((((safe_add_func_int32_t_s_s(((l_5467 >= ((((*g_499) = ((safe_rshift_func_uint8_t_u_u(((safe_add_func_int64_t_s_s((safe_lshift_func_int16_t_s_u(((**g_4950) | l_5471), 1)), ((safe_div_func_int16_t_s_s((18446744073709551609UL < (!(l_5516 > ((g_236[(g_2605 + 1)][g_2605] = ((void*)0 == l_5517[0])) < (g_5518 == g_5521))))), l_5456[9])) , 0xED1E99682D67778ALL))) != l_5523), (*g_499))) <= (-1L))) <= l_5471) >= 254UL)) ^ 0x61CEA224L), l_5493)) == (***g_248)) <= 0x68C3D5DA58BC6159LL) ^ l_5493), 7UL))) | (*l_5307)), (*g_250))), 5));
                    l_5535++;
                }
                for (g_2568 = 0; (g_2568 <= 0); g_2568 += 1)
                {
                    int32_t *l_5538 = &l_5497;
                    for (g_63 = 0; (g_63 <= 0); g_63 += 1)
                    {
                        (*g_5539) = l_5538;
                        return (*l_5307);
                    }
                }
                for (g_5281 = 0; (g_5281 >= 0); g_5281 -= 1)
                {
                    uint16_t l_5545 = 65532UL;
                    for (g_53 = 0; (g_53 >= 0); g_53 -= 1)
                    {
                        int32_t *l_5540 = &l_5358[1];
                        int32_t *l_5541 = &l_5358[0];
                        int32_t *l_5542 = &l_5313;
                        int32_t *l_5543[8][4] = {{&g_266[2][3],&l_5315[4],&l_5315[1],&l_5315[4]},{&l_5315[4],&g_266[4][7],&l_5315[1],&l_5315[1]},{&g_266[2][3],&g_266[2][3],&l_5315[4],&l_5315[1]},{(void*)0,&g_266[4][7],(void*)0,&l_5315[4]},{&l_5315[1],(void*)0,(void*)0,&l_5315[1]},{&l_5315[4],(void*)0,&g_266[4][7],(void*)0},{(void*)0,&g_266[2][3],&g_266[4][7],&g_266[4][7]},{&l_5315[4],&l_5315[4],(void*)0,&g_266[4][7]}};
                        int8_t l_5544 = 0x30L;
                        int i, j;
                        --l_5545;
                        return l_5545;
                    }
                    if (l_5471)
                        continue;
                }
            }
            for (g_3844 = 22; (g_3844 > (-24)); g_3844 = safe_sub_func_uint16_t_u_u(g_3844, 6))
            {
                int64_t l_5550 = 0x3ED1650E07C96EB2LL;
                if (l_5496[1])
                {
                    (*g_5139) = (*g_5139);
                    return l_5550;
                }
                else
                {
                    int32_t l_5554 = 1L;
                    struct S1 ** const ***l_5560 = &g_5559;
                    struct S0 *l_5563[6][7][6] = {{{&g_134,&g_3140,&g_5411[5],&g_1163,&g_1471,&g_5411[5]},{&g_1784[0][5][3],&g_2876[3],&g_1164,&g_1784[0][5][3],&g_1164,&g_1471},{&g_2876[3],&g_1471,(void*)0,(void*)0,&g_4551,(void*)0},{&g_3140,(void*)0,&g_1779,&g_1779,(void*)0,&g_3140},{&g_4551,&g_154,&g_1471,&g_134,(void*)0,(void*)0},{&g_3140,&g_1779,&g_322,(void*)0,&g_322,&g_1163},{&g_3140,&g_3015,(void*)0,&g_134,&g_154,&g_1779}},{{&g_4551,(void*)0,(void*)0,&g_1779,&g_2876[3],&g_3784},{&g_3140,&g_2876[3],&g_1469,(void*)0,&g_1163,&g_1562},{&g_2876[3],&g_1164,&g_4551,&g_1784[0][5][3],(void*)0,&g_154},{&g_1784[0][5][3],&g_1163,(void*)0,&g_1163,&g_1784[0][5][3],(void*)0},{&g_134,&g_1469,(void*)0,(void*)0,&g_1784[0][5][3],&g_1471},{&g_322,&g_154,&g_154,&g_1469,(void*)0,&g_1471},{&g_4551,&g_1469,(void*)0,(void*)0,&g_3015,(void*)0}},{{(void*)0,&g_2876[3],(void*)0,&g_1562,&g_1469,&g_154},{&g_1164,&g_134,&g_4551,&g_3140,&g_322,&g_1562},{&g_1562,&g_1469,&g_1469,&g_1471,&g_3784,&g_3784},{&g_1469,(void*)0,(void*)0,&g_1469,&g_1164,&g_1779},{&g_1779,&g_4551,(void*)0,&g_154,&g_134,&g_1163},{(void*)0,&g_5411[5],&g_322,&g_1784[0][5][3],&g_134,(void*)0},{&g_1779,&g_4551,&g_1471,&g_1164,&g_1164,&g_3140}},{{(void*)0,(void*)0,&g_1779,&g_2876[3],&g_3784,(void*)0},{&g_1562,&g_1469,(void*)0,(void*)0,&g_322,&g_1471},{&g_3015,&g_134,&g_1164,&g_5411[5],&g_1469,&g_5411[5]},{&g_5411[5],&g_2876[3],&g_5411[5],&g_154,&g_3015,&g_1164},{(void*)0,&g_1469,&g_2876[3],&g_3015,(void*)0,&g_154},{&g_2876[3],&g_154,&g_5411[5],&g_3015,&g_1784[0][5][3],&g_154},{(void*)0,&g_1469,&g_1163,&g_154,&g_1784[0][5][3],&g_2876[3]}},{{&g_5411[5],&g_1163,&g_1471,&g_5411[5],(void*)0,&g_322},{&g_2876[3],&g_1469,&g_1471,&g_154,&g_1164,&g_1562},{&g_1163,&g_1471,&g_154,&g_134,&g_134,&g_154},{&g_1562,&g_1562,&g_1163,&g_1469,&g_1163,(void*)0},{&g_1469,&g_2876[3],&g_4551,(void*)0,&g_2876[3],&g_1163},{&g_1469,&g_1469,&g_4551,&g_5411[5],&g_1562,(void*)0},{(void*)0,&g_5411[5],&g_1163,&g_3140,&g_4551,&g_154}},{{&g_3140,&g_4551,&g_154,(void*)0,&g_3140,&g_1562},{&g_3015,&g_322,&g_1471,(void*)0,&g_1469,&g_1779},{&g_1469,&g_322,(void*)0,&g_3015,(void*)0,&g_322},{&g_154,(void*)0,&g_1784[0][5][3],&g_4551,&g_1164,&g_2876[3]},{&g_3140,&g_3784,&g_1471,&g_1562,(void*)0,&g_5411[5]},{&g_1779,&g_3784,&g_134,&g_4551,&g_1164,&g_1469},{&g_154,(void*)0,&g_1779,&g_1784[0][5][3],(void*)0,&g_1471}}};
                    struct S2 ***l_5569 = &g_2746;
                    struct S2 ****l_5568 = &l_5569;
                    int i, j, k;
                    (*l_5307) = (((+(safe_rshift_func_int16_t_s_u((**g_249), 12))) , l_5554) , (l_5565 &= (safe_div_func_uint16_t_u_u((*l_5307), (safe_mul_func_int8_t_s_s((((((*l_5416) = (*l_5416)) == ((*l_5560) = g_5559)) , (l_5550 & ((safe_add_func_int16_t_s_s((****g_2283), (l_5563[4][1][4] == l_5564))) | 0x3B733227CA3A8C2CLL))) ^ l_5472), 0xD2L))))));
                    (*l_5307) |= (safe_sub_func_int16_t_s_s((**g_249), ((**g_565) |= ((***g_3958) , ((l_5550 == 0x77L) ^ (5L > (*g_1356)))))));
                    (*l_5568) = &g_2746;
                    for (l_5554 = 4; (l_5554 >= 1); l_5554 -= 1)
                    {
                        int i, j;
                        l_5497 |= ((*g_718) , (safe_add_func_int16_t_s_s(((0x70D6240DACC48BC5LL && (+((**g_565) &= ((((*g_5521) != (((g_266[l_5554][(l_5554 + 2)] ^ (*g_499)) < (!(safe_div_func_uint16_t_u_u((0xDBL <= ((((safe_mod_func_uint16_t_u_u((l_5550 > g_266[l_5554][(l_5554 + 2)]), (safe_add_func_uint64_t_u_u((***g_4949), 0xB39B2B42F82366E5LL)))) && l_5550) || 0xD62B4B3BC8EFFFB9LL) > 0xF5L)), l_5550)))) , (void*)0)) , (***g_3958)) ^ 5L)))) || (-2L)), (*g_250))));
                        (**g_1091) = (void*)0;
                    }
                }
                (*g_5583) = g_5580;
            }
            (*g_3172) = (*g_3172);
        }
    }
    for (g_3844 = (-1); (g_3844 < 13); ++g_3844)
    {
        int32_t **l_5587 = (void*)0;
        int32_t ***l_5586 = &l_5587;
        int32_t l_5589 = (-1L);
        struct S1 *l_5593 = &l_5592;
        int32_t l_5607 = (-1L);
        l_5588 &= ((void*)0 != l_5586);
        l_5315[3] ^= (l_5589 | (((((*l_5593) = l_5592) , (((((*l_5307) , (*g_1300)) , l_5589) != (safe_lshift_func_uint16_t_u_s((safe_mul_func_uint8_t_u_u((((safe_sub_func_uint8_t_u_u((safe_div_func_uint16_t_u_u((safe_add_func_uint64_t_u_u((*g_4556), g_5604)), (safe_mod_func_int16_t_s_s((l_5589 < ((*l_5307) >= (***g_248))), l_5589)))), l_5589)) && l_5607) <= (***g_5295)), (**g_3959))), 10))) >= (*l_5307))) > (*g_4556)) < 0UL));
    }
    return (*l_5307);
}







static int16_t func_6(int32_t p_7, int8_t * const p_8, int32_t p_9, int8_t * p_10)
{
    int16_t l_5306 = (-10L);
    return l_5306;
}







static uint64_t func_14(uint16_t p_15)
{
    struct S1 l_4853 = {-385,4366,-86,14,2309};
    uint64_t *** const l_4877 = &g_1616;
    const int32_t *l_4885 = &g_1853;
    uint64_t ***l_4917 = &g_1616;
    uint64_t ****l_4916 = &l_4917;
    uint64_t *****l_4915 = &l_4916;
    int32_t l_4923 = 1L;
    int32_t l_4924 = 0xD24EA03AL;
    int32_t l_4925 = (-2L);
    int32_t l_4926 = 0x13393E0EL;
    int32_t l_4927 = 0xF56F160EL;
    int16_t l_4928 = (-4L);
    int32_t l_4929 = 0xBF6F7BF0L;
    int32_t l_4930 = (-9L);
    int32_t l_4931 = 0x3788D386L;
    int32_t l_4932 = (-2L);
    int32_t l_4933 = 1L;
    int32_t l_4934 = 7L;
    int32_t l_4935 = 1L;
    int32_t l_4936 = (-1L);
    int32_t l_4937 = 0x2B3788D9L;
    int32_t l_4938 = (-6L);
    int32_t l_4939 = 1L;
    int32_t l_4940 = 4L;
    int32_t l_4941[10][8] = {{1L,0x7F9E8CB7L,0x7F9E8CB7L,1L,0x434F5C92L,0xFE241617L,0xE3CAABA5L,(-1L)},{0x5CCF24D9L,1L,(-1L),0x434F5C92L,0xB04E6E5BL,0x434F5C92L,(-1L),1L},{0x7F9E8CB7L,1L,(-1L),(-1L),0x6E250F0AL,0xFE241617L,0x9A016368L,0x9A016368L},{(-1L),0x7F9E8CB7L,0x2E4611A0L,0x2E4611A0L,0x7F9E8CB7L,(-1L),0x9A016368L,0xB04E6E5BL},{0xE3CAABA5L,0x2E4611A0L,(-1L),0xFE241617L,(-1L),0x5CCF24D9L,(-1L),0xFE241617L},{(-1L),0x5CCF24D9L,(-1L),0xFE241617L,(-1L),0x2E4611A0L,0xE3CAABA5L,0xB04E6E5BL},{0x9A016368L,(-1L),0x7F9E8CB7L,0x2E4611A0L,0x2E4611A0L,0x7F9E8CB7L,(-1L),0x9A016368L},{0x9A016368L,0xFE241617L,0x6E250F0AL,(-1L),(-1L),1L,0x7F9E8CB7L,1L},{(-1L),0x434F5C92L,0xB04E6E5BL,0x434F5C92L,(-1L),1L,0x5CCF24D9L,(-1L)},{0xE3CAABA5L,0xFE241617L,0x434F5C92L,1L,0x7F9E8CB7L,0x7F9E8CB7L,1L,0x434F5C92L}};
    int8_t ****l_5015 = &g_3958;
    int32_t l_5020 = 9L;
    uint32_t l_5042 = 0UL;
    int32_t *** const *l_5046 = &g_2476;
    int32_t *** const **l_5045[5];
    uint64_t l_5092 = 0x0F0F09F77B9AF894LL;
    uint16_t * const ***l_5102 = (void*)0;
    uint16_t *l_5108 = &g_3768;
    uint16_t **l_5107 = &l_5108;
    const int32_t l_5111 = 0xF8926C54L;
    const uint32_t **l_5158 = &g_1407;
    const uint32_t ***l_5157 = &l_5158;
    struct S0 *l_5159 = (void*)0;
    int64_t **l_5252 = &g_1835;
    const union U3 l_5256[9][7][1] = {{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}},{{{0}},{{0}},{{0}},{{0}},{{0}},{{0}},{{0}}}};
    const uint8_t l_5282 = 9UL;
    int32_t l_5300 = 0x3EDAAE60L;
    const struct S2 * const l_5301 = &g_5302;
    const struct S2 *l_5304 = &g_5305;
    const struct S2 **l_5303 = &l_5304;
    int i, j, k;
    for (i = 0; i < 5; i++)
        l_5045[i] = &l_5046;
lbl_5016:
    for (g_1553 = (-17); (g_1553 > (-4)); ++g_1553)
    {
        struct S1 l_4854[1][9] = {{{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309},{-105,1779,179,-12,1309}}};
        uint32_t ** const l_4865[5] = {&g_1962[1][5][3],&g_1962[1][5][3],&g_1962[1][5][3],&g_1962[1][5][3],&g_1962[1][5][3]};
        int32_t l_4868 = 1L;
        const int8_t *l_4878 = &g_53;
        int i, j;
        l_4854[0][1] = l_4853;
        l_4854[0][1].f4 = (safe_div_func_uint32_t_u_u(((((safe_rshift_func_int16_t_s_u((***g_248), 5)) ^ (safe_sub_func_uint8_t_u_u(p_15, (l_4854[0][1].f4 , (l_4853.f0 = (p_15 > 0L)))))) , ((safe_mul_func_int8_t_s_s((l_4865[4] == (*g_3753)), (safe_rshift_func_int16_t_s_s(l_4853.f2, 12)))) & ((*g_100) |= p_15))) ^ l_4868), 0xD386C450L));
        for (g_239 = 0; (g_239 <= 1); g_239 += 1)
        {
            int8_t **l_4884[4][2] = {{&g_52,&g_52},{&g_52,&g_52},{&g_52,&g_52},{&g_52,&g_52}};
            int32_t l_4892 = (-2L);
            int i, j;
            for (g_1824 = 0; (g_1824 <= 1); g_1824 += 1)
            {
                uint64_t l_4894[10][4][1] = {{{0xE46CB4B43DA27B69LL},{0x06B7BA052BDA17D4LL},{0x06B7BA052BDA17D4LL},{0xE46CB4B43DA27B69LL}},{{0x7449C3DC273E4448LL},{0x7A1EBA270D13C405LL},{0x7449C3DC273E4448LL},{0xE46CB4B43DA27B69LL}},{{0x06B7BA052BDA17D4LL},{0x06B7BA052BDA17D4LL},{0xE46CB4B43DA27B69LL},{0x7449C3DC273E4448LL}},{{0x7A1EBA270D13C405LL},{0x7449C3DC273E4448LL},{0xE46CB4B43DA27B69LL},{0x06B7BA052BDA17D4LL}},{{0x06B7BA052BDA17D4LL},{0xE46CB4B43DA27B69LL},{0x7449C3DC273E4448LL},{0x7A1EBA270D13C405LL}},{{0x7449C3DC273E4448LL},{0xE46CB4B43DA27B69LL},{0x06B7BA052BDA17D4LL},{0x06B7BA052BDA17D4LL}},{{0xE46CB4B43DA27B69LL},{0x7449C3DC273E4448LL},{0x7A1EBA270D13C405LL},{0x7449C3DC273E4448LL}},{{0xE46CB4B43DA27B69LL},{0x06B7BA052BDA17D4LL},{0x06B7BA052BDA17D4LL},{0xE46CB4B43DA27B69LL}},{{0x7449C3DC273E4448LL},{0x7A1EBA270D13C405LL},{0x7449C3DC273E4448LL},{0xE46CB4B43DA27B69LL}},{{0x06B7BA052BDA17D4LL},{0x06B7BA052BDA17D4LL},{0xE46CB4B43DA27B69LL},{0x7449C3DC273E4448LL}}};
                int i, j, k;
                for (g_3055 = 1; (g_3055 >= 0); g_3055 -= 1)
                {
                    int32_t l_4893 = (-7L);
                    int i, j;
                    if (((safe_rshift_func_int8_t_s_u(g_236[(g_3055 + 1)][g_3055], 5)) && ((((safe_sub_func_uint64_t_u_u((((safe_rshift_func_uint16_t_u_u((safe_lshift_func_uint8_t_u_s((((void*)0 != l_4877) && (*g_4556)), ((l_4878 == (void*)0) >= ((!(safe_rshift_func_uint16_t_u_u(((p_15 <= ((void*)0 == l_4884[3][1])) | (****g_2283)), 3))) , l_4854[0][1].f4)))), 6)) , 8L) , p_15), (*g_1835))) >= (*g_1617)) || (*g_4556)) > 0UL)))
                    {
                        const int32_t **l_4886 = &l_4885;
                        int32_t *l_4887 = &g_1853;
                        int32_t *l_4888 = &g_266[4][7];
                        int32_t *l_4889 = &g_266[4][7];
                        int32_t *l_4890 = (void*)0;
                        int32_t *l_4891[8] = {(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0};
                        int i;
                        (*l_4886) = l_4885;
                        ++l_4894[1][3][0];
                    }
                    else
                    {
                        if (p_15)
                            break;
                    }
                }
            }
            for (g_2353 = 0; (g_2353 <= 1); g_2353 += 1)
            {
                for (g_82 = 0; (g_82 <= 1); g_82 += 1)
                {
                    int i, j;
                    return g_236[(g_239 + 1)][g_239];
                }
            }
            (*g_100) |= (safe_mul_func_uint8_t_u_u(((safe_rshift_func_int16_t_s_s(0xAE2BL, (((((((*g_1835) = (safe_add_func_int16_t_s_s((18446744073709551615UL == p_15), 1UL))) != (((safe_add_func_int16_t_s_s(((safe_sub_func_int64_t_s_s(((((*g_52) | 0xCAL) <= (safe_lshift_func_int16_t_s_s(0x609DL, (((safe_add_func_int64_t_s_s(g_2077, 0x52334E5AC6DDDFF4LL)) > l_4892) && p_15)))) & p_15), 0x5BEF2BA9558AC5C9LL)) & (-3L)), (****g_2283))) < (*l_4885)) <= (-7L))) ^ l_4854[0][1].f1) , l_4854[0][1].f3) && 1L) || p_15))) || (*g_4556)), (**g_3959)));
        }
    }
    if ((*l_4885))
    {
        uint64_t ****l_4914[2];
        uint64_t *****l_4913 = &l_4914[0];
        int32_t l_4918 = 0x639A27D3L;
        int32_t *l_4919 = &g_1204;
        int32_t *l_4920 = (void*)0;
        int32_t *l_4921 = &g_3715;
        int32_t *l_4922[3][10] = {{(void*)0,&g_1853,&g_1853,(void*)0,&g_82,&g_3844,&g_82,(void*)0,&g_1853,&g_1853},{&g_82,&g_1853,&g_266[0][5],&g_117,&g_117,&g_266[0][5],&g_1853,&g_82,&g_1853,&g_266[0][5]},{&g_3844,(void*)0,&g_117,(void*)0,&g_3844,&g_266[0][5],&g_266[0][5],&g_3844,(void*)0,&g_117}};
        uint32_t l_4942 = 0xBDE2ABB4L;
        int i, j;
        for (i = 0; i < 2; i++)
            l_4914[i] = (void*)0;
        (*g_100) &= (&g_3080 == (l_4915 = l_4913));
        ++l_4942;
        (*g_100) = p_15;
        (*g_1092) = (*g_1092);
    }
    else
    {
        int32_t l_4945[3];
        uint64_t ***l_4948 = &g_1616;
        struct S1 l_4955 = {211,4551,74,12,-2297};
        uint64_t l_4962 = 0xA229F96BFD943087LL;
        uint8_t l_4966 = 0UL;
        int32_t l_5069 = 0L;
        int32_t l_5070 = 0xCCC712C4L;
        uint64_t l_5071 = 9UL;
        int8_t ***l_5103 = (void*)0;
        uint16_t *l_5106 = &g_2450;
        int8_t l_5120 = 0xDEL;
        uint8_t l_5124 = 1UL;
        union U3 **l_5136 = &g_458;
        int64_t l_5186[3];
        int32_t l_5232 = 5L;
        union U3 **l_5297[8] = {&g_458,&g_458,(void*)0,&g_458,&g_458,(void*)0,&g_458,&g_458};
        int i;
        for (i = 0; i < 3; i++)
            l_4945[i] = 1L;
        for (i = 0; i < 3; i++)
            l_5186[i] = 1L;
lbl_5150:
        if (((*l_4885) ^ l_4945[1]))
        {
            uint64_t * const * const **l_4951 = &g_4949;
            uint32_t *l_4954 = &g_978[0][7][1];
            int32_t l_4961 = 1L;
            struct S1 *l_4993 = (void*)0;
            int32_t l_5028 = 1L;
            int8_t *l_5041 = (void*)0;
            int32_t ****l_5044 = &g_2476;
            int32_t *****l_5043 = &l_5044;
            int64_t l_5047 = 4L;
            (*g_100) = (((safe_div_func_uint16_t_u_u(p_15, (((l_4945[0] != (***g_3958)) == ((p_15 , l_4948) != ((*l_4951) = g_4949))) ^ (safe_lshift_func_uint16_t_u_u((l_4954 != l_4954), 9))))) , (**g_2378)) == (void*)0);
            l_4853 = l_4955;
            for (g_2605 = (-15); (g_2605 != 6); g_2605 = safe_add_func_int64_t_s_s(g_2605, 3))
            {
                uint8_t l_4976 = 6UL;
                int32_t l_5026 = 9L;
                int32_t l_5029[3];
                uint16_t l_5030 = 0x8A0EL;
                int i;
                for (i = 0; i < 3; i++)
                    l_5029[i] = 0xD3FC94B0L;
                for (l_4931 = 0; (l_4931 <= 6); l_4931 += 1)
                {
                    int16_t l_4960 = 0x9F63L;
                    int32_t l_4965 = 0xEF905FF6L;
                    for (g_58 = 0; (g_58 <= 6); g_58 += 1)
                    {
                        int32_t *l_4958 = &l_4927;
                        int32_t *l_4959[8][5][6] = {{{&l_4926,(void*)0,(void*)0,&g_266[2][6],&g_266[2][6],(void*)0},{&l_4937,&l_4937,&g_3715,&l_4938,&l_4941[9][1],&g_1853},{&g_1853,&l_4934,&g_117,(void*)0,(void*)0,&g_3715},{(void*)0,&g_1853,&g_117,&l_4923,&l_4937,&g_1853},{&g_266[4][7],&l_4923,&g_3715,(void*)0,&l_4932,(void*)0}},{{(void*)0,&l_4932,(void*)0,&g_266[4][7],&l_4929,(void*)0},{&g_82,&l_4929,(void*)0,&l_4934,(void*)0,(void*)0},{(void*)0,&g_3844,&l_4923,&l_4925,&l_4929,&l_4925},{&l_4936,(void*)0,&l_4936,&l_4936,&l_4926,&l_4929},{&g_3844,(void*)0,(void*)0,(void*)0,&l_4929,&g_266[4][7]}},{{&g_1853,&g_266[4][7],&g_1853,(void*)0,&g_117,(void*)0},{&g_3844,(void*)0,&g_266[4][7],(void*)0,&g_117,&l_4936},{&l_4923,&l_4932,&g_3844,&g_3844,&l_4937,(void*)0},{&g_266[2][6],&l_4941[8][7],&l_4929,&l_4936,(void*)0,&l_4925},{(void*)0,(void*)0,&l_4941[9][1],&g_117,&l_4938,&l_4938}},{{&g_266[4][7],&g_3844,&g_3844,&g_266[4][7],&g_1853,(void*)0},{&g_3844,&l_4929,&g_117,&l_4941[8][1],(void*)0,&g_1853},{(void*)0,(void*)0,&l_4929,&g_3844,(void*)0,&g_3715},{&g_3844,&l_4929,&l_4929,&l_4926,&g_1853,&l_4934},{(void*)0,&g_3844,&g_1853,&l_4929,&l_4938,&l_4936}},{{&l_4936,(void*)0,&l_4932,(void*)0,(void*)0,&l_4929},{(void*)0,&l_4941[8][7],&g_117,&l_4929,&l_4937,&g_3844},{&g_117,&l_4932,&l_4935,&l_4932,&g_117,&l_4923},{(void*)0,(void*)0,&l_4936,&l_4937,&g_117,&g_1853},{&l_4926,&g_266[4][7],(void*)0,(void*)0,&l_4929,&g_1853}},{{&l_4935,(void*)0,&l_4936,&l_4941[9][1],&l_4936,&l_4923},{&l_4929,(void*)0,&l_4935,&g_266[2][6],&l_4923,&g_3844},{&g_1853,&l_4925,&g_117,&g_1853,&g_266[2][6],&l_4929},{(void*)0,&l_4923,&l_4932,&g_3844,&g_1853,&l_4936},{&l_4934,&l_4929,&g_1853,&g_1853,&l_4929,&l_4934}},{{&l_4941[8][7],&l_4941[8][1],&l_4929,(void*)0,(void*)0,&g_3715},{&l_4925,&g_3844,&l_4929,&g_117,(void*)0,&g_1853},{&l_4925,&l_4936,&g_117,(void*)0,&l_4935,(void*)0},{&l_4941[8][7],(void*)0,&g_3844,&g_1853,&l_4929,&l_4938},{&l_4934,(void*)0,&l_4941[9][1],&g_3844,(void*)0,&l_4925}},{{(void*)0,&l_4941[9][1],&l_4929,&g_1853,&g_82,(void*)0},{&g_1853,&l_4936,&g_3844,&g_266[2][6],&g_3844,&l_4936},{&l_4929,&l_4935,&g_266[4][7],&l_4941[9][1],(void*)0,(void*)0},{&l_4935,&g_3844,&g_1853,(void*)0,&l_4934,&g_266[4][7]},{&l_4926,&g_3844,(void*)0,&l_4937,(void*)0,&l_4929}}};
                        int i, j, k;
                        if (p_15)
                            break;
                        (**g_2379) = &l_4853;
                        --l_4962;
                        l_4966--;
                    }
                }
                for (l_4928 = 8; (l_4928 >= (-23)); l_4928 = safe_sub_func_uint64_t_u_u(l_4928, 3))
                {
                    int32_t l_4977 = (-1L);
                    struct S1 * const *l_4983 = (void*)0;
                    struct S1 * const **l_4982[4][6][4] = {{{&l_4983,&l_4983,&l_4983,(void*)0},{(void*)0,&l_4983,&l_4983,&l_4983},{(void*)0,&l_4983,&l_4983,(void*)0},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983}},{{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,(void*)0,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{(void*)0,&l_4983,&l_4983,&l_4983}},{{(void*)0,&l_4983,&l_4983,&l_4983},{&l_4983,(void*)0,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983}},{{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,(void*)0},{(void*)0,&l_4983,&l_4983,&l_4983},{(void*)0,&l_4983,&l_4983,(void*)0},{&l_4983,&l_4983,&l_4983,&l_4983},{&l_4983,&l_4983,&l_4983,&l_4983}}};
                    int8_t l_4990 = (-1L);
                    struct S1 **l_4994 = &g_258;
                    struct S1 **l_4995 = (void*)0;
                    struct S1 **l_4996 = (void*)0;
                    struct S1 **l_4997 = &g_258;
                    struct S1 **l_4998[7];
                    int8_t ****l_5014[5] = {(void*)0,(void*)0,(void*)0,(void*)0,(void*)0};
                    int8_t *****l_5013[5] = {&l_5014[3],&l_5014[3],&l_5014[3],&l_5014[3],&l_5014[3]};
                    int32_t l_5027 = 0xD1096411L;
                    int i, j, k;
                    for (i = 0; i < 7; i++)
                        l_4998[i] = &g_258;
                    (*g_100) = (safe_add_func_uint32_t_u_u(((-10L) <= (safe_lshift_func_int16_t_s_s((safe_unary_minus_func_uint64_t_u((l_4976 & l_4977))), ((***g_248) = (safe_add_func_int64_t_s_s(((safe_sub_func_uint32_t_u_u((l_4955.f2 || (((g_4984 = (**g_2378)) == (void*)0) > (safe_sub_func_uint16_t_u_u((l_4955.f3 , (safe_add_func_uint64_t_u_u(p_15, (((l_4990 == 0xA25B10B3DED55DDALL) , p_15) , (-10L))))), l_4961)))), l_4955.f3)) && (*g_499)), p_15)))))), l_4966));
                    if ((safe_div_func_int8_t_s_s(((l_4976 > (1UL != ((((0xC651L ^ l_4990) , l_4993) == (g_4999[1][5][0] = l_4993)) | ((***g_248) = l_4977)))) | ((safe_mod_func_int64_t_s_s((safe_mod_func_uint64_t_u_u((((safe_div_func_uint64_t_u_u(((+0x8AE6L) && (safe_mod_func_uint8_t_u_u(((l_5015 = (((safe_add_func_uint64_t_u_u((safe_rshift_func_int16_t_s_u(l_4955.f0, l_4976)), l_4961)) && p_15) , (void*)0)) == &g_3958), l_4976))), 6L)) >= l_4961) , p_15), 0x26E86FDBDC574557LL)), 0xB7D483662B88CFA5LL)) && l_4955.f1)), (**g_2196))))
                    {
                        int32_t *l_5017 = &l_4936;
                        int32_t **l_5018 = &g_100;
                        if (g_2077)
                            goto lbl_5016;
                        (*g_100) &= ((l_4977 = (l_4853 , g_2234[3])) , l_4961);
                        (*l_5018) = l_5017;
                    }
                    else
                    {
                        struct S1 l_5019 = {-248,5331,157,9,381};
                        int32_t *l_5021 = &g_266[4][5];
                        int32_t *l_5022 = &l_4924;
                        int32_t *l_5023 = (void*)0;
                        int32_t *l_5024 = &l_4977;
                        int32_t *l_5025[7][8] = {{&l_4938,&l_4940,&l_4933,&l_4938,&l_4927,&g_1204,&l_4940,&l_4940},{&l_4926,&l_4927,&l_4924,&l_4924,&l_4927,&l_4926,&l_4929,&l_4961},{&l_4938,&l_4929,&g_1853,&l_4940,&l_4926,&g_1853,&l_4927,&g_1853},{(void*)0,&l_4940,&l_4939,&l_4940,(void*)0,&l_4932,&l_4940,&l_4961},{&g_82,(void*)0,&l_4926,&l_4924,&l_4940,&g_82,&g_82,&l_4940},{&l_4938,&l_4926,&l_4926,&l_4938,&l_4929,&g_1853,&l_4940,&l_4926},{&l_4940,&l_4927,&l_4939,&l_4961,&l_4927,&l_4933,&l_4927,&l_4961}};
                        int i, j;
                        l_4955 = l_5019;
                        if (p_15)
                            break;
                        l_5030++;
                    }
                }
            }
            (*g_100) = (safe_sub_func_int8_t_s_s((l_4955.f3 = p_15), (((((safe_lshift_func_uint16_t_u_u(p_15, (safe_div_func_int8_t_s_s((*g_52), ((((((safe_lshift_func_uint8_t_u_u(246UL, 1)) < ((((l_4853 , (*l_4885)) , (*g_499)) , l_5043) == l_5045[0])) && l_5047) != 0xE8E3L) > 0xD821L) ^ 0x7CL))))) ^ 0UL) == p_15) != (*g_52)) | 0xC4L)));
        }
        else
        {
            int16_t l_5058 = 5L;
            int32_t l_5068[7][6] = {{0x81177F41L,0x93CE56C7L,0xAE3FB27AL,0x258D5C62L,0x93CE56C7L,0x258D5C62L},{0x81177F41L,(-1L),0x81177F41L,0x258D5C62L,(-1L),0xAE3FB27AL},{0x81177F41L,1L,0x258D5C62L,0x258D5C62L,1L,0x81177F41L},{0x81177F41L,0x93CE56C7L,0xAE3FB27AL,0x258D5C62L,0x93CE56C7L,0x258D5C62L},{0x81177F41L,(-1L),0x81177F41L,0x258D5C62L,(-1L),0xAE3FB27AL},{0x81177F41L,1L,0L,0L,0x258D5C62L,0x9CE8B640L},{0x9CE8B640L,0x81177F41L,0x492DED8AL,0L,0x81177F41L,0L}};
            const int32_t **l_5085 = (void*)0;
            const int32_t ***l_5084 = &l_5085;
            uint64_t *l_5093 = &l_4962;
            int i, j;
            if ((safe_rshift_func_int8_t_s_u((***g_3958), 0)))
            {
                int64_t l_5067 = (-3L);
                l_4955 = (((p_15 | (safe_mod_func_int16_t_s_s(0L, (safe_add_func_int8_t_s_s((***g_3958), (l_5068[2][4] = ((safe_rshift_func_uint16_t_u_s(0x294FL, 2)) <= (safe_lshift_func_uint16_t_u_s((l_5058 , (((safe_mod_func_uint32_t_u_u(l_5058, (safe_add_func_int32_t_s_s(((safe_div_func_uint16_t_u_u(((l_4966 < ((4L ^ (safe_lshift_func_int8_t_s_s(l_5067, 1))) != p_15)) != l_5067), l_5058)) <= (*l_4885)), l_4962)))) <= p_15) || 0x754F5382L)), 14))))))))) ^ (*g_1835)) , l_4955);
            }
            else
            {
                l_5068[4][2] |= (-1L);
                return p_15;
            }
            ++l_5071;
            (*g_100) ^= ((safe_sub_func_int32_t_s_s(l_4945[1], l_5071)) && (safe_lshift_func_int8_t_s_u((((**g_249) | (safe_div_func_uint8_t_u_u((((safe_add_func_uint64_t_u_u(((*l_5093) ^= (l_4955.f4 &= (((((safe_rshift_func_int8_t_s_u(p_15, (*l_4885))) , l_5084) == (void*)0) < (((((!(safe_mul_func_uint8_t_u_u((~(4L & ((*g_4556) = (((((safe_mod_func_uint64_t_u_u(((l_5058 & 1L) != 0xA1C2L), p_15)) | p_15) & l_5092) >= 8L) | 1UL)))), 255UL))) & l_5068[2][4]) <= (*l_4885)) == (*g_250)) == 0xC1E6L)) <= (*l_4885)))), l_4966)) < p_15) <= 65535UL), p_15))) >= p_15), (**g_2196))));
        }
        if ((safe_mul_func_uint8_t_u_u(3UL, (p_15 | ((((0x972D930DCE28D0EELL <= ((((p_15 <= (~(safe_div_func_uint64_t_u_u(l_5069, ((****l_4916) = ((l_5070 & ((safe_mul_func_uint16_t_u_u(((!p_15) , (((void*)0 != l_5102) && (***g_4949))), 0x90FDL)) && p_15)) , p_15)))))) | p_15) , (void*)0) == l_5103)) && l_4966) , p_15) ^ 0x3E0EL)))))
        {
            const uint16_t *** const *l_5109 = &g_4052[2];
            uint16_t *****l_5110 = &g_3527;
            int32_t l_5115 = 0x6126BD8EL;
            int32_t l_5116[4][8] = {{0xFB5D0F7DL,0x92618B4FL,0xE75CCD62L,4L,0xAC2E8B34L,0x31405C95L,0xAC2E8B34L,4L},{0xAC2E8B34L,0x31405C95L,0xAC2E8B34L,4L,0xE75CCD62L,0x92618B4FL,0xFB5D0F7DL,(-3L)},{0x807C62F0L,0xE75CCD62L,0x6E30A9BAL,0x92618B4FL,0x92618B4FL,0x6E30A9BAL,0xE75CCD62L,0x807C62F0L},{0x807C62F0L,4L,(-3L),0xAC2E8B34L,0xE75CCD62L,0x90CC599FL,0x6E30A9BAL,0x90CC599FL}};
            int32_t l_5118 = 0x22B6C2F9L;
            int64_t l_5119 = 0x55E71AF716D3BE43LL;
            const struct S1 l_5134 = {109,3898,143,2,41};
            union U3 **l_5140 = &g_458;
            int i, j;
            if ((safe_add_func_uint32_t_u_u(((*g_1356) = (((l_5106 != (void*)0) , l_5107) != (void*)0)), (((*g_499) , (((l_4955.f2 > (((*l_4885) , ((l_5109 != ((*l_5110) = (void*)0)) | 65535UL)) == l_5111)) , g_5112) <= (-1L))) >= p_15))))
            {
                int64_t l_5113 = 0xE956F82B264349E0LL;
                int32_t l_5114 = 8L;
                int32_t l_5117[10] = {(-8L),0x4B6E68F0L,0x4B6E68F0L,(-8L),0xD866284AL,(-8L),0x4B6E68F0L,0x4B6E68F0L,(-8L),0xD866284AL};
                uint32_t l_5121 = 18446744073709551609UL;
                uint64_t ****l_5133 = &l_4948;
                union U3 **l_5137 = &g_458;
                union U3 ***l_5138[10][5][5] = {{{&l_5137,(void*)0,(void*)0,&l_5137,&l_5136},{&l_5137,(void*)0,&l_5137,&l_5136,&l_5137},{&l_5137,&l_5137,&l_5137,&l_5136,&l_5137},{(void*)0,&l_5137,&l_5136,(void*)0,&l_5137},{(void*)0,&l_5137,&l_5136,&l_5137,&l_5137}},{{&l_5137,&l_5137,&l_5137,&l_5137,&l_5137},{&l_5136,&l_5137,&l_5136,&l_5136,&l_5136},{&l_5136,(void*)0,&l_5136,&l_5137,(void*)0},{&l_5136,(void*)0,&l_5136,&l_5136,&l_5137},{&l_5137,&l_5137,&l_5136,(void*)0,&l_5136}},{{(void*)0,&l_5136,&l_5136,(void*)0,&l_5136},{&l_5137,&l_5136,&l_5137,&l_5136,&l_5136},{&l_5136,&l_5136,&l_5136,(void*)0,&l_5136},{&l_5137,&l_5137,&l_5136,&l_5136,&l_5136},{&l_5136,(void*)0,&l_5137,(void*)0,&l_5136}},{{&l_5136,&l_5137,&l_5137,(void*)0,&l_5137},{&l_5137,&l_5136,(void*)0,&l_5136,(void*)0},{&l_5137,&l_5137,&l_5136,&l_5137,&l_5137},{&l_5137,&l_5136,&l_5136,&l_5136,&l_5136},{&l_5137,&l_5136,&l_5136,&l_5137,&l_5136}},{{&l_5136,&l_5137,&l_5137,&l_5137,&l_5136},{(void*)0,&l_5137,&l_5136,(void*)0,&l_5136},{&l_5136,&l_5137,&l_5136,&l_5136,&l_5136},{&l_5137,&l_5137,&l_5136,&l_5136,&l_5136},{&l_5137,&l_5136,&l_5136,&l_5137,&l_5137}},{{&l_5137,(void*)0,&l_5136,&l_5136,(void*)0},{&l_5137,&l_5136,&l_5136,&l_5136,&l_5136},{&l_5136,&l_5137,&l_5136,&l_5136,&l_5137},{&l_5136,&l_5137,&l_5136,&l_5136,&l_5136},{(void*)0,&l_5136,&l_5136,&l_5136,&l_5137}},{{&l_5137,(void*)0,&l_5136,&l_5136,&l_5137},{&l_5136,&l_5137,&l_5137,&l_5136,&l_5136},{&l_5136,&l_5136,&l_5136,(void*)0,&l_5136},{&l_5136,(void*)0,&l_5136,&l_5136,&l_5137},{&l_5137,&l_5137,(void*)0,(void*)0,&l_5136}},{{&l_5136,&l_5136,&l_5137,&l_5136,&l_5136},{(void*)0,&l_5136,&l_5137,&l_5136,(void*)0},{&l_5137,&l_5136,&l_5136,&l_5136,(void*)0},{&l_5136,&l_5137,&l_5136,&l_5136,(void*)0},{&l_5136,&l_5136,&l_5136,&l_5136,&l_5136}},{{(void*)0,(void*)0,&l_5137,&l_5136,&l_5136},{(void*)0,&l_5137,&l_5136,&l_5136,&l_5137},{&l_5136,&l_5136,&l_5136,&l_5137,&l_5136},{(void*)0,&l_5136,&l_5136,&l_5136,&l_5136},{(void*)0,(void*)0,&l_5137,&l_5136,&l_5137}},{{&l_5136,(void*)0,&l_5137,&l_5136,&l_5137},{&l_5136,&l_5136,&l_5137,(void*)0,&l_5136},{&l_5137,(void*)0,&l_5137,&l_5137,&l_5137},{(void*)0,(void*)0,(void*)0,&l_5137,&l_5136},{&l_5136,&l_5136,&l_5136,&l_5136,&l_5137}}};
                uint64_t ****l_5142 = &l_4948;
                uint64_t *****l_5141 = &l_5142;
                uint8_t *l_5143 = &g_3092;
                uint16_t *l_5147 = &g_2427;
                int i, j, k;
                ++l_5121;
                l_5124--;
                if (((safe_sub_func_int16_t_s_s((safe_add_func_uint32_t_u_u(l_5116[0][5], p_15)), p_15)) <= ((*l_5143) ^= ((l_5116[3][1] | (((safe_sub_func_int16_t_s_s(((((*l_4915) = l_5133) != ((*l_5141) = (l_5134 , (((!l_5113) < (l_5136 != (l_5140 = (g_5139 = l_5137)))) , &l_4948)))) >= l_5121), l_5134.f0)) && (-1L)) <= (*g_499))) && (***g_4949)))))
                {
                    return l_5116[0][5];
                }
                else
                {
                    uint32_t l_5144 = 0x829E5D3CL;
                    (****g_2737) = (*g_1092);
                    (*g_100) &= ((l_5144 = 0x5D70L) | l_5117[3]);
                }
                (*g_100) |= ((p_15 & (safe_mod_func_int32_t_s_s(p_15, 0xA143EBA9L))) < ((&p_15 != (l_5147 = (*l_5107))) & p_15));
            }
            else
            {
                (*g_100) = (-2L);
                for (l_4926 = 0; (l_4926 >= 17); l_4926 = safe_add_func_uint16_t_u_u(l_4926, 2))
                {
                    return (*l_4885);
                }
                if (g_1824)
                    goto lbl_5150;
            }
        }
        else
        {
            uint8_t l_5156 = 7UL;
            uint16_t *l_5174 = &g_2453;
            int32_t l_5179 = 0x1890744CL;
            int32_t l_5183 = (-1L);
            int32_t l_5199 = 0xB8986DA5L;
            uint32_t *l_5216 = &l_5042;
            const struct S0 *l_5225[6];
            int i;
            for (i = 0; i < 6; i++)
                l_5225[i] = &g_5226;
            if ((+(safe_sub_func_int8_t_s_s((safe_rshift_func_int16_t_s_u(l_5156, ((void*)0 == l_5157))), ((l_4955 , l_5159) == (void*)0)))))
            {
                uint8_t l_5170 = 255UL;
                struct S1 l_5171 = {339,4961,101,-9,316};
                int16_t *l_5175 = &g_1553;
                int32_t *l_5178 = &l_4927;
                l_4955.f0 = (((safe_rshift_func_uint16_t_u_u((p_15++), 5)) <= ((safe_lshift_func_uint16_t_u_u(((safe_mul_func_uint16_t_u_u((safe_div_func_int8_t_s_s(1L, 0xAFL)), l_5170)) == (l_5171 , l_4955.f4)), 12)) || (safe_add_func_int32_t_s_s(((((l_5156 , l_5174) == ((*l_5107) = (void*)0)) >= ((*l_5175) = ((***g_248) = (*g_250)))) ^ l_5156), l_5171.f4)))) || (-1L));
                for (g_82 = 0; (g_82 < 17); ++g_82)
                {
                    if (((void*)0 == l_5178))
                    {
                        return l_5120;
                    }
                    else
                    {
                        l_5179 ^= p_15;
                    }
                    for (l_5042 = 0; (l_5042 <= 51); l_5042 = safe_add_func_int32_t_s_s(l_5042, 8))
                    {
                        union U3 *l_5182 = &g_49;
                        (*g_5139) = l_5182;
                    }
                }
                if (l_4966)
                    goto lbl_5299;
            }
            else
            {
                int32_t l_5184 = 0x08D1A861L;
                int32_t l_5185 = 1L;
                int32_t l_5187 = 0x00981BB6L;
                uint16_t l_5188[3][1];
                int32_t l_5201[7][8] = {{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)},{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)},{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)},{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)},{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)},{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)},{0x65D2587AL,0x65D2587AL,(-6L),(-6L),0x65D2587AL,0x65D2587AL,(-6L),(-6L)}};
                int i, j;
                for (i = 0; i < 3; i++)
                {
                    for (j = 0; j < 1; j++)
                        l_5188[i][j] = 0x57A6L;
                }
lbl_5224:
                l_5188[0][0]++;
                for (g_239 = 9; (g_239 <= 47); g_239 = safe_add_func_uint8_t_u_u(g_239, 2))
                {
                    uint8_t l_5193 = 0xD0L;
                    int32_t l_5198 = 0x062FF62BL;
                    uint16_t l_5204[2][7][8] = {{{0xA1D3L,0xA0D2L,1UL,65535UL,0x6BFBL,1UL,65535UL,65535UL},{65535UL,0x6BFBL,0xD772L,5UL,65532UL,0x9CBFL,65526UL,0UL},{65534UL,0xA1D3L,65535UL,65529UL,0x8360L,65532UL,9UL,65535UL},{65532UL,0xD772L,65535UL,0x9CBFL,65535UL,0x9CBFL,65535UL,0xD772L},{5UL,65529UL,1UL,0x2E64L,3UL,0UL,0xF26DL,1UL},{65534UL,1UL,0x85CAL,65529UL,5UL,1UL,0xF26DL,65535UL},{0x9CBFL,65529UL,1UL,0UL,0UL,0UL,65535UL,5UL}},{{0UL,0UL,65535UL,5UL,2UL,9UL,0xA713L,1UL},{65535UL,65535UL,0x1A40L,65535UL,0xA0D2L,0x8360L,0x8360L,0xA0D2L},{0xA713L,0x9CBFL,0x9CBFL,0xA713L,1UL,0xA1D3L,2UL,65526UL},{0xAC85L,0x8360L,65526UL,0x20D4L,65535UL,0UL,1UL,0UL},{0xD772L,0x8360L,1UL,0x9864L,65529UL,0xA1D3L,1UL,0UL},{65528UL,0x9CBFL,0xA0D2L,0x9FC6L,0UL,0x8360L,65535UL,0xA713L},{0xA1D3L,65535UL,65529UL,0x8360L,65532UL,9UL,0x9FC6L,1UL}}};
                    int64_t **l_5223 = &g_1835;
                    int64_t ** const *l_5222 = &l_5223;
                    int64_t ** const ** const l_5221 = &l_5222;
                    int i, j, k;
                    l_5193++;
                    for (g_16 = 0; (g_16 != 24); g_16 = safe_add_func_int32_t_s_s(g_16, 4))
                    {
                        int32_t l_5200 = 0x18C8522DL;
                        int32_t l_5202 = 0xE1BC49F8L;
                        int32_t l_5203 = 1L;
                        ++l_5204[1][6][0];
                        (*g_100) ^= (l_5198 = 2L);
                        if (l_5187)
                            continue;
                    }
                    if (l_5193)
                        break;
                    for (g_3900 = (-6); (g_3900 > 6); ++g_3900)
                    {
                        int8_t l_5210[3];
                        int i;
                        for (i = 0; i < 3; i++)
                            l_5210[i] = 0x8CL;
                        (*g_100) = ((((!0L) != l_5210[0]) , (***g_3958)) || (((((safe_sub_func_uint8_t_u_u((~(*g_100)), l_5187)) == (safe_div_func_int64_t_s_s((((l_5216 != (void*)0) != (safe_lshift_func_int16_t_s_u(l_5183, 7))) <= (((safe_lshift_func_int16_t_s_s(l_5187, p_15)) , (void*)0) == l_5221)), l_5210[1]))) ^ 0x54E5DA44L) , (*g_100)) ^ p_15));
                        if (l_5124)
                            goto lbl_5224;
                    }
                }
                l_5225[2] = l_5225[2];
            }
            (*g_100) |= ((void*)0 == &g_2787);
        }
lbl_5299:
        for (g_1853 = 0; (g_1853 <= (-16)); g_1853 = safe_sub_func_uint64_t_u_u(g_1853, 4))
        {
            int64_t l_5231 = 7L;
            int32_t l_5233 = 0xB46BC5C9L;
            int32_t l_5236 = 0x8975AC91L;
            int32_t l_5238 = 0L;
            int32_t l_5246 = 0x83E54FCDL;
            int32_t l_5247 = 0x13C6DDC7L;
            struct S1 *l_5255 = &g_114;
            int16_t ** const *l_5286 = (void*)0;
            int16_t ** const **l_5285[7][2][6] = {{{&l_5286,(void*)0,&l_5286,&l_5286,&l_5286,(void*)0},{&l_5286,&l_5286,&l_5286,&l_5286,&l_5286,&l_5286}},{{&l_5286,&l_5286,&l_5286,&l_5286,&l_5286,&l_5286},{&l_5286,&l_5286,&l_5286,&l_5286,&l_5286,&l_5286}},{{(void*)0,&l_5286,&l_5286,(void*)0,&l_5286,&l_5286},{&l_5286,(void*)0,&l_5286,(void*)0,&l_5286,&l_5286}},{{(void*)0,&l_5286,&l_5286,&l_5286,&l_5286,(void*)0},{&l_5286,(void*)0,&l_5286,&l_5286,&l_5286,(void*)0}},{{&l_5286,&l_5286,&l_5286,&l_5286,&l_5286,&l_5286},{&l_5286,&l_5286,&l_5286,&l_5286,&l_5286,&l_5286}},{{&l_5286,&l_5286,&l_5286,&l_5286,&l_5286,&l_5286},{(void*)0,&l_5286,&l_5286,(void*)0,&l_5286,&l_5286}},{{&l_5286,(void*)0,&l_5286,(void*)0,&l_5286,&l_5286},{(void*)0,&l_5286,&l_5286,&l_5286,&l_5286,(void*)0}}};
            struct S1 ***l_5288 = &g_2380;
            union U3 **l_5298[10][2][4] = {{{&g_458,&g_458,(void*)0,&g_458},{&g_458,&g_458,&g_458,(void*)0}},{{(void*)0,(void*)0,&g_458,(void*)0},{&g_458,&g_458,&g_458,&g_458}},{{&g_458,&g_458,(void*)0,(void*)0},{&g_458,&g_458,(void*)0,&g_458}},{{&g_458,&g_458,(void*)0,(void*)0},{&g_458,&g_458,(void*)0,&g_458}},{{&g_458,&g_458,(void*)0,(void*)0},{(void*)0,(void*)0,&g_458,&g_458}},{{&g_458,(void*)0,&g_458,&g_458},{(void*)0,(void*)0,&g_458,&g_458}},{{&g_458,(void*)0,&g_458,&g_458},{(void*)0,(void*)0,&g_458,&g_458}},{{&g_458,(void*)0,&g_458,(void*)0},{&g_458,&g_458,&g_458,&g_458}},{{&g_458,&g_458,&g_458,(void*)0},{&g_458,&g_458,&g_458,&g_458}},{{&g_458,&g_458,&g_458,(void*)0},{&g_458,&g_458,&g_458,&g_458}}};
            int i, j, k;
            for (g_5112 = (-24); (g_5112 >= 9); g_5112 = safe_add_func_uint64_t_u_u(g_5112, 9))
            {
                int32_t l_5234 = (-10L);
                int32_t l_5235 = 0xDE6CF2CAL;
                int32_t l_5237 = (-1L);
                int32_t l_5239 = 0x7A4B30E8L;
                int32_t l_5240 = 0x96FFAFEFL;
                int32_t l_5241 = (-10L);
                int32_t l_5242 = 0x8A846E8FL;
                int32_t l_5243 = 0x6C666936L;
                int32_t l_5244 = 0x2E60472AL;
                int32_t l_5245 = 0L;
                g_5248[1][1][0]++;
            }
            l_4955.f2 &= (p_15 >= (~p_15));
            for (l_4937 = 6; (l_4937 >= 0); l_4937 -= 1)
            {
                int64_t ***l_5253 = &l_5252;
                struct S1 *l_5254[1];
                int32_t l_5284 = 0xFA481CC6L;
                int32_t *l_5289 = &l_5284;
                int i;
                for (i = 0; i < 1; i++)
                    l_5254[i] = &g_2981;
                (*l_5253) = l_5252;
                l_5255 = ((**g_2379) = l_5254[0]);
                for (l_4928 = 0; (l_4928 <= 4); l_4928 += 1)
                {
                    const uint8_t ***l_5265 = (void*)0;
                    const uint8_t ***l_5266[4] = {&g_2196,&g_2196,&g_2196,&g_2196};
                    int32_t *l_5280 = &g_2568;
                    int32_t **l_5279[7] = {&l_5280,&l_5280,&l_5280,&l_5280,&l_5280,&l_5280,&l_5280};
                    uint32_t **l_5291 = &g_1962[3][2][3];
                    int i, j, k;
                    if ((l_5256[5][2][0] , (safe_sub_func_int8_t_s_s((safe_div_func_int16_t_s_s(g_5248[l_4937][l_4928][l_4937], ((safe_mul_func_int8_t_s_s(g_5248[(l_4928 + 2)][l_4928][l_4928], ((safe_sub_func_int64_t_s_s(((g_2196 = &g_2197[0]) == ((safe_mul_func_int16_t_s_s((safe_sub_func_uint64_t_u_u((safe_rshift_func_uint16_t_u_s(((safe_mod_func_uint32_t_u_u(((((safe_lshift_func_int8_t_s_u((((safe_sub_func_int32_t_s_s((l_5279[3] != (void*)0), p_15)) >= p_15) | l_5246), 0)) , (*g_1835)) || p_15) , p_15), g_5281)) , l_5231), 5)), 1UL)), 0x74ECL)) , &g_2197[1])), 0x59FF4200ACEE3D93LL)) , l_5282))) | p_15))), p_15))))
                    {
                        uint64_t l_5283 = 0xC4B11AE101416728LL;
                        int64_t ***l_5293 = &l_5252;
                        int64_t ****l_5294 = &l_5293;
                        (*g_258) = func_31(p_15, (*g_3959), l_5283, l_5284, (((void*)0 != l_5285[0][1][0]) , ((safe_unary_minus_func_int32_t_s((p_15 & (*g_1835)))) && (((*g_2378) = l_5288) == (void*)0))));
                        l_5289 = &l_5236;
                        (*g_100) ^= ((((((+p_15) & (*l_5289)) >= l_5246) , l_4853) , ((((*g_1835) <= ((void*)0 != l_5291)) <= (~p_15)) == (((*l_5294) = l_5293) == g_5295))) , (*l_5289));
                        (*g_100) = (l_5297[6] == l_5298[1][0][3]);
                    }
                    else
                    {
                        (***g_2738) = (***g_2738);
                    }
                    (*l_5289) ^= 0L;
                }
            }
            (*g_100) = 0x67771B0FL;
        }
        return l_4955.f0;
    }
    (*g_100) |= l_5300;
    (*l_5303) = l_5301;
    return (**g_4950);
}







static uint32_t func_20(int8_t * p_21)
{
    int32_t *l_4838 = &g_3715;
    int32_t **l_4839 = (void*)0;
    int32_t **l_4840 = &g_100;
    struct S0 ***l_4845[9];
    int32_t l_4846[3];
    int8_t *l_4847 = &g_3562[1];
    const struct S1 l_4848 = {-104,1660,34,-10,-1285};
    struct S1 *l_4849 = &g_114;
    int i;
    for (i = 0; i < 9; i++)
        l_4845[i] = &g_2787;
    for (i = 0; i < 3; i++)
        l_4846[i] = 0L;
    (*l_4840) = l_4838;
    (*l_4849) = l_4848;
    return (**l_4840);
}







static uint16_t func_24(uint64_t p_25, int32_t p_26)
{
    union U3 **l_3587 = &g_458;
    int32_t l_3590 = 2L;
    const uint64_t *l_3634 = &g_2234[5];
    const uint64_t **l_3633[9][1] = {{&l_3634},{(void*)0},{&l_3634},{(void*)0},{&l_3634},{(void*)0},{&l_3634},{(void*)0},{&l_3634}};
    struct S1 l_3638 = {-364,1918,52,0,1062};
    uint8_t ** const l_3668 = &g_499;
    uint32_t **l_3689 = &g_1962[4][3][6];
    int32_t *l_3710 = &g_266[4][7];
    int32_t l_3716 = 0xCBBDBC3FL;
    int32_t l_3717[7];
    uint16_t * const l_3767 = &g_3768;
    uint16_t * const *l_3766 = &l_3767;
    uint16_t * const **l_3765 = &l_3766;
    int8_t *l_3834 = (void*)0;
    int32_t ** const **l_3851 = (void*)0;
    int32_t ** const ***l_3850 = &l_3851;
    union U3 ** const *l_3899 = (void*)0;
    union U3 ** const **l_3898 = &l_3899;
    uint8_t l_3903[4];
    struct S2 ** const l_3908 = &g_2747;
    uint32_t ****l_4003[5];
    uint32_t l_4022 = 4294967295UL;
    struct S1 l_4028 = {-52,3947,-152,14,-674};
    int32_t l_4062[1];
    uint8_t l_4112 = 1UL;
    int16_t l_4125 = 1L;
    uint32_t l_4131 = 0UL;
    struct S1 * const *l_4186 = (void*)0;
    uint32_t l_4265 = 0x0AC787F3L;
    uint32_t ** const *l_4280 = &l_3689;
    uint32_t ** const **l_4279 = &l_4280;
    int64_t **l_4344 = &g_1835;
    int64_t *** const l_4343 = &l_4344;
    uint64_t l_4358 = 1UL;
    struct S1 l_4376 = {471,1308,-211,-4,1632};
    uint64_t l_4378 = 0x2AB73A84520F04C2LL;
    int64_t * const l_4397 = &g_3636;
    uint8_t l_4408 = 253UL;
    struct S2 **l_4416 = &g_2747;
    int8_t l_4440 = 0x7FL;
    uint64_t l_4481 = 18446744073709551606UL;
    uint32_t l_4582 = 0x33685D55L;
    uint16_t l_4583 = 4UL;
    uint64_t * const *l_4596 = &g_4556;
    int32_t l_4624 = (-2L);
    uint8_t l_4686 = 255UL;
    struct S2 *l_4711 = &g_4712[4];
    struct S2 **l_4713 = &l_4711;
    struct S2 *l_4715 = &g_4716;
    struct S2 **l_4714 = &l_4715;
    struct S2 *l_4718 = &g_4719;
    struct S2 **l_4717 = &l_4718;
    int32_t l_4739 = (-4L);
    uint32_t l_4740 = 7UL;
    int32_t *l_4744[5];
    int64_t l_4745 = 0xA76410161A2CF42DLL;
    uint64_t l_4747 = 2UL;
    const int64_t *** const *l_4798 = (void*)0;
    int8_t *l_4836 = &g_3055;
    int32_t l_4837 = 1L;
    int i, j;
    for (i = 0; i < 7; i++)
        l_3717[i] = 0xB44F244FL;
    for (i = 0; i < 4; i++)
        l_3903[i] = 251UL;
    for (i = 0; i < 5; i++)
        l_4003[i] = &g_1927;
    for (i = 0; i < 1; i++)
        l_4062[i] = 0x524F4323L;
    for (i = 0; i < 5; i++)
        l_4744[i] = &g_117;
    for (g_2069 = 5; (g_2069 >= 0); g_2069 -= 1)
    {
        uint16_t l_3592[7][2] = {{65535UL,65535UL},{65535UL,0UL},{0xDCDCL,0UL},{0UL,0UL},{0xDCDCL,0UL},{65535UL,65535UL},{65535UL,0UL}};
        int32_t l_3594 = 0x030C8C72L;
        int32_t l_3596 = 1L;
        const union U3 *l_3614 = &g_1368;
        const union U3 **l_3613 = &l_3614;
        const union U3 ***l_3612[10][5][5] = {{{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{(void*)0,&l_3613,&l_3613,(void*)0,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,(void*)0,&l_3613},{&l_3613,&l_3613,&l_3613,(void*)0,&l_3613}},{{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{(void*)0,&l_3613,&l_3613,&l_3613,&l_3613}},{{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,(void*)0,(void*)0,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,(void*)0,&l_3613},{(void*)0,(void*)0,&l_3613,(void*)0,&l_3613}},{{&l_3613,&l_3613,(void*)0,&l_3613,&l_3613},{&l_3613,(void*)0,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613}},{{&l_3613,(void*)0,&l_3613,&l_3613,&l_3613},{&l_3613,(void*)0,&l_3613,(void*)0,&l_3613},{(void*)0,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,(void*)0,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613}},{{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,(void*)0,&l_3613,(void*)0},{&l_3613,(void*)0,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,(void*)0}},{{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,(void*)0,(void*)0},{(void*)0,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613}},{{(void*)0,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,(void*)0,(void*)0,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,(void*)0},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613}},{{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,(void*)0,&l_3613,&l_3613},{&l_3613,&l_3613,(void*)0,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,(void*)0},{(void*)0,&l_3613,(void*)0,&l_3613,&l_3613}},{{(void*)0,&l_3613,(void*)0,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,(void*)0,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,&l_3613},{&l_3613,&l_3613,&l_3613,&l_3613,(void*)0}}};
        const union U3 ****l_3611[3][9][1] = {{{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0}},{{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]}},{{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0},{&l_3612[2][0][2]},{(void*)0}}};
        uint32_t *l_3627 = &g_469[1];
        uint64_t * const *l_3635 = (void*)0;
        int32_t *l_3679 = &g_2077;
        int8_t *l_3680 = &g_3055;
        struct S1 l_3681 = {-9,842,-58,-13,1225};
        int32_t l_3684 = 1L;
        int8_t l_3712 = 0xC5L;
        int64_t **l_3877[7];
        int32_t **l_3887 = (void*)0;
        int32_t **l_3888 = &g_100;
        int i, j, k;
        for (i = 0; i < 7; i++)
            l_3877[i] = &g_1835;
        for (g_3055 = 0; (g_3055 >= 0); g_3055 -= 1)
        {
            struct S1 l_3586 = {176,4408,-142,-5,-1126};
            l_3586 = l_3586;
            for (p_25 = 0; (p_25 <= 5); p_25 += 1)
            {
                union U3 ***l_3588 = (void*)0;
                union U3 **l_3589 = &g_458;
                int i, j;
                l_3589 = l_3587;
                if (l_3586.f2)
                    break;
                l_3590 |= p_26;
                for (g_2077 = 1; (g_2077 >= 0); g_2077 -= 1)
                {
                    uint32_t l_3591 = 4294967295UL;
                    return l_3591;
                }
            }
        }
    }
    if (((~p_25) >= (!(((safe_unary_minus_func_int32_t_s((safe_mod_func_uint8_t_u_u(((safe_div_func_uint16_t_u_u((*l_3710), (*l_3710))) ^ (p_25 == (safe_sub_func_uint16_t_u_u((l_3898 != (void*)0), (((*g_1617) |= (g_3900 , ((safe_lshift_func_uint8_t_u_s((l_3903[0] , (p_26 <= 0xD497C242EA6AE202LL)), 5)) ^ 0x7EEDF66798564DBBLL))) < (*g_1835)))))), p_25)))) > p_25) | (*l_3710)))))
    {
        int32_t l_3906 = 0xF6D77974L;
        struct S2 **l_3907 = &g_2747;
        uint64_t l_3962 = 0x1D2751B8D994F87CLL;
        struct S0 ***l_3978 = &g_2787;
        int32_t l_4020 = (-1L);
        struct S1 l_4029 = {-89,1278,238,4,308};
        union U3 l_4080 = {0};
        int32_t l_4119 = (-8L);
        int32_t l_4120 = (-1L);
        int32_t l_4121 = 0x8EFB1E42L;
        int16_t l_4126 = 0xA6BBL;
        int32_t l_4127 = 0x3730888EL;
        int32_t l_4128 = 0x69B8B89DL;
        int32_t l_4129 = 1L;
        int32_t l_4130 = (-3L);
        uint16_t *****l_4136 = &g_3527;
        uint8_t l_4142 = 255UL;
        const struct S1 **l_4184 = (void*)0;
        struct S1 * const *l_4185 = &g_258;
        const union U3 *l_4252 = (void*)0;
        const union U3 **l_4251[9][4] = {{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252},{&l_4252,&l_4252,&l_4252,&l_4252}};
        const union U3 ***l_4250 = &l_4251[4][3];
        const union U3 **** const l_4249 = &l_4250;
        const uint16_t ***l_4260 = &g_4053[6][7];
        int32_t l_4295 = 0x2712E590L;
        uint16_t ****l_4338 = &g_932;
        int32_t *l_4360 = &l_3717[3];
        struct S1 ****l_4413[7][5][4] = {{{&g_2379,&g_2379,&g_2379,&g_2379},{(void*)0,(void*)0,&g_2379,&g_2379},{(void*)0,&g_2379,(void*)0,&g_2379},{&g_2379,&g_2379,(void*)0,(void*)0},{&g_2379,&g_2379,(void*)0,&g_2379}},{{&g_2379,&g_2379,&g_2379,&g_2379},{&g_2379,&g_2379,(void*)0,(void*)0},{&g_2379,&g_2379,(void*)0,&g_2379},{&g_2379,(void*)0,(void*)0,&g_2379},{(void*)0,&g_2379,&g_2379,(void*)0}},{{(void*)0,&g_2379,&g_2379,&g_2379},{&g_2379,(void*)0,(void*)0,&g_2379},{&g_2379,&g_2379,&g_2379,&g_2379},{&g_2379,(void*)0,&g_2379,&g_2379},{&g_2379,&g_2379,&g_2379,(void*)0}},{{(void*)0,&g_2379,&g_2379,&g_2379},{&g_2379,(void*)0,&g_2379,&g_2379},{(void*)0,&g_2379,(void*)0,(void*)0},{&g_2379,&g_2379,&g_2379,&g_2379},{(void*)0,&g_2379,&g_2379,&g_2379}},{{&g_2379,&g_2379,(void*)0,(void*)0},{(void*)0,&g_2379,&g_2379,&g_2379},{&g_2379,&g_2379,&g_2379,&g_2379},{(void*)0,(void*)0,&g_2379,&g_2379},{&g_2379,&g_2379,&g_2379,&g_2379}},{{&g_2379,&g_2379,&g_2379,&g_2379},{&g_2379,&g_2379,(void*)0,&g_2379},{&g_2379,&g_2379,&g_2379,&g_2379},{(void*)0,(void*)0,&g_2379,&g_2379},{(void*)0,&g_2379,(void*)0,&g_2379}},{{&g_2379,&g_2379,(void*)0,(void*)0},{&g_2379,&g_2379,(void*)0,&g_2379},{&g_2379,&g_2379,&g_2379,&g_2379},{&g_2379,&g_2379,(void*)0,(void*)0},{&g_2379,&g_2379,(void*)0,&g_2379}}};
        uint32_t l_4447[5] = {1UL,1UL,1UL,1UL,1UL};
        int i, j, k;
        if ((safe_mod_func_uint8_t_u_u((((((*l_3710) = (l_3906 > (**g_2196))) < (l_3907 != ((l_3906 || ((*g_2836) == (*g_2836))) , l_3908))) ^ (safe_lshift_func_int16_t_s_s(1L, ((l_3906 < (**g_2196)) , 1L)))) >= p_25), 0xFAL)))
        {
            int32_t **l_3912[4] = {(void*)0,(void*)0,(void*)0,(void*)0};
            int32_t ***l_3911 = &l_3912[2];
            struct S1 l_3922[10][8] = {{{268,3547,-182,-7,2113},{212,1411,-129,11,-1286},{-109,382,-200,14,2831},{-56,4437,-91,-1,-600},{494,597,80,-14,2299},{-133,1986,-86,-6,-2301},{-429,5613,166,1,-1645},{212,1411,-129,11,-1286}},{{-333,291,29,-8,2439},{212,1411,-129,11,-1286},{463,4608,-102,0,520},{476,2413,-180,13,2802},{-443,5080,39,-14,-2519},{488,2693,-134,-7,-960},{172,682,47,0,783},{488,2693,-134,-7,-960}},{{-39,5080,-22,-7,-2433},{40,1251,-212,-11,2565},{64,1305,-192,-4,1774},{-333,291,29,-8,2439},{40,1251,-212,-11,2565},{-133,1986,-86,-6,-2301},{488,2693,-134,-7,-960},{-57,2854,62,-4,2580}},{{476,2413,-180,13,2802},{-333,291,29,-8,2439},{-56,4437,-91,-1,-600},{373,4646,-145,-3,-1182},{51,2447,229,13,-1545},{-214,1744,197,-9,1421},{51,2447,229,13,-1545},{373,4646,-145,-3,-1182}},{{-344,3032,1,-7,-911},{42,1638,-102,14,2403},{-344,3032,1,-7,-911},{250,3578,-207,-5,755},{-39,5080,-22,-7,-2433},{64,1305,-192,-4,1774},{-109,382,-200,14,2831},{293,5671,-231,-0,-311}},{{488,2693,-134,-7,-960},{-133,1986,-86,-6,-2301},{40,1251,-212,-11,2565},{-333,291,29,-8,2439},{64,1305,-192,-4,1774},{40,1251,-212,-11,2565},{-39,5080,-22,-7,-2433},{172,682,47,0,783}},{{488,2693,-134,-7,-960},{373,4646,-145,-3,-1182},{494,597,80,-14,2299},{51,2447,229,13,-1545},{-39,5080,-22,-7,-2433},{-443,5080,39,-14,-2519},{120,2234,155,-2,-862},{197,202,37,-8,945}},{{-344,3032,1,-7,-911},{145,3647,62,-11,-740},{-57,2854,62,-4,2580},{436,2772,156,9,1136},{51,2447,229,13,-1545},{-193,407,-7,12,938},{51,4300,-31,3,-566},{-39,5080,-22,-7,-2433}},{{476,2413,-180,13,2802},{250,3578,-207,-5,755},{436,2772,156,9,1136},{463,4608,-102,0,520},{40,1251,-212,-11,2565},{40,1251,-212,-11,2565},{463,4608,-102,0,520},{436,2772,156,9,1136}},{{-39,5080,-22,-7,-2433},{-39,5080,-22,-7,-2433},{268,3547,-182,-7,2113},{488,2693,-134,-7,-960},{-267,2890,209,5,-679},{104,4812,98,3,382},{42,1638,-102,14,2403},{476,2413,-180,13,2802}}};
            int32_t l_3966 = 0x139C9446L;
            uint64_t l_3997 = 18446744073709551608UL;
            int32_t * const l_4004 = &g_1204;
            int32_t l_4019[2];
            int i, j;
            for (i = 0; i < 2; i++)
                l_4019[i] = (-7L);
lbl_4162:
            if ((((l_3911 == &g_2836) || (!((*g_1835) = ((+(((safe_mul_func_int8_t_s_s(l_3906, (--(*g_499)))) ^ (l_3638 , ((*l_3710) , ((((!(safe_sub_func_int8_t_s_s((((****g_2283) , p_26) == (4UL && ((l_3922[4][2] , p_25) && 4294967295UL))), l_3906))) >= 0UL) >= (**g_1616)) != 1UL)))) < (-1L))) | 4294967289UL)))) && 18446744073709551606UL))
            {
                int16_t l_3957 = 0x3FE1L;
                struct S1 l_3979 = {24,624,-200,1,2815};
                struct S0 ** const **l_3980 = (void*)0;
                struct S0 *l_3984 = &g_3985;
                struct S0 ** const l_3983 = &l_3984;
                struct S0 ** const *l_3982 = &l_3983;
                struct S0 ** const **l_3981 = &l_3982;
                int32_t *l_3986 = (void*)0;
                int32_t *l_3987 = (void*)0;
                int32_t *l_3988 = (void*)0;
                int32_t *l_3989 = &g_3715;
                uint32_t ****l_4000 = &g_1927;
                for (g_2353 = (-27); (g_2353 != 49); g_2353 = safe_add_func_uint16_t_u_u(g_2353, 1))
                {
                    int8_t ** const l_3925 = &g_52;
                    int8_t **l_3927 = &l_3834;
                    int8_t ***l_3926 = &l_3927;
                    (*l_3926) = l_3925;
                    if (p_25)
                        continue;
                }
                l_3906 |= (l_3922[4][2].f3 , 0L);
lbl_4008:
                for (g_1824 = (-1); (g_1824 != 10); g_1824 = safe_add_func_uint32_t_u_u(g_1824, 1))
                {
                    int32_t l_3940[3];
                    uint16_t l_3961 = 65535UL;
                    int i;
                    for (i = 0; i < 3; i++)
                        l_3940[i] = 0xDA46A136L;
                    for (g_188 = 0; (g_188 >= 20); g_188 = safe_add_func_int64_t_s_s(g_188, 7))
                    {
                        int8_t ****l_3960[2];
                        int32_t *l_3963 = &l_3716;
                        int i;
                        for (i = 0; i < 2; i++)
                            l_3960[i] = &g_3958;
                        (*l_3963) = ((((((0L == (safe_rshift_func_int16_t_s_s(((safe_sub_func_uint16_t_u_u(p_25, (safe_sub_func_int8_t_s_s(0xF0L, (((1UL & (l_3940[0] == (l_3906 = ((*l_3710) |= 0xD27CF1A4L)))) == (safe_unary_minus_func_int8_t_s((!(safe_mul_func_uint16_t_u_u(((safe_mod_func_int32_t_s_s((l_3922[4][2].f3 = (safe_lshift_func_int16_t_s_s((safe_lshift_func_int8_t_s_s((safe_add_func_int8_t_s_s((safe_add_func_uint32_t_u_u(((safe_lshift_func_int8_t_s_s(l_3957, 2)) && (((**g_2196) == ((((g_3958 = g_3958) == (void*)0) != l_3961) , (***g_3958))) <= p_26)), p_26)), 1UL)), (**g_3959))), l_3962))), 0x3F915BDEL)) <= l_3922[4][2].f0), (-3L))))))) ^ 0xE5L))))) == p_25), 8))) ^ 249UL) ^ p_26) != 0x2FF5L) | l_3962) , 0x07C9B672L);
                        if (l_3957)
                            break;
                    }
                }
                if (((*l_3989) ^= (((*l_3710) ^= (safe_sub_func_uint32_t_u_u(l_3966, p_26))) , (!((safe_add_func_int32_t_s_s(p_25, (safe_mod_func_uint16_t_u_u((safe_add_func_int64_t_s_s(0x6BEA79CFC9B2F6CALL, (safe_div_func_uint8_t_u_u((safe_lshift_func_int8_t_s_s(((l_3978 == ((l_3922[4][2] = ((*l_3710) , l_3979)) , ((*l_3981) = &g_2787))) , (p_25 | 0xD48FL)), p_26)), l_3966)))), 65528UL)))) , 0x5104L)))))
                {
                    int16_t l_3990 = 0x5A1FL;
                    int32_t *l_3991 = (void*)0;
                    int32_t *l_3992 = &g_266[4][7];
                    int32_t *l_3993 = &g_1853;
                    int32_t *l_3994 = &g_3715;
                    int32_t *l_3995 = (void*)0;
                    int32_t *l_3996[9][8][3] = {{{&l_3590,&g_266[4][7],(void*)0},{&l_3590,&g_266[3][4],&l_3590},{&l_3716,&g_1853,&g_266[4][7]},{&l_3590,&l_3590,&g_82},{&l_3716,&l_3590,&g_3715},{&g_266[4][7],&g_82,&l_3590},{&l_3717[4],&l_3906,&g_1853},{&g_1204,&g_266[4][7],&l_3590}},{{&l_3590,(void*)0,&g_3715},{&g_266[4][7],&l_3716,&g_82},{&g_1853,(void*)0,&g_266[4][7]},{&g_82,&l_3717[0],&l_3590},{&g_82,&g_82,(void*)0},{&g_1853,&g_82,&l_3717[4]},{&g_266[4][7],&l_3717[0],&g_1853},{&l_3590,(void*)0,&l_3590}},{{&g_1853,&l_3716,&g_82},{&g_82,(void*)0,&g_3844},{(void*)0,&g_266[4][7],&l_3716},{&g_82,&l_3906,&l_3716},{(void*)0,&g_82,&l_3717[0]},{&g_82,&l_3590,&g_82},{&g_1853,&l_3590,(void*)0},{&l_3590,&g_1853,&g_266[3][4]}},{{&g_266[4][7],&g_266[3][4],&g_82},{&g_1853,&g_266[4][7],&g_82},{&g_82,&l_3717[1],&g_266[3][4]},{&g_82,(void*)0,(void*)0},{&g_1853,&g_1853,&g_82},{&g_266[4][7],(void*)0,&l_3717[0]},{&l_3590,&l_3590,&l_3716},{&g_1204,&g_82,&l_3716}},{{&l_3717[4],&l_3590,&g_3844},{&g_266[4][7],(void*)0,&g_82},{&l_3716,&g_1853,&l_3590},{&l_3590,(void*)0,&g_1853},{&l_3716,&l_3717[1],&l_3717[4]},{&l_3590,&g_266[4][7],(void*)0},{&l_3590,&g_266[3][4],&l_3590},{&l_3716,&g_1853,&g_266[4][7]}},{{&l_3590,&l_3590,&g_82},{&l_3716,&g_1853,&g_82},{&g_266[3][4],&l_3590,(void*)0},{&g_82,&l_3590,&g_82},{(void*)0,&g_266[3][4],(void*)0},{(void*)0,&l_3717[2],&g_82},{&g_1853,&l_3717[4],&g_1853},{&g_82,&l_3717[1],&l_3590}},{{&g_82,&g_266[4][7],&g_266[4][7]},{&l_3716,&g_1853,&g_1853},{&g_82,&g_1853,&g_82},{&l_3590,&g_266[4][7],&g_82},{&g_1853,&l_3717[1],&g_1853},{&g_82,&l_3717[4],&g_266[4][7]},{&l_3716,&l_3717[2],&l_3590},{&l_3717[2],&g_266[3][4],&g_1204}},{{&l_3590,&l_3590,&l_3717[4]},{&l_3717[2],&l_3590,&g_266[4][7]},{&l_3716,&g_1853,&l_3716},{&g_82,&g_266[4][7],&l_3590},{&g_1853,(void*)0,&l_3716},{&l_3590,&l_3716,&l_3590},{&g_82,&g_1853,&l_3590},{&l_3716,(void*)0,&l_3716}},{{&g_82,&l_3590,&l_3590},{&g_82,&g_82,&l_3716},{&g_1853,&g_82,&g_266[4][7]},{(void*)0,&g_3844,&l_3717[4]},{(void*)0,&g_82,&g_1204},{&g_82,&g_3844,&l_3590},{&g_266[3][4],&g_82,&g_266[4][7]},{(void*)0,&g_82,&g_1853}}};
                    int i, j, k;
                    --l_3997;
                    if (((*l_3992) = ((*l_3989) = p_26)))
                    {
                        uint32_t *****l_4001 = (void*)0;
                        uint32_t *****l_4002[10];
                        int32_t **l_4005 = &l_3987;
                        int i;
                        for (i = 0; i < 10; i++)
                            l_4002[i] = (void*)0;
                        (*l_3994) ^= p_26;
                        (*l_3992) ^= ((*l_3989) = 0x77D5BCDDL);
                        l_4003[4] = l_4000;
                        (*l_4005) = l_4004;
                    }
                    else
                    {
                        return p_25;
                    }
                }
                else
                {
                    int16_t l_4009[1];
                    int32_t l_4017 = (-1L);
                    int32_t l_4018 = 0xB93B745AL;
                    int32_t l_4021[4];
                    int i;
                    for (i = 0; i < 1; i++)
                        l_4009[i] = (-6L);
                    for (i = 0; i < 4; i++)
                        l_4021[i] = 0L;
                    for (g_2568 = 15; (g_2568 <= 27); ++g_2568)
                    {
                        int32_t *l_4010 = &l_3906;
                        int32_t *l_4011 = &g_3715;
                        int32_t *l_4012 = &g_82;
                        int32_t *l_4013 = &l_3717[1];
                        int32_t *l_4014 = (void*)0;
                        int32_t *l_4015 = &g_3715;
                        int32_t *l_4016[3][10][3] = {{{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]}},{{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82}},{{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0},{(void*)0,(void*)0,&l_3717[2]},{(void*)0,(void*)0,&g_82},{(void*)0,&l_3717[1],(void*)0}}};
                        int i, j, k;
                        if (l_3638.f2)
                            goto lbl_4008;
                        if (p_25)
                            continue;
                        --l_4022;
                    }
                    if ((safe_mul_func_int8_t_s_s(1L, ((*g_499) ^= p_25))))
                    {
                        int8_t *l_4027 = &g_3562[0];
                        l_4029 = l_4028;
                        return p_25;
                    }
                    else
                    {
                        return p_25;
                    }
                }
            }
            else
            {
                uint8_t l_4032 = 0x04L;
                struct S1 l_4057 = {-132,3276,-103,-15,-1919};
                volatile struct S0 *l_4069 = &g_4070;
                for (g_858 = 29; (g_858 >= 32); g_858 = safe_add_func_int16_t_s_s(g_858, 3))
                {
                    const struct S1 l_4033 = {164,2328,-186,-15,-1845};
                    int32_t l_4056 = 1L;
                    int32_t l_4059 = 1L;
                    int32_t l_4060 = 1L;
                    int32_t l_4061 = 0xBC64D5B5L;
                    int32_t l_4063 = (-8L);
                    int32_t l_4064 = 0xB115D69AL;
                    int32_t l_4065[2][1];
                    int i, j;
                    for (i = 0; i < 2; i++)
                    {
                        for (j = 0; j < 1; j++)
                            l_4065[i][j] = (-1L);
                    }
                    if (l_4032)
                        break;
                    if (((*l_3710) = 0xCB06A753L))
                    {
                        struct S1 *l_4034 = &l_3922[4][2];
                        (*l_4034) = l_4033;
                    }
                    else
                    {
                        const struct S1 l_4051 = {492,3315,202,-14,-1018};
                        const uint16_t ****l_4055 = &g_4052[4];
                        int32_t *l_4058[10][10] = {{&l_4019[0],&l_3716,&g_1204,&g_1204,&g_82,&g_82,&g_1204,&g_1204,&l_3716,&g_82},{(void*)0,&l_3716,&l_3717[1],&g_82,&l_4056,&g_82,&l_4056,&g_82,&l_3717[1],&l_3716},{&g_266[4][7],(void*)0,&g_82,&l_3716,&l_4056,&g_1204,&g_1204,&l_4056,&l_3716,&g_82},{&l_4056,&l_4056,(void*)0,&g_266[4][7],&l_4019[0],&g_1204,&l_3717[1],&g_1204,&l_4019[0],&g_266[4][7]},{&g_266[4][7],&g_1204,&g_266[4][7],&g_1204,&g_82,&g_82,&l_3717[1],&l_3717[1],&g_82,&g_82},{(void*)0,&l_4056,&l_4056,(void*)0,&g_266[4][7],&l_4019[0],&g_1204,&l_3717[1],&g_1204,&l_4019[0]},{&g_82,(void*)0,&g_266[4][7],(void*)0,&g_82,&l_3716,&l_4056,&g_1204,&g_1204,&l_4056},{&l_3717[1],&l_3716,(void*)0,(void*)0,&l_3716,&l_3717[1],&g_82,&l_4056,&g_82,&l_4056},{(void*)0,(void*)0,&g_82,&g_1204,&g_82,(void*)0,(void*)0,&g_82,&l_4019[0],&l_4019[0]},{(void*)0,&l_4019[0],&l_3717[1],&g_266[4][7],&g_266[4][7],&l_3717[1],&l_4019[0],(void*)0,&l_3716,&g_82}};
                        uint16_t l_4066 = 0x315BL;
                        int i, j;
                        l_4056 = (safe_mod_func_uint64_t_u_u((safe_sub_func_uint32_t_u_u((+(((&g_1299 != &g_1299) , (safe_sub_func_int64_t_s_s(p_25, ((*g_1835) = l_3962)))) > ((safe_lshift_func_int8_t_s_u((!(safe_mul_func_uint8_t_u_u((0x535DD0E7L >= ((safe_lshift_func_uint8_t_u_s(((safe_mul_func_int8_t_s_s(0x1EL, ((*g_3527) != ((*l_4055) = (l_4051 , g_4052[4]))))) & (****g_2283)), (*g_52))) >= p_25)), (**g_3959)))), l_4051.f0)) ^ 1UL))), 0L)), 0x3CC9FF2BBDB723FBLL));
                        if (p_26)
                            break;
                        l_4057 = l_3922[1][7];
                        l_4066--;
                    }
                    l_4069 = g_97;
                }
                if (((*l_4004) |= 0xA91B588CL))
                {
                    uint16_t l_4084 = 65530UL;
                    struct S1 *l_4085 = &g_2981;
                    uint16_t *l_4105 = &g_2450;
                    int32_t l_4113[9] = {0x358D3A4AL,0x358D3A4AL,0x358D3A4AL,0x358D3A4AL,0x358D3A4AL,0x358D3A4AL,0x358D3A4AL,0x358D3A4AL,0x358D3A4AL};
                    int i;
                    (*l_3710) = (((*g_499) = ((~(((*l_4004) = p_25) >= (l_4020 != 0x48L))) ^ 0xB9614A23L)) != (safe_mul_func_int16_t_s_s((((safe_add_func_uint16_t_u_u((((safe_rshift_func_uint16_t_u_s(((((l_4057.f3 , ((safe_div_func_uint16_t_u_u(((l_4080 , ((**g_3959) = (l_4057.f4 && 18446744073709551615UL))) >= ((((safe_div_func_int16_t_s_s((+(l_4057 , p_25)), 0x4F1AL)) >= 0xA2662F6EA9D6E646LL) & p_25) != p_26)), 65528UL)) ^ l_4084)) > l_4084) || (***g_248)) == p_26), 11)) >= p_26) | p_26), l_4057.f2)) ^ p_26) , 0xCAF7L), (****g_2283))));
                    (*l_4085) = (l_3638 = l_3922[4][2]);
                    l_4113[5] &= (l_4029.f0 |= ((g_2568 = (safe_div_func_int64_t_s_s(l_4084, (~(safe_rshift_func_int8_t_s_s(p_26, 7)))))) , (((((***g_3958) ^= (safe_add_func_int32_t_s_s((safe_div_func_int32_t_s_s(((((safe_mul_func_uint8_t_u_u((((((safe_lshift_func_uint8_t_u_u((safe_add_func_uint32_t_u_u(p_25, (safe_add_func_uint16_t_u_u(p_26, (safe_mul_func_uint16_t_u_u(((*l_3767) = (*l_4004)), (((*l_4105)++) == p_25))))))), (safe_add_func_uint64_t_u_u(1UL, l_4020)))) != (l_4029.f3 = (safe_add_func_int16_t_s_s(p_25, l_4112)))) ^ (***g_248)) , (void*)0) == &g_2379), 0x1AL)) , (**g_249)) , 0xDBL) < (*g_499)), 1UL)), p_25))) == p_26) > p_26) >= (*l_3710))));
                }
                else
                {
                    return p_26;
                }
            }
            for (g_3092 = 1; (g_3092 <= 8); g_3092 += 1)
            {
                int32_t *l_4114 = (void*)0;
                int32_t *l_4115 = (void*)0;
                int32_t *l_4116 = (void*)0;
                int32_t *l_4117 = &g_266[4][7];
                int32_t *l_4118[10];
                uint32_t l_4122 = 18446744073709551608UL;
                uint16_t ***** const l_4137 = &g_3527;
                struct S1 l_4149 = {490,3751,122,-6,-2338};
                uint8_t l_4219[1];
                union U3 ** const **l_4222 = &l_3899;
                int8_t *l_4257 = (void*)0;
                int i;
                for (i = 0; i < 10; i++)
                    l_4118[i] = &l_4019[0];
                for (i = 0; i < 1; i++)
                    l_4219[i] = 252UL;
                --l_4122;
                l_4131++;
                (*l_4004) = (((((safe_mul_func_uint16_t_u_u((((void*)0 != &l_3997) <= (l_4136 == l_4137)), (safe_rshift_func_int16_t_s_s((p_25 , (safe_mod_func_int32_t_s_s((((*l_4004) != (l_4080 , l_4142)) && (safe_mul_func_int8_t_s_s((safe_rshift_func_uint16_t_u_s(p_25, 14)), 0x3FL))), p_25))), (*g_250))))) | (*l_4117)) , (void*)0) == &g_458) != (**g_1616));
                if ((((safe_div_func_uint32_t_u_u(((*l_4004) < ((l_4149 , (!(safe_mod_func_int32_t_s_s(((((safe_mod_func_uint64_t_u_u(((((**g_1616) >= (0x57E23EE4L != 0xA835BAE9L)) >= (**g_1616)) <= (((((safe_lshift_func_uint16_t_u_u((safe_lshift_func_int16_t_s_u((safe_lshift_func_int8_t_s_u(((*l_3689) == (l_3922[6][6] , (*l_3689))), (*l_3710))), 1)), p_25)) || p_25) | l_4120) < p_25) , l_4119)), 0x2DE50FDF09AB0105LL)) , (void*)0) != (*g_3079)) > 3L), (-4L))))) ^ p_25)), (-2L))) < (-3L)) , p_25))
                {
                    return l_4029.f1;
                }
                else
                {
                    uint64_t l_4181 = 0x3416860C1E190E94LL;
                    int32_t l_4187 = 0x7283F89CL;
                    int32_t l_4188 = (-5L);
                    int32_t l_4243 = 9L;
                    int32_t l_4245 = (-6L);
                    int32_t l_4253 = 0xF6BCE999L;
                    const int32_t *l_4256 = &l_4062[0];
                    const int32_t **l_4255 = &l_4256;
                    const int32_t ***l_4254 = &l_4255;
                    struct S1 l_4262 = {-311,1311,13,-15,-178};
                    for (g_3055 = 0; (g_3055 >= 0); g_3055 -= 1)
                    {
                        uint64_t l_4161 = 0xD67A6BB68870B58BLL;
                        int32_t l_4189 = 1L;
                        l_4161 &= 0x109C62E9L;
                        if (g_3055)
                            goto lbl_4162;
                        if (l_4161)
                            break;
                        l_4189 |= (safe_add_func_uint16_t_u_u(((safe_add_func_int8_t_s_s(p_25, (safe_rshift_func_uint16_t_u_s(((*l_4004) &= (l_4188 = (l_4187 = ((safe_rshift_func_int16_t_s_s((p_25 < (safe_rshift_func_int8_t_s_s((safe_rshift_func_uint8_t_u_s(0x15L, 4)), (safe_rshift_func_uint8_t_u_u((0x02C1L | (safe_div_func_int16_t_s_s((safe_div_func_uint16_t_u_u((l_4181 , (safe_mul_func_uint8_t_u_u(((**l_3668) = (l_4184 != (l_4186 = l_4185))), (0L == ((*l_3767) &= 0xF8BCL))))), (***g_248))), l_4129))), p_26))))), (***g_248))) <= 0x2DL)))), 14)))) == (*g_250)), 1L));
                    }
                    if (l_4029.f2)
                        continue;
                    if ((l_4181 , (((safe_mod_func_uint64_t_u_u(18446744073709551610UL, (safe_add_func_int8_t_s_s(((safe_mul_func_int16_t_s_s((((safe_sub_func_uint64_t_u_u((((safe_div_func_uint8_t_u_u(((p_26 != 0xF208EF9871B74799LL) != 246UL), (+((safe_lshift_func_uint8_t_u_s(0x03L, ((**g_3959) = (0xC23C6D60L > l_4119)))) && ((*l_3710) = (safe_sub_func_int32_t_s_s(((safe_add_func_uint16_t_u_u((g_2605 = (++(***l_3765))), (((((safe_sub_func_int32_t_s_s((safe_sub_func_uint64_t_u_u(((safe_add_func_int32_t_s_s((safe_add_func_int64_t_s_s((safe_mod_func_uint64_t_u_u(((*l_4004) | p_26), 1L)), p_25)), l_4181)) ^ p_25), 18446744073709551615UL)), p_25)) ^ (*l_4004)) || l_4188) > p_26) & p_25))) | (*l_4004)), l_4219[0]))))))) >= (*g_1356)) <= 0x7C08E446L), (*g_1617))) <= (*l_4004)) , 1L), 0x2DF7L)) | (*l_4004)), p_25)))) == l_4142) >= (*g_1835))))
                    {
                        int64_t l_4244 = 1L;
                        l_4253 ^= (l_4188 | (((*g_1617) , ((safe_add_func_uint64_t_u_u(((l_4222 == ((p_26 ^ ((safe_lshift_func_uint8_t_u_u(((safe_add_func_int64_t_s_s(((*g_1835) = ((*l_4004) , ((safe_lshift_func_uint16_t_u_u(((***l_3765) |= ((safe_lshift_func_uint8_t_u_s((((((l_4245 |= (safe_mod_func_int16_t_s_s(((((*g_2196) == (((safe_lshift_func_uint16_t_u_u(65534UL, (safe_lshift_func_int8_t_s_u(p_26, (((*l_4004) = ((((*g_499) ^= (safe_div_func_uint16_t_u_u((((*l_4117) |= (l_4243 = (((*g_1356)++) || (safe_mod_func_uint16_t_u_u(((l_4187 && 0x829D97DAL) , 0UL), 1L))))) && 0L), p_26))) & p_25) != l_4244)) & p_26))))) == 0x9AL) , (*g_2196))) ^ 5L) ^ l_4188), (*g_250)))) == 6UL) ^ p_26) , (void*)0) != g_4246[0][4][0]), 2)) & 0x19FB8A1492C3A3E3LL)), 4)) > p_26))), l_4244)) >= 0x38L), p_26)) , 0x4870L)) , l_4249)) , (*g_1617)), 18446744073709551615UL)) || 0L)) || (*g_1835)));
                    }
                    else
                    {
                        uint32_t l_4259 = 1UL;
                        struct S1 *l_4261[1];
                        int i;
                        for (i = 0; i < 1; i++)
                            l_4261[i] = &l_3638;
                        (*l_4117) = (((void*)0 != l_4254) > (l_3922[8][2] , 0x9A54L));
                        (*g_2746) = (*g_2746);
                        l_4262 = l_4149;
                    }
                    return p_25;
                }
            }
        }
        else
        {
            int8_t ** const l_4271 = &l_3834;
            int32_t l_4298 = 0x17E3FEF0L;
            union U3 l_4300[9] = {{0},{0},{0},{0},{0},{0},{0},{0},{0}};
            int16_t * const *l_4306 = &g_250;
            int32_t *l_4317 = (void*)0;
            uint16_t ****l_4334 = &g_932;
            int32_t l_4335 = 1L;
            uint16_t l_4375 = 0x8AC7L;
            int32_t l_4434 = 2L;
            int32_t l_4435 = 0x7AAD4BFAL;
            struct S1 l_4493 = {37,5673,-36,0,-1433};
            int i;
            if ((safe_rshift_func_int16_t_s_u(0x40E3L, 13)))
            {
                const int32_t l_4272 = 0xCF04D7CDL;
                struct S1 l_4273 = {-245,1505,-114,13,326};
                struct S1 *l_4274[3];
                int i;
                for (i = 0; i < 3; i++)
                    l_4274[i] = (void*)0;
                g_892 = l_4273;
            }
            else
            {
                uint32_t ** const ***l_4281 = (void*)0;
                uint32_t ** const ***l_4282 = &l_4279;
                uint8_t l_4294 = 0x7FL;
                struct S1 l_4296[10][8][1] = {{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}},{{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}},{{258,1564,14,2,552}}}};
                uint8_t l_4311 = 0x62L;
                const struct S0 *l_4321 = &g_4322;
                int64_t ***l_4346 = &l_4344;
                int64_t ****l_4345 = &l_4346;
                union U3 l_4349 = {0};
                int32_t **l_4359[8];
                int i, j, k;
                for (i = 0; i < 8; i++)
                    l_4359[i] = &l_4317;
lbl_4299:
                if ((0xB2DAFED50DEDFE4CLL != (safe_mul_func_uint8_t_u_u((safe_mul_func_int8_t_s_s(1L, (((*l_4282) = l_4279) == &g_3753))), 0L))))
                {
                    uint8_t **l_4293 = &g_499;
                    struct S1 *l_4297 = &l_4296[2][0][0];
                    (*l_4297) = l_4296[0][0][0];
                    if (g_16)
                        goto lbl_4299;
                }
                else
                {
                    l_4298 = 0x2B8F4BFBL;
                }
                if ((l_4300[3] , (0L < (safe_rshift_func_int16_t_s_s(((p_25 < (((+((*g_1835) = p_26)) >= (p_26 || 0UL)) == 0xFA4F42AAL)) & (g_2069 = (((**g_1616) &= (((void*)0 != l_4306) , p_26)) == l_4294))), l_4029.f0)))))
                {
                    int8_t l_4307 = 1L;
                    int32_t *l_4308 = &l_3716;
                    int32_t *l_4309 = &l_4295;
                    int32_t *l_4310[8][3][8] = {{{&l_3590,&g_117,&l_4119,&l_4120,&l_3716,&l_4127,&g_266[2][6],&l_4121},{&g_82,&l_4119,&l_4119,&g_266[3][1],(void*)0,&g_266[0][2],(void*)0,&g_1853},{&l_3716,&l_4119,&g_1853,&g_266[2][6],&g_266[4][7],&l_4020,&g_1204,&l_4128}},{{&l_4120,&l_3906,&l_4130,&l_3716,&g_1204,&g_3715,&l_3590,&l_3590},{&l_4295,&g_266[3][1],&l_4127,&l_4127,&g_266[3][1],&l_4295,&l_3590,(void*)0},{&l_4127,(void*)0,&l_3716,&l_4127,(void*)0,&g_1204,&l_4130,(void*)0}},{{&g_117,(void*)0,&g_266[3][1],&l_4127,&g_3715,(void*)0,&l_4119,(void*)0},{(void*)0,&g_3715,&g_117,&l_4127,&l_4119,&l_4130,&g_266[3][1],&l_3590},{&g_1204,&l_4119,&g_117,&l_3716,&g_82,&l_3590,(void*)0,&l_4128}},{{&g_266[0][2],&l_4119,&l_3590,&g_266[2][6],&l_3590,&l_4119,&g_266[0][2],&g_1853},{(void*)0,(void*)0,&g_3715,&g_266[3][1],&l_4119,&l_4119,&l_4120,&l_4121},{&l_4119,&l_4020,(void*)0,&l_4120,&l_4119,&l_4128,&l_3716,&g_266[2][6]}},{{(void*)0,&l_4295,&g_3715,&l_4121,&l_3590,(void*)0,&g_1853,&g_82},{&g_266[0][2],(void*)0,(void*)0,&l_4295,&g_82,&g_266[2][6],(void*)0,&g_3715},{&g_1204,&l_3716,(void*)0,&l_4119,&l_4119,(void*)0,&l_3716,&g_1204}},{{(void*)0,&l_4127,&l_4119,(void*)0,&g_3715,&l_3716,&g_117,(void*)0},{&g_117,&g_3715,&g_82,&l_3590,(void*)0,&l_3716,&l_4119,&l_4295},{&l_4127,&l_4127,&g_1204,&g_266[4][7],&g_266[3][1],(void*)0,(void*)0,(void*)0}},{{&l_4295,&l_3716,&g_266[4][7],&g_1853,&g_1204,&g_266[2][6],(void*)0,&l_4119},{&l_4120,(void*)0,&l_3906,(void*)0,&g_266[4][7],(void*)0,&l_3906,(void*)0},{&l_3716,&l_4295,&l_4121,&g_117,(void*)0,&l_4128,&l_4127,&l_4130}},{{&g_82,&l_4020,&l_4127,(void*)0,&l_3716,&l_4119,&l_4127,(void*)0},{&g_117,(void*)0,&l_4121,&l_4128,&l_3590,&l_4119,&l_3906,&l_4119},{&l_3590,&l_4119,&l_3906,&l_4119,&g_266[0][2],&l_3590,(void*)0,(void*)0}}};
                    int i, j, k;
                    ++l_4311;
                    for (g_2427 = 0; (g_2427 > 33); ++g_2427)
                    {
                        int32_t **l_4316[7][10][3] = {{{(void*)0,(void*)0,&l_4310[5][2][1]},{(void*)0,(void*)0,&l_4309},{(void*)0,&l_4310[4][1][7],&l_3710},{&l_4308,&g_100,&l_4310[4][1][7]},{&l_3710,&l_3710,(void*)0},{(void*)0,&l_4309,&g_100},{&g_100,&g_100,&l_4310[6][2][7]},{&l_4309,&g_100,&l_3710},{(void*)0,&l_4310[6][2][7],&l_3710},{&l_4310[6][2][7],&l_4309,&l_3710}},{{&l_4308,&l_4308,&l_4308},{&l_4309,&l_4308,&l_4310[6][0][4]},{&l_4308,&l_4309,&l_3710},{&l_3710,&l_4309,(void*)0},{&l_4310[6][2][7],&l_4308,&l_4308},{&l_4308,&l_4308,&l_4310[6][2][7]},{&l_4310[4][1][7],&l_4309,(void*)0},{&l_4308,&l_4310[6][2][7],&l_4308},{&l_4310[5][2][1],&g_100,&g_100},{&l_3710,&g_100,&l_4309}},{{&l_4310[6][2][7],&l_4309,(void*)0},{&l_3710,&l_3710,&l_4310[6][2][7]},{&g_100,&g_100,(void*)0},{(void*)0,&l_4310[4][1][7],&g_100},{&g_100,(void*)0,&l_4310[6][2][7]},{(void*)0,(void*)0,&g_100},{&l_4310[6][2][1],(void*)0,(void*)0},{&l_4310[6][2][7],&l_4310[2][2][5],&l_4310[6][2][7]},{&l_3710,(void*)0,(void*)0},{&g_100,&g_100,&l_4309}},{{(void*)0,&g_100,&g_100},{(void*)0,&l_4309,&l_4308},{&g_100,&g_100,(void*)0},{&l_4309,&g_100,&l_4310[6][2][7]},{&l_4310[6][0][4],&l_3710,&l_4308},{&l_4309,&g_100,(void*)0},{&l_3710,&g_100,&l_3710},{&l_3710,&l_4310[5][2][1],&l_4310[6][0][4]},{&l_4309,&l_3710,&l_4308},{&l_4310[6][0][4],&l_4309,&l_3710}},{{&l_4309,&l_3710,&l_3710},{&g_100,&l_4308,&l_3710},{(void*)0,&l_4309,&l_4310[6][2][7]},{(void*)0,&l_4308,&g_100},{&g_100,(void*)0,(void*)0},{&l_3710,&l_4310[4][0][2],&l_4310[4][1][7]},{&l_4310[6][2][7],&l_4309,&l_3710},{&l_4310[6][2][1],&l_4310[6][2][7],&l_4309},{(void*)0,&l_4308,&l_4310[5][2][1]},{&g_100,&l_4310[6][2][7],&l_4308}},{{(void*)0,&l_4309,(void*)0},{&g_100,&l_4310[4][0][2],(void*)0},{&l_3710,(void*)0,(void*)0},{&l_4310[6][2][7],&l_4308,&l_4310[6][2][1]},{&l_3710,&l_4309,&l_3710},{&l_4310[5][2][1],&l_4308,&l_4310[4][0][2]},{&l_4308,&l_3710,&l_4310[6][2][7]},{&l_4310[4][1][7],&l_4309,&g_100},{&l_4308,&l_3710,&l_4308},{&l_4310[6][2][7],&l_4310[5][2][1],&g_100}},{{&l_3710,&g_100,&g_100},{&l_4308,&g_100,&l_4308},{&l_4309,&l_3710,&g_100},{&l_4308,&g_100,&l_4308},{&g_100,&l_4310[6][2][7],(void*)0},{(void*)0,&l_4309,&l_4309},{&l_4310[4][0][2],&l_4309,&l_4309},{&l_4310[6][0][4],&l_4309,&l_3710},{&g_100,&l_3710,&g_100},{(void*)0,&l_3710,&g_100}}};
                        struct S1 *l_4318 = &l_4028;
                        int i, j, k;
                        l_4317 = (l_4310[6][2][7] = l_4310[3][1][6]);
                        (*l_4318) = l_4296[0][0][0];
                    }
                    if ((((l_4321 != g_1470) && (safe_rshift_func_uint16_t_u_s((~(l_4296[0][0][0].f0 &= (7UL > p_26))), 14))) ^ (l_4296[0][0][0].f2 = ((*l_3710) > (safe_rshift_func_int8_t_s_s(((safe_div_func_uint64_t_u_u(((*g_1617) = (safe_add_func_int32_t_s_s(p_26, p_25))), p_26)) != ((safe_div_func_uint64_t_u_u((((((*l_4136) = (void*)0) != l_4334) <= 0xEEL) && (*l_3710)), (*g_1835))) >= l_4335)), 4))))))
                    {
                        return p_26;
                    }
                    else
                    {
                        l_4296[0][0][0].f3 = (-1L);
                        (*l_4308) = (safe_sub_func_int32_t_s_s((((*l_4136) = l_4338) != (p_25 , (void*)0)), (safe_add_func_int8_t_s_s((-1L), p_25))));
                        (****g_2737) = (void*)0;
                    }
                }
                else
                {
                    for (g_3844 = (-4); (g_3844 >= (-4)); g_3844 = safe_add_func_uint64_t_u_u(g_3844, 6))
                    {
                        (***g_2738) = (****g_2737);
                    }
                }
                (*l_4345) = l_4343;
                l_4360 = (((9UL ^ 0x9513B74CL) != ((*g_458) , (safe_mul_func_int16_t_s_s((((*g_458) = (l_4349 = (*g_458))) , (safe_mod_func_uint32_t_u_u(((safe_rshift_func_uint8_t_u_u((((((+0x0DL) < (**g_1616)) & p_25) || (safe_add_func_int8_t_s_s((~l_4126), l_4358))) | l_4296[0][0][0].f0), p_25)) > (*l_3710)), 0x82C61DAAL))), (-5L))))) , &l_4020);
            }
            if ((safe_mod_func_uint64_t_u_u((**g_1616), (((safe_sub_func_uint8_t_u_u((safe_rshift_func_int16_t_s_u((safe_mul_func_uint16_t_u_u(0UL, (safe_rshift_func_uint8_t_u_s(((((*g_458) , (p_25 <= (safe_sub_func_uint64_t_u_u((safe_div_func_int32_t_s_s(((0UL < 1UL) , ((void*)0 == l_4003[4])), p_26)), (*g_1617))))) && p_26) & p_26), 4)))), 14)), p_25)) , (*l_4360)) ^ l_4375))))
            {
                struct S1 *l_4377 = &l_3638;
                (**g_2379) = (***g_2378);
                (*l_4377) = l_4376;
            }
            else
            {
                int16_t l_4384 = 0x0DA0L;
                uint32_t *l_4396 = &g_121;
                int32_t l_4430 = 0xC291B8A2L;
                int32_t l_4432 = 9L;
                int32_t l_4433 = 9L;
                for (g_2427 = 0; (g_2427 <= 7); g_2427 += 1)
                {
                    int64_t *l_4389 = &g_236[2][0];
                    int i;
                    l_4378 ^= p_25;
                    (*l_3710) &= (safe_div_func_int64_t_s_s(((*g_1835) = (*g_1835)), (((!(safe_mul_func_int8_t_s_s(l_4384, ((safe_div_func_int64_t_s_s(((safe_sub_func_int16_t_s_s(((((*l_4389) = g_2234[5]) == (((p_26 != ((((safe_lshift_func_int16_t_s_s(((*g_250) = ((++(*g_499)) && (g_1480.f1 & 2UL))), (&g_2069 == ((((safe_sub_func_int8_t_s_s(p_25, ((l_4396 == l_4360) != (*l_4360)))) , (void*)0) != (void*)0) , l_4397)))) != p_26) , 0UL) | p_26)) & g_154.f3) || 8L)) == p_26), p_26)) , p_26), 0x9682EE9422CDAD82LL)) != l_4384)))) > g_2633[8][3].f2) , 0x4D91C60B18401084LL)));
                    for (g_3844 = 0; g_3844 < 5; g_3844 += 1)
                    {
                        l_4003[g_3844] = &g_1927;
                    }
                    for (l_4265 = 0; (l_4265 <= 4); l_4265 += 1)
                    {
                        return p_25;
                    }
                }
                for (g_1853 = 0; (g_1853 >= 8); g_1853 = safe_add_func_int16_t_s_s(g_1853, 5))
                {
                    volatile struct S0 **l_4400 = (void*)0;
                    volatile struct S0 **l_4401 = &g_97;
                    int32_t *l_4407 = &l_4121;
                    (*l_4401) = g_97;
                    for (g_188 = 0; (g_188 <= 6); g_188 += 1)
                    {
                        int i;
                        (*l_3710) |= l_3717[g_188];
                        (*l_3710) &= ((*l_4360) = (p_25 ^ 0x43L));
                    }
                    for (g_58 = (-10); (g_58 < 13); g_58++)
                    {
                        int32_t *l_4404[4];
                        int32_t **l_4405 = &l_3710;
                        int32_t **l_4406 = (void*)0;
                        int i;
                        for (i = 0; i < 4; i++)
                            l_4404[i] = &g_3715;
                        l_4407 = ((*l_4405) = l_4404[1]);
                    }
                    for (l_4384 = 1; (l_4384 <= 6); l_4384 += 1)
                    {
                        return l_4408;
                    }
                }
                for (g_2568 = 4; (g_2568 >= 1); g_2568 -= 1)
                {
                    struct S1 *****l_4411 = (void*)0;
                    struct S1 *****l_4412[10][7] = {{&g_2378,(void*)0,(void*)0,&g_2378,&g_2378,&g_2378,&g_2378},{&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378},{&g_2378,&g_2378,&g_2378,&g_2378,(void*)0,&g_2378,&g_2378},{&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,(void*)0},{&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378},{(void*)0,(void*)0,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378},{&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378},{&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378},{&g_2378,(void*)0,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378},{&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378,&g_2378}};
                    struct S2 ***l_4414 = (void*)0;
                    struct S2 ***l_4415[2];
                    int32_t l_4425 = 0L;
                    int32_t l_4446 = 0x8F9A9C5AL;
                    uint32_t l_4489 = 5UL;
                    int i, j;
                    for (i = 0; i < 2; i++)
                        l_4415[i] = &g_2746;
                    if (((p_25 < ((((safe_lshift_func_int16_t_s_s(((l_4413[5][2][2] = (void*)0) != &g_2379), 3)) , (l_4416 = l_3907)) == (void*)0) > ((safe_rshift_func_int8_t_s_s(((safe_lshift_func_int16_t_s_u(((***g_248) = p_25), 2)) != (*g_1835)), (safe_lshift_func_uint8_t_u_s(((*g_499) = p_25), (l_4425 = ((*g_52) |= (safe_div_func_int32_t_s_s(p_26, 1L)))))))) , p_25))) > p_26))
                    {
                        int32_t *l_4426 = &g_1204;
                        int32_t *l_4427 = &l_4020;
                        int32_t *l_4428 = &g_266[4][7];
                        int32_t *l_4429 = (void*)0;
                        int32_t *l_4431[8][10][3] = {{{&l_4119,(void*)0,&l_4335},{&g_1853,&l_4295,&l_4335},{(void*)0,&g_3715,(void*)0},{&l_4119,&g_266[4][7],&l_4425},{&l_3906,&l_4335,&l_4425},{&l_4127,&l_4119,(void*)0},{&l_4119,&g_266[4][7],(void*)0},{&l_4335,&g_1853,(void*)0},{&l_4121,&l_4130,&l_4425},{&g_266[2][5],(void*)0,&l_4425}},{{&l_4335,(void*)0,(void*)0},{&g_3844,&l_4298,&l_4335},{&g_3715,&l_3717[4],&l_4335},{&l_3906,&g_1853,(void*)0},{&g_266[4][7],&l_4119,&l_4425},{&l_4020,&g_3844,&l_4425},{&g_1853,&l_4335,(void*)0},{&g_3844,&l_4119,(void*)0},{&l_3716,&l_4127,(void*)0},{&g_1204,&l_4295,&l_4425}},{{&l_4121,&l_4298,&l_4425},{&l_3716,(void*)0,(void*)0},{&l_4335,&g_3715,&l_4335},{&l_4127,&l_4130,&l_4335},{&l_4020,&l_4127,(void*)0},{&g_3715,&g_3715,&l_4425},{(void*)0,&l_4119,&l_4425},{&g_3715,&g_3844,(void*)0},{&l_4335,&g_3715,(void*)0},{&l_4430,&g_3715,(void*)0}},{{&g_266[2][5],&l_3717[4],&l_4425},{&g_1204,&g_3715,&l_4425},{&l_4430,&l_3717[2],(void*)0},{&l_4119,(void*)0,&l_4335},{&l_4119,&l_3717[2],&l_4335},{(void*)0,&g_1853,&g_3715},{&g_3715,&g_3715,&g_3715},{&l_4430,&l_3717[0],(void*)0},{&g_266[1][7],&g_3715,&g_1204},{&g_3715,&g_3715,(void*)0}},{{&l_4121,&l_4119,&g_1204},{&g_82,&l_4335,(void*)0},{(void*)0,&g_3844,&g_3715},{&l_4121,&l_4119,&g_3715},{&l_4430,&l_4119,&l_4335},{&g_1853,&l_3590,&l_4335},{&l_4430,&l_4119,&g_3715},{&g_3715,&g_3715,&g_3715},{&g_1853,&l_4430,(void*)0},{&l_4119,&l_3717[0],&g_1204}},{{&l_4430,&g_3715,(void*)0},{(void*)0,&g_266[1][7],&g_1204},{(void*)0,&l_3717[2],(void*)0},{&g_82,&l_4119,&g_3715},{(void*)0,&g_1204,&g_3715},{&l_3717[0],&l_3590,&l_4335},{&g_266[1][7],&l_4335,&l_4335},{&g_1853,&g_266[1][7],&g_3715},{&l_3716,&l_3716,&g_3715},{(void*)0,&g_3715,(void*)0}},{{&g_1853,&l_4430,&g_1204},{&l_3717[0],&l_3716,(void*)0},{&l_4430,&g_1853,&g_1204},{(void*)0,&l_3590,(void*)0},{(void*)0,&l_3590,&g_3715},{&l_4430,&g_3715,&g_3715},{&g_3715,&g_3844,&l_4335},{&l_4119,&l_3717[2],&l_4335},{(void*)0,&g_1853,&g_3715},{&g_3715,&g_3715,&g_3715}},{{&l_4430,&l_3717[0],(void*)0},{&g_266[1][7],&g_3715,&g_1204},{&g_3715,&g_3715,(void*)0},{&l_4121,&l_4119,&g_1204},{&g_82,&l_4335,(void*)0},{(void*)0,&g_3844,&g_3715},{&l_4121,&l_4119,&g_3715},{&l_4430,&l_4119,&l_4335},{&g_1853,&l_3590,&l_4335},{&l_4430,&l_4119,&g_3715}}};
                        uint32_t l_4436 = 0x4DEAB7ACL;
                        int i, j, k;
                        --l_4436;
                    }
                    else
                    {
                        int32_t *l_4439 = (void*)0;
                        int32_t *l_4441 = &l_4020;
                        int32_t *l_4442 = (void*)0;
                        int32_t *l_4443 = &l_4430;
                        int32_t *l_4444 = &l_4435;
                        int32_t *l_4445[4][10][5] = {{{&l_4128,&l_4128,&l_4433,&l_4128,&l_4128},{(void*)0,&l_4119,(void*)0,(void*)0,&l_4119},{&l_4128,(void*)0,(void*)0,&l_4128,(void*)0},{&l_4119,&l_4119,&l_4425,&l_4119,&l_4119},{(void*)0,&l_4128,(void*)0,(void*)0,&l_4128},{&l_4119,(void*)0,(void*)0,&l_4119,(void*)0},{&l_4128,&l_4128,&l_4433,&l_4128,&l_4128},{(void*)0,&l_4119,(void*)0,(void*)0,&l_4119},{&l_4128,(void*)0,(void*)0,&l_4128,(void*)0},{&l_4119,&l_4119,&l_4425,&l_4119,&l_4119}},{{(void*)0,&l_4128,&l_4433,&l_4433,(void*)0},{(void*)0,&l_4425,&l_4425,(void*)0,&l_4425},{(void*)0,(void*)0,&l_4128,(void*)0,(void*)0},{&l_4425,(void*)0,&l_4425,&l_4425,(void*)0},{(void*)0,&l_4433,&l_4433,(void*)0,&l_4433},{(void*)0,(void*)0,&l_4119,(void*)0,(void*)0},{&l_4433,(void*)0,&l_4433,&l_4433,(void*)0},{(void*)0,&l_4425,&l_4425,(void*)0,&l_4425},{(void*)0,(void*)0,&l_4128,(void*)0,(void*)0},{&l_4425,(void*)0,&l_4425,&l_4425,(void*)0}},{{(void*)0,&l_4433,&l_4433,(void*)0,&l_4433},{(void*)0,(void*)0,&l_4119,(void*)0,(void*)0},{&l_4433,(void*)0,&l_4433,&l_4433,(void*)0},{(void*)0,&l_4425,&l_4425,(void*)0,&l_4425},{(void*)0,(void*)0,&l_4128,(void*)0,(void*)0},{&l_4425,(void*)0,&l_4425,&l_4425,(void*)0},{(void*)0,&l_4433,&l_4433,(void*)0,&l_4433},{(void*)0,(void*)0,&l_4119,(void*)0,(void*)0},{&l_4433,(void*)0,&l_4433,&l_4433,(void*)0},{(void*)0,&l_4425,&l_4425,(void*)0,&l_4425}},{{(void*)0,(void*)0,&l_4128,(void*)0,(void*)0},{&l_4425,(void*)0,&l_4425,&l_4425,(void*)0},{(void*)0,&l_4433,&l_4433,(void*)0,&l_4433},{(void*)0,(void*)0,&l_4119,(void*)0,(void*)0},{&l_4433,(void*)0,&l_4433,&l_4433,(void*)0},{(void*)0,&l_4425,&l_4425,(void*)0,&l_4425},{(void*)0,(void*)0,&l_4128,(void*)0,(void*)0},{&l_4425,(void*)0,&l_4425,&l_4425,(void*)0},{(void*)0,&l_4433,&l_4433,(void*)0,&l_4433},{(void*)0,(void*)0,&l_4119,(void*)0,(void*)0}}};
                        struct S1 *l_4458 = &l_4029;
                        int i, j, k;
                        l_4447[1]--;
                        l_4481 = ((safe_add_func_int8_t_s_s(((safe_div_func_uint16_t_u_u(((safe_add_func_int64_t_s_s((safe_rshift_func_int8_t_s_s(6L, ((l_4458 = ((*g_2380) = &l_3638)) == (void*)0))), (safe_div_func_uint8_t_u_u((safe_mul_func_uint8_t_u_u((safe_div_func_int8_t_s_s((((((safe_rshift_func_uint8_t_u_u((*g_499), 3)) >= (safe_add_func_uint8_t_u_u((safe_lshift_func_uint8_t_u_u((safe_lshift_func_int8_t_s_u((safe_div_func_uint8_t_u_u(p_26, (**g_2196))), p_26)), 1)), 0x79L))) <= (safe_lshift_func_uint16_t_u_u((safe_sub_func_uint16_t_u_u((safe_add_func_uint8_t_u_u(p_26, (***g_3958))), (*l_4444))), 13))) ^ (-1L)) > l_4430), 9UL)), (**g_2196))), p_25)))) & (*l_4360)), p_25)) < (****g_2283)), p_25)) < (*g_499));
                        (*g_2746) = (*g_2746);
                        (*l_4444) |= p_25;
                    }
                    (*g_2746) = (*l_3907);
                    if ((safe_unary_minus_func_uint8_t_u((safe_rshift_func_uint8_t_u_s(p_25, 2)))))
                    {
                        int32_t *l_4485 = (void*)0;
                        int32_t *l_4486 = &l_3717[5];
                        int32_t *l_4487 = (void*)0;
                        int32_t *l_4488[1];
                        struct S1 l_4492 = {300,2308,-250,-11,2701};
                        int i;
                        for (i = 0; i < 1; i++)
                            l_4488[i] = &l_4435;
                        l_4489--;
                        l_4493 = l_4492;
                    }
                    else
                    {
                        return p_25;
                    }
                    for (g_1525 = 0; (g_1525 <= 4); g_1525 += 1)
                    {
                        int8_t ****l_4494 = (void*)0;
                        int8_t *****l_4495 = &l_4494;
                        int i, j;
                        (*l_3710) &= ((*l_4360) = (((*l_4495) = l_4494) == (void*)0));
                        if (g_266[g_1525][(g_1525 + 2)])
                            continue;
                    }
                }
            }
            l_4317 = &l_4434;
            (*l_3710) &= ((safe_mod_func_int8_t_s_s(((safe_rshift_func_uint8_t_u_s(((((&g_3527 != (((safe_mul_func_int8_t_s_s((*l_4360), ((((*g_250) , &l_4300[3]) == ((*l_4317) , (((safe_sub_func_uint64_t_u_u((((*l_4360) , (void*)0) != &l_4335), ((**g_1616) |= ((p_26 , (*l_4317)) > 0xDE36BD6FL)))) & (**g_3959)) , &l_4300[0]))) & 0x89B77384L))) >= 0xF4701C68L) , (void*)0)) | p_26) >= 0x145EL) , p_25), (*l_4360))) | (-10L)), (*l_4317))) | 0x643065BBL);
        }
    }
    else
    {
        uint32_t * const *l_4510 = &g_1929;
        uint32_t * const **l_4509 = &l_4510;
        int32_t l_4511 = 0L;
        const struct S1 l_4512 = {-257,2824,-98,12,2324};
        uint32_t l_4586 = 0UL;
        struct S2 **l_4601 = &g_2747;
        union U3 *l_4607 = &g_1368;
        int16_t l_4629 = (-3L);
        uint8_t l_4638 = 249UL;
        int32_t *l_4646 = (void*)0;
        int32_t l_4663 = 0x1817A098L;
        int64_t l_4673 = 0L;
        int32_t l_4674 = 0xBBC85B4CL;
        int32_t l_4676 = (-9L);
        int32_t l_4677 = 0xB3E92832L;
        int32_t l_4679[1];
        int i;
        for (i = 0; i < 1; i++)
            l_4679[i] = 3L;
        (*g_1092) = (****g_2737);
        for (l_4358 = 0; (l_4358 > 22); l_4358++)
        {
            int32_t l_4506 = 0x671F5F07L;
            struct S1 *l_4513 = (void*)0;
            struct S1 *l_4514[9][3][6] = {{{&g_2981,&g_114,&g_2981,&l_4376,&g_892,&l_3638},{&l_3638,(void*)0,&g_2981,(void*)0,&l_3638,&g_114},{&l_3638,&g_2981,(void*)0,&l_4376,&g_114,&g_114}},{{&g_2981,&g_892,&g_892,&g_2981,&g_2981,&g_114},{&g_2981,&g_114,(void*)0,&g_114,&l_4376,&g_114},{&g_2981,&g_892,&g_2981,&l_3638,&l_4376,&l_3638}},{{(void*)0,&g_114,&g_2981,&g_2981,&g_2981,&g_2981},{&g_892,&l_3638,&g_892,&g_114,&l_3638,(void*)0},{&g_892,&g_892,&g_2981,(void*)0,&g_2981,&g_892}},{{&g_114,&g_892,&g_2981,&g_2981,&l_3638,(void*)0},{&g_2981,&g_2981,&g_892,&g_892,&g_2981,&g_2981},{&g_892,&g_2981,&g_2981,&g_114,&l_3638,&g_2981}},{{&g_2981,&g_892,&g_114,&g_892,&g_2981,&g_2981},{&g_2981,&g_892,&g_892,&g_114,&l_3638,&l_3638},{&g_892,&l_3638,&l_3638,&g_892,&g_114,&l_3638}},{{&g_2981,&l_3638,&g_892,&g_2981,&g_114,&g_2981},{&g_114,&l_4376,&g_114,(void*)0,&g_114,&g_2981},{&g_892,&l_3638,&g_2981,&g_114,&g_114,&g_2981}},{{&l_3638,&l_3638,&g_892,&g_114,&l_3638,(void*)0},{&g_892,&g_892,&g_2981,(void*)0,&g_2981,&g_892},{&g_114,&g_892,&g_2981,&g_2981,&l_3638,(void*)0}},{{&g_2981,&g_2981,&g_892,&g_892,&g_2981,&g_2981},{&g_892,&g_2981,&g_2981,&g_114,&l_3638,&g_2981},{&g_2981,&g_892,&g_114,&g_892,&g_2981,&g_2981}},{{&g_2981,&g_892,&g_892,&g_114,&l_3638,&l_3638},{&g_892,&l_3638,&l_3638,&g_892,&g_114,&l_3638},{&g_2981,&l_3638,&g_892,&g_2981,&g_114,&g_2981}}};
            int i, j, k;
            l_4376 = l_4512;
        }
        for (g_121 = 0; (g_121 >= 47); g_121 = safe_add_func_uint32_t_u_u(g_121, 3))
        {
            int16_t l_4524 = 0xD4FBL;
            union U3 l_4542 = {0};
            int32_t l_4543 = 1L;
            struct S0 *l_4550 = &g_4551;
            struct S0 **l_4552 = (void*)0;
            struct S0 **l_4553 = (void*)0;
            struct S0 **l_4554 = &l_4550;
            uint64_t *l_4555 = &g_2234[4];
            uint32_t l_4627[7] = {4294967292UL,0x6D8CEC57L,4294967292UL,4294967292UL,0x6D8CEC57L,4294967292UL,4294967292UL};
            int32_t l_4628 = 0x0D797F2BL;
            const uint16_t l_4640 = 1UL;
            int32_t l_4667 = (-4L);
            int32_t l_4670 = 0x0863DDFFL;
            int32_t l_4675 = 0x272CCA4AL;
            int32_t l_4678 = 0x8716EA4AL;
            int32_t l_4680 = 7L;
            int32_t l_4681 = 1L;
            int32_t l_4682 = (-1L);
            int32_t l_4683 = (-10L);
            int32_t l_4684 = 0x2CC78C3BL;
            int32_t l_4685[6] = {1L,1L,(-1L),1L,1L,(-1L)};
            int i;
        }
    }
    if ((safe_sub_func_int8_t_s_s((((**g_3959) = (safe_div_func_int8_t_s_s((safe_sub_func_uint32_t_u_u(0x62AAA903L, (safe_mod_func_uint64_t_u_u(((safe_unary_minus_func_int16_t_s((((((**g_249) = (safe_rshift_func_int8_t_s_u(((safe_add_func_int32_t_s_s((((((safe_div_func_uint8_t_u_u((safe_mul_func_uint8_t_u_u(((void*)0 != &g_3172), (((safe_add_func_int8_t_s_s((**g_3959), (safe_mod_func_int64_t_s_s((((*l_3908) = (*l_4416)) == ((*l_4717) = (l_4028 , ((*l_4714) = ((*l_4713) = l_4711))))), 4L)))) | ((***g_3958) >= 8L)) | 4294967295UL))), (***g_3958))) , (-1L)) | p_26) , p_25) && (***g_248)), 4294967295UL)) ^ (*l_3710)), p_25))) | p_26) <= p_25) <= 0xECE9L))) && (*l_3710)), (*l_3710))))), p_25))) | (*l_3710)), p_26)))
    {
        struct S1 l_4720 = {-456,5232,54,1,-2128};
        struct S1 *l_4721[6][9] = {{&g_114,&l_3638,&g_2981,&l_3638,&g_114,(void*)0,&g_114,&l_3638,&g_2981},{&l_4028,&l_4028,&l_4376,&l_4028,&l_4028,&l_4376,&l_4028,&l_4028,&l_4376},{&g_114,&l_3638,&g_2981,&l_3638,&g_114,(void*)0,&g_114,&l_3638,&g_2981},{&l_4028,&l_4028,&l_4376,&l_4028,&l_4028,&l_4376,&l_4028,&l_4028,&l_4376},{&g_114,&l_3638,&g_2981,&l_3638,&g_114,(void*)0,&g_114,&l_3638,&g_2981},{&l_4028,&l_4028,&l_4376,&l_4028,&l_4028,&l_4376,&l_4028,&l_4028,&l_4376}};
        const uint32_t *l_4730 = &g_239;
        int32_t l_4736 = 0x2F89057BL;
        int32_t l_4737[10][9][2] = {{{(-1L),(-1L)},{0xE7D97B1BL,0x486A448FL},{0xE7D97B1BL,0xE432C061L},{0x71CC9A37L,0xEF33837BL},{(-2L),0x71CC9A37L},{0L,0x1FE96446L},{0L,0x71CC9A37L},{(-2L),0xEF33837BL},{0x71CC9A37L,0xE432C061L}},{{0xE7D97B1BL,0L},{(-1L),0xC5028124L},{0xC5028124L,0xE432C061L},{6L,0L},{(-2L),6L},{0x9189DFFBL,0x1FE96446L},{0xEF33837BL,(-8L)},{(-2L),0x9189DFFBL},{(-8L),0xE432C061L}},{{0L,0xE7D97B1BL},{(-1L),0xE7D97B1BL},{0L,0xE432C061L},{(-8L),0x9189DFFBL},{(-2L),(-8L)},{0xEF33837BL,0x1FE96446L},{0x9189DFFBL,6L},{(-2L),0L},{6L,0xE432C061L}},{{0xC5028124L,0xC5028124L},{(-1L),0L},{0xE7D97B1BL,0xE432C061L},{0x71CC9A37L,0xEF33837BL},{(-2L),0x71CC9A37L},{0L,0x1FE96446L},{0L,0x71CC9A37L},{(-2L),0xEF33837BL},{0x71CC9A37L,0xE432C061L}},{{0xE7D97B1BL,0L},{(-1L),0xC5028124L},{0xC5028124L,0xE432C061L},{6L,0L},{(-2L),6L},{0x9189DFFBL,0x1FE96446L},{0xEF33837BL,(-8L)},{(-2L),0x9189DFFBL},{(-8L),0xE432C061L}},{{0L,0xE7D97B1BL},{(-1L),0xE7D97B1BL},{0L,0xE432C061L},{(-8L),0x9189DFFBL},{(-2L),(-8L)},{0xEF33837BL,0x1FE96446L},{0x9189DFFBL,6L},{(-2L),0L},{6L,0xE432C061L}},{{0xC5028124L,0xC5028124L},{(-1L),0L},{0xE7D97B1BL,0xE432C061L},{0x71CC9A37L,0xEF33837BL},{(-2L),0x71CC9A37L},{0L,0x1FE96446L},{0L,0x71CC9A37L},{(-2L),0xEF33837BL},{0x71CC9A37L,0xE432C061L}},{{0xE7D97B1BL,0L},{(-1L),0xC5028124L},{0xC5028124L,0xE432C061L},{6L,0L},{(-2L),6L},{0x9189DFFBL,0x1FE96446L},{0xEF33837BL,(-8L)},{(-2L),0x9189DFFBL},{(-8L),0xE432C061L}},{{0L,0xE7D97B1BL},{(-1L),0xE7D97B1BL},{0L,0xE432C061L},{(-8L),0x9189DFFBL},{(-2L),(-8L)},{0xEF33837BL,0x1FE96446L},{0x9189DFFBL,6L},{(-2L),0L},{6L,0xE432C061L}},{{0xC5028124L,0xC5028124L},{(-1L),0L},{0xE7D97B1BL,0xE432C061L},{0x71CC9A37L,0xEF33837BL},{(-2L),0x71CC9A37L},{0L,0x1FE96446L},{0L,0x71CC9A37L},{(-2L),0xEF33837BL},{0x71CC9A37L,0xE432C061L}}};
        int16_t l_4746 = (-1L);
        int i, j, k;
        l_3638 = l_4720;
        for (l_4583 = 0; (l_4583 == 17); l_4583 = safe_add_func_int32_t_s_s(l_4583, 5))
        {
            union U3 l_4729 = {0};
            int32_t l_4731 = 1L;
            int32_t l_4733 = 8L;
            int32_t l_4734 = 0x38C4D495L;
            int32_t l_4735[3];
            int i;
            for (i = 0; i < 3; i++)
                l_4735[i] = 0x108A74E7L;
            for (g_121 = 0; (g_121 <= 8); g_121 = safe_add_func_uint32_t_u_u(g_121, 9))
            {
                uint32_t *l_4728 = &l_4022;
                int32_t *l_4732[3];
                int8_t l_4738 = 0L;
                int i;
                for (i = 0; i < 3; i++)
                    l_4732[i] = &g_1204;
                (*l_3710) |= (safe_rshift_func_int16_t_s_u(0x57C6L, 11));
                (*l_3710) &= (l_4728 != (l_4729 , l_4730));
                --l_4740;
            }
            (*l_3710) = (-8L);
            for (g_1824 = 0; (g_1824 <= 2); g_1824 += 1)
            {
                int32_t **l_4743 = &g_100;
                int i;
                if (g_3562[g_1824])
                    break;
                (*l_4743) = &l_4734;
                (*l_4743) = l_4744[3];
            }
        }
        l_4747--;
        l_4376 = l_3638;
    }
    else
    {
        struct S1 l_4752[5][9] = {{{227,5170,151,4,-899},{202,5124,240,7,-2360},{92,732,125,2,556},{202,5124,240,7,-2360},{227,5170,151,4,-899},{-76,2383,-88,-0,-274},{227,5170,151,4,-899},{202,5124,240,7,-2360},{92,732,125,2,556}},{{-399,851,19,12,2503},{-399,851,19,12,2503},{222,3704,-24,-12,-1020},{-399,851,19,12,2503},{-399,851,19,12,2503},{222,3704,-24,-12,-1020},{-357,2483,240,-5,-2213},{-357,2483,240,-5,-2213},{-399,851,19,12,2503}},{{-370,1781,-107,4,-2796},{-419,2357,-174,0,-1869},{227,5170,151,4,-899},{-419,2357,-174,0,-1869},{-370,1781,-107,4,-2796},{202,5124,240,7,-2360},{-370,1781,-107,4,-2796},{-419,2357,-174,0,-1869},{227,5170,151,4,-899}},{{-357,2483,240,-5,-2213},{-357,2483,240,-5,-2213},{-399,851,19,12,2503},{-357,2483,240,-5,-2213},{-357,2483,240,-5,-2213},{-399,851,19,12,2503},{-357,2483,240,-5,-2213},{-357,2483,240,-5,-2213},{-399,851,19,12,2503}},{{-370,1781,-107,4,-2796},{-419,2357,-174,0,-1869},{227,5170,151,4,-899},{-419,2357,-174,0,-1869},{-370,1781,-107,4,-2796},{202,5124,240,7,-2360},{-370,1781,-107,4,-2796},{-419,2357,-174,0,-1869},{227,5170,151,4,-899}}};
        int32_t l_4772[8][9] = {{0x88FAE51BL,0L,0xB5FA1896L,0xB5FA1896L,0L,0x88FAE51BL,0L,0xB5FA1896L,0xB5FA1896L},{(-1L),(-1L),0x8BF617F9L,0xF9B031ADL,0x8BF617F9L,(-1L),(-1L),0x8BF617F9L,0xF9B031ADL},{0x65171C65L,0L,0x65171C65L,0x88FAE51BL,0x88FAE51BL,0x65171C65L,0L,0x65171C65L,0x88FAE51BL},{0x9FF1576AL,0x8BF617F9L,0x8BF617F9L,0x9FF1576AL,2L,0x9FF1576AL,0x8BF617F9L,0x8BF617F9L,0x9FF1576AL},{0x46AA0E41L,0x88FAE51BL,0xB5FA1896L,0x88FAE51BL,0x46AA0E41L,0x46AA0E41L,0x88FAE51BL,0xB5FA1896L,0x88FAE51BL},{0x8BF617F9L,2L,0xF9B031ADL,0xF9B031ADL,2L,0x8BF617F9L,2L,0xF9B031ADL,0xF9B031ADL},{0x46AA0E41L,0x46AA0E41L,0x88FAE51BL,0xB5FA1896L,0x88FAE51BL,0x46AA0E41L,0x46AA0E41L,0x88FAE51BL,0xB5FA1896L},{0x9FF1576AL,2L,0x9FF1576AL,0x8BF617F9L,0x8BF617F9L,0x9FF1576AL,2L,0x9FF1576AL,0x8BF617F9L}};
        int32_t *l_4805 = &l_4739;
        struct S1 l_4818 = {89,3351,-19,-15,-31};
        struct S1 *l_4819 = &g_892;
        union U3 ***l_4826 = &l_3587;
        uint32_t l_4829 = 0xD1D1E77AL;
        int i, j;
        for (g_1853 = (-14); (g_1853 >= (-16)); --g_1853)
        {
            uint16_t l_4757 = 65527UL;
            const int64_t *l_4794 = (void*)0;
            const int64_t **l_4793 = &l_4794;
            const int64_t ***l_4792 = &l_4793;
            const int64_t *** const *l_4791 = &l_4792;
            int32_t *l_4804 = &l_4739;
            int32_t **l_4806 = &l_4805;
            if ((((1UL <= (*l_3710)) , 0x1404E88A8AAD80AELL) <= ((((0xDCD46E7AL == 4294967295UL) > ((*g_1617) >= (-1L))) , l_4752[4][3]) , ((safe_sub_func_uint64_t_u_u((safe_sub_func_uint8_t_u_u(l_4757, 0xDCL)), 18446744073709551615UL)) < p_26))))
            {
                uint64_t l_4771[10] = {18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL};
                int i;
                (*g_1092) = (*g_1092);
                for (g_1525 = 0; (g_1525 >= 20); g_1525 = safe_add_func_uint32_t_u_u(g_1525, 9))
                {
                    if (l_4757)
                        break;
                }
                l_4772[1][4] |= (((*g_250) = ((*g_1835) >= (safe_div_func_uint64_t_u_u(((((*g_1617) = (*l_3710)) && l_4757) || (((~((**l_4596) = l_4752[4][3].f1)) , ((*g_1356)++)) , ((~(!(l_4752[4][3].f4 , ((--(*g_1356)) ^ (safe_lshift_func_uint16_t_u_s((((l_4752[4][3] , (l_4771[4] , p_25)) , ((*g_1616) = (*g_1616))) == &p_25), p_25)))))) < l_4757))), 0xF53979C900D4E628LL)))) > p_26);
            }
            else
            {
                struct S1 l_4783[1][3] = {{{98,2256,-119,6,2401},{98,2256,-119,6,2401},{98,2256,-119,6,2401}}};
                int i, j;
                (***g_2738) = ((safe_mod_func_int64_t_s_s((safe_div_func_int64_t_s_s(((***l_4343) = p_26), (p_25 , (safe_mul_func_int16_t_s_s((safe_lshift_func_uint16_t_u_u((((l_4772[1][4] <= ((void*)0 == (*g_3527))) || l_4752[4][3].f2) < ((l_4757 > (safe_sub_func_uint32_t_u_u(((void*)0 == (*g_2283)), p_25))) > (*g_1617))), p_26)), l_4752[4][3].f3))))), 0x263817A203AF5218LL)) , (void*)0);
                (*l_3710) &= 1L;
                g_114 = l_4783[0][0];
            }
            if ((safe_lshift_func_uint8_t_u_u(p_25, 4)))
            {
                uint8_t l_4786 = 1UL;
                if (l_4786)
                    break;
            }
            else
            {
                for (p_25 = 0; (p_25 <= 5); p_25 += 1)
                {
                    const int64_t *** const **l_4795 = (void*)0;
                    const int64_t *** const **l_4796 = (void*)0;
                    const int64_t *** const **l_4797[1][10][9] = {{{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791},{&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791,&l_4791}}};
                    int i, j, k;
                    if ((safe_mul_func_int8_t_s_s(g_469[p_25], (safe_mod_func_int64_t_s_s(((((l_4798 = l_4791) != (void*)0) , g_469[p_25]) < p_26), (safe_add_func_uint64_t_u_u((**g_1616), (safe_rshift_func_int16_t_s_s(p_25, ((***g_248) = p_25))))))))))
                    {
                        return p_25;
                    }
                    else
                    {
                        int32_t *l_4803[6][9][4] = {{{&g_1204,&l_3590,&g_82,(void*)0},{(void*)0,(void*)0,&g_3715,&l_3590},{&g_3715,&l_3590,&l_3717[2],&l_4772[1][4]},{(void*)0,&g_1204,&l_3717[2],(void*)0},{&g_266[1][0],&g_117,&g_1853,&l_3590},{&g_1853,&g_117,&l_4772[1][4],&g_3844},{&l_3717[2],&g_266[1][0],(void*)0,&g_1853},{&l_4772[5][8],&l_4772[7][5],(void*)0,&g_82},{(void*)0,(void*)0,(void*)0,(void*)0}},{{&l_3717[2],&g_3715,&g_1853,&l_4739},{&l_4772[1][4],&l_3717[6],(void*)0,(void*)0},{(void*)0,(void*)0,&g_82,&l_3717[0]},{&l_3717[2],&g_3844,&l_4772[2][1],&l_3717[2]},{&g_82,&l_3590,(void*)0,&g_3715},{&g_3844,&l_3590,(void*)0,&l_4772[2][1]},{&g_266[1][0],&l_4772[1][4],&l_4772[1][4],&g_266[1][0]},{(void*)0,&g_3715,&g_1204,&g_3844},{(void*)0,&g_1853,&g_266[4][7],&g_3715}},{{&l_4739,&l_3590,(void*)0,&g_3715},{&l_3717[2],&g_1853,&l_3717[4],&g_3844},{&g_3844,&g_3715,&l_4772[2][2],&g_266[1][0]},{&l_3717[6],&l_4772[1][4],&l_3590,&l_4772[2][1]},{(void*)0,&l_3590,(void*)0,&g_3715},{&g_266[4][7],&l_3590,&l_4772[5][8],&l_3717[2]},{&g_3715,&g_3844,&g_266[4][7],&l_3717[0]},{&l_3590,(void*)0,(void*)0,(void*)0},{&g_1853,&l_3717[6],&l_4772[4][6],&l_4739}},{{&l_4739,&g_3715,&l_4772[1][4],(void*)0},{&g_3844,(void*)0,&g_3844,&g_82},{&l_4739,&l_4772[7][5],&g_3715,&g_1853},{&g_266[4][7],&g_266[1][0],&g_3844,&g_3844},{&l_3717[2],&g_117,(void*)0,&l_3590},{&g_82,&g_117,&l_3590,(void*)0},{&g_1204,&g_1204,&l_3717[4],&l_4772[1][4]},{&l_4739,&l_3590,&l_4772[2][1],&l_3590},{&l_4772[5][8],(void*)0,&g_3844,(void*)0}},{{&l_3590,&l_3590,&l_3717[2],&l_4739},{(void*)0,&g_82,&l_3717[6],&l_4739},{(void*)0,&g_117,&l_3717[2],&l_3717[2]},{&l_3590,&l_4739,&g_3844,&g_3715},{&l_4772[5][8],&l_3717[2],&l_4772[2][1],&l_3717[0]},{&l_4739,&g_266[1][0],&l_3717[4],(void*)0},{&g_1204,&g_3844,&l_3590,&l_4772[2][2]},{&g_82,&l_4772[1][4],(void*)0,&l_4739},{&g_82,&g_266[4][7],(void*)0,&l_3590}},{{&g_3844,(void*)0,&l_4772[1][4],&g_266[4][7]},{&g_3844,(void*)0,&l_4772[1][4],&l_4772[2][2]},{(void*)0,&l_3717[2],&l_4772[5][8],&l_3717[0]},{&g_1853,&l_4739,&l_4772[4][4],&l_4772[1][4]},{&g_82,&l_4739,&l_4772[2][1],&g_1204},{&l_4772[7][5],&g_117,&g_3844,(void*)0},{&l_4772[1][4],&l_4772[1][4],&g_1853,&g_82},{&g_3844,&g_82,&l_3590,(void*)0},{&g_3715,&l_3590,&l_4772[4][6],&l_4772[4][6]}}};
                        int i, j, k;
                        l_4804 = l_4803[1][1][1];
                        return p_25;
                    }
                }
            }
            l_4805 = &l_4772[4][4];
            (*l_4806) = l_4804;
        }
        for (g_1047 = 1; (g_1047 <= 10); g_1047++)
        {
            const struct S2 * const l_4809 = &g_4810[0];
            const struct S2 *l_4812 = &g_4813;
            const struct S2 **l_4811 = &l_4812;
            (*l_4811) = l_4809;
        }
        (*l_3710) &= (((*l_4805) = ((((((*l_3767) = (safe_sub_func_uint8_t_u_u((safe_lshift_func_int8_t_s_s(((*l_4805) && (((*l_4819) = l_4818) , p_26)), 5)), ((**l_3668) = ((safe_div_func_int16_t_s_s((safe_add_func_int32_t_s_s((l_3638 , (-1L)), (*l_4805))), (*l_4805))) | 0x64AAL))))) , (**g_3959)) <= p_26) | (*l_4805)) <= p_25)) , (*l_4805));
        (*l_4805) &= l_4837;
    }
    return p_26;
}







static uint64_t func_27(uint32_t p_28, const int8_t p_29, const struct S1 p_30)
{
    int16_t *l_3096 = (void*)0;
    const int16_t *l_3098 = &g_3099;
    const int16_t **l_3097 = &l_3098;
    int32_t l_3117 = 0x126B945CL;
    int32_t l_3144 = 1L;
    int8_t *l_3145[3];
    struct S0 **l_3170 = &g_1470;
    struct S0 ***l_3169 = &l_3170;
    const uint64_t *l_3222 = &g_2234[5];
    const uint64_t ** const l_3221 = &l_3222;
    int32_t l_3251 = 1L;
    int32_t l_3257 = 1L;
    int32_t l_3260 = 1L;
    int32_t l_3294 = 0xE26008C7L;
    int32_t l_3295 = 0L;
    uint64_t l_3296 = 0x760647247EF6028ALL;
    int64_t l_3303 = (-1L);
    const union U3 *l_3363 = &g_1368;
    const union U3 **l_3362[6] = {&l_3363,&l_3363,&l_3363,&l_3363,&l_3363,&l_3363};
    const union U3 ***l_3361 = &l_3362[4];
    const union U3 ****l_3360 = &l_3361;
    int32_t *l_3403 = &g_1853;
    int32_t **l_3402 = &l_3403;
    const struct S2 *l_3433 = &g_3434;
    const struct S2 **l_3432 = &l_3433;
    const struct S2 *** const l_3431[9] = {&l_3432,(void*)0,&l_3432,(void*)0,&l_3432,(void*)0,&l_3432,(void*)0,&l_3432};
    int32_t l_3450 = 0xA4E69982L;
    int32_t l_3452 = 0L;
    int32_t l_3454 = 0xEB9F2AE8L;
    int32_t l_3455 = 8L;
    uint16_t ****l_3525 = (void*)0;
    uint16_t *****l_3524 = &l_3525;
    int32_t l_3553 = 2L;
    int16_t l_3563 = 0L;
    uint16_t l_3581 = 65526UL;
    const uint64_t ** const *l_3585 = &l_3221;
    const uint64_t ** const **l_3584 = &l_3585;
    int i;
    for (i = 0; i < 3; i++)
        l_3145[i] = &g_53;
    if ((l_3096 == ((*l_3097) = (*g_249))))
    {
        int16_t l_3107 = 0xF92FL;
        union U3 l_3115 = {0};
        int32_t *l_3118 = &g_117;
        int8_t *l_3141[10][9] = {{&g_53,&g_3055,&g_3055,&g_3055,&g_3055,&g_53,&g_58,&g_58,&g_53},{&g_53,&g_2209,&g_2209,&g_2209,&g_53,&g_2209,&g_2209,&g_2209,&g_53},{&g_2209,&g_53,&g_53,&g_2209,&g_53,&g_58,&g_58,&g_53,&g_3055},{&g_53,&g_2209,&g_53,&g_53,(void*)0,&g_2209,(void*)0,&g_53,&g_53},{&g_3055,&g_3055,&g_58,&g_53,&g_53,&g_53,&g_3055,&g_58,&g_2209},{&g_53,(void*)0,&g_58,&g_53,&g_53,&g_53,&g_58,(void*)0,&g_53},{&g_53,&g_3055,&g_58,&g_2209,&g_3055,&g_58,&g_53,&g_53,&g_53},{(void*)0,(void*)0,&g_53,&g_2209,&g_53,(void*)0,(void*)0,&g_2209,(void*)0},{&g_53,&g_3055,&g_53,&g_3055,&g_2209,&g_53,&g_58,&g_58,&g_53},{&g_53,&g_2209,&g_2209,&g_2209,&g_53,&g_2209,&g_2209,&g_2209,&g_53}};
        struct S0 **l_3167 = &g_1470;
        int32_t l_3188 = 1L;
        uint32_t l_3191 = 4294967294UL;
        uint32_t l_3220 = 0xCFF51FF6L;
        int32_t l_3256 = 0L;
        int32_t l_3259 = 1L;
        uint32_t l_3261 = 18446744073709551614UL;
        uint32_t l_3345 = 1UL;
        struct S1 l_3350[9][8] = {{{-384,3833,7,14,13},{459,5487,-141,-3,1288},{-140,1562,-74,-4,850},{7,1243,11,-4,1587},{368,259,176,1,-970},{7,1243,11,-4,1587},{-140,1562,-74,-4,850},{459,5487,-141,-3,1288}},{{-22,1052,-21,-1,1404},{368,259,176,1,-970},{404,2860,-130,0,1560},{-339,866,74,-11,-558},{-121,1690,58,-5,2737},{-260,529,-61,-1,-774},{-384,3833,7,14,13},{313,4731,231,-6,-2858}},{{-339,866,74,-11,-558},{-302,2723,157,-0,1511},{459,5487,-141,-3,1288},{404,2860,-130,0,1560},{-22,1052,-21,-1,1404},{-384,3833,7,14,13},{-384,3833,7,14,13},{-22,1052,-21,-1,1404}},{{-260,529,-61,-1,-774},{404,2860,-130,0,1560},{404,2860,-130,0,1560},{-260,529,-61,-1,-774},{459,5487,-141,-3,1288},{-121,1690,58,-5,2737},{-140,1562,-74,-4,850},{45,5660,251,0,-355}},{{459,5487,-141,-3,1288},{-121,1690,58,-5,2737},{-140,1562,-74,-4,850},{45,5660,251,0,-355},{-22,1600,-215,-8,1121},{-189,2570,-177,5,-2488},{-22,1052,-21,-1,1404},{-302,2723,157,-0,1511}},{{7,1243,11,-4,1587},{-121,1690,58,-5,2737},{-501,2212,86,-3,-821},{508,3251,51,-3,-1232},{-501,2212,86,-3,-821},{-121,1690,58,-5,2737},{-302,2723,157,-0,1511},{-260,529,-61,-1,-774}},{{-384,3833,7,14,13},{368,259,176,1,-970},{-260,529,-61,-1,-774},{-501,2212,86,-3,-821},{-189,2570,-177,5,-2488},{299,73,64,-8,2873},{-22,1600,-215,-8,1121},{404,2860,-130,0,1560}},{{404,2860,-130,0,1560},{45,5660,251,0,-355},{-339,866,74,-11,-558},{-140,1562,-74,-4,850},{-189,2570,-177,5,-2488},{-189,2570,-177,5,-2488},{-140,1562,-74,-4,850},{-339,866,74,-11,-558}},{{-384,3833,7,14,13},{-384,3833,7,14,13},{-22,1052,-21,-1,1404},{404,2860,-130,0,1560},{459,5487,-141,-3,1288},{-302,2723,157,-0,1511},{-339,866,74,-11,-558},{7,1243,11,-4,1587}}};
        uint32_t l_3376 = 0UL;
        union U3 **l_3379 = &g_458;
        union U3 ***l_3378 = &l_3379;
        union U3 ****l_3377 = &l_3378;
        uint32_t l_3436[7][4][6] = {{{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L}},{{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L}},{{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L}},{{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L}},{{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L}},{{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L}},{{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x5FC0E5A0L,0x71E9E112L,0x71E9E112L},{18446744073709551611UL,0x62C53BA7L,0x62C53BA7L,18446744073709551611UL,0x71E9E112L,0x7966B754L},{0xF66B21E7L,0x62C53BA7L,0x71E9E112L,0xF66B21E7L,0x71E9E112L,0x62C53BA7L},{0x5FC0E5A0L,0x62C53BA7L,0x7966B754L,0x71E9E112L,7UL,7UL}}};
        int16_t l_3446 = 0x028BL;
        int32_t l_3448 = (-3L);
        int32_t l_3449 = 0x50C06214L;
        int32_t l_3456 = 0x633D7938L;
        uint64_t l_3457 = 0x2518DDB3607B71DELL;
        int i, j, k;
        if ((safe_sub_func_int16_t_s_s(((*g_250) = (*g_250)), (safe_add_func_int64_t_s_s((!((safe_rshift_func_int16_t_s_s(l_3107, 2)) > (254UL ^ ((g_3108 , (safe_lshift_func_uint16_t_u_u((safe_add_func_uint64_t_u_u((safe_sub_func_uint32_t_u_u(((p_28 , l_3115) , 9UL), (~0x5E884CEEL))), (l_3117 <= p_30.f3))), 3))) , l_3107)))), (*g_1835))))))
        {
            int32_t **l_3119 = &g_100;
            struct S0 *l_3139 = &g_3140;
            int16_t l_3143 = 0x25B5L;
            const int64_t l_3158 = 0L;
            int32_t l_3184 = 0x24FF543BL;
            int32_t l_3185 = (-4L);
            int32_t l_3186 = 1L;
            int32_t l_3187 = 0xE9B48A61L;
            int32_t l_3189 = 2L;
            uint8_t ** const *l_3233 = (void*)0;
            int64_t l_3286 = (-1L);
lbl_3175:
            (*l_3119) = l_3118;
            if (p_30.f4)
            {
                const int32_t *l_3120 = &g_266[4][6];
                struct S1 l_3142[4][7] = {{{284,893,-44,-4,-1169},{48,5186,145,13,-683},{-10,1114,-62,-1,573},{-434,127,22,3,-511},{-428,1128,230,-9,1055},{-434,127,22,3,-511},{-10,1114,-62,-1,573}},{{381,3787,-157,9,-931},{381,3787,-157,9,-931},{-25,1411,-18,-9,-1768},{-384,4630,-141,5,1893},{48,5186,145,13,-683},{-300,3309,235,4,2725},{284,893,-44,-4,-1169}},{{284,893,-44,-4,-1169},{-434,127,22,3,-511},{-25,1411,-18,-9,-1768},{-25,1411,-18,-9,-1768},{-434,127,22,3,-511},{284,893,-44,-4,-1169},{-428,1128,230,-9,1055}},{{-408,5214,-35,-3,-1238},{-25,1411,-18,-9,-1768},{-10,1114,-62,-1,573},{-428,1128,230,-9,1055},{48,5186,145,13,-683},{48,5186,145,13,-683},{-428,1128,230,-9,1055}}};
                uint16_t * const **l_3163 = (void*)0;
                uint16_t * const ***l_3162[1][2];
                uint16_t * const ****l_3161 = &l_3162[0][0];
                int32_t l_3190 = 0x6848BF7DL;
                uint32_t l_3208 = 0x19DBC56EL;
                struct S2 ***l_3228[10];
                int32_t l_3241 = (-7L);
                int32_t l_3264[2];
                uint16_t l_3265 = 0x119EL;
                int i, j;
                for (i = 0; i < 1; i++)
                {
                    for (j = 0; j < 2; j++)
                        l_3162[i][j] = &l_3163;
                }
                for (i = 0; i < 10; i++)
                    l_3228[i] = (void*)0;
                for (i = 0; i < 2; i++)
                    l_3264[i] = 0xE3AF02F4L;
                for (g_2099 = 0; (g_2099 <= 2); g_2099 += 1)
                {
                    struct S1 l_3122 = {-467,2922,-20,3,1290};
                    for (g_1047 = 0; (g_1047 <= 2); g_1047 += 1)
                    {
                        const int32_t **l_3121 = &g_383;
                        (*l_3121) = l_3120;
                    }
                    l_3122 = l_3122;
                    if (p_30.f4)
                        break;
                    for (g_1401 = 0; (g_1401 <= 5); g_1401 += 1)
                    {
                        (*l_3119) = &l_3117;
                    }
                }
                for (g_3092 = 0; (g_3092 <= 5); g_3092 += 1)
                {
                    int32_t l_3148 = 0x050F2234L;
                    int i;
                    (*g_258) = func_31((((((**l_3119) = (g_1928[g_3092] != g_1928[g_3092])) | ((safe_lshift_func_uint16_t_u_s(p_28, (((safe_lshift_func_uint16_t_u_s((safe_sub_func_uint8_t_u_u((**g_2196), p_28)), ((***g_248) = (safe_lshift_func_uint8_t_u_u((safe_mul_func_uint16_t_u_u(((((safe_mul_func_int16_t_s_s((safe_add_func_uint32_t_u_u(((l_3142[3][0] , (void*)0) == &g_2747), 0xF1FB699BL)), p_30.f3)) , l_3143) > l_3144) , p_30.f1), l_3144)), 0))))) , 0xA845CF0F440AE4F6LL) && l_3144))) != (*l_3120))) != p_30.f0) ^ (-1L)), l_3145[2], (*g_499), (**g_2196), p_30.f1);
                    for (g_1824 = 0; (g_1824 <= 5); g_1824 += 1)
                    {
                        const uint16_t l_3164 = 0x54BAL;
                        (*g_258) = func_31((((*l_3118) = (*l_3120)) != (((safe_add_func_uint32_t_u_u(((l_3148 , (safe_rshift_func_int16_t_s_s(p_30.f1, (safe_lshift_func_uint8_t_u_s(((p_28 || (*g_499)) , (p_30.f1 == (safe_unary_minus_func_int32_t_s((((safe_mod_func_int64_t_s_s(((*g_1835) = (safe_rshift_func_uint16_t_u_u(l_3158, 10))), ((**g_1616) = (safe_mul_func_uint16_t_u_u(((void*)0 != l_3161), (-7L)))))) >= p_28) & 0UL))))), p_28))))) != 65530UL), l_3148)) , 1UL) , 1L)), &g_3055, l_3148, (*l_3120), l_3164);
                    }
                }
                if (((***g_248) , l_3117))
                {
                    struct S0 ***l_3168 = &g_2787;
                    int32_t l_3173[7][7];
                    int64_t l_3174 = 1L;
                    int i, j;
                    for (i = 0; i < 7; i++)
                    {
                        for (j = 0; j < 7; j++)
                            l_3173[i][j] = 0xE4584282L;
                    }
                    if (((safe_add_func_int8_t_s_s((((p_30.f4 , l_3167) == ((*l_3168) = &g_1470)) >= ((l_3169 == g_3171) & (l_3173[1][6] ^ ((&g_499 == (void*)0) < (0x9325L >= 0x12F3L))))), l_3174)) & p_30.f3))
                    {
                        int32_t *l_3176 = &l_3117;
                        int32_t *l_3177 = &g_1204;
                        int32_t *l_3178 = &g_1853;
                        int32_t *l_3179 = &g_117;
                        int32_t *l_3180 = &l_3173[1][6];
                        int32_t *l_3181 = (void*)0;
                        int32_t *l_3182 = &g_82;
                        int32_t *l_3183[6][1];
                        int i, j;
                        for (i = 0; i < 6; i++)
                        {
                            for (j = 0; j < 1; j++)
                                l_3183[i][j] = (void*)0;
                        }
                        if (p_30.f0)
                            goto lbl_3175;
                        l_3191--;
                    }
                    else
                    {
                        (*g_1092) = (*g_1092);
                        return (*g_1617);
                    }
                }
                else
                {
                    const int32_t l_3210 = (-1L);
                    int32_t *l_3212 = (void*)0;
                    int8_t *l_3223 = &g_53;
                    int8_t l_3252[4][9][5];
                    int32_t l_3258[8] = {0L,6L,0L,0L,6L,0L,0L,6L};
                    struct S1 l_3280 = {471,3964,201,-14,1470};
                    int i, j, k;
                    for (i = 0; i < 4; i++)
                    {
                        for (j = 0; j < 9; j++)
                        {
                            for (k = 0; k < 5; k++)
                                l_3252[i][j][k] = (-1L);
                        }
                    }
                    for (g_1401 = 0; (g_1401 > 5); g_1401++)
                    {
                        uint32_t *l_3209[9] = {&g_239,&g_1525,&g_1525,&g_239,&g_1525,&g_1525,&g_239,&g_1525,&g_1525};
                        int32_t *l_3211 = &l_3185;
                        uint64_t ***l_3216[1];
                        uint64_t ****l_3215 = &l_3216[0];
                        int32_t l_3218 = 0x8804B2B7L;
                        int i;
                        for (i = 0; i < 1; i++)
                            l_3216[i] = &g_1616;
                    }
                    for (l_3107 = 0; (l_3107 <= 7); l_3107 += 1)
                    {
                        uint8_t **l_3235 = &g_499;
                        uint8_t ***l_3234 = &l_3235;
                        int32_t l_3240[8][8] = {{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L},{0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L,0xB6519585L,0xA0388008L}};
                        int i, j;
                        (**l_3119) = (safe_mul_func_int8_t_s_s((((l_3144 | (((safe_mod_func_uint16_t_u_u(((void*)0 == l_3228[7]), (safe_rshift_func_int8_t_s_u((safe_sub_func_int16_t_s_s((l_3233 != l_3234), (safe_add_func_int32_t_s_s((((4294967291UL != ((*g_499) > ((p_30.f3 >= (safe_rshift_func_int16_t_s_s((l_3144 | 0L), (*g_250)))) <= l_3144))) ^ l_3240[7][1]) < p_28), 2UL)))), p_30.f0)))) != 0x2D0677C3L) && l_3241)) , l_3144) < p_30.f1), p_30.f0));
                        l_3252[0][5][2] ^= (safe_mod_func_uint8_t_u_u((0x90B7L > (p_30.f2 > ((*g_458) , ((*l_3120) && (((((*g_1356) = (+((***g_248) = ((((safe_add_func_uint32_t_u_u((p_28 = (safe_rshift_func_int16_t_s_s((p_28 < (p_30.f4 < (**l_3119))), ((-3L) != (*g_1835))))), l_3240[2][7])) >= 0L) | 4L) != (****g_2283))))) < p_30.f2) > (**l_3119)) , (-2L)))))), l_3251));
                    }
                    for (l_3241 = 18; (l_3241 > 13); l_3241 = safe_sub_func_uint64_t_u_u(l_3241, 1))
                    {
                        int32_t *l_3255[5][10][3] = {{{&l_3144,(void*)0,&l_3117},{(void*)0,(void*)0,(void*)0},{&l_3144,&l_3185,&l_3117},{&l_3144,&g_1853,&l_3144},{(void*)0,&l_3185,&l_3144},{&l_3144,(void*)0,&l_3117},{(void*)0,(void*)0,(void*)0},{&l_3144,&l_3185,&l_3117},{&l_3144,&g_1853,&l_3144},{(void*)0,&l_3185,&l_3144}},{{&l_3144,(void*)0,&l_3117},{(void*)0,(void*)0,(void*)0},{&l_3144,&l_3185,&l_3117},{&l_3144,&g_1853,&l_3144},{(void*)0,&l_3185,&l_3144},{&l_3144,(void*)0,&l_3117},{(void*)0,(void*)0,(void*)0},{&l_3144,&l_3185,&l_3117},{&l_3144,&g_1853,&l_3144},{(void*)0,&l_3185,&l_3144}},{{&l_3144,(void*)0,&l_3117},{(void*)0,(void*)0,(void*)0},{&l_3144,&l_3185,&l_3117},{&l_3144,&g_1853,&l_3144},{(void*)0,&l_3185,&l_3144},{&l_3144,(void*)0,&l_3117},{(void*)0,&l_3144,(void*)0},{&l_3187,(void*)0,&g_1853},{&l_3187,&l_3144,(void*)0},{(void*)0,(void*)0,(void*)0}},{{(void*)0,&l_3144,&g_1853},{(void*)0,&l_3144,(void*)0},{&l_3187,(void*)0,&g_1853},{&l_3187,&l_3144,(void*)0},{(void*)0,(void*)0,(void*)0},{(void*)0,&l_3144,&g_1853},{(void*)0,&l_3144,(void*)0},{&l_3187,(void*)0,&g_1853},{&l_3187,&l_3144,(void*)0},{(void*)0,(void*)0,(void*)0}},{{(void*)0,&l_3144,&g_1853},{(void*)0,&l_3144,(void*)0},{&l_3187,(void*)0,&g_1853},{&l_3187,&l_3144,(void*)0},{(void*)0,(void*)0,(void*)0},{(void*)0,&l_3144,&g_1853},{(void*)0,&l_3144,(void*)0},{&l_3187,(void*)0,&g_1853},{&l_3187,&l_3144,(void*)0},{(void*)0,(void*)0,(void*)0}}};
                        int i, j, k;
                        l_3255[0][5][2] = &l_3188;
                        l_3261--;
                        ++l_3265;
                    }
                    l_3280 = ((***g_2379) = func_31(p_30.f0, &g_53, (safe_sub_func_int16_t_s_s((safe_lshift_func_int8_t_s_u((p_30.f2 , (*g_52)), (safe_add_func_int64_t_s_s((safe_add_func_int16_t_s_s((*l_3120), p_30.f3)), (*g_1835))))), (((((safe_add_func_int16_t_s_s((safe_div_func_uint16_t_u_u((((p_30.f3 , ((*g_1617) , (**l_3119))) , (*l_3120)) > (*l_3118)), 0x5F24L)), p_30.f1)) || (*l_3120)) >= l_3251) || p_30.f3) > 0xA0912A6BL))), (**l_3119), (*l_3118)));
                }
                if (g_114.f1)
                    goto lbl_3175;
            }
            else
            {
                int32_t l_3284 = 0L;
                int32_t l_3285 = 0x44C9ADFBL;
                int32_t l_3289 = (-3L);
                int32_t l_3290[8][6][3];
                const union U3 *l_3302 = (void*)0;
                const union U3 **l_3301 = &l_3302;
                const union U3 ***l_3300 = &l_3301;
                const union U3 ****l_3299 = &l_3300;
                int i, j, k;
                for (i = 0; i < 8; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 3; k++)
                            l_3290[i][j][k] = 4L;
                    }
                }
                for (g_3055 = 4; (g_3055 >= 1); g_3055 -= 1)
                {
                    int16_t l_3287 = 0x6E47L;
                    int32_t l_3288 = 0xAED70A26L;
                    int32_t l_3291 = 2L;
                    int32_t l_3293 = (-8L);
                    for (l_3187 = 5; (l_3187 >= 0); l_3187 -= 1)
                    {
                        int32_t *l_3281 = &g_117;
                        int32_t *l_3282[8];
                        int16_t l_3283[4][10][6] = {{{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L}},{{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L}},{{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L}},{{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L},{0x0118L,1L,0x0118L,1L,0x0118L,1L}}};
                        int8_t l_3292[3];
                        int i, j, k;
                        for (i = 0; i < 8; i++)
                            l_3282[i] = &l_3188;
                        for (i = 0; i < 3; i++)
                            l_3292[i] = 2L;
                        ++l_3296;
                    }
                    for (l_3256 = 0; (l_3256 <= 5); l_3256 += 1)
                    {
                        if (l_3261)
                            goto lbl_3175;
                    }
                }
                (*l_3299) = (void*)0;
                (*g_100) &= l_3303;
            }
        }
        else
        {
            uint8_t l_3337 = 0x08L;
            int8_t l_3338 = 0xC8L;
            int32_t l_3339 = 0xAD263DCDL;
            uint8_t l_3375 = 0x5AL;
            uint64_t l_3400 = 0x0726887FCC3D0EF3LL;
            union U3 *l_3410[3][5] = {{&g_1368,&g_1368,&g_1368,&g_1368,&g_1368},{&l_3115,&l_3115,&l_3115,&l_3115,&l_3115},{&g_1368,&g_1368,&g_1368,&g_1368,&g_1368}};
            int32_t l_3438 = 0x4F362D01L;
            int8_t l_3440 = 0xCAL;
            int32_t l_3442 = 1L;
            int32_t l_3443 = 0x00319BBBL;
            int32_t l_3444[4];
            int32_t l_3447[4][9] = {{0x25099F5AL,1L,0x25099F5AL,0x25099F5AL,1L,0x25099F5AL,0x25099F5AL,1L,0x25099F5AL},{4L,0x613B5239L,4L,4L,0x613B5239L,4L,4L,0x613B5239L,4L},{0x25099F5AL,1L,0x25099F5AL,0x25099F5AL,1L,0x25099F5AL,0x25099F5AL,1L,0x25099F5AL},{4L,0x613B5239L,4L,4L,0x613B5239L,4L,4L,0x613B5239L,4L}};
            int64_t *l_3493 = &g_236[2][0];
            const uint64_t **l_3505[4];
            const uint64_t ***l_3504 = &l_3505[0];
            const uint64_t ****l_3503 = &l_3504;
            const uint64_t *****l_3502 = &l_3503;
            uint16_t *l_3514 = &g_2427;
            uint16_t **l_3513 = &l_3514;
            uint16_t * const *l_3523 = (void*)0;
            uint16_t * const ** const l_3522[4] = {&l_3523,&l_3523,&l_3523,&l_3523};
            uint16_t * const ** const *l_3521[8] = {&l_3522[2],&l_3522[2],&l_3522[2],&l_3522[2],&l_3522[2],&l_3522[2],&l_3522[2],&l_3522[2]};
            uint16_t * const ** const **l_3520 = &l_3521[2];
            struct S1 l_3550[7] = {{43,352,-139,0,-474},{43,352,-139,0,-474},{43,352,-139,0,-474},{43,352,-139,0,-474},{43,352,-139,0,-474},{43,352,-139,0,-474},{43,352,-139,0,-474}};
            int i, j;
            for (i = 0; i < 4; i++)
                l_3444[i] = 0x09625099L;
            for (i = 0; i < 4; i++)
                l_3505[i] = &l_3222;
            for (l_3303 = 0; (l_3303 > 29); ++l_3303)
            {
                uint8_t l_3328 = 0UL;
                int32_t l_3340 = 0L;
                int32_t l_3341[5];
                uint64_t l_3381 = 6UL;
                union U3 *l_3408 = &g_1368;
                int64_t l_3435 = 0x3C5182843A7DC8DBLL;
                struct S2 * const l_3467 = &g_3468;
                int i;
                for (i = 0; i < 5; i++)
                    l_3341[i] = (-3L);
            }
            for (l_3450 = 24; (l_3450 != 15); l_3450--)
            {
                union U3 l_3528[7] = {{0},{0},{0},{0},{0},{0},{0}};
                int32_t l_3541[5][5][7] = {{{0x971F75D4L,(-3L),0x7F9EEC95L,(-8L),4L,1L,0x0EF43BBCL},{0L,0x7F9EEC95L,(-1L),0xBD1D6F7FL,0xEA80A74FL,0x950D9102L,0x5403102FL},{4L,0x5036CED8L,(-1L),1L,1L,(-1L),0x5036CED8L},{(-2L),(-1L),0xBD1D6F7FL,1L,0xA5428EA7L,(-5L),0xCCB91468L},{0x5036CED8L,0xFB629C0AL,0x7321637BL,0x1CD2F5A8L,0xB53CE802L,1L,1L}},{{(-5L),0xCEA0E771L,(-4L),1L,0L,0x706DDE0DL,0x501D9C7AL},{0x4B5666BBL,1L,0x139DFFD1L,1L,0x9C05D1D1L,0xCEA0E771L,0xDB6057C1L},{(-4L),0x9AB473A5L,0x1CA567DAL,0xBD1D6F7FL,0x0EF43BBCL,1L,1L},{1L,0xEA80A74FL,1L,(-8L),0xF5A2F4ACL,3L,0x7321637BL},{0xF930E974L,0xCCB91468L,(-2L),4L,0L,3L,(-1L)}},{{0x9AB473A5L,(-4L),(-3L),(-1L),0x1CD2F5A8L,1L,0x0280DE9BL},{0x4C107D44L,0xF930E974L,(-6L),0L,(-3L),0xCEA0E771L,0xCEA0E771L},{1L,(-4L),0x4B5666BBL,(-4L),1L,0x706DDE0DL,0xF930E974L},{(-8L),(-1L),0x1CD2F5A8L,0x094DDE12L,0xCCB91468L,1L,0x971F75D4L},{0xC9DDB726L,1L,(-6L),0x5403102FL,0x7321637BL,(-5L),0L}},{{(-8L),0x094DDE12L,0L,(-1L),(-5L),(-1L),(-1L)},{1L,0x7321637BL,0x1AD8BFB2L,0x971F75D4L,0x094DDE12L,0x950D9102L,3L},{0x4C107D44L,1L,0x0280DE9BL,0x1CA567DAL,1L,1L,0x7F9EEC95L},{0x9AB473A5L,0L,(-10L),0xFB629C0AL,(-6L),0xA4EEBBD1L,0x4C107D44L},{0xF930E974L,0x0EF43BBCL,(-10L),0x4C107D44L,0x5036CED8L,(-3L),0xEF70DFC9L}},{{1L,(-2L),0x0280DE9BL,0x950D9102L,(-1L),(-4L),(-6L)},{(-4L),0x1CA567DAL,0x1AD8BFB2L,0L,(-1L),0L,0x1AD8BFB2L},{0x4B5666BBL,0x4B5666BBL,0L,0xF5A2F4ACL,0x5403102FL,0xEA80A74FL,0x9AB473A5L},{(-5L),0xB53CE802L,(-6L),(-6L),0L,(-1L),0L},{0x5036CED8L,0x0280DE9BL,0x1CD2F5A8L,0xCCB91468L,0x5403102FL,0x501D9C7AL,1L}}};
                int i, j, k;
                (**l_3402) |= ((l_3520 != (g_3526 = l_3524)) < (((*g_250) &= 0xF92EL) == p_30.f0));
                for (g_1853 = 0; (g_1853 <= 5); g_1853 += 1)
                {
                    return l_3444[0];
                }
                (*g_258) = l_3550[3];
                l_3339 &= ((safe_rshift_func_uint16_t_u_s((&g_499 == (l_3541[2][4][3] , &g_2197[1])), 5)) && ((((l_3444[1] ^= ((**g_2283) == (*g_248))) || (l_3553 , (((safe_lshift_func_uint8_t_u_u((((g_3562[0] |= (((*l_3493) = (safe_mul_func_uint8_t_u_u((safe_add_func_uint64_t_u_u((l_3541[2][4][3] <= (safe_add_func_int16_t_s_s((-1L), (p_30.f4 < 0x5005555DL)))), (**l_3402))), l_3438))) < (*g_1617))) | 1UL) != p_30.f2), (*l_3118))) > p_28) == (*g_52)))) > l_3541[4][3][2]) , l_3563));
            }
            (**g_2379) = (void*)0;
        }
    }
    else
    {
        struct S1 ***l_3570 = &g_2380;
        int32_t l_3578 = (-1L);
        l_3578 = ((((0xE415L ^ (((safe_add_func_uint16_t_u_u(0x80E0L, (&l_3432 == (((((*g_458) , (~((*g_1835) |= ((((~((((safe_rshift_func_int16_t_s_s((((*g_2378) = l_3570) != l_3570), (safe_mod_func_uint32_t_u_u((p_30.f0 != (~((***g_248) = ((p_30.f1 | (((safe_sub_func_uint8_t_u_u((((*g_52) ^= (safe_sub_func_uint32_t_u_u((0x2E2A10A1L == l_3578), (*l_3403)))) >= p_28), 1L)) <= 0xCB0064FBL) && p_30.f3)) == (-5L))))), p_30.f4)))) && (**l_3402)) ^ p_30.f0) > 7L)) , (void*)0) == (void*)0) || (**l_3402))))) >= p_28) & p_29) , &l_3432)))) > l_3578) >= 0xEF21D640882BED89LL)) ^ 0x61L) | p_30.f4) | p_28);
        for (g_107 = 1; (g_107 <= 4); g_107 += 1)
        {
            int i;
            (*l_3403) = (-10L);
            (**l_3402) &= (0x9C986C63L <= (safe_lshift_func_int8_t_s_s(1L, 1)));
            return g_1232[g_107];
        }
    }
    l_3581--;
    (*l_3584) = &l_3221;
    return p_30.f4;
}







static struct S1 func_31(int32_t p_32, int8_t * p_33, uint8_t p_34, uint8_t p_35, const int32_t p_36)
{
    int32_t l_2762 = 0x6406F093L;
    int32_t l_2764 = 1L;
    union U3 **l_2769 = (void*)0;
    union U3 **l_2770 = (void*)0;
    union U3 **l_2771 = &g_458;
    int32_t l_2792 = (-1L);
    uint32_t l_2797 = 0xC958EBD2L;
    const uint64_t ** const *l_2818[1];
    uint32_t **l_2899[6][4] = {{&g_1929,&g_1929,&g_1929,&g_1929},{&g_1929,&g_1929,(void*)0,&g_1929},{&g_1929,(void*)0,(void*)0,&g_1929},{(void*)0,&g_1929,(void*)0,(void*)0},{&g_1929,&g_1929,&g_1929,&g_1929},{&g_1929,(void*)0,(void*)0,&g_1929}};
    uint8_t l_2935 = 0x43L;
    struct S0 *l_3014 = &g_3015;
    int32_t *l_3025[3][5][8] = {{{&g_1204,&l_2764,&g_1204,&g_117,&g_1204,&l_2764,&g_82,&g_1853},{&g_82,&g_1853,&g_82,(void*)0,&l_2764,&l_2762,&g_1204,&l_2792},{&g_82,&l_2762,&l_2762,&g_266[4][7],&g_1204,&g_1204,&g_266[4][7],&l_2762},{&g_1204,&g_1204,&g_82,&l_2764,&g_1853,&l_2762,&l_2792,&g_1853},{&g_1853,&g_82,&g_1204,&l_2762,&g_266[4][7],&l_2792,&l_2764,&g_1853}},{{&g_82,&l_2762,(void*)0,&l_2764,&g_82,&l_2764,(void*)0,&l_2762},{&g_82,&g_82,&g_82,&g_266[4][7],&g_266[3][2],&g_117,&l_2762,&l_2792},{&l_2764,&g_1204,&g_1853,(void*)0,&g_82,(void*)0,&l_2762,&g_1853},{&l_2764,(void*)0,&g_82,&g_117,&g_117,&g_82,(void*)0,&l_2764},{&g_117,&g_82,(void*)0,&l_2764,&g_1204,&g_266[4][7],&l_2764,&g_1853}},{{(void*)0,&g_1853,&g_1204,&l_2764,&l_2764,&g_266[4][7],&l_2792,&l_2764},{&g_266[4][7],&g_82,&g_82,&g_82,&g_82,&g_82,&g_266[4][7],&g_266[3][2]},{&l_2764,(void*)0,&l_2762,&g_82,&l_2792,(void*)0,&g_1204,&g_1204},{&l_2762,&g_1204,&g_82,&g_1853,&l_2792,&g_117,&g_82,&g_82},{&l_2764,&g_82,&g_1204,&g_1204,&g_82,&l_2764,&g_1853,&l_2762}}};
    int32_t *l_3026 = &g_82;
    const struct S1 l_3065 = {322,840,-73,14,-1476};
    uint16_t l_3074[4][10][4] = {{{65535UL,0xB6F5L,65535UL,65535UL},{0xB6F5L,65531UL,0x5582L,0xFA8FL},{0xFA8FL,0x513DL,0x01CBL,65531UL},{65527UL,0x01CBL,0x01CBL,65527UL},{0xFA8FL,65535UL,0x5582L,65535UL},{0xB6F5L,65526UL,65535UL,0xAC87L},{65535UL,0xAC87L,0x76CCL,0xAC87L},{0x01CBL,65526UL,0x513DL,65535UL},{0xC103L,65535UL,0xAC87L,65527UL},{0x513DL,0x01CBL,65531UL,65531UL}},{{0x513DL,0x513DL,0xAC87L,0xFA8FL},{0xC103L,65531UL,0x513DL,65535UL},{0x01CBL,0xB6F5L,0x76CCL,0x513DL},{65535UL,0xB6F5L,65531UL,65526UL},{0xFA8FL,0x01CBL,0x76CCL,65535UL},{65535UL,0xAC87L,65527UL,0x01CBL},{0x513DL,65527UL,65527UL,0x513DL},{65535UL,65526UL,0x76CCL,65531UL},{0xFA8FL,0x5582L,65531UL,0xB6F5L},{65531UL,0xB6F5L,0xC103L,0xB6F5L}},{{65527UL,0x5582L,0xAC87L,65531UL},{65535UL,65526UL,0xB6F5L,0x513DL},{0xAC87L,65527UL,0x01CBL,0x01CBL},{0xAC87L,0xAC87L,0xB6F5L,65535UL},{65535UL,0x01CBL,0xAC87L,65526UL},{65527UL,0xFA8FL,0xC103L,0xAC87L},{65531UL,0xFA8FL,65531UL,65526UL},{0xFA8FL,0x01CBL,0x76CCL,65535UL},{65535UL,0xAC87L,65527UL,0x01CBL},{0x513DL,65527UL,65527UL,0x513DL}},{{65535UL,65526UL,0x76CCL,65531UL},{0xFA8FL,0x5582L,65531UL,0xB6F5L},{65531UL,0xB6F5L,0xC103L,0xB6F5L},{65527UL,0x5582L,0xAC87L,65531UL},{65535UL,65526UL,0xB6F5L,0x513DL},{0xAC87L,65527UL,0x01CBL,0x01CBL},{0xAC87L,0xAC87L,0xB6F5L,65535UL},{65535UL,0x01CBL,0xAC87L,65526UL},{65527UL,0xFA8FL,0xC103L,0xAC87L},{65531UL,0xFA8FL,65531UL,65526UL}}};
    uint64_t *****l_3075 = (void*)0;
    union U3 *l_3095 = (void*)0;
    int i, j, k;
    for (i = 0; i < 1; i++)
        l_2818[i] = (void*)0;
    if (((*g_100) &= (((safe_sub_func_uint32_t_u_u(l_2762, (l_2764 = (!p_36)))) != ((safe_div_func_uint8_t_u_u(((((l_2770 = (l_2769 = l_2769)) == l_2771) , (-3L)) > (((safe_mod_func_uint32_t_u_u((((safe_mod_func_uint8_t_u_u(((((*g_1835) ^= ((safe_lshift_func_int16_t_s_s(((l_2762 & p_34) > (*p_33)), ((*l_2771) != (*l_2771)))) && 4294967288UL)) & p_32) , (*g_499)), 1L)) < 9L) || p_36), l_2762)) , p_36) != l_2762)), p_34)) , l_2762)) >= (*p_33))))
    {
        struct S0 **l_2784[2][2][8];
        int32_t l_2794 = 0xBE862FECL;
        union U3 l_2830 = {0};
        const uint8_t **l_2852 = &g_2197[0];
        uint64_t **** const *l_2898 = (void*)0;
        int32_t l_2904 = (-1L);
        uint8_t l_2905 = 0xBAL;
        int32_t *l_2910 = &g_1204;
        int32_t l_2933 = 0L;
        struct S1 ***l_2945 = &g_2380;
        int i, j, k;
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 2; j++)
            {
                for (k = 0; k < 8; k++)
                    l_2784[i][j][k] = &g_1470;
            }
        }
        for (g_82 = 21; (g_82 > (-4)); g_82--)
        {
            struct S0 ***l_2785 = (void*)0;
            struct S0 ***l_2786[9][8] = {{(void*)0,(void*)0,&l_2784[1][0][6],(void*)0,(void*)0,&l_2784[1][0][6],(void*)0,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0},{(void*)0,(void*)0,&l_2784[1][0][6],(void*)0,(void*)0,&l_2784[1][0][6],(void*)0,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0},{(void*)0,(void*)0,&l_2784[1][0][6],(void*)0,(void*)0,&l_2784[1][0][6],(void*)0,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0},{(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0}};
            uint8_t *l_2793 = &g_1824;
            int8_t *l_2795[6] = {&g_53,&g_53,&g_53,&g_53,&g_53,&g_53};
            int32_t l_2796 = 0L;
            int32_t *l_2798 = &g_266[4][7];
            uint8_t l_2841 = 0x6AL;
            int32_t l_2934 = 1L;
            int i, j;
        }
        if (g_82)
            goto lbl_3027;
    }
    else
    {
        (*g_100) |= (safe_add_func_uint64_t_u_u(((*p_33) ^ l_2935), (safe_mod_func_int64_t_s_s((safe_rshift_func_uint16_t_u_u((0L == 1L), 11)), (p_35 ^ ((*g_499)--))))));
        return (*g_258);
    }
lbl_3027:
    l_3026 = (l_3025[2][4][4] = &l_2792);
    for (g_2568 = 2; (g_2568 >= 0); g_2568 -= 1)
    {
        struct S2 ** const *l_3028 = &g_2746;
        int32_t * const ** const **l_3037[7];
        uint64_t ***l_3078 = &g_1616;
        uint64_t ****l_3077 = &l_3078;
        uint64_t *****l_3076[8][5][6] = {{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}},{{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077},{&l_3077,&l_3077,&l_3077,&l_3077,&l_3077,&l_3077}}};
        uint16_t l_3091 = 0x95F1L;
        int64_t l_3093 = 0xD2D02FBC7390DC93LL;
        uint16_t *l_3094 = &l_3074[1][5][3];
        int i, j, k;
        for (i = 0; i < 7; i++)
            l_3037[i] = (void*)0;
    }
    (*l_3026) = 0L;
    return l_3065;
}







static int8_t * func_37(int8_t * p_38)
{
    struct S1 l_2037[4] = {{-63,2869,-200,14,-1828},{-63,2869,-200,14,-1828},{-63,2869,-200,14,-1828},{-63,2869,-200,14,-1828}};
    int16_t l_2038 = 1L;
    uint64_t *l_2041 = &g_188;
    struct S0 ***l_2051 = (void*)0;
    uint64_t **l_2056 = &g_1617;
    const uint32_t ***l_2111 = &g_2109[2][3];
    int32_t *l_2113 = &g_2077;
    int32_t *l_2117 = &g_82;
    union U3 *l_2195 = &g_49;
    int16_t l_2219 = 0x800BL;
    uint32_t l_2220 = 0UL;
    int16_t l_2241 = 0x68C5L;
    int32_t l_2245 = 0xC87F85A7L;
    int32_t l_2246 = 0L;
    int32_t l_2247 = 2L;
    int32_t l_2248 = 0xE0CA5072L;
    int32_t l_2249 = 7L;
    int32_t l_2250 = 0x62EB62D4L;
    int32_t l_2251 = 0x978E78E5L;
    int32_t l_2252 = 7L;
    int32_t l_2254 = 1L;
    int32_t l_2257 = 0x044C6168L;
    int32_t l_2260 = (-1L);
    int32_t l_2261[7] = {(-5L),(-5L),(-5L),(-5L),(-5L),(-5L),(-5L)};
    int16_t ****l_2282 = &g_248;
    int8_t *l_2310[4];
    uint32_t * const l_2312 = (void*)0;
    uint64_t l_2317 = 18446744073709551615UL;
    const uint64_t l_2344 = 0xC13961912F3A5789LL;
    struct S1 **l_2377 = (void*)0;
    struct S1 ***l_2376[9] = {(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0,(void*)0};
    struct S1 ****l_2375[9] = {&l_2376[4],&l_2376[4],&l_2376[4],&l_2376[4],&l_2376[4],&l_2376[4],&l_2376[4],&l_2376[4],&l_2376[4]};
    int32_t l_2405 = 0x00386451L;
    uint64_t l_2442 = 0x5A2C0974074CA524LL;
    int8_t l_2451[10] = {8L,8L,8L,8L,8L,8L,8L,8L,8L,8L};
    uint32_t l_2460 = 0x6361CC89L;
    uint16_t l_2482 = 65526UL;
    int32_t l_2523[9][5][3] = {{{0x607E1222L,0xF33CCCD7L,(-1L)},{0xEA65E061L,0x62B185F8L,0L},{0x62B185F8L,0xF33CCCD7L,0x88DB94ACL},{0x6C3FE8E9L,0x07B0EF6DL,0x6C3FE8E9L},{0xF33CCCD7L,1L,0x6C3FE8E9L}},{{(-1L),0x34BC6E8FL,0x88DB94ACL},{0x88DB94ACL,0x6C3FE8E9L,0L},{1L,0x04AC60B3L,(-1L)},{0x88DB94ACL,1L,0xEA65E061L},{(-1L),(-1L),0x07B0EF6DL}},{{0xF33CCCD7L,(-1L),0x34BC6E8FL},{0x6C3FE8E9L,1L,(-1L)},{0x62B185F8L,0x04AC60B3L,5L},{0xEA65E061L,0x6C3FE8E9L,(-1L)},{0x607E1222L,0x34BC6E8FL,0x34BC6E8FL}},{{(-10L),1L,0x07B0EF6DL},{(-10L),0x07B0EF6DL,0xEA65E061L},{0x607E1222L,0xF33CCCD7L,(-1L)},{0xEA65E061L,0x62B185F8L,0L},{0x62B185F8L,0xF33CCCD7L,0x88DB94ACL}},{{0x6C3FE8E9L,0x07B0EF6DL,0x6C3FE8E9L},{0xF33CCCD7L,1L,0x6C3FE8E9L},{(-1L),0x34BC6E8FL,0x88DB94ACL},{0x88DB94ACL,0x6C3FE8E9L,0L},{1L,0x04AC60B3L,(-1L)}},{{0x88DB94ACL,1L,0xEA65E061L},{(-1L),(-1L),0x07B0EF6DL},{0xF33CCCD7L,(-1L),0x34BC6E8FL},{0x6C3FE8E9L,1L,(-1L)},{0x62B185F8L,0x04AC60B3L,5L}},{{0xEA65E061L,0x6C3FE8E9L,(-1L)},{0x607E1222L,0x34BC6E8FL,0x34BC6E8FL},{(-10L),1L,0x07B0EF6DL},{(-10L),0x07B0EF6DL,0xEA65E061L},{0x607E1222L,0xF33CCCD7L,(-1L)}},{{0xEA65E061L,0x62B185F8L,0L},{0x62B185F8L,0x34BC6E8FL,0x6C3FE8E9L},{5L,0x04AC60B3L,5L},{0x34BC6E8FL,0x607E1222L,5L},{0xEA65E061L,0L,0x6C3FE8E9L}},{{0x6C3FE8E9L,5L,5L},{0x607E1222L,0x62B185F8L,0xF33CCCD7L},{0x6C3FE8E9L,(-1L),1L},{0xEA65E061L,0xEA65E061L,0x04AC60B3L},{0x34BC6E8FL,0xEA65E061L,0L}}};
    int8_t l_2608 = 0x7AL;
    uint64_t ***l_2625 = &l_2056;
    uint64_t ****l_2624 = &l_2625;
    int32_t ***l_2739[9][9] = {{&g_2477[3][0],&g_2477[0][0],(void*)0,&g_2477[0][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[0][0],(void*)0},{&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0]},{&g_2477[3][0],&g_2477[0][0],(void*)0,&g_2477[0][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[0][0],(void*)0},{&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0]},{&g_2477[3][0],&g_2477[0][0],(void*)0,&g_2477[0][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[0][0],(void*)0},{&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0]},{&g_2477[3][0],&g_2477[0][0],(void*)0,&g_2477[0][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[0][0],(void*)0},{&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0]},{&g_2477[3][0],&g_2477[0][0],(void*)0,&g_2477[0][0],&g_2477[3][0],&g_2477[3][0],&g_2477[3][0],&g_2477[0][0],(void*)0}};
    struct S2 *l_2754 = &g_2755;
    int i, j, k;
    for (i = 0; i < 4; i++)
        l_2310[i] = (void*)0;
    return p_38;
}







static int8_t * func_39(const int8_t * p_40, int8_t * p_41, int8_t p_42, int32_t p_43, int8_t * p_44)
{
    uint16_t *l_2011 = &g_107;
    uint16_t **l_2012 = &l_2011;
    int32_t l_2013 = 0xD7498733L;
    uint16_t *l_2014 = &g_107;
    int32_t l_2030[9][10][2] = {{{0xE6A3A859L,8L},{0x154E9BBAL,0xE6A3A859L},{8L,(-1L)},{8L,0xE6A3A859L},{0x154E9BBAL,8L},{0xE6A3A859L,(-1L)},{1L,1L},{0x154E9BBAL,1L},{1L,(-1L)},{0xE6A3A859L,8L}},{{0x154E9BBAL,0xE6A3A859L},{8L,(-1L)},{8L,0xE6A3A859L},{0x154E9BBAL,8L},{0xE6A3A859L,(-1L)},{1L,1L},{0x154E9BBAL,1L},{1L,(-1L)},{0xE6A3A859L,8L},{0x154E9BBAL,0xE6A3A859L}},{{8L,(-1L)},{8L,0xE6A3A859L},{0x154E9BBAL,8L},{0xE6A3A859L,(-1L)},{1L,1L},{0x154E9BBAL,1L},{1L,(-1L)},{0xE6A3A859L,8L},{0x154E9BBAL,0xE6A3A859L},{8L,(-1L)}},{{8L,0xE6A3A859L},{0x154E9BBAL,8L},{0xE6A3A859L,(-1L)},{1L,1L},{0x154E9BBAL,1L},{1L,(-1L)},{0xE6A3A859L,8L},{0x154E9BBAL,0xE6A3A859L},{8L,(-1L)},{8L,0xE6A3A859L}},{{0x154E9BBAL,8L},{0xE6A3A859L,(-1L)},{0x154E9BBAL,0x154E9BBAL},{(-1L),0x154E9BBAL},{0x154E9BBAL,0L},{(-1L),0xDC3E7CD7L},{(-1L),(-1L)},{0xDC3E7CD7L,0L},{0xDC3E7CD7L,(-1L)},{(-1L),0xDC3E7CD7L}},{{(-1L),0L},{0x154E9BBAL,0x154E9BBAL},{(-1L),0x154E9BBAL},{0x154E9BBAL,0L},{(-1L),0xDC3E7CD7L},{(-1L),(-1L)},{0xDC3E7CD7L,0L},{0xDC3E7CD7L,(-1L)},{(-1L),0xDC3E7CD7L},{(-1L),0L}},{{0x154E9BBAL,0x154E9BBAL},{(-1L),0x154E9BBAL},{0x154E9BBAL,0L},{(-1L),0xDC3E7CD7L},{(-1L),(-1L)},{0xDC3E7CD7L,0L},{0xDC3E7CD7L,(-1L)},{(-1L),0xDC3E7CD7L},{(-1L),0L},{0x154E9BBAL,0x154E9BBAL}},{{(-1L),0x154E9BBAL},{0x154E9BBAL,0L},{(-1L),0xDC3E7CD7L},{(-1L),(-1L)},{0xDC3E7CD7L,0L},{0xDC3E7CD7L,(-1L)},{(-1L),0xDC3E7CD7L},{(-1L),0L},{0x154E9BBAL,0x154E9BBAL},{(-1L),0x154E9BBAL}},{{0x154E9BBAL,0L},{(-1L),0xDC3E7CD7L},{(-1L),(-1L)},{0xDC3E7CD7L,0L},{0xDC3E7CD7L,(-1L)},{(-1L),0xDC3E7CD7L},{(-1L),0L},{0x154E9BBAL,0x154E9BBAL},{(-1L),0x154E9BBAL},{0x154E9BBAL,0L}}};
    uint64_t l_2033 = 18446744073709551610UL;
    int8_t *l_2034 = &g_53;
    int i, j, k;
    if ((((((*l_2012) = l_2011) == (l_2013 , l_2014)) | (safe_unary_minus_func_uint16_t_u((18446744073709551609UL > ((safe_add_func_uint16_t_u_u(((safe_lshift_func_uint16_t_u_s((safe_div_func_int16_t_s_s((safe_mul_func_uint8_t_u_u((((safe_lshift_func_int16_t_s_s(((safe_add_func_int64_t_s_s((safe_lshift_func_uint8_t_u_s(0x0DL, 4)), ((l_2013 <= l_2013) != ((*g_1617)--)))) >= l_2030[5][1][0]), (((18446744073709551615UL != 18446744073709551615UL) <= p_43) | (*g_499)))) > l_2013) & 1L), (*g_499))), l_2013)), 13)) ^ p_43), (**g_249))) <= l_2013))))) ^ l_2033))
    {
        return &g_53;
    }
    else
    {
        return l_2034;
    }
}







static const int8_t * func_45(union U3 p_46, int32_t p_47, int64_t p_48)
{
    uint64_t l_54[4];
    int32_t l_62[8] = {4L,4L,4L,4L,4L,4L,4L,4L};
    struct S1 *l_115 = &g_114;
    int8_t *l_159[3][10][5] = {{{&g_53,(void*)0,&g_58,&g_53,&g_53},{&g_53,(void*)0,&g_53,&g_53,&g_53},{&g_53,(void*)0,&g_58,(void*)0,(void*)0},{&g_53,&g_58,&g_53,(void*)0,(void*)0},{&g_58,(void*)0,&g_53,&g_53,&g_53},{(void*)0,&g_58,(void*)0,(void*)0,&g_58},{(void*)0,&g_58,&g_53,&g_53,&g_58},{&g_58,(void*)0,&g_53,(void*)0,&g_58},{&g_58,&g_58,&g_58,&g_58,(void*)0},{&g_58,(void*)0,&g_53,&g_53,(void*)0}},{{&g_58,&g_58,&g_58,(void*)0,&g_58},{(void*)0,(void*)0,&g_53,&g_53,&g_58},{&g_58,&g_53,&g_53,&g_58,&g_58},{&g_53,&g_53,&g_58,(void*)0,&g_53},{&g_53,&g_53,(void*)0,&g_53,(void*)0},{&g_53,(void*)0,(void*)0,(void*)0,(void*)0},{&g_53,&g_58,&g_53,&g_53,&g_53},{&g_58,(void*)0,(void*)0,(void*)0,(void*)0},{&g_53,&g_58,&g_58,&g_58,&g_58},{&g_58,(void*)0,&g_53,(void*)0,(void*)0}},{{&g_53,&g_53,(void*)0,&g_58,&g_58},{(void*)0,&g_58,&g_58,(void*)0,&g_58},{&g_53,&g_58,(void*)0,(void*)0,&g_53},{&g_58,&g_53,&g_58,&g_53,&g_58},{&g_53,&g_53,&g_53,&g_58,&g_58},{&g_53,&g_53,&g_53,&g_53,&g_58},{(void*)0,&g_58,&g_58,&g_58,(void*)0},{&g_58,(void*)0,(void*)0,(void*)0,&g_53},{(void*)0,&g_53,&g_58,(void*)0,&g_58},{(void*)0,&g_53,(void*)0,(void*)0,&g_53}}};
    int16_t *l_196 = &g_109[9][5];
    int8_t l_200 = 7L;
    int32_t *l_256 = &g_117;
    uint32_t l_270 = 0x240FBB52L;
    int8_t l_506[3][2] = {{0xBDL,0xF0L},{0xF0L,0xBDL},{0xF0L,0xF0L}};
    uint32_t l_664 = 18446744073709551615UL;
    uint16_t *l_713 = &g_107;
    uint16_t * const *l_712 = &l_713;
    int8_t l_765 = 0xCEL;
    uint16_t l_808 = 65535UL;
    union U3 *l_889 = (void*)0;
    uint16_t ***l_935 = &g_933;
    int32_t l_963 = 0x8F403D01L;
    int64_t l_964 = 0x97CE742EE3AB1C8CLL;
    uint16_t l_973 = 2UL;
    uint16_t l_974 = 7UL;
    uint64_t l_977[7][9] = {{1UL,2UL,8UL,0UL,0UL,8UL,2UL,1UL,1UL},{0UL,0UL,0UL,0xBF22EC7741B62883LL,0UL,0UL,0UL,0xBF22EC7741B62883LL,0UL},{1UL,0UL,2UL,2UL,0UL,1UL,8UL,8UL,1UL},{0xF656272369CB65C8LL,0xBF22EC7741B62883LL,0x7240768C57A960A6LL,0xBF22EC7741B62883LL,0xF656272369CB65C8LL,0xBF22EC7741B62883LL,0x7240768C57A960A6LL,0xBF22EC7741B62883LL,0xF656272369CB65C8LL},{0UL,2UL,2UL,0UL,1UL,8UL,8UL,1UL,0UL},{0UL,0xBF22EC7741B62883LL,0UL,0UL,0UL,0xBF22EC7741B62883LL,0UL,0UL,0UL},{0UL,0UL,8UL,2UL,1UL,1UL,2UL,8UL,0UL}};
    uint64_t l_1048 = 18446744073709551615UL;
    struct S1 **l_1058 = &g_258;
    struct S1 ***l_1057 = &l_1058;
    volatile int32_t ***l_1096 = &g_1092;
    uint32_t *l_1128 = &g_469[1];
    uint8_t *l_1132[7][10] = {{(void*)0,&g_85[1][6][0],&g_85[1][6][0],&g_85[2][4][2],(void*)0,&g_85[2][4][2],&g_85[1][6][0],&g_85[1][6][0],(void*)0,&g_85[4][4][0]},{&g_85[1][6][0],&g_85[2][4][2],(void*)0,&g_85[1][7][1],&g_85[3][5][2],&g_85[1][6][0],&g_85[1][6][0],&g_85[1][6][0],&g_85[3][5][2],&g_85[1][7][1]},{&g_85[1][6][0],&g_85[1][6][0],&g_85[1][6][0],&g_85[1][7][1],&g_85[1][6][0],&g_85[4][4][0],(void*)0,&g_16,(void*)0,&g_16},{&g_85[1][6][0],&g_85[1][6][0],(void*)0,&g_85[2][4][2],(void*)0,&g_85[1][6][0],&g_85[1][6][0],&g_16,&g_85[1][6][0],&g_85[1][6][0]},{&g_85[1][6][0],&g_16,&g_85[1][6][0],&g_85[1][6][0],(void*)0,&g_85[2][4][2],(void*)0,&g_85[1][6][0],&g_85[1][6][0],&g_16},{(void*)0,&g_16,(void*)0,&g_85[4][4][0],&g_85[1][6][0],&g_85[1][7][1],&g_85[1][6][0],&g_85[1][6][0],&g_85[3][5][2],&g_85[4][4][0]},{(void*)0,&g_16,(void*)0,&g_16,(void*)0,&g_85[4][4][0],&g_85[1][6][0],&g_85[1][7][1],&g_85[1][6][0],&g_85[1][6][0]}};
    int64_t *l_1139[4][9];
    uint64_t l_1212 = 0xBEB1AFC78B08ABC1LL;
    int32_t *l_1312 = (void*)0;
    int32_t l_1313 = 0x6B9A3F7BL;
    int8_t l_1314 = 0x15L;
    uint8_t *l_1315 = &g_1232[1];
    union U3 l_1367 = {0};
    uint32_t ** const *l_1437 = (void*)0;
    union U3 **l_1541 = &g_458;
    union U3 ***l_1540 = &l_1541;
    struct S1 ** const l_1557 = &l_115;
    uint64_t l_1558[8][9] = {{18446744073709551615UL,2UL,0x3A8D870EB6EE66BELL,18446744073709551615UL,0x06B3EEA4DDB4C8E3LL,18446744073709551612UL,18446744073709551615UL,0x558258BD99EE6A55LL,2UL},{18446744073709551614UL,18446744073709551614UL,0x558258BD99EE6A55LL,0UL,0x5E099B36462A1B2BLL,9UL,0x43377CF406F18D6FLL,0x8BCAB06C25E8C3DELL,0x9D1D52DE9DB2057CLL},{18446744073709551612UL,0x8DD6B7A7D31E0A6ALL,18446744073709551615UL,0x9D1D52DE9DB2057CLL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,0x9D1D52DE9DB2057CLL},{0xA0285ABE6854CAACLL,0x5E099B36462A1B2BLL,0xA0285ABE6854CAACLL,18446744073709551615UL,18446744073709551615UL,0UL,9UL,0xA0285ABE6854CAACLL,2UL},{0xF8F218475CB8BEECLL,18446744073709551614UL,18446744073709551615UL,0UL,9UL,18446744073709551614UL,18446744073709551614UL,2UL,0xA0285ABE6854CAACLL},{0x8DD6B7A7D31E0A6ALL,18446744073709551615UL,0x03DFE7F8F217DC33LL,18446744073709551615UL,18446744073709551612UL,0x8DD6B7A7D31E0A6ALL,18446744073709551615UL,0x9D1D52DE9DB2057CLL,18446744073709551615UL},{9UL,0x43377CF406F18D6FLL,0x8BCAB06C25E8C3DELL,0x9D1D52DE9DB2057CLL,0x5E099B36462A1B2BLL,18446744073709551615UL,0x5E099B36462A1B2BLL,0x9D1D52DE9DB2057CLL,0x8BCAB06C25E8C3DELL},{18446744073709551615UL,18446744073709551615UL,18446744073709551615UL,0UL,18446744073709551615UL,18446744073709551612UL,0x3DEB96AE9A8462A2LL,2UL,0x558258BD99EE6A55LL}};
    const struct S0 *l_1560[3][3] = {{(void*)0,&g_1562,(void*)0},{&g_1561,&g_1561,&g_1561},{(void*)0,&g_1562,(void*)0}};
    const struct S0 ** const l_1559 = &l_1560[0][0];
    int8_t l_1594[5];
    int8_t l_1598 = 0x17L;
    struct S0 **l_1609 = &g_1470;
    int64_t l_1612[3][7];
    int16_t l_1629 = 0x6379L;
    uint32_t l_1676 = 0x30761993L;
    struct S1 **l_1711[5] = {&l_115,&l_115,&l_115,&l_115,&l_115};
    int32_t l_1750 = (-4L);
    uint64_t l_1810 = 18446744073709551607UL;
    uint32_t l_1876 = 0x9B0ECBA5L;
    uint64_t ***l_1906 = (void*)0;
    uint8_t l_1933 = 0x4CL;
    uint64_t l_1973[3];
    int8_t l_2003 = 0L;
    int i, j, k;
    for (i = 0; i < 4; i++)
        l_54[i] = 1UL;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 9; j++)
            l_1139[i][j] = (void*)0;
    }
    for (i = 0; i < 5; i++)
        l_1594[i] = 5L;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 7; j++)
            l_1612[i][j] = 0L;
    }
    for (i = 0; i < 3; i++)
        l_1973[i] = 0x0D91824DA47CC04BLL;
    return &g_53;
}





int main (int argc, char* argv[])
{
    int i, j, k;
    int print_hash_value = 0;
    if (argc == 2 && strcmp(argv[1], "1") == 0) print_hash_value = 1;
    platform_main_begin();
    crc32_gentab();
    func_1();
    transparent_crc(g_13, "g_13", print_hash_value);
    transparent_crc(g_16, "g_16", print_hash_value);
    transparent_crc(g_53, "g_53", print_hash_value);
    transparent_crc(g_58, "g_58", print_hash_value);
    transparent_crc(g_63, "g_63", print_hash_value);
    transparent_crc(g_82, "g_82", print_hash_value);
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            for (k = 0; k < 6; k++)
            {
                transparent_crc(g_85[i][j][k], "g_85[i][j][k]", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    transparent_crc(g_98.f0, "g_98.f0", print_hash_value);
    transparent_crc(g_98.f1, "g_98.f1", print_hash_value);
    transparent_crc(g_98.f2, "g_98.f2", print_hash_value);
    transparent_crc(g_98.f3, "g_98.f3", print_hash_value);
    transparent_crc(g_98.f4, "g_98.f4", print_hash_value);
    transparent_crc(g_98.f5, "g_98.f5", print_hash_value);
    transparent_crc(g_98.f6, "g_98.f6", print_hash_value);
    transparent_crc(g_98.f7, "g_98.f7", print_hash_value);
    transparent_crc(g_107, "g_107", print_hash_value);
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 9; j++)
        {
            transparent_crc(g_109[i][j], "g_109[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_114.f0, "g_114.f0", print_hash_value);
    transparent_crc(g_114.f1, "g_114.f1", print_hash_value);
    transparent_crc(g_114.f2, "g_114.f2", print_hash_value);
    transparent_crc(g_114.f3, "g_114.f3", print_hash_value);
    transparent_crc(g_114.f4, "g_114.f4", print_hash_value);
    transparent_crc(g_117, "g_117", print_hash_value);
    transparent_crc(g_121, "g_121", print_hash_value);
    transparent_crc(g_134.f0, "g_134.f0", print_hash_value);
    transparent_crc(g_134.f1, "g_134.f1", print_hash_value);
    transparent_crc(g_134.f2, "g_134.f2", print_hash_value);
    transparent_crc(g_134.f3, "g_134.f3", print_hash_value);
    transparent_crc(g_134.f4, "g_134.f4", print_hash_value);
    transparent_crc(g_134.f5, "g_134.f5", print_hash_value);
    transparent_crc(g_134.f6, "g_134.f6", print_hash_value);
    transparent_crc(g_134.f7, "g_134.f7", print_hash_value);
    transparent_crc(g_154.f0, "g_154.f0", print_hash_value);
    transparent_crc(g_154.f1, "g_154.f1", print_hash_value);
    transparent_crc(g_154.f2, "g_154.f2", print_hash_value);
    transparent_crc(g_154.f3, "g_154.f3", print_hash_value);
    transparent_crc(g_154.f4, "g_154.f4", print_hash_value);
    transparent_crc(g_154.f5, "g_154.f5", print_hash_value);
    transparent_crc(g_154.f6, "g_154.f6", print_hash_value);
    transparent_crc(g_154.f7, "g_154.f7", print_hash_value);
    transparent_crc(g_155.f0, "g_155.f0", print_hash_value);
    transparent_crc(g_155.f1, "g_155.f1", print_hash_value);
    transparent_crc(g_155.f2, "g_155.f2", print_hash_value);
    transparent_crc(g_155.f3, "g_155.f3", print_hash_value);
    transparent_crc(g_155.f4, "g_155.f4", print_hash_value);
    transparent_crc(g_155.f5, "g_155.f5", print_hash_value);
    transparent_crc(g_155.f6, "g_155.f6", print_hash_value);
    transparent_crc(g_155.f7, "g_155.f7", print_hash_value);
    transparent_crc(g_177, "g_177", print_hash_value);
    transparent_crc(g_188, "g_188", print_hash_value);
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 2; j++)
        {
            transparent_crc(g_236[i][j], "g_236[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_239, "g_239", print_hash_value);
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            transparent_crc(g_266[i][j], "g_266[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 2; j++)
        {
            for (k = 0; k < 10; k++)
            {
                transparent_crc(g_291[i][j][k], "g_291[i][j][k]", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    transparent_crc(g_322.f0, "g_322.f0", print_hash_value);
    transparent_crc(g_322.f1, "g_322.f1", print_hash_value);
    transparent_crc(g_322.f2, "g_322.f2", print_hash_value);
    transparent_crc(g_322.f3, "g_322.f3", print_hash_value);
    transparent_crc(g_322.f4, "g_322.f4", print_hash_value);
    transparent_crc(g_322.f5, "g_322.f5", print_hash_value);
    transparent_crc(g_322.f6, "g_322.f6", print_hash_value);
    transparent_crc(g_322.f7, "g_322.f7", print_hash_value);
    transparent_crc(g_378, "g_378", print_hash_value);
    transparent_crc(g_382, "g_382", print_hash_value);
    transparent_crc(g_396.f0, "g_396.f0", print_hash_value);
    transparent_crc(g_396.f1, "g_396.f1", print_hash_value);
    transparent_crc(g_396.f2, "g_396.f2", print_hash_value);
    transparent_crc(g_396.f3, "g_396.f3", print_hash_value);
    transparent_crc(g_396.f4, "g_396.f4", print_hash_value);
    transparent_crc(g_396.f5, "g_396.f5", print_hash_value);
    transparent_crc(g_396.f6, "g_396.f6", print_hash_value);
    transparent_crc(g_396.f7, "g_396.f7", print_hash_value);
    transparent_crc(g_430, "g_430", print_hash_value);
    for (i = 0; i < 6; i++)
    {
        transparent_crc(g_469[i], "g_469[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_719.f0, "g_719.f0", print_hash_value);
    transparent_crc(g_719.f1, "g_719.f1", print_hash_value);
    transparent_crc(g_719.f2, "g_719.f2", print_hash_value);
    transparent_crc(g_719.f3, "g_719.f3", print_hash_value);
    transparent_crc(g_719.f4, "g_719.f4", print_hash_value);
    transparent_crc(g_719.f5, "g_719.f5", print_hash_value);
    transparent_crc(g_858, "g_858", print_hash_value);
    transparent_crc(g_892.f0, "g_892.f0", print_hash_value);
    transparent_crc(g_892.f1, "g_892.f1", print_hash_value);
    transparent_crc(g_892.f2, "g_892.f2", print_hash_value);
    transparent_crc(g_892.f3, "g_892.f3", print_hash_value);
    transparent_crc(g_892.f4, "g_892.f4", print_hash_value);
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 10; j++)
        {
            for (k = 0; k < 10; k++)
            {
                transparent_crc(g_978[i][j][k], "g_978[i][j][k]", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    transparent_crc(g_1047, "g_1047", print_hash_value);
    transparent_crc(g_1094, "g_1094", print_hash_value);
    transparent_crc(g_1144, "g_1144", print_hash_value);
    transparent_crc(g_1163.f0, "g_1163.f0", print_hash_value);
    transparent_crc(g_1163.f1, "g_1163.f1", print_hash_value);
    transparent_crc(g_1163.f2, "g_1163.f2", print_hash_value);
    transparent_crc(g_1163.f3, "g_1163.f3", print_hash_value);
    transparent_crc(g_1163.f4, "g_1163.f4", print_hash_value);
    transparent_crc(g_1163.f5, "g_1163.f5", print_hash_value);
    transparent_crc(g_1163.f6, "g_1163.f6", print_hash_value);
    transparent_crc(g_1163.f7, "g_1163.f7", print_hash_value);
    transparent_crc(g_1164.f0, "g_1164.f0", print_hash_value);
    transparent_crc(g_1164.f1, "g_1164.f1", print_hash_value);
    transparent_crc(g_1164.f2, "g_1164.f2", print_hash_value);
    transparent_crc(g_1164.f3, "g_1164.f3", print_hash_value);
    transparent_crc(g_1164.f4, "g_1164.f4", print_hash_value);
    transparent_crc(g_1164.f5, "g_1164.f5", print_hash_value);
    transparent_crc(g_1164.f6, "g_1164.f6", print_hash_value);
    transparent_crc(g_1164.f7, "g_1164.f7", print_hash_value);
    transparent_crc(g_1204, "g_1204", print_hash_value);
    for (i = 0; i < 5; i++)
    {
        transparent_crc(g_1232[i], "g_1232[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_1264.f0, "g_1264.f0", print_hash_value);
    transparent_crc(g_1264.f1, "g_1264.f1", print_hash_value);
    transparent_crc(g_1264.f2, "g_1264.f2", print_hash_value);
    transparent_crc(g_1264.f3, "g_1264.f3", print_hash_value);
    transparent_crc(g_1264.f4, "g_1264.f4", print_hash_value);
    transparent_crc(g_1264.f5, "g_1264.f5", print_hash_value);
    transparent_crc(g_1264.f6, "g_1264.f6", print_hash_value);
    transparent_crc(g_1264.f7, "g_1264.f7", print_hash_value);
    transparent_crc(g_1382.f0, "g_1382.f0", print_hash_value);
    transparent_crc(g_1382.f1, "g_1382.f1", print_hash_value);
    transparent_crc(g_1382.f2, "g_1382.f2", print_hash_value);
    transparent_crc(g_1382.f3, "g_1382.f3", print_hash_value);
    transparent_crc(g_1382.f4, "g_1382.f4", print_hash_value);
    transparent_crc(g_1382.f5, "g_1382.f5", print_hash_value);
    transparent_crc(g_1401, "g_1401", print_hash_value);
    transparent_crc(g_1469.f0, "g_1469.f0", print_hash_value);
    transparent_crc(g_1469.f1, "g_1469.f1", print_hash_value);
    transparent_crc(g_1469.f2, "g_1469.f2", print_hash_value);
    transparent_crc(g_1469.f3, "g_1469.f3", print_hash_value);
    transparent_crc(g_1469.f4, "g_1469.f4", print_hash_value);
    transparent_crc(g_1469.f5, "g_1469.f5", print_hash_value);
    transparent_crc(g_1469.f6, "g_1469.f6", print_hash_value);
    transparent_crc(g_1469.f7, "g_1469.f7", print_hash_value);
    transparent_crc(g_1471.f0, "g_1471.f0", print_hash_value);
    transparent_crc(g_1471.f1, "g_1471.f1", print_hash_value);
    transparent_crc(g_1471.f2, "g_1471.f2", print_hash_value);
    transparent_crc(g_1471.f3, "g_1471.f3", print_hash_value);
    transparent_crc(g_1471.f4, "g_1471.f4", print_hash_value);
    transparent_crc(g_1471.f5, "g_1471.f5", print_hash_value);
    transparent_crc(g_1471.f6, "g_1471.f6", print_hash_value);
    transparent_crc(g_1471.f7, "g_1471.f7", print_hash_value);
    transparent_crc(g_1480.f0, "g_1480.f0", print_hash_value);
    transparent_crc(g_1480.f1, "g_1480.f1", print_hash_value);
    transparent_crc(g_1480.f2, "g_1480.f2", print_hash_value);
    transparent_crc(g_1480.f3, "g_1480.f3", print_hash_value);
    transparent_crc(g_1480.f4, "g_1480.f4", print_hash_value);
    transparent_crc(g_1480.f5, "g_1480.f5", print_hash_value);
    transparent_crc(g_1525, "g_1525", print_hash_value);
    transparent_crc(g_1553, "g_1553", print_hash_value);
    transparent_crc(g_1561.f0, "g_1561.f0", print_hash_value);
    transparent_crc(g_1561.f1, "g_1561.f1", print_hash_value);
    transparent_crc(g_1561.f2, "g_1561.f2", print_hash_value);
    transparent_crc(g_1561.f3, "g_1561.f3", print_hash_value);
    transparent_crc(g_1561.f4, "g_1561.f4", print_hash_value);
    transparent_crc(g_1561.f5, "g_1561.f5", print_hash_value);
    transparent_crc(g_1561.f6, "g_1561.f6", print_hash_value);
    transparent_crc(g_1561.f7, "g_1561.f7", print_hash_value);
    transparent_crc(g_1562.f0, "g_1562.f0", print_hash_value);
    transparent_crc(g_1562.f1, "g_1562.f1", print_hash_value);
    transparent_crc(g_1562.f2, "g_1562.f2", print_hash_value);
    transparent_crc(g_1562.f3, "g_1562.f3", print_hash_value);
    transparent_crc(g_1562.f4, "g_1562.f4", print_hash_value);
    transparent_crc(g_1562.f5, "g_1562.f5", print_hash_value);
    transparent_crc(g_1562.f6, "g_1562.f6", print_hash_value);
    transparent_crc(g_1562.f7, "g_1562.f7", print_hash_value);
    transparent_crc(g_1585, "g_1585", print_hash_value);
    transparent_crc(g_1603.f0, "g_1603.f0", print_hash_value);
    transparent_crc(g_1603.f1, "g_1603.f1", print_hash_value);
    transparent_crc(g_1603.f2, "g_1603.f2", print_hash_value);
    transparent_crc(g_1603.f3, "g_1603.f3", print_hash_value);
    transparent_crc(g_1603.f4, "g_1603.f4", print_hash_value);
    transparent_crc(g_1603.f5, "g_1603.f5", print_hash_value);
    transparent_crc(g_1687.f0, "g_1687.f0", print_hash_value);
    transparent_crc(g_1687.f1, "g_1687.f1", print_hash_value);
    transparent_crc(g_1687.f2, "g_1687.f2", print_hash_value);
    transparent_crc(g_1687.f3, "g_1687.f3", print_hash_value);
    transparent_crc(g_1687.f4, "g_1687.f4", print_hash_value);
    transparent_crc(g_1687.f5, "g_1687.f5", print_hash_value);
    for (i = 0; i < 4; i++)
    {
        transparent_crc(g_1690[i].f0, "g_1690[i].f0", print_hash_value);
        transparent_crc(g_1690[i].f1, "g_1690[i].f1", print_hash_value);
        transparent_crc(g_1690[i].f2, "g_1690[i].f2", print_hash_value);
        transparent_crc(g_1690[i].f3, "g_1690[i].f3", print_hash_value);
        transparent_crc(g_1690[i].f4, "g_1690[i].f4", print_hash_value);
        transparent_crc(g_1690[i].f5, "g_1690[i].f5", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_1779.f0, "g_1779.f0", print_hash_value);
    transparent_crc(g_1779.f1, "g_1779.f1", print_hash_value);
    transparent_crc(g_1779.f2, "g_1779.f2", print_hash_value);
    transparent_crc(g_1779.f3, "g_1779.f3", print_hash_value);
    transparent_crc(g_1779.f4, "g_1779.f4", print_hash_value);
    transparent_crc(g_1779.f5, "g_1779.f5", print_hash_value);
    transparent_crc(g_1779.f6, "g_1779.f6", print_hash_value);
    transparent_crc(g_1779.f7, "g_1779.f7", print_hash_value);
    for (i = 0; i < 7; i++)
    {
        transparent_crc(g_1780[i].f0, "g_1780[i].f0", print_hash_value);
        transparent_crc(g_1780[i].f1, "g_1780[i].f1", print_hash_value);
        transparent_crc(g_1780[i].f2, "g_1780[i].f2", print_hash_value);
        transparent_crc(g_1780[i].f3, "g_1780[i].f3", print_hash_value);
        transparent_crc(g_1780[i].f4, "g_1780[i].f4", print_hash_value);
        transparent_crc(g_1780[i].f5, "g_1780[i].f5", print_hash_value);
        transparent_crc(g_1780[i].f6, "g_1780[i].f6", print_hash_value);
        transparent_crc(g_1780[i].f7, "g_1780[i].f7", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_1781.f0, "g_1781.f0", print_hash_value);
    transparent_crc(g_1781.f1, "g_1781.f1", print_hash_value);
    transparent_crc(g_1781.f2, "g_1781.f2", print_hash_value);
    transparent_crc(g_1781.f3, "g_1781.f3", print_hash_value);
    transparent_crc(g_1781.f4, "g_1781.f4", print_hash_value);
    transparent_crc(g_1781.f5, "g_1781.f5", print_hash_value);
    transparent_crc(g_1781.f6, "g_1781.f6", print_hash_value);
    transparent_crc(g_1781.f7, "g_1781.f7", print_hash_value);
    transparent_crc(g_1782.f0, "g_1782.f0", print_hash_value);
    transparent_crc(g_1782.f1, "g_1782.f1", print_hash_value);
    transparent_crc(g_1782.f2, "g_1782.f2", print_hash_value);
    transparent_crc(g_1782.f3, "g_1782.f3", print_hash_value);
    transparent_crc(g_1782.f4, "g_1782.f4", print_hash_value);
    transparent_crc(g_1782.f5, "g_1782.f5", print_hash_value);
    transparent_crc(g_1782.f6, "g_1782.f6", print_hash_value);
    transparent_crc(g_1782.f7, "g_1782.f7", print_hash_value);
    transparent_crc(g_1783.f0, "g_1783.f0", print_hash_value);
    transparent_crc(g_1783.f1, "g_1783.f1", print_hash_value);
    transparent_crc(g_1783.f2, "g_1783.f2", print_hash_value);
    transparent_crc(g_1783.f3, "g_1783.f3", print_hash_value);
    transparent_crc(g_1783.f4, "g_1783.f4", print_hash_value);
    transparent_crc(g_1783.f5, "g_1783.f5", print_hash_value);
    transparent_crc(g_1783.f6, "g_1783.f6", print_hash_value);
    transparent_crc(g_1783.f7, "g_1783.f7", print_hash_value);
    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < 7; j++)
        {
            for (k = 0; k < 10; k++)
            {
                transparent_crc(g_1784[i][j][k].f0, "g_1784[i][j][k].f0", print_hash_value);
                transparent_crc(g_1784[i][j][k].f1, "g_1784[i][j][k].f1", print_hash_value);
                transparent_crc(g_1784[i][j][k].f2, "g_1784[i][j][k].f2", print_hash_value);
                transparent_crc(g_1784[i][j][k].f3, "g_1784[i][j][k].f3", print_hash_value);
                transparent_crc(g_1784[i][j][k].f4, "g_1784[i][j][k].f4", print_hash_value);
                transparent_crc(g_1784[i][j][k].f5, "g_1784[i][j][k].f5", print_hash_value);
                transparent_crc(g_1784[i][j][k].f6, "g_1784[i][j][k].f6", print_hash_value);
                transparent_crc(g_1784[i][j][k].f7, "g_1784[i][j][k].f7", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    transparent_crc(g_1824, "g_1824", print_hash_value);
    transparent_crc(g_1840, "g_1840", print_hash_value);
    transparent_crc(g_1853, "g_1853", print_hash_value);
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 8; j++)
        {
            transparent_crc(g_1904[i][j].f0, "g_1904[i][j].f0", print_hash_value);
            transparent_crc(g_1904[i][j].f1, "g_1904[i][j].f1", print_hash_value);
            transparent_crc(g_1904[i][j].f2, "g_1904[i][j].f2", print_hash_value);
            transparent_crc(g_1904[i][j].f3, "g_1904[i][j].f3", print_hash_value);
            transparent_crc(g_1904[i][j].f4, "g_1904[i][j].f4", print_hash_value);
            transparent_crc(g_1904[i][j].f5, "g_1904[i][j].f5", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_2069, "g_2069", print_hash_value);
    transparent_crc(g_2077, "g_2077", print_hash_value);
    transparent_crc(g_2099, "g_2099", print_hash_value);
    transparent_crc(g_2209, "g_2209", print_hash_value);
    for (i = 0; i < 6; i++)
    {
        transparent_crc(g_2234[i], "g_2234[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_2353, "g_2353", print_hash_value);
    transparent_crc(g_2411.f0, "g_2411.f0", print_hash_value);
    transparent_crc(g_2411.f1, "g_2411.f1", print_hash_value);
    transparent_crc(g_2411.f2, "g_2411.f2", print_hash_value);
    transparent_crc(g_2411.f3, "g_2411.f3", print_hash_value);
    transparent_crc(g_2411.f4, "g_2411.f4", print_hash_value);
    transparent_crc(g_2411.f5, "g_2411.f5", print_hash_value);
    transparent_crc(g_2427, "g_2427", print_hash_value);
    transparent_crc(g_2450, "g_2450", print_hash_value);
    transparent_crc(g_2453, "g_2453", print_hash_value);
    for (i = 0; i < 7; i++)
    {
        for (j = 0; j < 6; j++)
        {
            for (k = 0; k < 2; k++)
            {
                transparent_crc(g_2487[i][j][k].f0, "g_2487[i][j][k].f0", print_hash_value);
                transparent_crc(g_2487[i][j][k].f1, "g_2487[i][j][k].f1", print_hash_value);
                transparent_crc(g_2487[i][j][k].f2, "g_2487[i][j][k].f2", print_hash_value);
                transparent_crc(g_2487[i][j][k].f3, "g_2487[i][j][k].f3", print_hash_value);
                transparent_crc(g_2487[i][j][k].f4, "g_2487[i][j][k].f4", print_hash_value);
                transparent_crc(g_2487[i][j][k].f5, "g_2487[i][j][k].f5", print_hash_value);
                transparent_crc(g_2487[i][j][k].f6, "g_2487[i][j][k].f6", print_hash_value);
                transparent_crc(g_2487[i][j][k].f7, "g_2487[i][j][k].f7", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    transparent_crc(g_2490.f0, "g_2490.f0", print_hash_value);
    transparent_crc(g_2490.f1, "g_2490.f1", print_hash_value);
    transparent_crc(g_2490.f2, "g_2490.f2", print_hash_value);
    transparent_crc(g_2490.f3, "g_2490.f3", print_hash_value);
    transparent_crc(g_2490.f4, "g_2490.f4", print_hash_value);
    transparent_crc(g_2490.f5, "g_2490.f5", print_hash_value);
    transparent_crc(g_2490.f6, "g_2490.f6", print_hash_value);
    transparent_crc(g_2490.f7, "g_2490.f7", print_hash_value);
    transparent_crc(g_2568, "g_2568", print_hash_value);
    transparent_crc(g_2605, "g_2605", print_hash_value);
    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 5; j++)
        {
            transparent_crc(g_2633[i][j].f0, "g_2633[i][j].f0", print_hash_value);
            transparent_crc(g_2633[i][j].f1, "g_2633[i][j].f1", print_hash_value);
            transparent_crc(g_2633[i][j].f2, "g_2633[i][j].f2", print_hash_value);
            transparent_crc(g_2633[i][j].f3, "g_2633[i][j].f3", print_hash_value);
            transparent_crc(g_2633[i][j].f4, "g_2633[i][j].f4", print_hash_value);
            transparent_crc(g_2633[i][j].f5, "g_2633[i][j].f5", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_2745.f0, "g_2745.f0", print_hash_value);
    transparent_crc(g_2745.f1, "g_2745.f1", print_hash_value);
    transparent_crc(g_2745.f2, "g_2745.f2", print_hash_value);
    transparent_crc(g_2745.f3, "g_2745.f3", print_hash_value);
    transparent_crc(g_2745.f4, "g_2745.f4", print_hash_value);
    transparent_crc(g_2745.f5, "g_2745.f5", print_hash_value);
    for (i = 0; i < 1; i++)
    {
        for (j = 0; j < 9; j++)
        {
            transparent_crc(g_2748[i][j].f0, "g_2748[i][j].f0", print_hash_value);
            transparent_crc(g_2748[i][j].f1, "g_2748[i][j].f1", print_hash_value);
            transparent_crc(g_2748[i][j].f2, "g_2748[i][j].f2", print_hash_value);
            transparent_crc(g_2748[i][j].f3, "g_2748[i][j].f3", print_hash_value);
            transparent_crc(g_2748[i][j].f4, "g_2748[i][j].f4", print_hash_value);
            transparent_crc(g_2748[i][j].f5, "g_2748[i][j].f5", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    transparent_crc(g_2752.f0, "g_2752.f0", print_hash_value);
    transparent_crc(g_2752.f1, "g_2752.f1", print_hash_value);
    transparent_crc(g_2752.f2, "g_2752.f2", print_hash_value);
    transparent_crc(g_2752.f3, "g_2752.f3", print_hash_value);
    transparent_crc(g_2752.f4, "g_2752.f4", print_hash_value);
    transparent_crc(g_2752.f5, "g_2752.f5", print_hash_value);
    transparent_crc(g_2755.f0, "g_2755.f0", print_hash_value);
    transparent_crc(g_2755.f1, "g_2755.f1", print_hash_value);
    transparent_crc(g_2755.f2, "g_2755.f2", print_hash_value);
    transparent_crc(g_2755.f3, "g_2755.f3", print_hash_value);
    transparent_crc(g_2755.f4, "g_2755.f4", print_hash_value);
    transparent_crc(g_2755.f5, "g_2755.f5", print_hash_value);
    transparent_crc(g_2759, "g_2759", print_hash_value);
    transparent_crc(g_2838, "g_2838", print_hash_value);
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            for (k = 0; k < 4; k++)
            {
                transparent_crc(g_2856[i][j][k], "g_2856[i][j][k]", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    for (i = 0; i < 6; i++)
    {
        transparent_crc(g_2876[i].f0, "g_2876[i].f0", print_hash_value);
        transparent_crc(g_2876[i].f1, "g_2876[i].f1", print_hash_value);
        transparent_crc(g_2876[i].f2, "g_2876[i].f2", print_hash_value);
        transparent_crc(g_2876[i].f3, "g_2876[i].f3", print_hash_value);
        transparent_crc(g_2876[i].f4, "g_2876[i].f4", print_hash_value);
        transparent_crc(g_2876[i].f5, "g_2876[i].f5", print_hash_value);
        transparent_crc(g_2876[i].f6, "g_2876[i].f6", print_hash_value);
        transparent_crc(g_2876[i].f7, "g_2876[i].f7", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_2981.f0, "g_2981.f0", print_hash_value);
    transparent_crc(g_2981.f1, "g_2981.f1", print_hash_value);
    transparent_crc(g_2981.f2, "g_2981.f2", print_hash_value);
    transparent_crc(g_2981.f3, "g_2981.f3", print_hash_value);
    transparent_crc(g_2981.f4, "g_2981.f4", print_hash_value);
    transparent_crc(g_3015.f0, "g_3015.f0", print_hash_value);
    transparent_crc(g_3015.f1, "g_3015.f1", print_hash_value);
    transparent_crc(g_3015.f2, "g_3015.f2", print_hash_value);
    transparent_crc(g_3015.f3, "g_3015.f3", print_hash_value);
    transparent_crc(g_3015.f4, "g_3015.f4", print_hash_value);
    transparent_crc(g_3015.f5, "g_3015.f5", print_hash_value);
    transparent_crc(g_3015.f6, "g_3015.f6", print_hash_value);
    transparent_crc(g_3015.f7, "g_3015.f7", print_hash_value);
    transparent_crc(g_3055, "g_3055", print_hash_value);
    transparent_crc(g_3084, "g_3084", print_hash_value);
    transparent_crc(g_3085, "g_3085", print_hash_value);
    transparent_crc(g_3086, "g_3086", print_hash_value);
    transparent_crc(g_3087, "g_3087", print_hash_value);
    transparent_crc(g_3092, "g_3092", print_hash_value);
    transparent_crc(g_3099, "g_3099", print_hash_value);
    transparent_crc(g_3108, "g_3108", print_hash_value);
    transparent_crc(g_3140.f0, "g_3140.f0", print_hash_value);
    transparent_crc(g_3140.f1, "g_3140.f1", print_hash_value);
    transparent_crc(g_3140.f2, "g_3140.f2", print_hash_value);
    transparent_crc(g_3140.f3, "g_3140.f3", print_hash_value);
    transparent_crc(g_3140.f4, "g_3140.f4", print_hash_value);
    transparent_crc(g_3140.f5, "g_3140.f5", print_hash_value);
    transparent_crc(g_3140.f6, "g_3140.f6", print_hash_value);
    transparent_crc(g_3140.f7, "g_3140.f7", print_hash_value);
    transparent_crc(g_3401, "g_3401", print_hash_value);
    transparent_crc(g_3434.f0, "g_3434.f0", print_hash_value);
    transparent_crc(g_3434.f1, "g_3434.f1", print_hash_value);
    transparent_crc(g_3434.f2, "g_3434.f2", print_hash_value);
    transparent_crc(g_3434.f3, "g_3434.f3", print_hash_value);
    transparent_crc(g_3434.f4, "g_3434.f4", print_hash_value);
    transparent_crc(g_3434.f5, "g_3434.f5", print_hash_value);
    transparent_crc(g_3468.f0, "g_3468.f0", print_hash_value);
    transparent_crc(g_3468.f1, "g_3468.f1", print_hash_value);
    transparent_crc(g_3468.f2, "g_3468.f2", print_hash_value);
    transparent_crc(g_3468.f3, "g_3468.f3", print_hash_value);
    transparent_crc(g_3468.f4, "g_3468.f4", print_hash_value);
    transparent_crc(g_3468.f5, "g_3468.f5", print_hash_value);
    for (i = 0; i < 3; i++)
    {
        transparent_crc(g_3562[i], "g_3562[i]", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_3636, "g_3636", print_hash_value);
    transparent_crc(g_3715, "g_3715", print_hash_value);
    transparent_crc(g_3768, "g_3768", print_hash_value);
    transparent_crc(g_3784.f0, "g_3784.f0", print_hash_value);
    transparent_crc(g_3784.f1, "g_3784.f1", print_hash_value);
    transparent_crc(g_3784.f2, "g_3784.f2", print_hash_value);
    transparent_crc(g_3784.f3, "g_3784.f3", print_hash_value);
    transparent_crc(g_3784.f4, "g_3784.f4", print_hash_value);
    transparent_crc(g_3784.f5, "g_3784.f5", print_hash_value);
    transparent_crc(g_3784.f6, "g_3784.f6", print_hash_value);
    transparent_crc(g_3784.f7, "g_3784.f7", print_hash_value);
    transparent_crc(g_3787.f0, "g_3787.f0", print_hash_value);
    transparent_crc(g_3787.f1, "g_3787.f1", print_hash_value);
    transparent_crc(g_3787.f2, "g_3787.f2", print_hash_value);
    transparent_crc(g_3787.f3, "g_3787.f3", print_hash_value);
    transparent_crc(g_3787.f4, "g_3787.f4", print_hash_value);
    transparent_crc(g_3787.f5, "g_3787.f5", print_hash_value);
    transparent_crc(g_3844, "g_3844", print_hash_value);
    transparent_crc(g_3865, "g_3865", print_hash_value);
    transparent_crc(g_3900, "g_3900", print_hash_value);
    transparent_crc(g_3985.f0, "g_3985.f0", print_hash_value);
    transparent_crc(g_3985.f1, "g_3985.f1", print_hash_value);
    transparent_crc(g_3985.f2, "g_3985.f2", print_hash_value);
    transparent_crc(g_3985.f3, "g_3985.f3", print_hash_value);
    transparent_crc(g_3985.f4, "g_3985.f4", print_hash_value);
    transparent_crc(g_3985.f5, "g_3985.f5", print_hash_value);
    transparent_crc(g_3985.f6, "g_3985.f6", print_hash_value);
    transparent_crc(g_3985.f7, "g_3985.f7", print_hash_value);
    transparent_crc(g_4070.f0, "g_4070.f0", print_hash_value);
    transparent_crc(g_4070.f1, "g_4070.f1", print_hash_value);
    transparent_crc(g_4070.f2, "g_4070.f2", print_hash_value);
    transparent_crc(g_4070.f3, "g_4070.f3", print_hash_value);
    transparent_crc(g_4070.f4, "g_4070.f4", print_hash_value);
    transparent_crc(g_4070.f5, "g_4070.f5", print_hash_value);
    transparent_crc(g_4070.f6, "g_4070.f6", print_hash_value);
    transparent_crc(g_4070.f7, "g_4070.f7", print_hash_value);
    transparent_crc(g_4322.f0, "g_4322.f0", print_hash_value);
    transparent_crc(g_4322.f1, "g_4322.f1", print_hash_value);
    transparent_crc(g_4322.f2, "g_4322.f2", print_hash_value);
    transparent_crc(g_4322.f3, "g_4322.f3", print_hash_value);
    transparent_crc(g_4322.f4, "g_4322.f4", print_hash_value);
    transparent_crc(g_4322.f5, "g_4322.f5", print_hash_value);
    transparent_crc(g_4322.f6, "g_4322.f6", print_hash_value);
    transparent_crc(g_4322.f7, "g_4322.f7", print_hash_value);
    transparent_crc(g_4551.f0, "g_4551.f0", print_hash_value);
    transparent_crc(g_4551.f1, "g_4551.f1", print_hash_value);
    transparent_crc(g_4551.f2, "g_4551.f2", print_hash_value);
    transparent_crc(g_4551.f3, "g_4551.f3", print_hash_value);
    transparent_crc(g_4551.f4, "g_4551.f4", print_hash_value);
    transparent_crc(g_4551.f5, "g_4551.f5", print_hash_value);
    transparent_crc(g_4551.f6, "g_4551.f6", print_hash_value);
    transparent_crc(g_4551.f7, "g_4551.f7", print_hash_value);
    for (i = 0; i < 6; i++)
    {
        transparent_crc(g_4712[i].f0, "g_4712[i].f0", print_hash_value);
        transparent_crc(g_4712[i].f1, "g_4712[i].f1", print_hash_value);
        transparent_crc(g_4712[i].f2, "g_4712[i].f2", print_hash_value);
        transparent_crc(g_4712[i].f3, "g_4712[i].f3", print_hash_value);
        transparent_crc(g_4712[i].f4, "g_4712[i].f4", print_hash_value);
        transparent_crc(g_4712[i].f5, "g_4712[i].f5", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_4716.f0, "g_4716.f0", print_hash_value);
    transparent_crc(g_4716.f1, "g_4716.f1", print_hash_value);
    transparent_crc(g_4716.f2, "g_4716.f2", print_hash_value);
    transparent_crc(g_4716.f3, "g_4716.f3", print_hash_value);
    transparent_crc(g_4716.f4, "g_4716.f4", print_hash_value);
    transparent_crc(g_4716.f5, "g_4716.f5", print_hash_value);
    transparent_crc(g_4719.f0, "g_4719.f0", print_hash_value);
    transparent_crc(g_4719.f1, "g_4719.f1", print_hash_value);
    transparent_crc(g_4719.f2, "g_4719.f2", print_hash_value);
    transparent_crc(g_4719.f3, "g_4719.f3", print_hash_value);
    transparent_crc(g_4719.f4, "g_4719.f4", print_hash_value);
    transparent_crc(g_4719.f5, "g_4719.f5", print_hash_value);
    for (i = 0; i < 2; i++)
    {
        transparent_crc(g_4810[i].f0, "g_4810[i].f0", print_hash_value);
        transparent_crc(g_4810[i].f1, "g_4810[i].f1", print_hash_value);
        transparent_crc(g_4810[i].f2, "g_4810[i].f2", print_hash_value);
        transparent_crc(g_4810[i].f3, "g_4810[i].f3", print_hash_value);
        transparent_crc(g_4810[i].f4, "g_4810[i].f4", print_hash_value);
        transparent_crc(g_4810[i].f5, "g_4810[i].f5", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_4813.f0, "g_4813.f0", print_hash_value);
    transparent_crc(g_4813.f1, "g_4813.f1", print_hash_value);
    transparent_crc(g_4813.f2, "g_4813.f2", print_hash_value);
    transparent_crc(g_4813.f3, "g_4813.f3", print_hash_value);
    transparent_crc(g_4813.f4, "g_4813.f4", print_hash_value);
    transparent_crc(g_4813.f5, "g_4813.f5", print_hash_value);
    transparent_crc(g_5112, "g_5112", print_hash_value);
    transparent_crc(g_5226.f0, "g_5226.f0", print_hash_value);
    transparent_crc(g_5226.f1, "g_5226.f1", print_hash_value);
    transparent_crc(g_5226.f2, "g_5226.f2", print_hash_value);
    transparent_crc(g_5226.f3, "g_5226.f3", print_hash_value);
    transparent_crc(g_5226.f4, "g_5226.f4", print_hash_value);
    transparent_crc(g_5226.f5, "g_5226.f5", print_hash_value);
    transparent_crc(g_5226.f6, "g_5226.f6", print_hash_value);
    transparent_crc(g_5226.f7, "g_5226.f7", print_hash_value);
    for (i = 0; i < 7; i++)
    {
        for (j = 0; j < 5; j++)
        {
            for (k = 0; k < 7; k++)
            {
                transparent_crc(g_5248[i][j][k], "g_5248[i][j][k]", print_hash_value);
                if (print_hash_value) printf("index = [%d][%d][%d]\n", i, j, k);

            }
        }
    }
    transparent_crc(g_5281, "g_5281", print_hash_value);
    transparent_crc(g_5302.f0, "g_5302.f0", print_hash_value);
    transparent_crc(g_5302.f1, "g_5302.f1", print_hash_value);
    transparent_crc(g_5302.f2, "g_5302.f2", print_hash_value);
    transparent_crc(g_5302.f3, "g_5302.f3", print_hash_value);
    transparent_crc(g_5302.f4, "g_5302.f4", print_hash_value);
    transparent_crc(g_5302.f5, "g_5302.f5", print_hash_value);
    transparent_crc(g_5305.f0, "g_5305.f0", print_hash_value);
    transparent_crc(g_5305.f1, "g_5305.f1", print_hash_value);
    transparent_crc(g_5305.f2, "g_5305.f2", print_hash_value);
    transparent_crc(g_5305.f3, "g_5305.f3", print_hash_value);
    transparent_crc(g_5305.f4, "g_5305.f4", print_hash_value);
    transparent_crc(g_5305.f5, "g_5305.f5", print_hash_value);
    for (i = 0; i < 9; i++)
    {
        for (j = 0; j < 10; j++)
        {
            transparent_crc(g_5357[i][j], "g_5357[i][j]", print_hash_value);
            if (print_hash_value) printf("index = [%d][%d]\n", i, j);

        }
    }
    for (i = 0; i < 9; i++)
    {
        transparent_crc(g_5411[i].f0, "g_5411[i].f0", print_hash_value);
        transparent_crc(g_5411[i].f1, "g_5411[i].f1", print_hash_value);
        transparent_crc(g_5411[i].f2, "g_5411[i].f2", print_hash_value);
        transparent_crc(g_5411[i].f3, "g_5411[i].f3", print_hash_value);
        transparent_crc(g_5411[i].f4, "g_5411[i].f4", print_hash_value);
        transparent_crc(g_5411[i].f5, "g_5411[i].f5", print_hash_value);
        transparent_crc(g_5411[i].f6, "g_5411[i].f6", print_hash_value);
        transparent_crc(g_5411[i].f7, "g_5411[i].f7", print_hash_value);
        if (print_hash_value) printf("index = [%d]\n", i);

    }
    transparent_crc(g_5604, "g_5604", print_hash_value);
    platform_main_end(crc32_context ^ 0xFFFFFFFFUL, print_hash_value);
    return 0;
}
