/* ARM64's SUBS-based `cmp` immediate only encodes an unsigned 12-bit
 * value (0-4095), optionally left-shifted by 12 (a multiple of 4096 up
 * to 0xffffff). asm_cmp_imm() (codegen_asm.h) didn't check this and
 * always encoded with shift 0, so a case value that's an exact nonzero
 * multiple of 4096 silently truncated to `cmp w, #0` -- matching (and
 * mis-dispatching) input 0 instead of the real case value. Predates the
 * sparse-switch lowering pass entirely: reproduces on the plain linear
 * chain at -O0, with as few as two cases (nowhere near the dense jump
 * table's or binary search's/perfect hash's eligibility thresholds).
 */
#include <stdlib.h>

static int f(int x) {
    switch (x) {
    case 4096: return 1;
    case 1: return 2;
    default: return -1;
    }
}

static int g(int x) {
    /* Also exercises the case-range lower/upper bound compares, which
     * share the same immediate-encoding path. */
    switch (x) {
    case 4096 ... 4100: return 1;
    case 8192: return 2;
    default: return -1;
    }
}

int main(void) {
    if (f(4096) != 1) abort();
    if (f(1) != 2) abort();
    if (f(0) != -1) abort();
    if (f(4097) != -1) abort();
    if (g(4096) != 1) abort();
    if (g(4098) != 1) abort();
    if (g(4100) != 1) abort();
    if (g(8192) != 2) abort();
    if (g(0) != -1) abort();
    if (g(4101) != -1) abort();
    if (g(4095) != -1) abort();
    return 0;
}
