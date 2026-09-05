/* GCC Bug #67819 - -Wduplicated-cond should take macros into account
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67819
 */
/* { dg-do compile } */


#define FOO 4
#define BAR 4
int
fn (int n)
{
  if (n <= FOO)
    return 1;
  else if (n <= BAR)
    return 2;
  return 0;
}


