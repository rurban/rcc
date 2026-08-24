/* Regression test: `->` (arrow) member access on a VLA (variable-length
 * array) failed to parse -- "not a pointer to struct or union".
 *
 * `apply_postfix_ops()`'s `->` handler (parser.c) checked
 * `node->ty->kind != TY_PTR && node->ty->kind != TY_ARRAY` to reject a
 * non-pointer/non-array base, matching C's implicit array-to-pointer
 * decay for a FIXED-size array's `->`. rcc represents a variable-length
 * array with a DISTINCT `TY_VLA` kind (not `TY_ARRAY`), and that kind
 * was missing from the check -- `mb->as64` on a `BitBuf mb[cnt]` VLA
 * local was rejected outright, even though `check_type()`'s own
 * `ND_DEREF` case (which the `->` handler feeds into one line later)
 * already correctly included `TY_VLA` in its own decay check.
 *
 * Found via slimcc's (a third-party C compiler with C2Y `defer`
 * support) `bitint.c`: `BitBuf mb[cnt], sb[cnt];` (a VLA sized by a
 * runtime `cnt`) followed by `(&mb->as64)[i] = -1;` -- every VLA-typed
 * local used with `->` throughout the file failed to compile.
 */
#include <assert.h>
#include <stdint.h>

typedef union {
    uint64_t as64;
    uint32_t as32;
    uint8_t as8;
} BitBuf;

static void fill(int32_t cnt) {
    BitBuf mb[cnt];
    for (int32_t i = 0; i < cnt; i++)
        (&mb->as64)[i] = (uint64_t)(i + 1) * 0x1111111111111111ULL;
    for (int32_t i = 0; i < cnt; i++)
        assert(mb[i].as64 == (uint64_t)(i + 1) * 0x1111111111111111ULL);
}

int main(void) {
    fill(1);
    fill(3);
    fill(8);
    return 0;
}
