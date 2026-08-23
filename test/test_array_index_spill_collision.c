/* Regression test for a register-allocator bug found via nettle's twofish
 * cipher (test/third_party/test_nettle): h_byte() in twofish.c indexes a
 * 2D array-of-pointers 5 levels deep, `tbl[i][k][x]`, where `x` at each
 * level is itself `l ^ tbl[i][k-1][...]` -- register pressure forces
 * alloc_reg() to spill a still-live outer address (e.g. `tbl[i][1]`'s
 * loaded pointer) to make room for a short-lived row/column-offset
 * scratch temp.
 *
 * Two distinct bugs combined to corrupt the outer address:
 *
 * 1. free_reg() unconditionally cleared used_regs after restoring a
 *    spilled register, even when the restore just returned a still-live
 *    OUTER value (displaced by alloc_reg()'s own victim selection) to its
 *    physical register. The very next alloc_reg() call then found that
 *    register "fully free" and handed it to an unrelated value with NO
 *    spill emitted, silently clobbering the just-restored outer address
 *    before its real use. Fixed by tracking, per register, whether its
 *    CURRENT occupant was placed there by alloc_reg()'s spill-victim path
 *    (spill_victim bit); free_reg() now protects (restores but leaves
 *    used_regs set) only in that genuine case, while a stale spilled_regs
 *    bit left dangling by gen_funcall()'s argument-staging release (which
 *    bypasses free_reg() on purpose) still degrades to the old harmless
 *    behavior.
 *
 * 2. gen()'s ND_DEREF fast path for `*(lvar + idx)` and the generic
 *    binary-op dispatch's r_lhs==r_rhs collision handling both called
 *    free_reg() on a VReg that ALIASED the physical register holding the
 *    live combined/returned result, freeing (and thus permitting reuse
 *    of) a register whose value was about to be read or returned.
 *
 * 3. gen()'s ND_COND (ternary) called free_reg(cond) AFTER emitting the
 *    conditional branch to the else-label. If cond's register had been
 *    borrowed from a spilled outer value, the restore instruction only
 *    existed in the (conditionally skipped) "then" branch's code, so the
 *    "else" path resumed with a stale/wrong register value. Fixed by
 *    moving the free to right after the flags-setting compare, before
 *    any branch is emitted, so it is unconditionally reached by both
 *    sides.
 *
 * All three combined to segfault (or silently return the wrong byte)
 * inside a 5-level nested array index chain under real register
 * pressure. This test reproduces the shape directly (independent of the
 * nettle checkout) with a deterministic expected result.
 */
#include <assert.h>
#include <stdint.h>

static const uint8_t q0[256] = {0};
static const uint8_t q1[256] = {0};

/* Mirrors twofish's q_table/h_byte: a 2x5 array of byte-lookup-table
 * pointers, indexed 5 levels deep with each level's index XORed against
 * the next table lookup -- forces alloc_reg() into real spill pressure
 * evaluating the nested row/column address arithmetic. */
static const uint8_t *const tbl[2][5] = {
    {q0, q1, q0, q1, q0},
    {q1, q0, q1, q0, q1},
};

static uint8_t h_byte(int i, uint8_t x, uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3) {
    return tbl[i][4][l0 ^
           tbl[i][3][l1 ^
           tbl[i][2][l2 ^
           tbl[i][1][l3 ^ tbl[i][0][x]]]]];
}

/* Same shape but with a ternary condition (itself a function call) deep
 * in the index chain -- ND_COND's cond register can ALSO be borrowed
 * from an outer spilled address; this exercises fix 3 together with
 * fixes 1-2. */
static int is_two(int k) { return k == 2; }

static uint8_t h_byte_ternary(int i, int k, uint8_t x, uint8_t l1, uint8_t l2, uint8_t l3) {
    return tbl[i][3][l1 ^
           tbl[i][2][is_two(k) ? x : l2 ^
           tbl[i][1][l3 ^ tbl[i][0][x]]]];
}

int main(void) {
    /* Every table entry is zero-initialized, so every lookup yields 0
     * regardless of index -- the only thing under test is whether the
     * index-chain ADDRESS ARITHMETIC survives intact (a corrupted outer
     * address dereferences into unmapped memory and segfaults; a merely
     * clobbered index still reads *some* byte of q0/q1, always 0 here).
     */
    assert(h_byte(0, 5, 1, 2, 3, 4) == 0);
    assert(h_byte(1, 0, 4, 3, 2, 1) == 0);
    assert(h_byte_ternary(0, 0, 5, 2, 3, 4) == 0);
    assert(h_byte_ternary(1, 2, 5, 2, 3, 4) == 0);
    return 0;
}
