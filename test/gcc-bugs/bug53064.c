/* GCC Bug #53064 - -Wsequence-point behaves inconsistently
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53064
 */
/* { dg-options "-Wall" } */

int f(int a)
{
  return 0;
}

// Compiling this with -Wall gives a -Wsequence-point warning:
void
test_via_call (void)
{
  int a = 0;
  a + f(++a ? 0 : 0); /* { dg-warning "may be undefined" } */
}

// while compiling this with -Wall produces no warning about sequence
// points, even though the reporter believes both are equally UB:
void
test_direct (void)
{
  int a = 0;
  a + (++a ? 0 : 0);
}

int
main (void)
{
  test_via_call ();
  test_direct ();
  return 0;
}
