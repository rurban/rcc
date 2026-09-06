/* GCC Bug #92667 - spurious missing sentinel in function call with a local sentinel variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92667
 */
/* { dg-do compile } */


void f (void)
{
  const char* const null = 0;

  __builtin_execl ("foo", "bar", null);
}
// (The file above was shown twice in the original report's terminal
// transcript, via `cat t.c && cat t.c`; only one definition belongs here.)
//
// t.c: In function 'f':
// t.c:5:3: warning: missing sentinel in function call [-Wformat=]
//     5 |   __builtin_execl ("foo", "bar", null);


