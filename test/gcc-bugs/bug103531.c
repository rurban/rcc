/* GCC Bug #103531 - Propose compiler warning when ceil/ceilf used on integral value
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103531
 */
/* { dg-do compile } */
/* { dg-options "-Wtraditional-conversion" } */

#include <math.h>

void foo(int x)
{
  int covers_half = ceil(x / 2); /* { dg-warning "as floating rather than integer due to prototype" } */
}

/* If x is a floating-point value, this code acts as expected; but if it's
 * integral, it places the floor of x / 2.0 in covers_half (bug proposal).
 * -Wtraditional-conversion (comment 3) already diagnoses it. */


