/* GCC Bug #59753 - -Woverflow warning inconsistency with signed constant conversion between T_MAX+1 and UT_MAX vs larger than UT_MAX
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59753
 */


#include <stdio.h>
int main (void)
{
  short a = 32768;
  short b = 65535;
  short c = 65536;
  int d = 2147483648;
  int e = 4294967295;
  int f = 4294967296;
  printf ("%d %d %d %d %d %d\n", a, b, c, d, e, f);
  return 0;
}
// tst.c: In function 'main':
// tst.c:6:3: warning: overflow in implicit constant conversion [-Woverflow]
   short c = 65536;
//    ^
// tst.c:9:3: warning: overflow in implicit constant conversion [-Woverflow]
   int f = 4294967296;
//    ^
// This occurs with:
// and older versions (4.7 and 4.8 at least).


