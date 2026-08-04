/* GCC Bug #15338 - There should be a __format__ attribute for syslog.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=15338
 */
/* { dg-do compile } */
/* { dg-options "-Wformat -Wpedantic" } */


#include <syslog.h>

void f (void)
{
  syslog (0, "%m"); /* { dg-warning "ISO C does not support the .%m. gnu_printf format" } */
}
// + for o in ''\'''\''' ''\''-Wpedantic'\'''
// + /build/gcc-svn/gcc/xgcc -B /build/gcc-svn/gcc -S -Wformat b.c
// + for o in ''\'''\''' ''\''-Wpedantic'\'''
// + /build/gcc-svn/gcc/xgcc -B /build/gcc-svn/gcc -S -Wformat -Wpedantic b.c
// b.c: In function ‘f’:
// b.c:5:16: warning: ISO C does not support the ‘%m’ gnu_printf format [-Wformat=]
// syslog (0, "%m");
//                 ^


