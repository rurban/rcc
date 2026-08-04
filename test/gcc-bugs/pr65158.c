/* GCC Bug #65158 - printf attribute error reporting assumes single-byte characters
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65158
 */


void foo(void) {
    __builtin_printf("%ñ%中");
//     __builtin_printf ("%\x1B$B"); /* Taken from <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED DUPLICATE - format warnings don't take input charset into account"
//    href="show_bug.cgi?id=33748">PR33748</a> */
 }
// <source>: In function 'void foo()':
// <source>:2:22: warning: unknown conversion type character '\xc3' in format [-Wformat=]
// 2 |     __builtin_printf("%ñ%中");
//   |                      ^~~~~~~~~
// <source>:2:22: warning: unknown conversion type character '\xe4' in format [-Wformat=]
// <source>:3:23: warning: unknown conversion type character '\x1b' in format [-Wformat=]
// 3 |     __builtin_printf ("%\x1B$B");
//   |                       ^~~~~~~~~
// Note that ñ and 中 are multi-byte but the message only shows one byte.
// Another bug is that the location info fails to point within the format string (ccing David):

void foo(int i) {
     __builtin_printf("%i%i%i%i%í%i%i",i,i,i,i,i,i);
    }
// <source>:2:23: warning: unknown conversion type character '\xc3' in format [-Wformat=]
// 2 |      __builtin_printf("%i%i%i%i%í%i%i",i,i,i,i,i,i);
//   |                       ^~~~~~~~~~~~~~~~~


