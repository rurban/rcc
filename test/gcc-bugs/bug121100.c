/* GCC Bug #121100 - ICE: Segmentation fault at contains_struct_check
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121100
 */
/* { dg-do compile } */


// 2 | void f(int n; int (*(*b)(void))[n]) { sizeof(*(*b)(); int n, int n; int (*(*b)(void))[n]) { sizeof(*(*b)()); }
//     2 | void f(int n; int (*(*b)(void))[n]) { sizeof(*(*b)(); int n, int n; int (*(*b)(void))[n]) { sizeof(*(*b)()); }
// <source>:2:90: error: expected ';' before '{' token
//     2 | void f(int n; int (*(*b)(void))[n]) { sizeof(*(*b)(); int n, int n; int (*(*b)(void))[n]) { sizeof(*(*b)()); }
//     6 | void g(void) { f(1, &a); }
//     6 | void g(void) { f(1, &a); }
//     2 | void f(int n; int (*(*b)(void))[n]) { sizeof(*(*b)(); int n, int n; int (*(*b)(void))[n]) { sizeof(*(*b)()); }
//     6 | void g(void) { f(1, &a); }
//     2 | void f(int n; int (*(*b)(void))[n]) { sizeof(*(*b)(); int n, int n; int (*(*b)(void))[n]) { sizeof(*(*b)()); }
//     6 | void g(void) { f(1, &a); }
//     2 | void f(int n; int (*(*b)(void))[n]) { sizeof(*(*b)(); int n, int n; int (*(*b)(void))[n]) { sizeof(*(*b)()); }


