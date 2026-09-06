/* GCC Bug #51437 - GCC should warn on the use of reserved identifier/macro names
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51437
 */
/* { dg-do compile } */


int foo (void)
{
  int signbit = 0;
  return signbit;
}
// should also trigger a warning.


