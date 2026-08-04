/* GCC Bug #111816 - [gimple FE] ICE with _GIMPLE(ssa) and 2 returns
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111816
 */
/* { dg-do compile } */

/* Comment 0's testcase: two returns in a _GIMPLE(ssa) function used to
 * segfault gcc in c_parser_parse_gimple_body (make_edge returning NULL
 * when building CFG edges, comment 3).  __GIMPLE is gcc's internal
 * GIMPLE front-end for testsuite use only. */
__GIMPLE (ssa) int foo (int *a)
{
  return 0;
  return 0;
}