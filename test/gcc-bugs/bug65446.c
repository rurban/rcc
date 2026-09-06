/* GCC Bug #65446 - Improve -Wformat-signedness
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65446
 */
/* { dg-do compile } */

#include <stdio.h>

// As PR 65040 shows, the current implementation of -Wformat-signedness is
// not optimal. (When it is more robust, one could consider to re-enable it
// with -Wformat=2.)
// The idea of the warning is to warn for
//    "%ld",   size_t_variable
// as one has to use "%lu" to print the negative values. Or reversely using
// %u for a signed value, where it is even more likely that the issue occurs.
// See also "cppcheck --enable=warning" which supports this warning. (But
// its warning pattern makes more sense than GCC's.)
// GCC's current implementation warns too often - and misses some cases. The
// main bug is that it doesn't take type promotion into account.
void f (unsigned short unsigned_short, short _short)
{
  // It warns for the following, but shouldn't: first, %u and
  // unsigned_short are both unsigned. (It also shouldn't and doesn't warn
  // for %d with unsigned short as all values are representable by %d.)
  printf ("%u\n", unsigned_short);

  // It doesn't warn but should warn for the following, as passing, e.g.,
  // (short)-1 to %u will print the wrong value (UINT_MAX instead of -1).
  // (Comment #1: actually it does correctly warn here, since 'short' is
  // promoted to 'int' when passed through varargs.)
  printf ("%u\n", _short);

  // GCC currently also warns for the following, which is very questionable.
  printf ("%x\n", 1);
}
