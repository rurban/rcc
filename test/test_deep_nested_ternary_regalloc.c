/* A deeply nested ternary expression (4+ levels), where each branch is
 * itself another ternary and a non-trivial sub-expression (`p ^ q`) is
 * repeatedly re-embedded at every level, silently computed the WRONG
 * result -- found via tcpdump's `print-802_11.c` radiotap decoder,
 * which uses exactly this shape (a classic bit-scan idiom: BITNO_32
 * calls BITNO_16 calls BITNO_8 calls BITNO_4 calls BITNO_2, each a
 * `((x) >> N) ? K + inner((x) >> N) : inner(x)`) to find the lowest
 * set bit of a 32-bit "present flags" word. rcc's own preprocessor
 * expansion of that macro chain was byte-for-byte identical to gcc's
 * (confirmed via `-E` diff) -- the bug was purely in codegen.
 *
 * Root cause (ND_COND / ternary codegen, codegen.c): the ternary's
 * RESULT register was allocated via `alloc_reg()` up front, before
 * evaluating the condition or either branch, and held reserved
 * through the ENTIRE recursive evaluation of both. For a plain
 * (non-nested) ternary this is harmless, but for a NESTED one -- each
 * branch itself another ND_COND -- every nesting level's own result
 * register stacked up simultaneously even though none of them held a
 * real value until the very end, exhausting the register pool (12 GP
 * registers on x86-64) several levels sooner than an equivalent
 * unnested computation of the same total complexity would, forcing
 * spills under artificial pressure. The register allocator's
 * spill/reload bookkeeping under that specific pressure pattern then
 * silently corrupted one live value (confirmed via disassembly: a
 * spilled register was reloaded from its stack slot and then
 * immediately overwritten by an unrelated store before ever being
 * read back). Fixed by deferring the result register's allocation
 * until each branch's own value is actually ready to be moved into
 * it, cutting one register's worth of pressure per nesting level.
 *
 * Reproduces reliably at exactly 4 levels of ternary nesting with a
 * multi-token (non-bare-variable) shared sub-expression; 3 levels or
 * a bare-variable argument were not enough to trigger it. */
#include <assert.h>
#include <stdint.h>

#define BITNO_32(x) (((x) >> 16) ? 16 + BITNO_16((x) >> 16) : BITNO_16((x)))
#define BITNO_16(x) (((x) >> 8) ? 8 + BITNO_8((x) >> 8) : BITNO_8((x)))
#define BITNO_8(x) (((x) >> 4) ? 4 + BITNO_4((x) >> 4) : BITNO_4((x)))
#define BITNO_4(x) (((x) >> 2) ? 2 + BITNO_2((x) >> 2) : BITNO_2((x)))
#define BITNO_2(x) (((x) & 2) ? 1 : 0)

/* Lowest-set-bit index, computed a boring/obviously-correct way, as
 * the reference oracle for every present-flags value tested below. */
static int ref_bitno(uint32_t v) {
    int n = 0;
    while (!(v & 1)) {
        v >>= 1;
        n++;
    }
    return n;
}

int main(void) {
    for (uint32_t bit = 0; bit < 32; bit++) {
        uint32_t present = 1u << bit;
        uint32_t next_present = present & (present - 1); /* always 0 here */
        int bitno = BITNO_32(present ^ next_present);
        assert(bitno == ref_bitno(present));
    }

    /* The exact tcpdump-derived shape: a value assigned from the
     * macro-expanded expression, not the raw literal itself. */
    uint32_t present = 0x4000, next_present = 0;
    int bitno = BITNO_32(present ^ next_present);
    assert(bitno == 14);

    return 0;
}
