/* GCC Bug #111059 - ICE: in gimplify_expr, at gimplify.cc:17253
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111059
 */
/* { dg-do compile } */


void f() {
  (_Bool) (1 << -1);
}
// ```


