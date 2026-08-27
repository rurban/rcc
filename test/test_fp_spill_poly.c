/* x86-64 FP binary-op register-spill corruption.
 *
 * Computing a Horner-chain polynomial like
 *     p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))))
 * builds a deeply nested FP expression.  Under register pressure,
 * gen(rhs) for an inner `z * C` step could reuse the VReg that gen(lhs)
 * had allocated for `z` (alloc_reg() spilled the lhs and returned the
 * same index).  The x86-64 FP binary-op path then loaded the SAME
 * register into both xmm0 and xmm1, so `z * C` degenerated into `C * C`
 * and the polynomial evaluated with the wrong coefficient.
 *
 * Found via jerryscript's unit-test-math: `acos(0.5)` returned
 * 0x3ff0c2a6874d5540 instead of 0x3ff0c152382d7366.  The ARM64 path
 * already handled this (preserving rhs in x16); the x86 path did not.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define pS0 1.66666666666666657415e-01
#define pS1 -3.25565818622400915405e-01
#define pS2 2.01212532134862925881e-01
#define pS3 -4.00555345006794114027e-02
#define pS4 7.91534994289814532176e-04
#define pS5 3.47933107596021167570e-05
#define qS1 -2.40339491173441421878e+00
#define qS2 2.02094576023350569471e+00
#define qS3 -6.88283971605453293030e-01
#define qS4 7.70381505559019352791e-02

/* The acos() x>0.5 polynomial tail for z=0.25: returns 1 + 2*(p/q)*0.5,
 * which for the acos(0.5) case must be pi/3 = 0x3ff0c152382d7366. */
static double poly(double z) {
    double p, q, r, w;
    p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
    q = 1 + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
    r = p / q;
    w = r * 0.5;
    return 1 + 2 * w;
}

int main(void) {
    double v = poly(0.25);
    uint64_t bits;
    memcpy(&bits, &v, 8);
    if (bits != 0x3ff0c152382d7366ULL) {
        printf("FAIL: poly(0.25) = %016llx, expected 3ff0c152382d7366\n",
               (unsigned long long)bits);
        return 1;
    }
    printf("OK: poly(0.25) = %016llx\n", (unsigned long long)bits);
    return 0;
}