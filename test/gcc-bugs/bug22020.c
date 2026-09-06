/* GCC Bug #22020 - poor error message for invalid cast in constant initializer
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=22020
 */
/* { dg-do compile } */


int a;
int b = (int)&a; /* { dg-error "initializer element is not constant" } */
short c = (short)&a; /* { dg-error "initializer element is not constant" } */


