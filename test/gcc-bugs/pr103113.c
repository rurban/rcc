/* GCC Bug #103113 - Bad error message with multiply indirect pointer to struct
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103113
 */


int f() {
 struct { int m; } **x;
 return x->m;
}
// q.c: In function ‘f’:
// q.c:3:17: error: ‘*x’ is a pointer; did you mean to use ‘->’?
//     3 |         return x->m;
//       |                 ^~
//       |                 ->


