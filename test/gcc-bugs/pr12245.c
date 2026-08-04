/* GCC Bug #12245 - [13/14/15/16/17 regression] Uses lots of memory when compiling large initialized arrays
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=12245
 */


int a[2][100000000] = { { huge NATIVE_ENCODE_RANGE_EXPR initializer here }, [0][42] = 42 };
// For the non-compressed target dependent initializer we actually have a tree already, STRING_CST, and we actually since <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - missing strlen optimization on different array initialization style"
//    href="show_bug.cgi?id=71625">PR71625</a> use it for char/signed char/unsigned char array initializers, but decide to use it and convert to it only after the initializer parsing is done, while to avoid using lots of memory we'd need to decide for that already during parsing, say after parsing a couple hundreds or thousands elements.  And we might consider using it for other types as well and just natively encode/decode stuff from/to the STRING_CST as needed.


