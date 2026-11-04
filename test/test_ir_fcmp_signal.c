/* x86-64: float ordering comparisons (<, <=, >, >=) compiled by the -O2 IR
 * backend must raise the SSE invalid-operation exception (MXCSR IE, bit 0) when
 * an operand is a quiet NaN, like gcc (comiss/comisd); == and != must not
 * (ucomiss/ucomisd). test_float_nan_sig_compare covers the AST codegen; the IR
 * emitter had its own copy of the compare and used the non-signaling
 * instruction for every predicate. The operands come from noinline functions'
 * parameters and loads so nothing is folded at compile time, in registers and
 * as a memory operand folded into the compare, float and double.
 */
#include <math.h>
#include <stdio.h>

#if !defined(__x86_64__) && !defined(_M_X64)
int main(void) {
    printf("skipped\n");
    return 0;
}
#else

#define NOINL __attribute__((noinline))

static unsigned int rd_mxcsr(void) {
    unsigned int sw;
    __asm__ __volatile__("stmxcsr %0" : "=m"(sw));
    return sw;
}
static void clr_mxcsr(void) {
    unsigned int sw = rd_mxcsr();
    sw &= ~0x3FU;
    __asm__ __volatile__("ldmxcsr %0" : : "m"(sw));
}

NOINL static int lt_f(float a, float b) { return a < b; }
NOINL static int le_f(float a, float b) { return a <= b; }
NOINL static int gt_f(float a, float b) { return a > b; }
NOINL static int ge_f(float a, float b) { return a >= b; }
NOINL static int eq_f(float a, float b) { return a == b; }
NOINL static int ne_f(float a, float b) { return a != b; }
NOINL static int lt_d(double a, double b) { return a < b; }
NOINL static int le_d(double a, double b) { return a <= b; }
NOINL static int gt_d(double a, double b) { return a > b; }
NOINL static int ge_d(double a, double b) { return a >= b; }
NOINL static int eq_d(double a, double b) { return a == b; }
NOINL static int ne_d(double a, double b) { return a != b; }
// the second operand is a load the backend folds into the compare
NOINL static int lt_m(double a, double *p) { return a < *p; }
NOINL static int ge_m(double a, double *p) { return a >= *p; }
NOINL static int gt_mf(float a, float *p) { return a > *p; }
NOINL static int eq_m(double a, double *p) { return a == *p; }
// a branch on the compare (fused compare + jump)
NOINL static int br_lt(double a, double b) {
    if (a < b) return 1;
    return 2;
}
NOINL static int br_ge(float a, float b) {
    if (a >= b) return 1;
    return 2;
}

static int fail;
#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); \
            fail++; \
        } \
    } while (0)
#define RAISES(call, want_ret) \
    do { \
        clr_mxcsr(); \
        CHECK((call) == (want_ret)); \
        CHECK((rd_mxcsr() & 0x1U) != 0); \
    } while (0)
#define QUIET(call, want_ret) \
    do { \
        clr_mxcsr(); \
        CHECK((call) == (want_ret)); \
        CHECK((rd_mxcsr() & 0x1U) == 0); \
    } while (0)

int main(int argc, char **argv) {
    (void)argv;
    float fn = argc > 100 ? 1.0f : NAN, fz = (float)(argc - 1);
    double dn = argc > 100 ? 1.0 : NAN, dz = (double)(argc - 1);

    RAISES(lt_f(fn, fz), 0);
    RAISES(le_f(fn, fz), 0);
    RAISES(gt_f(fn, fz), 0);
    RAISES(ge_f(fn, fz), 0);
    RAISES(lt_f(fz, fn), 0);
    RAISES(ge_f(fz, fn), 0);
    RAISES(lt_d(dn, dz), 0);
    RAISES(le_d(dn, dz), 0);
    RAISES(gt_d(dn, dz), 0);
    RAISES(ge_d(dn, dz), 0);
    RAISES(lt_d(dz, dn), 0);
    RAISES(le_d(dz, dn), 0);
    RAISES(lt_m(dn, &dz), 0);
    RAISES(ge_m(dn, &dz), 0);
    RAISES(lt_m(dz, &dn), 0);
    RAISES(gt_mf(fn, &fz), 0);
    /* fused compare + branch. gcc -O2 turns these two into a non-signaling setcc, so the
     * exception state is only asserted for rcc */
#ifdef __RCC__
    RAISES(br_lt(dn, dz), 2);
    RAISES(br_ge(fn, fz), 2);
#else
    CHECK(br_lt(dn, dz) == 2);
    CHECK(br_ge(fn, fz) == 2);
#endif

    QUIET(eq_f(fn, fz), 0);
    QUIET(ne_f(fn, fz), 1);
    QUIET(eq_d(dn, dz), 0);
    QUIET(ne_d(dn, dz), 1);
    QUIET(eq_m(dn, &dz), 0);

    /* ordinary operands: same answers, no exception */
    QUIET(lt_f(fz, fz + 1.0f), 1);
    QUIET(ge_d(dz + 1.0, dz), 1);
    QUIET(lt_m(dz, &dz), 0);
    QUIET(br_lt(dz, dz + 1.0), 1);

    if (fail) return 1;
    printf("OK\n");
    return 0;
}
#endif
