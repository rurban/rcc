/* GCC Bug #91951 - missed (optimization-level dependent) diagnostic for goto when cleanup attribute callback is not invoked
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91951
 */


#include <stdio.h>

static void
// func (void **ptr)
{
  printf ("free(%p)\n", *ptr);
}

// main (void)
{
  goto out;

  __attribute__((__cleanup__(func))) void *x = NULL;

  return 0;
}
// free(0x559190032050)

  __attribute__((__cleanup__(func))) void *x = NULL;
  goto out;
// test.c:14:44: note: jump bypasses initialization of variable with __attribute__((cleanup))
  __attribute__((__cleanup__(func))) void *x = NULL;

// clang version 7.0.1-8 (tags/RELEASE_701/final)


