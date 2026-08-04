/* GCC Bug #53129 - Wself-assign
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53129
 */


void func()
{
  int i = 2;
  int x = 0;   /* warn */
  float f = 5.0;
  double a[3];

//   i = i + 0;   /* not warn */
  f = f / 1;   /* not warn */
  a[1] = a[1]; /* warn */
//   i += 0;      /* not warn */
  x = x;      /* warn */
}
// manuel@gcc12:~$ clang -fsyntax-only -Wself-assign wselfassign.c
// wselfassign.c:12:5: warning: explicitly assigning a variable of type 'int' to itself [-Wself-assign]
  x = x;      /* warn */
//   ~ ^ ~
// 1 warning generated.
// An implementation was submitted to GCC but never approved here:
// <a href="http://gcc.gnu.org/ml/gcc-patches/2010-06/msg02390.html">http://gcc.gnu.org/ml/gcc-patches/2010-06/msg02390.html</a>


