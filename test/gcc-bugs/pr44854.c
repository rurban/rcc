/* GCC Bug #44854 - Improve diagnostic for missing member name or ';' in a struct
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44854
 */
/* { dg-do compile } */


struct foo { int };
// gcc-4.6:
// test.c:1:18: error: expected identifier or ‘(’ before ‘}’ token
// test.c:1:18: error: expected specifier-qualifier-list at end of input
clang:
// test.c:1:18: error: expected member name or ';' after declaration specifiers
struct foo { int };
//              ~~~ ^
// Clang knows what you mean and the diagnostic message is better.


