/* GCC Bug #10360 - __alignof__(double) answer 8
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=10360
 */


#include <stdio.h>

struct bla
{
  char c;
  double v;
};

void main (void)
{
  struct bla b;
//   printf("align double %d\neffective alignement %d\n", __alignof__(double),
// 	 ((char *)(&b.v)) - ((char *)(&b.c)));
}


