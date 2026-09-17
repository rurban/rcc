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
#if defined(__BOOL_WIDTH__) && __BOOL_WIDTH__ == 8
/* Whichever compiler's own <limits.h> wrapper won the #include_next
   chain defined __BOOL_WIDTH__ as 8 (seen with clang's, Apple's and
   upstream alike, on a glibc new enough to honor it) -- check the
   macro that actually determines BOOL_WIDTH's value directly rather
   than guessing from __clang__/__APPLE__: rcc's own self-reported
   identity macros don't necessarily track which compiler's headers
   sysinc_paths.h resolved for this build. */
_Static_assert (BOOL_WIDTH == 8, "bad BOOL_WIDTH");
#else
_Static_assert (BOOL_WIDTH == 1, "bad BOOL_WIDTH");
#endif
