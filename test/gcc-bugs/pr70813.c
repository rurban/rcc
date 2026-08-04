/* GCC Bug #70813 - Wrong warning "'0' flag ignored with precision and ‘%d’ gnu_printf format"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70813
 */
/* { dg-do compile } */


#include <stdio.h>

int main()
{
//   printf("abc"
// 	 "%05.*d\n", -1, 1);
}
// I get the following error message:
// example.c: In function ‘main’:
// example.c:5:10: warning: '0' flag ignored with precision and ‘%d’ gnu_printf format [-Wformat=]
//    printf("abc"
//           ^~~~~
// The output from the program:
// abc00001
// There are two issues here.
// 1. Wrong position for the warning.
// 2. The warning itself is wrong in this corner case. Yes, '0' flag is ordinarily ignored with precision and %d per C11, 7.21.6.1p6: "For d, i, o, u, x, and X conversions, if a precision is specified, the 0 flag is ignored." But the precision is negative here and C11, 7.21.6.1p5, reads: "A negative precision argument is taken as if the precision were omitted."
// The output from the program demonstrates that indeed '0' flag is not ignored.
// Maybe it's better to limit this warning to cases where precision is known at compile time (e.g., for simplicity, when it's not '*')?
// This gcc bug was pointed out to me in <a href="https://twitter.com/spun_off/status/723558401599524865">https://twitter.com/spun_off/status/723558401599524865</a> .


