/* GCC Bug #67488 - Improve diagnostic on call of declared function in a different scope
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67488
 */
/* { dg-do compile } */


void g() {
  void f(void);
}
void k() {int f(int); f(1);} /* { dg-error "conflicting types for" } */
// Saying f was declared in a different scope too.


