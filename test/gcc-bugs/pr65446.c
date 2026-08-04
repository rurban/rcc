/* GCC Bug #65446 - Improve -Wformat-signedness
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65446
 */


// As <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - [5 Regression] gcc-5 -Wformat broken"
//    href="show_bug.cgi?id=65040">PR 65040</a> shows, the current implementation of -Wformat-signedness is not optimal. (When it is more robust, one could consider to re-enable it with -Wformat=2.)
// The idea of the warning is to warn for
//    "%ld",   size_t_variable
// as one has to use "%lu" to print the negative values. Or reversely using %u for a signed value, where it is even more likely that the issue occurs.
// See also "cppcheck --enable=warning" which supports this warning. (But its warning pattern makes more sense than GCC's.)
// GCC's current implementation warns too often - and misses some cases. The main bug is that it doesn't take type promotion into account. In particular:
// It warns for:
   printf ("%u\n", unsigned_short);
// but shouldn't: First, %u and unsigned_short are both unsigned. (It also shouldn't and doesn't warn for %d with unsigned short as all values are representible by %d.
// It doesn't warn but should warn for
//    printf("%u\n", _short);

// As passing, e.g., (short)-1 to %u will print the wrong value (UINT_MAX instead of -1).
// GCC currently also warns for printf("%x\n", 1), which is very questionable.


