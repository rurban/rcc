/* GCC Bug #86894 - error for a zero-length array initialized with empty braced list
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86894
 */


char a[] = { };
// e.c:1:12: warning: ISO C forbids empty initializer braces [-Wpedantic]
 char a[] = { };
//             ^
// e.c:1:6: error: zero or negative size array ‘a’
 char a[] = { };
//       ^


