/* GCC Bug #125169 - gcc misparses arguments to expf function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125169
 */


#include <math.h>
#define PI2            6.28318530717958647692528676654f
float pwr[2];
float f() {
float cPwr = 0.0266058277;
pwr[1] = 0.00304872775;
float s = sqrtf(1/(cPwr * cPwr * PI2));
float s2 = 1/(cPwr * cPwr * PI2);
float res = pwr[1]  - expf(-(1 * 1)/(2*s2)) / (s*sqrtf(PI2));
return res;
}

int main()
{
    __builtin_printf("%f\n", f());
}
// ```
// And it prints out -0.023498 on all versions I tried.


