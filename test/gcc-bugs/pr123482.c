/* GCC Bug #123482 - in -frounding-math, GCC moves floating-point operation across fesetround call
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123482
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fenv.h>

#ifdef __TRUSTINSOFT_ANALYZER__
#include <tis_builtin.h>
// display values even if they are not singletons:
#define printf tis_show_each
#endif
#pragma STDC FENV_ACCESS ON

int main(void)
{
//     /* IEEE-754 rounding modes required by the C standard */
    const int rounding_modes[] = {
//         FE_TONEAREST,
//         FE_DOWNWARD,
//         FE_UPWARD,
//         FE_TOWARDZERO
    };

    const char *rounding_names[] = {
//         "FE_TONEAREST",
//         "FE_DOWNWARD",
//         "FE_UPWARD",
//         "FE_TOWARDZERO"
    };

    const size_t num_modes = sizeof(rounding_modes) / sizeof(int);

//     /* Pick a random rounding mode */
//    srand((unsigned)time(NULL));
   size_t idx = (size_t)(rand() % num_modes);
    size_t idx = 2;
    int mode = rounding_modes[idx];

//     /* Set rounding mode */
//     fesetround(mode);

//     /* Operands */
    double a = 0.1;
    double b = 0.3;

//     /* Multiplication performed under selected rounding mode */
    double result = a * b;

//     /* Set mode back to NE for printing */
//     fesetround(FE_TONEAREST);

//     printf("Rounding mode: %s\n", rounding_names[idx]);
//     printf("a = %.17g\n", a);
//     printf("b = %.17g\n", b);
//     printf("a * b = %.17g\n", result);
}
// CE link: <a href="https://godbolt.org/z/TKse9MKs3">https://godbolt.org/z/TKse9MKs3</a>
// GCC 15.2 prints:
// Rounding mode: FE_UPWARD
// a = 0.10000000000000001
// b = 0.29999999999999999
// a * b = 0.029999999999999999
// I expected:
// Rounding mode: FE_UPWARD
// a = 0.10000000000000001
// b = 0.29999999999999999
// a * b = 0.030000000000000002
// Looking at the assembly code, the multiplication has been re-ordered after the second call to fesetround. Unfortunately, in the context of this program, the second call is necessary, as the rounding mode influences the binary-to-decimal conversion behavior of printf:
main:
//         subq    $8, %rsp
//         movl    $2048, %edi
        call    fesetround    ;;;;;;;;;;;;;; HERE
//         xorl    %edi, %edi
        call    fesetround    ;;;;;;;;;;;;;; HERE
//         movl    $.LC0, %esi
//         movl    $.LC1, %edi
//         xorl    %eax, %eax
//         call    printf
//         movl    $.LC3, %edi
//         movl    $1, %eax
//         movsd   .LC2(%rip), %xmm0
//         call    printf
//         movl    $.LC5, %edi
//         movl    $1, %eax
//         movsd   .LC4(%rip), %xmm0
//         call    printf
//         movl    $.LC6, %edi
//         movl    $1, %eax
//         movsd   .LC4(%rip), %xmm0
        mulsd   .LC2(%rip), %xmm0    ;;;;;;;;;;;;;; HERE
//         call    printf
//         xorl    %eax, %eax
//         addq    $8, %rsp
//         ret


