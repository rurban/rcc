#ifndef RCC_MATH_H
#define RCC_MATH_H

double sin(double);
double cos(double);
double tan(double);
double sqrt(double);
double pow(double, double);
double exp(double);
double log(double);
double log2(double);
double log10(double);
double fabs(double);
double floor(double);
double ceil(double);
double round(double);
double trunc(double);
double fmod(double, double);
double fma(double, double, double);
float fmaf(float, float, float);
double atan2(double, double);
double asin(double);
double acos(double);
double atan(double);
double sinh(double);
double cosh(double);
double tanh(double);
double asinh(double);
double acosh(double);
double atanh(double);
double exp2(double);
double expm1(double);
double log1p(double);
double cbrt(double);
double hypot(double, double);
double erf(double);
double erfc(double);
double copysign(double, double);
double remainder(double, double);
double fdim(double, double);
double fmax(double, double);
double fmin(double, double);
double nearbyint(double);
double rint(double);
long lround(double);
long lrint(double);
long long llround(double);
long long llrint(double);
double scalbn(double, int);
double ldexp(double, int);
double frexp(double, int *);
double modf(double, double *);
#ifndef _WIN32
int ilogb(double); /* mingw-w64 provides no ilogb symbol at all */
#endif
/* IEEE 754 comparison macros (C99 7.12.14): type-generic, no libc symbols.
 * glibc defines these as __builtin_isgreater etc. under GCC >= 3.1, but
 * rcc's bundled <math.h> chains via #include_next only for system libs
 * that need it -- for self-contained builds these must be defined here. */
#define isunordered(x, y)   ((x) != (x) || (y) != (y))
#define isgreater(x, y)     (!isunordered(x, y) && (x) > (y))
#define isgreaterequal(x, y) (!isunordered(x, y) && (x) >= (y))
#define isless(x, y)        (!isunordered(x, y) && (x) < (y))
#define islessequal(x, y)   (!isunordered(x, y) && (x) <= (y))
#define islessgreater(x, y) (!isunordered(x, y) && ((x) < (y) || (x) > (y)))

/* Classification: standard C requires these to be type-generic and they have
 * no backing libc symbols (fpclassify/isfinite/isnormal are macros-only in
 * glibc). Map them to rcc's runtime builtins, dispatched on operand size.
 * signbit is already provided as a builtin macro by the preprocessor. */
#define fpclassify(x) \
    (sizeof(x) == sizeof(float)  ? __builtin_fpclassifyf(x) : \
     sizeof(x) == sizeof(double) ? __builtin_fpclassify(x)  : \
                                   __builtin_fpclassifyl(x))
#define isinf(x) \
    (sizeof(x) == sizeof(float)  ? __builtin_isinff(x) : \
     sizeof(x) == sizeof(double) ? __builtin_isinf(x)  : \
                                   __builtin_isinfl(x))
#define isnan(x) ((x) != (x))
#define isfinite(x) \
    (sizeof(x) == sizeof(float)  ? __builtin_isfinitef(x) : \
     sizeof(x) == sizeof(double) ? __builtin_isfinite(x)  : \
                                   __builtin_isfinitel(x))
#define isnormal(x) \
    (sizeof(x) == sizeof(float)  ? __builtin_isnormalf(x) : \
     sizeof(x) == sizeof(double) ? __builtin_isnormal(x)  : \
                                   __builtin_isnormall(x))

float sinf(float);
float cosf(float);
float tanf(float);
float sqrtf(float);
float powf(float, float);
float expf(float);
float logf(float);
float log2f(float);
float log10f(float);
float fabsf(float);
float floorf(float);
float ceilf(float);
float roundf(float);
float truncf(float);
float fmodf(float, float);
float atan2f(float, float);
float asinf(float);
float acosf(float);
float atanf(float);
float sinhf(float);
float coshf(float);
float tanhf(float);
float asinhf(float);
float acoshf(float);
float atanhf(float);
float exp2f(float);
float expm1f(float);
float log1pf(float);
float cbrtf(float);
float hypotf(float, float);
float erff(float);
float erfcf(float);
float copysignf(float, float);
float remainderf(float, float);
float fdimf(float, float);
float fmaxf(float, float);
float fminf(float, float);
float nearbyintf(float);
float rintf(float);
long lroundf(float);
long lrintf(float);
long long llroundf(float);
long long llrintf(float);
float scalbnf(float, int);
float ldexpf(float, int);
float frexpf(float, int *);
float modff(float, float *);
#ifndef _WIN32
int ilogbf(float);
#endif

