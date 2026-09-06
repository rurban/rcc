/* GCC Bug #56397 - Floating Point constant diagnotic in C and x87 and -fexcess-precision=standard
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56397
 */


#include <math.h>
#include <float.h>
#include <stdio.h>
int main(void) {
    double x = DBL_MIN / 1024.0;
    long double z = LDBL_MIN / 1024.0;
//     printf("x == %a\n\nClass of x == %X\n\nClass of z == %X\n", 
//            x, fpclassify(x), fpclassify(z));
}
// ------------------------------------------------------------------------------
// (As always, I am running on Windows 7, MinGW, GCC 4.7.2, 
// command line option -std=c99, and I have FLT_EVAL_METHOD == 2).
// My output is:
// ------------------------
// x == 0x8p-1035            /* This is a 'double subnormal' value */
// Class of x == 400         /* x is 'double' and normal, so this seems wrong */
// Class of z == 4400        /* z is 'long double' and 'subnormal': OK */
// ------------------------


