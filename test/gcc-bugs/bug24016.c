/* GCC Bug #24016 - Missing warning for unspecified evaluation order
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=24016
 */
/* { dg-do compile } */


#include <string.h>
int foo(void *x) {
    return strcmp(x + 1, "test");
}

// does not cause warnings when compiled with -Wpointer-arith -O1 (glibc v. 2.17). It can be reduced to:
//
// int foo(void *x) {
//     return __extension__({ __builtin_strcmp(x + 1, "test"); });
// }
// Note, that we do warn about
//
// int foo(void *x) {
//     return ({ __builtin_strcmp(x + 1, "test"); });
// }


