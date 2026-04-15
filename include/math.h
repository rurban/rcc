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
int isinf(double);
int isnan(double);
int isfinite(double);
int signbit(double);

float sinf(float);
float cosf(float);
float tanf(float);
float sqrtf(float);
float powf(float, float);
float expf(float);
float logf(float);
float log10f(float);
float fabsf(float);
float floorf(float);
float ceilf(float);
float roundf(float);
float fmodf(float, float);
float atan2f(float, float);
float asinf(float);
float acosf(float);
float atanf(float);
float sinhf(float);
float coshf(float);
float tanhf(float);

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

#define INFINITY (1.0/0.0)
#define NAN (0.0/0.0)
#define HUGE_VAL (1.0/0.0)
#define HUGE_VALF ((float)(1.0/0.0))

#endif
