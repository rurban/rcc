/* GCC Bug #118871 - internal compiler error: Segmentation fault caused by asm goto with non local label
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118871
 */
/* { dg-do compile } */


void f() {
  __label__ lab4;
// lab4:;
  void foo(void) { asm goto("" : : : : lab4); }
// foo();
}
// ```


