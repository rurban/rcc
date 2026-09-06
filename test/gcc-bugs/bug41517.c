/* GCC Bug #41517 - Unexpected behaviour of #pragma in statement context
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41517
 */
/* { dg-do compile } */


#include <stdio.h>

int x;
void foo (void)
{
  int i, j;
  for (i = 0; i < 10; i++)
#pragma GCC visibility push(default)
    for (j = 0; j < 10; j++)
      x++;
}

int main(void)
{
  foo ();
  printf ("x = %d (should be 100)\n", x);
  return 0;
}

