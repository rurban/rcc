/* GCC Bug #37874 - gcc sometimes accepts attribute in identifier list
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37874
 */
/* { dg-do compile } */


void f2(y, __attribute__(()) x); /* { dg-error "expected .\\). before .__attribute__." } */
void f3(__attribute__(()) x, y);


