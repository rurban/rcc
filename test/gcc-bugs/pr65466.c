/* GCC Bug #65466 - Unnecessary source line output for "note: each undeclared identifier is reported only once"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65466
 */
/* { dg-do compile } */


#define FOO(x) x
int foo(void) {return FOO(bar);}
// foo.c: In function 'foo':
// foo.c:2:27: error: 'bar' undeclared (first use in this function)
// int foo(void) {return FOO(bar);}
//                            ^
// foo.c:1:16: note: in definition of macro 'FOO'
// #define FOO(x) x
//                 ^
// foo.c:2:27: note: each undeclared identifier is reported only once for each function it appears in
// int foo(void) {return FOO(bar);}
//                            ^
// foo.c:1:16: note: in definition of macro 'FOO'
// #define FOO(x) x
//                 ^


