/* Regression: rcc's bundled <math.h> didn't define the IEEE 754
 * comparison macros (isgreater, isless, isunordered, etc.) — C99 7.12.14.
 * glibc's <math.h> defines them as __builtin_isgreater etc. under GCC,
 * but rcc's bundled header chains via #include_next only for system libs
 * that need it, so these macros were simply absent → treated as function
 * calls → link errors in any project using them (e.g. test_file's
 * libmagic). */
#include <math.h>
#include <stdio.h>

int main(void) {
    double a = 1.0, b = 2.0, nan_val = 0.0 / 0.0;

    /* isgreater: false if either NaN, otherwise x > y */
    if (!isgreater(b, a)) return 1;   /* 2 > 1 → true */
    if (isgreater(a, b)) return 2;    /* 1 > 2 → false */
    if (isgreater(nan_val, a)) return 3; /* NaN > 1 → false */
    if (isgreater(a, nan_val)) return 4; /* 1 > NaN → false */

    /* isless: false if either NaN, otherwise x < y */
    if (!isless(a, b)) return 5;      /* 1 < 2 → true */
    if (isless(b, a)) return 6;       /* 2 < 1 → false */
    if (isless(nan_val, a)) return 7;

    /* isunordered: true iff either operand is NaN */
    if (!isunordered(nan_val, a)) return 8;
    if (!isunordered(a, nan_val)) return 9;
    if (isunordered(a, b)) return 10; /* both numeric → false */

    /* isgreaterequal, islessequal, islessgreater */
    if (!isgreaterequal(a, a)) return 11; /* 1 >= 1 → true */
    if (!islessequal(b, b)) return 12;    /* 2 <= 2 → true */
    if (islessgreater(a, a)) return 13;   /* 1 <> 1 → false */

    printf("OK\n");
    return 0;
}
