/* GCC Bug #67569 - wrong type in error message with float on x86 (387)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67569
 */
/* { dg-do compile } */

int main(void)
{
   int   *A = (int *)533 ;
   float  F = 1.F        ;
   A - F                 ;   /* { dg-error "invalid operands to binary" } */
   return 0             ;
}
