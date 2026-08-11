/* __sync_fetch_and_add/sub/or/xor/and/nand's value argument keeps its own
 * (possibly narrower) static type from parsing -- e.g.
 * `__sync_fetch_and_add(long_ptr, -1)` where the literal `-1` is a plain
 * `int` (4 bytes) but `*long_ptr` is `long` (8 bytes). codegen used to
 * materialize the value at its OWN width and feed that register straight
 * into the sz-wide (xadd / cmpxchg-loop) operation without extending it
 * to match: on x86-64, writing a 32-bit register implicitly zeroes the
 * upper 32 bits, so a negative int argument landed as its zero-extended
 * bit pattern (e.g. -1 -> 0x00000000FFFFFFFF) instead of being
 * sign-extended (0xFFFFFFFFFFFFFFFF) -- turning "subtract 1" into "add
 * 4294967295".
 *
 * Real-world case: nginx's ngx_rwlock_unlock() calls
 * ngx_atomic_fetch_add(lock, -1) (a macro for __sync_fetch_and_add on
 * platforms with GCC atomic builtins); the corrupted add left the rwlock
 * permanently non-zero, hanging every subsequent ngx_rwlock_wlock() spin
 * loop forever (nginx-tests upstream_resolve_reload.t timed out).
 */
#include <stdio.h>

int main(void) {
    /* (1) The exact bug shape: a narrow negative literal fetch_add'd
     * into a wide (8-byte) location starting at 1, decrementing to 0. */
    long lock = 1;
    long old1 = __sync_fetch_and_add(&lock, -1);
    if (old1 != 1) return 1;
    if (lock != 0) return 2;

    /* (2) fetch_sub with a narrow positive literal on a wide location. */
    long l2 = 10;
    long old2 = __sync_fetch_and_sub(&l2, 3);
    if (old2 != 10) return 3;
    if (l2 != 7) return 4;

    /* (3) A narrow *variable* (not just a literal) whose own type is
     * `int`, negative, fed into a wide fetch_add -- rules out any fix
     * that only special-cases constant-folded operands. */
    long l3 = 5;
    int delta = -6;
    long old3 = __sync_fetch_and_add(&l3, delta);
    if (old3 != 5) return 5;
    if (l3 != -1) return 6;

    /* (4) fetch_or/xor/and/nand must also extend the value operand --
     * a truncated -1 (all-bits-set) mask would corrupt these too. */
    long l4 = 0;
    long old4 = __sync_fetch_and_or(&l4, -1);
    if (old4 != 0) return 7;
    if (l4 != -1) return 8; /* all 64 bits set, not just the low 32 */

    long l5 = -1;
    long old5 = __sync_fetch_and_and(&l5, -2); /* clear bit 0 */
    if (old5 != -1) return 9;
    if (l5 != -2) return 10;

    /* (5) An unsigned narrow value must zero-extend, not sign-extend. */
    long l6 = 0;
    unsigned int uval = 0xFFFFFFFFU;
    long old6 = __sync_fetch_and_add(&l6, uval);
    if (old6 != 0) return 11;
    if (l6 != 0xFFFFFFFFL) return 12; /* zero-extended, not sign-extended */

    printf("OK\n");
    return 0;
}
