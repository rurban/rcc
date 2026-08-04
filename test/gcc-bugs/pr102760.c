/* GCC Bug #102760 - ICE: in decompose, at wide-int.h:984
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102760
 */
/* { dg-do compile } */


int li_4, li_5, us_8;
unsigned char func_7_ptr_13, func_7_uc_14;
void func_7_ptr_18() {
  if (li_5) {
    for (;;)
      ;
    short s_15;
    for (; func_7_uc_14;) {
      us_8 = 7;
      for (; us_8; us_8 += 1)
        if (us_8)
          li_4 = 1;
      func_7_uc_14 += func_7_ptr_18;
      if (func_7_ptr_13 & 1 && (func_7_uc_14 &= func_7_ptr_13))
        s_15 %= func_7_uc_10li_19(s_15);
    }
  }
  goto lblD2AF1FAB;
}
//    title="RESOLVED FIXED - Segmentation fault while compiling (RH 7.0/Kernel 2.2/PII)"
//    title="RESOLVED FIXED - Segmentation fault while compiling (RH 7.0/Kernel 2.2/PII)"
//    title="RESOLVED FIXED - Segmentation fault while compiling (RH 7.0/Kernel 2.2/PII)"
//    href="show_bug.cgi?id=876">bug876</a>.c:14:20: warning: assignment to ‘unsigned char’ from ‘void (*)()’ makes integer from pointer without a cast [-Wint-conversion]
//    14 |       func_7_uc_14 += func_7_ptr_18;
//    title="RESOLVED FIXED - Segmentation fault while compiling (RH 7.0/Kernel 2.2/PII)"
//    16 |         s_15 %= func_7_uc_10li_19(s_15);
//    title="RESOLVED FIXED - Segmentation fault while compiling (RH 7.0/Kernel 2.2/PII)"
//     3 | void func_7_ptr_18() {


