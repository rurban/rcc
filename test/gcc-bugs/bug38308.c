/* GCC Bug #38308 - -Wformat does not work for wide strings
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38308
 */
/* { dg-do compile } */

#include <stdio.h>
#include <wchar.h>

int main(void)
{
  wprintf (L"%s", 5); /* gcc should diagnose the %s/5 mismatch here
                         like it does for narrow strings - the bug */
  return 0;
}
