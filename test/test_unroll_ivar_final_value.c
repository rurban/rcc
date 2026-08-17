// Unit test for -funroll (enabled at -O2/-O3): the induction variable's
// real final value after unrolling.
//
// try_unroll() clones the loop body N times, substituting every READ of
// the induction variable inside each clone with its compile-time
// constant value for that iteration -- correct for the clones
// themselves, since the loop's own `inc` clause is gone (unrolling
// replaces it). But nothing else ever wrote the variable's real runtime
// storage past its `init` value, so any code AFTER the loop that still
// reads the (function-scope, block-scope-escaping) induction variable
// saw whatever `init` left it at, not the value a real (non-unrolled)
// loop's exit would leave it at (init + count).
//
// Found via gzip 1.14's trees.c ct_init(): a `for (code = 0; code < 16;
// code++) { ... }` loop is immediately followed by a second loop reusing
// the same variable with an empty init clause, `for (; code < D_CODES;
// code++) { ... }`, relying on `code` continuing from 16. Under -O2 the
// first loop unrolled and left `code` at 0 instead of 16, so the second
// loop's `1 << (extra_dbits[code] - 7)` computed a negative shift count
// (masked to 25 on x86 SHL), looping 2^25 times and writing dist_code[]
// far past its 512-byte bound -- segfaulted `gzip -c` on every input.
#include <assert.h>

// Same shape as ct_init: induction variable declared once, used by two
// consecutive loops, the second continuing from the first's exit value
// via an empty init clause.
static int two_loops_reusing_ivar(void) {
    int i;
    int sum = 0;
    for (i = 0; i < 8; i++)
        sum += 1; // trivial body; count alone should still trigger unrolling
    // i must be 8 here, matching a real loop's post-exit value.
    for (; i < 12; i++)
        sum += 10;
    return i * 1000 + sum; // encodes both i's final value and the sum
}

// The induction variable read after the loop without any second loop at
// all -- the simplest case where its final value must be materialized.
static int ivar_read_after_loop(void) {
    int i;
    for (i = 0; i < 6; i++)
        ; // empty body
    return i;
}

// Non-zero start value: final value must be start + count, not just count.
static int ivar_nonzero_start(void) {
    int i;
    for (i = 5; i < 9; i++)
        ;
    return i;
}

// <= condition: final value is end + 1, not end.
static int ivar_le_condition(void) {
    int i;
    for (i = 0; i <= 3; i++)
        ;
    return i;
}

int main(void) {
    // First loop: i ends at 8 (sum += 1, 8 times = 8).
    // Second loop: i goes 8..11 (4 iterations, sum += 10 each = 40).
    // Final i = 12, sum = 8 + 40 = 48.
    assert(two_loops_reusing_ivar() == 12 * 1000 + 48);

    assert(ivar_read_after_loop() == 6);
    assert(ivar_nonzero_start() == 9);
    assert(ivar_le_condition() == 4);

    return 0;
}
