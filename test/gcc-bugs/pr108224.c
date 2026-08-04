/* GCC Bug #108224 - Suggest stdlib.h header for rand
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108224
 */
/* { dg-do compile } */

/* Comment 1's testcase: rand() without <stdlib.h> should suggest the
 * header in the -Wimplicit-function-declaration diagnostic. */
int f(void)
{
  return rand(); /* { dg-error "implicit declaration of function .rand." } */
}