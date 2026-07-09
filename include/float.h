#ifndef RCC_FLOAT_H
#define RCC_FLOAT_H

#define FLT_RADIX 2
#define FLT_ROUNDS 1
#define FLT_EVAL_METHOD 0

#define DECIMAL_DIG DBL_DECIMAL_DIG

/* float */
#define FLT_MANT_DIG 24
#define FLT_DIG 6
#define FLT_DECIMAL_DIG 9
#define FLT_MIN_EXP (-125)
#define FLT_MIN_10_EXP (-37)
#define FLT_MAX_EXP 128
#define FLT_MAX_10_EXP 38
#define FLT_MIN 1.17549435082228750797e-38F
#define FLT_MAX 3.40282346638528859812e+38F
#define FLT_EPSILON 1.19209289550781250000e-7F
#define FLT_TRUE_MIN 1.40129846432481707092e-45F
#define FLT_HAS_SUBNORM 1

/* double */
#define DBL_MANT_DIG 53
#define DBL_DIG 15
#define DBL_DECIMAL_DIG 17
#define DBL_MIN_EXP (-1021)
#define DBL_MIN_10_EXP (-307)
#define DBL_MAX_EXP 1024
#define DBL_MAX_10_EXP 308
#define DBL_MIN 2.22507385850720138309e-308
#define DBL_MAX 1.79769313486231570815e+308
#define DBL_EPSILON 2.22044604925031308085e-16
#define DBL_TRUE_MIN 4.94065645841246544177e-324
#define DBL_HAS_SUBNORM 1

/* long double (stored as 64-bit double by rcc codegen) */
#define LDBL_MANT_DIG DBL_MANT_DIG
#define LDBL_DIG DBL_DIG
#define LDBL_DECIMAL_DIG DBL_DECIMAL_DIG
#define LDBL_MIN_EXP DBL_MIN_EXP
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP
#define LDBL_MAX_EXP DBL_MAX_EXP
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP
#define LDBL_MIN DBL_MIN
#define LDBL_MAX DBL_MAX
#define LDBL_EPSILON DBL_EPSILON
#define LDBL_TRUE_MIN DBL_TRUE_MIN
#define LDBL_HAS_SUBNORM 1

/* Macros added to <float.h> in C23. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_FLOAT_H__ 202311L

#define FLT_NORM_MAX FLT_MAX
#define DBL_NORM_MAX DBL_MAX
#define LDBL_NORM_MAX LDBL_MAX

#define FLT_IS_IEC_60559 1
#define DBL_IS_IEC_60559 1
#define LDBL_IS_IEC_60559 1

#define INFINITY (__builtin_inff())
#define NAN (__builtin_nanf(""))

#define FLT_SNAN (__builtin_nansf(""))
#define DBL_SNAN (__builtin_nans(""))
#define LDBL_SNAN (__builtin_nansl(""))
#endif

#endif
