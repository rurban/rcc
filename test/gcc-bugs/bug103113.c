/* GCC Bug #103113 - Bad error message with multiply indirect pointer to struct
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103113
 */
/* { dg-do compile } */


int f() {
 struct { int m; } **x;
 return x->m; /* { dg-error "x.*is a pointer to pointer" } */
}
// q.c: In function ‘f’:
// q.c:3:17: error: ‘*x’ is a pointer; did you mean to use ‘->’?
//     3 |         return x->m;
//       |                 ^~
//       |                 ->


