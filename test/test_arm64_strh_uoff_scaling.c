/* ARM64: three shared byte-offset helper wrappers (asm_ldr_reg_off,
 * asm_str_reg_off, asm_str_reg_off_phy in src/codegen_asm.h) dispatch by
 * transfer size to arm64_{ldrb,ldrh,ldr}_uoff / arm64_{strb,strh,str}_uoff.
 * ARM64's LDR/STR "unsigned offset" immediate is SCALED by the transfer
 * size (encoded as byte_offset / size), so each wrapper must divide its
 * caller-supplied BYTE offset by the transfer size before handing it to
 * the low-level *_uoff encoder. The size=1 (byte) and size=4/8 (word/
 * doubleword) cases did this correctly (size=1 needs no division; size=4
 * and size=8 divided by 4 and 8 respectively) -- but the size=2
 * (halfword) case passed the raw byte offset straight through to
 * arm64_strh_uoff/arm64_ldrh_uoff without dividing by 2, doubling every
 * nonzero halfword-store/load offset.
 *
 * Found via a real CPython build (see test_struct_return_exact_width.c
 * for the underlying x86-64 store-width bug this shares a fix commit
 * with) cross-verified on ARM64 via the GCC torture-style c-testsuite:
 * `struct s11 { char x[11]; } fr_s11(void)` returns an 11-byte struct in
 * X0:X1 (SysV/AAPCS64 <=16-byte all-integer aggregate). The call site's
 * new exact-width store split X1's 3 remaining bytes into a 2-byte
 * store at byte offset 8 followed by a 1-byte store at offset 10 --
 * the halfword store landed at offset 16 instead of 8, corrupting
 * unrelated adjacent stack memory (here, a completely different local
 * variable 8 bytes further out) instead of writing t11 itself.
 *
 * This test exercises the wrappers directly through a case guaranteed
 * to need a nonzero halfword-offset store: a 6-byte struct return
 * (4-byte + 2-byte split, matching struct_returns_in_gp_regs' single-
 * register <=8-byte path) and an 11-byte struct return (8-byte +
 * 2-byte + 1-byte split across X0/X1, matching the two-register path).
 */
#if defined(__aarch64__)
#include <stdio.h>
#include <string.h>

struct s6 { char x[6]; };
struct s11 { char x[11]; };

static struct s6 g6 = { "abcdef" };
static struct s11 g11 = { "0123456789A" };

static struct s6 fr_s6(void) { return g6; }
static struct s11 fr_s11(void) { return g11; }

int main(void) {
    /* Guard local with a known pattern on both sides of each returned
     * struct's stack slot -- an offset-scaling bug writes past the
     * slot's own bytes into neighboring memory, corrupting one of these. */
    struct { unsigned char guard_lo[8]; struct s6 v; unsigned char guard_hi[8]; } b6;
    struct { unsigned char guard_lo[8]; struct s11 v; unsigned char guard_hi[8]; } b11;
    memset(&b6, 0xAA, sizeof(b6));
    memset(&b11, 0xAA, sizeof(b11));

    b6.v = fr_s6();
    b11.v = fr_s11();

    int fail = 0;
    if (memcmp(b6.v.x, "abcdef", 6) != 0) {
        printf("FAIL: 6-byte struct return value wrong: %.6s\n", b6.v.x);
        fail = 1;
    }
    for (int i = 0; i < 8; i++) {
        if (b6.guard_lo[i] != 0xAA || b6.guard_hi[i] != 0xAA) {
            printf("FAIL: 6-byte struct return corrupted guard bytes\n");
            fail = 1;
            break;
        }
    }
    if (memcmp(b11.v.x, "0123456789A", 11) != 0) {
        printf("FAIL: 11-byte struct return value wrong: %.11s\n", b11.v.x);
        fail = 1;
    }
    for (int i = 0; i < 8; i++) {
        if (b11.guard_lo[i] != 0xAA || b11.guard_hi[i] != 0xAA) {
            printf("FAIL: 11-byte struct return corrupted guard bytes\n");
            fail = 1;
            break;
        }
    }

    if (fail) return 1;
    printf("OK ARM64 halfword-offset stores land at the correct byte offset\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
