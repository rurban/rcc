/* GCC Bug #104764 - gcc hangs when compiling an invalid c program
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104764
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu89" } */

/* Reporter's mutant.c: error recovery in sizeof with an unbracketed
 * VLA dimension caused a hang inside c_expr_sizeof_expr.  Modern gcc
 * errors out cleanly. */
static a();
b(void) {sizeof ( int [ a} /* { dg-error "expected .\\]. before .}." } */
static c();
d(void) {sizeof((int[c /* { dg-error "expected .\\]. at end of input" } */