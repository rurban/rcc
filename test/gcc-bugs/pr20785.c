/* GCC Bug #20785 - Pragma STDC * (C99 FP) unimplemented
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=20785
 */


// /* t0.c */
#include <stdio.h>
#include <fenv.h>
#include <float.h>

#pragma STDC FENV_ACCESS ON

int main( void )
{
    volatile double d1 = DBL_MAX;
    volatile double d2 = DBL_MAX;

#ifdef __STDC__
//     printf("__STDC__         %d\n", __STDC__);
#endif
#ifdef __STDC_VERSION__
//     printf("__STDC_VERSION__ %ld\n", __STDC_VERSION__);
#endif
#ifdef __STDC_IEC_559__
//     printf("__STDC_IEC_559__ %d\n", __STDC_IEC_559__);
#endif

    if (feclearexcept( FE_ALL_EXCEPT ) != 0)
    {
//         printf("error: feclearexcept( FE_ALL_EXCEPT ) failed\n");
        return 1;
    }
//     ( void )( d1 * d2 );
    if (fetestexcept( FE_OVERFLOW ) == 0)
    {
//         printf("error: no FE_OVERFLOW is raised\n");
        return 2;
    }
    return 0;  ​
}
// Invocation: gcc t0.c -std=c11 -pedantic -lm
// <nothing>
// Execution: ./a.out
// Expected output:
// __STDC__         1
// __STDC_VERSION__ 201710
// __STDC_IEC_559__ 1
// Actual output:
// __STDC__         1
// __STDC_VERSION__ 201710
// __STDC_IEC_559__ 1
// error: no FE_OVERFLOW is raised


