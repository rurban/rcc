/* GCC Bug #91199 - In -fexcess-precision=standard mode, the warning “floating constant truncated to zero” is misleading
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91199
 */


#include <stdio.h>
#include <float.h>

#define MY_HUGE_VALF 0x1.0p255f

float f;

int main(void) {
  f = 0x1.0p-255f * MY_HUGE_VALF;
//   printf("%d, %f\n", (int)FLT_EVAL_METHOD, f);
}
// The first warning is wrong: it implies that 0x1.0p-255f will be interpreted as 0 by the compiler, while it (correctly) isn't.


