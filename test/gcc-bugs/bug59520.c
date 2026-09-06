/* GCC Bug #59520 - a possible inconsistency in error diagnostics with "-pedantic -std=c99"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59520
 */
/* { dg-do compile } */
/* { dg-options "-pedantic -std=c99" } */

// I wonder whether or not the following examples illustrate an unintended
// inconsistency with error diagnostics in GCC.  In particular, should the
// diagnostic message be a warning for a3 (error3.c) rather than an error?
//
// error1.c: int a[1] = {};   -> only a warning (ISO C forbids empty initializer braces)
// error2.c: int a[];         -> only a warning (array assumed to have one element)
// error3.c: int a[] = {};    -> a warning AND a hard error (zero or negative size array)

int a1[1] = {}; /* { dg-warning "empty initializer" } */
int a2[]; /* { dg-warning "assumed to have one element" } */
int a3[] = {}; /* { dg-warning "empty initializer" } */ /* { dg-error "zero or negative size array" } */

