/* GCC Bug #80959 - -Wreturn-type "control reaches end of non-void function" false positive with -fsanitize=address
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80959
 */
/* { dg-do compile } */
/* { dg-options "-Wreturn-type -Werror -O0" } */

volatile int i;

void bar (int *n);

int
foo (void)
{
  int n;
  bar (&n);

  switch (i)
    {
    case 1:
      switch (i)
	{
	default:
	  return 0;
	}
      break;
    default:
      return 0;
    }
}

// This is a false positive: every path through the switch returns, so
// control never actually reaches the closing brace.  Richard Biener's
// analysis (comment 2): with -fsanitize=address, decide_copy_try_finally
// no longer duplicates the (empty here) finally block, so a dispatch
// switch on finally_tmp plus its problematic fallthrough return block are
// left behind after the redundant "case 1" break is optimized away, and
// the CFG cleanup pass fails to notice the remaining path is dead:
//
//   <bb 5> [0.00%]:
//   ASAN_MARK (POISON, &n, 4);
//   switch (finally_tmp.2) <default: <L8> [0.00%], case 1: <L5> [0.00%]>
//
// <L5> [0.00%]:
//   <bb 7> [0.00%]:
//   return;
//
// <L8> [0.00%]:
//   return D.2131;
//
// The problem disappears with -O1 or higher, or without -fsanitize=address.
// Not reproducible with current gcc (16.2.1 here): the closing brace no
// longer warns, i.e. this false positive appears to have been fixed since
// the 7.1.0/master-as-of-2017-06 snapshot the bug was filed against.



