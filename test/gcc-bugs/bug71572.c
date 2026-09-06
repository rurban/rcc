/* GCC Bug #71572 - ICE with VLA and "+g" inline-asm in force_constant_size, at gimplify.c:671
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71572
 */
/* { dg-do compile } */


void f() {
  int a[0 / 0];
//   asm("" : "+r" (a));
}


