/* GCC Bug #44515 - improve message for missing ";"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44515
 */
/* { dg-do compile } */


void bar(void);
void foo(void)
{
  bar()


} /* { dg-error "expected" } */
