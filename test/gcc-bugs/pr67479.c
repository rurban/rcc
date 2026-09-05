/* GCC Bug #67479 - Support for -Wformat-pedantic
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67479
 */
/* { dg-do compile } */

#include <stdio.h>

int main(void)
{
  printf("%1$d\n", 1);
  return 0;
}
