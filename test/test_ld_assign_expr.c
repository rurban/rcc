/* Long double assignment-expression value (codegen.c ND_ASSIGN).
 *
 * `(d = f * 10.)` is an assignment expression: its value must be the
 * stored (truncated-to-double) value. rcc's TY_LDOUBLE ND_ASSIGN path
 * used to return a zeroed dummy register instead of the assigned value,
 * so `(long)(d = f * 10.)` evaluated to 0 and a loop like sfcvt's
 * `while ((long)(d = f*10.) == 0)` never terminated.
 *
 * Found via ksh93's arith.sh: `$(( atan(1.) ))` hung forever inside
 * libast's _sfcvt (sfio float-to-string) normalization loop, and every
 * math function appeared to return its argument unchanged.
 */
#include <stdio.h>

int main(void) {
    long double f = 0.7853981633974483L;
    long double d;
    long n = (long)(d = f * 10.);
    if (n != 7) {
        printf("FAIL: (long)(d = f*10.) = %ld, want 7\n", n);
        return 1;
    }
    /* Same shape on plain double (the LDOUBLE path shares the code). */
    double g = 0.5;
    double e;
    long m = (long)(e = g * 10.);
    if (m != 5) {
        printf("FAIL: (long)(e = g*10.) = %ld, want 5\n", m);
        return 1;
    }
    /* The exact sfcvt-style guard that used to spin forever. */
    long double x = 0.7853981633974483L;
    long double y;
    int iters = 0;
    while ((long)(y = x * 10.) == 0) {
        if (++iters > 16) {
            printf("FAIL: normalization loop never exits\n");
            return 1;
        }
        x = y;
    }
    printf("OK\n");
    return 0;
}
