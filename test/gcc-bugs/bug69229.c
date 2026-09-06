/* GCC Bug #69229 - missing type information in diagnostics about type mismatch in conditional expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69229
 */
/* { dg-do compile } */


void* foo (int *p, float *q) {
    return 1 ? p : q;  /* { dg-error "pointer type mismatch in conditional expression" } */
}

void* bar (int *p, int i) {
    return 1 ? p : i;  /* { dg-error "pointer.integer type mismatch in conditional expression" } */
}

struct S { int i; };
void* baz (int i, struct S s) {
    return 1 ? i : s;  /* { dg-error "type mismatch in conditional expression" } */
}
// a.c: In function ‘foo’:
// a.c:2:18: warning: pointer type mismatch in conditional expression
//     return 1 ? p : q;
//                   ^
// a.c: In function ‘bar’:
// a.c:6:18: warning: pointer/integer type mismatch in conditional expression
//     return 1 ? p : i;
//                   ^
// a.c: In function ‘baz’:
// a.c:11:18: error: type mismatch in conditional expression
//     return 1 ? i : s;
//                   ^
// For example, g++ issues the following:
// a.c: In function ‘void* foo(int*, float*)’:
// a.c:2:20: error: conditional expression between distinct pointer types ‘int*’ and ‘float*’ lacks a cast [-fpermissive]
//     return 1 ? p : q;
//                     ^
// a.c: In function ‘void* bar(int*, int)’:
// a.c:6:14: error: operands to ?: have different types ‘int*’ and ‘int’
//     return 1 ? p : i;
//             ~~^~~~~~~
// a.c: In function ‘void* baz(int, S)’:
// a.c:11:14: error: operands to ?: have different types ‘int’ and ‘S’
//     return 1 ? i : s;
//             ~~^~~~~~~
// And Clang (in C mode):

// a.c:2:14: warning: pointer type mismatch ('int *' and 'float *')
//       [-Wpointer-type-mismatch]
//    return 1 ? p : q;
//              ^ ~   ~
// a.c:6:14: warning: pointer/integer type mismatch in conditional expression
//       ('int *' and 'int') [-Wconditional-type-mismatch]
//    return 1 ? p : i;
//              ^ ~   ~
// a.c:11:14: error: incompatible operand types ('int' and 'struct S')
//    return 1 ? i : s;
//              ^ ~   ~
// 2 warnings and 1 error generated.
//
// Note: with modern GCC (16.x) the foo/bar pointer-type-mismatch
// diagnostics have themselves been promoted from warnings to hard
// errors ([-Wincompatible-pointer-types] / [-Wint-conversion]), so
// all three functions above now fail to compile; only baz's message
// is unaffected by that unrelated change.


