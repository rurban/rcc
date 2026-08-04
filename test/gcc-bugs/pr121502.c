/* GCC Bug #121502 - [15/16 Regresion] ICE after invalid use of va_start
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121502
 */
/* { dg-do compile } */


#include <stdarg.h>
  int n, m, r;
  r = va_start(arg, n);


