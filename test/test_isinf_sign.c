/* __builtin_isinf must be sign-preserving like glibc's isinf macro.
 *
 * glibc's `isinf(x)` returns +1 for +INFINITY, -1 for -INFINITY, and 0
 * for finite values (not just a boolean 0/1).  rcc's __builtin_isinf
 * only returned 0/1, so `isinf(-INFINITY)` was 1 instead of -1.
 *
 * Found via jerryscript's unit-test-math (`isinf (-INFINITY)` expected
 * -1).  The fix preserves the sign bit of the original value and applies
 * it to the 0/1 inf flag.
 */
#include <math.h>
#include <stdio.h>

int main(void) {
    if (isinf(INFINITY) != 1) {
        printf("FAIL: isinf(+INFINITY) = %d, expected 1\n", isinf(INFINITY));
        return 1;
    }
    if (isinf(-INFINITY) != -1) {
        printf("FAIL: isinf(-INFINITY) = %d, expected -1\n", isinf(-INFINITY));
        return 1;
    }
    if (isinf(0.0) != 0) {
        printf("FAIL: isinf(0.0) = %d, expected 0\n", isinf(0.0));
        return 1;
    }
    if (isinf(3.14) != 0) {
        printf("FAIL: isinf(3.14) = %d, expected 0\n", isinf(3.14));
        return 1;
    }
    printf("OK: isinf sign-preserving\n");
    return 0;
}