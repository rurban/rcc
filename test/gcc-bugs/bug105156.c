/* GCC Bug #105156 - No diagnostic for `enum { toobig = UINT_MAX }`
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105156
 */
/* { dg-do compile } */
/* { dg-options "-std=c17 -pedantic-errors" } */

#include <limits.h>
enum { toobig    = (0x7fffffff * 2U + 1U) }; /* { dg-error "ISO C restricts enumerator values to range of .int." } */
enum { toobigtoo = UINT_MAX };              /* missing diagnostic: macro expansion in a system
                                               header suppresses the -Wpedantic error (comment 6) */