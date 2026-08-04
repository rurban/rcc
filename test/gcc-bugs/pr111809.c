/* GCC Bug #111809 - gimpleFE: unreferenced inline function with _GIMPLE(ssa) definition causes ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111809
 */
/* { dg-do compile } */
/* { dg-options "-fgimple -O0" } */

/* Comment 2's reduced testcase: an unreferenced inline function with a
 * _GIMPLE(ssa) definition used to ICE in cgraph_node::release_body
 * ("SSA name with no definition").  Modern gcc errors out cleanly. */
inline
__GIMPLE (ssa) void
bar (void)
{
  int _3;

__BB(2):
  return;
}