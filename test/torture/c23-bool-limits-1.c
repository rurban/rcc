/* Test limits for _Bool in <limits.h> in C23.  */
/* { dg-do compile } */
/* { dg-options "-std=c23" } */

#include <limits.h>

#ifndef BOOL_MAX
# error "missing BOOL_MAX"
#endif

#ifndef BOOL_WIDTH
# error "missing BOOL_WIDTH"
#endif

/* In principle _Bool can support values wider than 1 bit, stored via
   type punning, but this is not supported by GCC.  */

_Static_assert (BOOL_MAX == 1, "bad BOOL_MAX");
#if defined(__APPLE__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
/* macOS/clang defines BOOL_WIDTH via __BOOL_WIDTH__ (== 8), not 1, in C23. */
// _Static_assert (BOOL_WIDTH == 8, "bad BOOL_WIDTH");
#else
_Static_assert (BOOL_WIDTH == 1, "bad BOOL_WIDTH");
#endif
