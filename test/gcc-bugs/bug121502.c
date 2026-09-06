/* GCC Bug #121502 - [15/16 Regresion] ICE after invalid use of va_start
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121502
 */
/* { dg-do compile } */

/* va_start at file scope (invalid) used to ICE without an error message
 * (comment 0).  gcc now reports it cleanly. */
#include <stdarg.h>
  int n, m, r;
  int arg;
  r = va_start(arg, n); /* { dg-error "type defaults to .int. in declaration of .r." } */