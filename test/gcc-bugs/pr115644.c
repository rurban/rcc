/* GCC Bug #115644 - [gcc][trunk] ICE if redeclare a variable with different type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115644
 */
/* { dg-do compile } */


void f()
{
  int p;
  unsigned v;
  v = (~0u) >> p;
  int p[1] = p;
}
// ```


