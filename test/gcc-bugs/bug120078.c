/* GCC Bug #120078 - -Wjump-misses-init should be enabled by -Wc++-compat
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120078
 */
/* { dg-do compile } */


int f(int a)
{
  goto b;
  int t = a;
b:
  return t;
}
// ```


