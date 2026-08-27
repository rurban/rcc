/* x86-64: float ordering comparisons (<, <=, >, >=) must raise the SSE
 * invalid-operation exception (MXCSR IE, bit 0) when either operand is
 * NaN, and must yield false for every NaN comparison.
 *
 * rcc emitted the unordered `ucomisd` for < / <= too. `ucomisd` never
 * raises, so IEEE-754 signaling comparisons -- code reading
 * fetestexcept(FE_INVALID) after `a < b`, and emulators whose soft-float
 * layers rely on the host raising the invalid flag for RISC-V FLT/FLE
 * (e.g. rvvm's FPU, which defines FPU_LIB_CORRECT_SIGNALING_COMPARE on
 * x86-64) -- silently missed the NV flag. gcc uses the ordered `comisd`
 * for these, which sets MXCSR bit 0 on NaN operands. The fixed codegen
 * emits `comisd` for ND_LT/ND_LE (and the swapped > / >= forms) while
 * keeping `ucomisd` for == / != (which must not raise).
 *
 * Found via rvvm: `rv32uf-p-fcmp` failed test #11 `flt.s NaN, 0`
 * expecting the NV (invalid) FCSR flag, because rcc's `NaN < 0.0f`
 * never set the host's IE flag.
 */
#include <stdio.h>
#include <math.h>

/* MXCSR is x86-only; skip on ARM64/WASM/other targets. */
#if !defined(__x86_64__) && !defined(_M_X64) && !defined(_M_IX86) && !defined(__i386__)
int main(void) { printf("skipped\n"); return 0; }
#else

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

/* Read MXCSR. Bit 0 = IE (invalid), bit 7 = IE mask. */
static unsigned int rd_mxcsr(void) {
    unsigned int sw;
    __asm__ __volatile__("stmxcsr %0" : "=m"(sw));
    return sw;
}
static void clr_mxcsr_flags(void) {
    unsigned int sw = rd_mxcsr();
    /* clear all six status flags (bits 0-5), keep masks and control bits */
    sw &= ~0x3FU;
    __asm__ __volatile__("ldmxcsr %0" : : "m"(sw));
}

int main(void) {
    volatile float nan = NAN, zero = 0.0f, one = 1.0f, two = 2.0f;

    /* Ordering compares against NaN must be false. */
    CHECK(!(nan < zero));
    CHECK(!(nan <= zero));
    CHECK(!(nan > zero));
    CHECK(!(nan >= zero));
    CHECK(!(zero < nan));
    CHECK(!(nan < nan));
    CHECK(!(nan <= nan));
    /* == / != must also be false/true respectively against NaN. */
    CHECK(!(nan == zero));
    CHECK(nan != zero);

    /* Ordinary ordering compares still work. */
    CHECK(one < two);
    CHECK(!(two < one));
    CHECK(one <= one);
    CHECK(two > one);
    CHECK(one >= one);

    /* The signaling compare must raise MXCSR IE (bit 0) on NaN operands. */
    clr_mxcsr_flags();
    CHECK(!(nan < zero));
    CHECK((rd_mxcsr() & 0x1U) != 0); /* IE set after NaN < */
    clr_mxcsr_flags();
    CHECK(!(nan <= zero));
    CHECK((rd_mxcsr() & 0x1U) != 0); /* IE set after NaN <= */
    clr_mxcsr_flags();
    CHECK(!(nan > zero));
    CHECK((rd_mxcsr() & 0x1U) != 0); /* IE set after NaN > (swapped <) */
    clr_mxcsr_flags();
    CHECK(!(nan >= zero));
    CHECK((rd_mxcsr() & 0x1U) != 0); /* IE set after NaN >= (swapped <=) */

    /* == / != must NOT raise. */
    clr_mxcsr_flags();
    CHECK(!(nan == zero));
    CHECK((rd_mxcsr() & 0x1U) == 0); /* no IE after NaN == */
    clr_mxcsr_flags();
    CHECK(nan != zero);
    CHECK((rd_mxcsr() & 0x1U) == 0); /* no IE after NaN != */

    /* Ordered compares must not raise either. */
    clr_mxcsr_flags();
    CHECK(one < two);
    CHECK((rd_mxcsr() & 0x1U) == 0); /* no IE after 1 < 2 */

    if (fail) {
        fprintf(stderr, "%d FAILURES\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
#endif /* x86 */
