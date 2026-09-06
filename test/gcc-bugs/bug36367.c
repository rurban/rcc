/* GCC Bug #36367 - warning for questionable compound expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36367
 */
/* { dg-do compile } */


int foo (int a)
{
  return a;
}
int bar (int c)
{
  if (foo (1), c) // Warn foo(1) don't have obvious side-effects here?
    return 1;
//   else
    return 0;
}
// ===================


