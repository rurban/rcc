/* GCC Bug #81631 - -Wcast-qual false positive for pointer to array
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81631
 */
/* { dg-do compile } */


typedef int footype[6];

extern void const * bar;

void baz(void)
{
  footype const * x;
  x = (footype const *) bar;
}
// ------
// with -Wcast-qual, this yields
// ------
// foo.c:12:7: warning: cast discards 'const' qualifier from pointer target type [-Wcast-qual]
   x = (footype const *) bar;
//        ^
// ------
// If "footype" is any other type than an array, I don't get the warning. I tested gcc 4.5.1, 6.3.0, 7.1.0, on cygwin and Debian, all show this behavior. clang doesn't warn.


