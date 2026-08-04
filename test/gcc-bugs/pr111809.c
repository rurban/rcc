/* GCC Bug #111809 - gimpleFE: unreferenced inline function with _GIMPLE(ssa) definition causes ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111809
 */
/* { dg-do compile } */


{
  int _3;

// __BB(2):
//   return;
}
// ```


