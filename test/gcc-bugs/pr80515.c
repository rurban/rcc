/* GCC Bug #80515 - __attribute__ ((__noreturn__)) false alarm for 'main'
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80515
 */


int main() {
   for (;;)
     ;
   return 0;
 }
// But Ok, clang and icc accept the noreturn attribute for main already.
// So lets reopen as an enhancement.


