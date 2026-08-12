/* IEEE truthiness of floating-point values in boolean contexts.
 *
 * C11 6.8.4.1/6.5.15: a scalar condition is "true" iff it compares
 * unequal to 0. IEEE-754: -0.0 == +0.0, so `if (-0.0)` is FALSE even
 * though its bit pattern is nonzero. rcc's codegen tested flonum
 * conditions with a bitwise `cmp $0` on the GP register holding the
 * double bits, so -0.0 (0x8000000000000000) was wrongly truthy.
 *
 * Found via ksh93's arith.sh: `$(( -1.0*0))` printed "0" instead of
 * "-0" and `printf "%g" $((x))` for x=-0 lost the sign, because the
 * arithmetic result's signbit was destroyed by a spurious `%jd`
 * integer path taken when the -0.0 condition tested true.
 */
#include <stdio.h>

int main(void) {
    double d0 = -0.0;
    long double ld0 = -0.0L;
    double dpos = 0.0;
    double dval = -1.5;

    /* if() */
    if (d0) { printf("FAIL: if(dbl -0.0) truthy\n"); return 1; }
    if (ld0) { printf("FAIL: if(ldbl -0.0) truthy\n"); return 1; }
    if (!dval) { printf("FAIL: if(-1.5) falsy\n"); return 1; }
    if (dpos) { printf("FAIL: if(+0.0) truthy\n"); return 1; }

    /* && / || short-circuit values */
    if (d0 && 1) { printf("FAIL: -0.0 && 1\n"); return 1; }
    if (!(d0 || 1)) { printf("FAIL: -0.0 || 1\n"); return 1; }
    if (!(dval || 1)) { printf("FAIL: 1.5 || 1\n"); return 1; }
    if (dval && d0) { printf("FAIL: 1.5 && -0.0\n"); return 1; }

    /* ternary */
    if (d0 ? 1 : 0) { printf("FAIL: tern(-0.0)\n"); return 1; }
    if (!(dval ? 1 : 0)) { printf("FAIL: tern(-1.5)\n"); return 1; }

    /* while() and do-while() */
    int j = 0;
    double y = -0.0;
    while (y) { j++; y = 1.0; }
    if (j != 0) { printf("FAIL: while(-0.0) entered\n"); return 1; }
    int i = 0;
    double x = -1.5;
    do { i++; } while ((x = 0.0, x != 0.0));
    if (i != 1) { printf("FAIL: do-while ran %d times, want 1\n", i); return 1; }

    /* _Bool cast (C11 6.3.1.2): -0.0 -> 0, NaN -> 1, 1.5 -> 1 */
    if ((_Bool)(-0.0) != 0) { printf("FAIL: (bool)-0.0\n"); return 1; }
    if ((_Bool)(0.0 / 0.0) != 1) { printf("FAIL: (bool)NaN\n"); return 1; }
    if ((_Bool)1.5 != 1) { printf("FAIL: (bool)1.5\n"); return 1; }

    printf("OK\n");
    return 0;
}
