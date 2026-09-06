/* GCC Bug #87310 - -Wc90-c99-compat does not warn about bool usage
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87310
 */
/* { dg-do compile } */
/* { dg-options "-std=c90 -Wc90-c99-compat" } */

#include <stdio.h>
#include <stdbool.h>
main() {
 bool ok = true;
 printf("%u\n", ok);
}
