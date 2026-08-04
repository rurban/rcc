/* GCC Bug #87569 - defining type in ‘sizeof’ expression is invalid in C++ references wrong operator
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87569
 */


int i = sizeof (__typeof (enum { e0 }));   // type defined in typeof

__typeof (sizeof (enum { e1 })) e;         // type defined in sizeof

// x.c:1:32: warning: defining type in ‘sizeof’ expression is invalid in C++ [-Wc++-compat]
// 1 | int i = sizeof (__typeof (enum { e0 }));   // type defined in typeof
//   |                                ^
// x.c:3:24: warning: defining type in ‘sizeof’ expression is invalid in C++ [-Wc++-compat]
// 3 | __typeof (sizeof (enum { e1 })) e;         // type defined in sizeof
//   |                        ^


