/* GCC Bug #57205 - for unfinished function declaration, recover by skipping until matched parenthesis and report non-matched parenthesis
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57205
 */
/* { dg-do compile } */


int foo(int *file,
#include "something.h"
// );
// can be correct depending on the contents of something.h.
// Clang shows:
// /usr/include/assert.h:71:1: error: invalid storage class specifier in function declarator
extern void __assert_fail (__const char *__assertion, __const char *__file,
// ^
// /usr/include/assert.h:73:44: error: expected ')'
     __THROW __attribute__ ((__noreturn__));
//                                            ^
// /home/manuel/test.c:1:8: note: to match this '('
int foo(int *file,
//        ^
// which is very similar to GCC.
// GCC could recover better by skipping everything up to the first non-matched parenthesis, and then report the location of the non-matched parenthesis when not found, like Clang does. 
// I am not sure what heuristics Clang uses to decide when to skip, but being in a different file or finding something like "extern" is definitely a good moment to think that something went terribly wrong and skip the whole function.