long double sinl(long double);
long double cosl(long double);
long double tanl(long double);
long double sqrtl(long double);
long double powl(long double, long double);
long double expl(long double);
long double logl(long double);
long double log2l(long double);
long double log10l(long double);
long double fabsl(long double);
long double floorl(long double);
long double ceill(long double);
long double roundl(long double);
long double truncl(long double);
long double fmodl(long double, long double);
long double fmal(long double, long double, long double);
long double atan2l(long double, long double);
long double asinl(long double);
long double acosl(long double);
long double atanl(long double);
long double sinhl(long double);
long double coshl(long double);
long double tanhl(long double);
long double asinhl(long double);
long double acoshl(long double);
long double atanhl(long double);
long double exp2l(long double);
long double expm1l(long double);
long double log1pl(long double);
long double cbrtl(long double);
long double hypotl(long double, long double);
long double erfl(long double);
long double erfcl(long double);
long double copysignl(long double, long double);
long double remainderl(long double, long double);
long double fdiml(long double, long double);
long double fmaxl(long double, long double);
long double fminl(long double, long double);
long double nearbyintl(long double);
long double rintl(long double);
long lroundl(long double);
long lrintl(long double);
long long llroundl(long double);
long long llrintl(long double);
long double scalbnl(long double, int);
long double ldexpl(long double, int);
long double frexpl(long double, int *);
long double modfl(long double, long double *);
#ifndef _WIN32
int ilogbl(long double);
#endif
/* glibc's <math.h> declares `extern int signgam;` (used by gamma/lgamma;
 * zsh's math module reads it directly). The bundled header is
 * self-contained, so declare it here too. */
extern int signgam;

/* gamma-family: glibc declares these (under _GNU_SOURCE/_XOPEN2K8), and
 * zsh's math module calls tgamma()/lgamma() directly. Without the
 * declaration rcc calls them as implicit-int functions, reading the int
 * return register instead of xmm0 (tgamma(2) came back 0). */
double tgamma(double);
double lgamma(double);
double gamma(double);
float tgammaf(float);
float lgammaf(float);
float gammaf(float);
long double tgammal(long double);
long double lgammal(long double);
long double gammal(long double);

#define M_PI    3.14159265358979323846
#define M_E     2.71828182845904523536
#define M_LN2   0.69314718055994530942
#define M_LN10  2.30258509299404568402
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.43429448190325182765
#define M_SQRT2 1.41421356237309504880
#define M_PI_2  1.57079632679489661923
#define M_PI_4  0.78539816339744830962
#define M_1_PI  0.31830988618379067154
#define M_2_PI  0.63661977236758134308
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT1_2 0.70710678118654752440

// INFINITY / NAN are required by C11 7.12p3/p5 to be constant
// expressions of type float (glibc: __builtin_inff() / __builtin_nanf
// ("")). Do NOT define NAN as (0.0/0.0): on x86, hardware division's
// invalid-operand fallback sets the sign bit, so 0.0/0.0 legitimately
// evaluates to a NEGATIVE NaN at runtime -- observably different from
// glibc's NAN and enough to break output-diff tests that print it
// (json-c's test_cast: "nan" expected, "-nan" from a wrongly-signed
// NAN macro). Matches float.h's NAN, so both headers agree when a
// program includes both.
#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))
#define HUGE_VAL (1.0/0.0)
#define HUGE_VALF ((float)(1.0/0.0))
#define HUGE_VALL ((long double)(1.0/0.0))

#define FP_NAN          0
#define FP_INFINITE     1
#define FP_NORMAL       2
#define FP_SUBNORMAL    3
#define FP_ZERO         4

#ifndef _WIN32
/* Special ilogb()/ilogbf()/ilogbl() return values (C99 7.12.6.5p2):
 * argument 0 and NaN respectively. Matches glibc's own values on every
 * target this compiler runs on (INT_MIN / INT_MAX). Guarded like the
 * functions above: mingw-w64 provides neither. */
#define FP_ILOGB0    (-2147483647 - 1)
#define FP_ILOGBNAN  (-2147483647 - 1)
#endif

#endif
