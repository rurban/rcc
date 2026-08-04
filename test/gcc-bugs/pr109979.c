/* GCC Bug #109979 - -Wformat-overflow false positive for %d and non-basic expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109979
 */
/* { dg-do compile } */


#include <stdio.h>

void f (int *);

void g (void)
{
  int e;
  char s[4];

  f (&e);
  sprintf (s, "%d", e);
  sprintf (s, "%d", e - 1);
}

// I get on my Linux/x86_64 machine with gcc-12 (Debian 12.2.0-14) 12.2.0:
// zira:~> gcc-12 -Wformat-overflow -c tst.c
// tst.c: In function ‘g’:
// tst.c:12:16: warning: ‘%d’ directive writing between 1 and 11 bytes into a region of size 4 [-Wformat-overflow=]
//    12 |   sprintf (s, "%d", e - 1);
//       |                ^~
// tst.c:12:15: note: directive argument in the range [-2147483648, 2147483646]
//    12 |   sprintf (s, "%d", e - 1);
//       |               ^~~~
// tst.c:12:3: note: ‘sprintf’ output between 2 and 12 bytes into a destination of size 4
//    12 |   sprintf (s, "%d", e - 1);
//       |   ^~~~~~~~~~~~~~~~~~~~~~~~
// Note that the warning occurs for "e - 1" but not for "e".
// This bug was found when compiling GNU MPFR 4.2.0 with "-std=c90 -Werror=format -m32" (compilation failure for get_d64.c).


