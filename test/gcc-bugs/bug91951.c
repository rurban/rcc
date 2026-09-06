/* GCC Bug #91951 - missed (optimization-level dependent) diagnostic for goto when cleanup attribute callback is not invoked
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91951
 */


#include <stdio.h>

static void
func (void **ptr)
{
  printf ("free(%p)\n", *ptr);
}

int
main (void)
{
  goto out;

  __attribute__((__cleanup__(func))) void *x = NULL;

out:
  return 0;
}

// This blog post implies that GCC gives a helpful warning about this
// problem, but modern GCC versions seem to have lost this warning.  By
// comparison, clang gives a warning and turns the goto itself into a hard
// error in response to detecting the problem.
// clang version 7.0.1-8 (tags/RELEASE_701/final)


