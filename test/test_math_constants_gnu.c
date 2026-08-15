/* rcc's bundled <math.h> was missing two of glibc's standard POSIX/XSI
 * math constants: M_SQRT1_2 (1/sqrt(2)) and M_2_SQRTPI (2/sqrt(pi)) --
 * every other M_* constant (M_PI, M_E, M_LN2, ..., M_2_PI) was present,
 * these two alone were silently absent, so any real-world source
 * referencing them (e.g. njs's Math object property table) hit
 * "undeclared variable" instead of a working double constant.
 */
#include <math.h>
#include <stdio.h>

int main(void) {
    if (M_SQRT1_2 < 0.7071 || M_SQRT1_2 > 0.7072) {
        printf("FAIL: M_SQRT1_2 wrong: %f\n", M_SQRT1_2);
        return 1;
    }
    if (M_2_SQRTPI < 1.1283 || M_2_SQRTPI > 1.1284) {
        printf("FAIL: M_2_SQRTPI wrong: %f\n", M_2_SQRTPI);
        return 2;
    }
    printf("OK\n");
    return 0;
}
