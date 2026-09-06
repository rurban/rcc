/* GCC Bug #40564 - Invalid -Wc++-compat warning about stringized C++ operator name
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=40564
 */
/* { dg-do compile } */


#define S(...) #__VA_ARGS__
// const char *s = S(The first, second, and third items.);
// qaa:~> gcc -c -Werror=c++-compat tst.c
// tst.c:2:38: error: identifier "and" is a special operator name in C++ [-Werror=c++-compat]
//     2 | const char *s = S(The first, second, and third items.);
//       |                                      ^
// cc1: some warnings being treated as errors
// In particular, this triggers an error in autoconf's _AC_C_C99_TEST_GLOBALS:
// [...]
#define showlist(...) puts (#__VA_ARGS__)
// [...]
//   showlist (The first, second, and third items.);


