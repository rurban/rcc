/* GCC Bug #79959 - -Wimplicit-fallthrough doesn't recognize some more complex exit cases
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79959
 */
/* { dg-do compile } */

#include <stdlib.h>
int
main(int argc, char **argv)
{
 switch (argc)
 {
 case 1:
  do { if (__builtin_constant_p(20) && (20) >= 20) abort(); } while(0);
 default:
  return 0;
 }
}
// gcc-7 -Wimplicit-fallthrough -O2 -c test.c
// test.c: In function 'main':
// test.c:9:11: warning: this statement may fall through [-Wimplicit-fallthrough=]
// do { if (__builtin_constant_p(20) && (20) >= 20) abort(); } while(0);
// I would expect no warning here.
// This concoction is a macro expansion that intends to communicate "noreturn" based on some constant argument value, and that generally works well for avoiding uninitialized variable warnings and things like that, so the control flow analysis is smart enough to recognize that this is an unconditional exit.  But the fallthrough analysis doesn't appear to be that smart.  It only detects a plain abort() or something like that.


