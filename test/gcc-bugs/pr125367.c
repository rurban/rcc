/* GCC Bug #125367 - [OpenMP] Missing diagnostic for list item in both map and private clauses
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125367
 */
/* { dg-do compile } */


void f() {
int x, y;
#pragma omp target private(y) map(tofrom:x,y) 
  ;
#pragma omp target firstprivate(x) map(tofrom:x,y) 
  ;
}
// This gives the two expected errors for both PRIVATE and FIRSTPRIVATE:
// error: 'y' appears both in data and map clauses
//     3 | #pragma omp target private(y) map(tofrom:x,y)
//       |                           ^
// error: 'x' appears both in data and map clauses
//     5 | #pragma omp target firstprivate(x) map(tofrom:x,y)
//       |                                ^
// However, if we reverse the order of clauses:
void f() {
int x, y;
#pragma omp target map(tofrom:x,y) private(y)
  ;
#pragma omp target map(tofrom:x,y) firstprivate(x)
  ;
} 
// Then we only get the error for FIRSTPRIVATE:
// error: ‘x’ appears both in data and map clauses
//     5 | #pragma omp target map(tofrom:x,y) firstprivate(x) 
// The error for PRIVATE is missing.


