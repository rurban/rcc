/* GCC Bug #53064 - -Wsequence-point behaves inconsistently
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53064
 */


int f(int a)
{
  return 0;
}
// main()
{
  int a = 0;
  a + f(++a ? 0 : 0);
}
// main()
{
  int a = 0;
  a + (++a ? 0 : 0);
}
// I think (not sure though) both are UB.


