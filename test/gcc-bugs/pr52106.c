/* GCC Bug #52106 - [C11] missing -Wunused-but-set-variable warning with the a = b = ... construct
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52106
 */


int foo (void)
{
  int a, b, c, d;
  a = b = 0;
  c = d = 0;
  return a + d;
}
gives:
// tst.c: In function 'foo':
// tst.c:3:13: warning: variable 'c' set but not used [-Wunused-but-set-variable]

// One would expect the same warning for variable 'b'. Even though it appears to be used to assign variable 'a', it shouldn't count in such a construct. Indeed a variable assignment is useless if the variable isn't used in a later expression (if the goal is to convert the value, then a cast would be more readable).
// Tested with:


