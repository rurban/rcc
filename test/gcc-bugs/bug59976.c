/* GCC Bug #59976 - Spurious warning on converting const int variable to unsigned long (Also inconsistency between O0 and O1)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59976
 */
/* { dg-do compile } */


unsigned g;
void fn1() {
  int a;
  const unsigned char b = 0;
  a = b & g;
}


