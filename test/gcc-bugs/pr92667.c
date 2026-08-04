/* GCC Bug #92667 - spurious missing sentinel in function call with a local sentinel variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92667
 */
/* { dg-do compile } */


void f (void)
{
  const char* const null = 0;

  __builtin_execl ("foo", "bar", null);
}
void f (void)
{
  const char* const null = 0;

  __builtin_execl ("foo", "bar", null);
}
//     5 |   __builtin_execl ("foo", "bar", null);


