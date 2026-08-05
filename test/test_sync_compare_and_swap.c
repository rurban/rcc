/* __sync_val_compare_and_swap / __sync_bool_compare_and_swap:
 *
 * 1. Their `oldval` argument is passed BY VALUE per GCC semantics (it
 *    may be any expression -- a literal, a function call result, ...),
 *    unlike __atomic_compare_exchange's `expected`, which is already a
 *    pointer. The parser used to take oldval's ADDRESS directly, which
 *    crashed codegen ("Invalid register -1") whenever oldval was a
 *    non-addressable rvalue -- e.g. json-c's linkhash.c:
 *    `__sync_val_compare_and_swap(&random_seed, -1, seed)`.
 *
 * 2. __sync_val_compare_and_swap must return the ORIGINAL *ptr value
 *    (whether or not the swap happened), not a success/fail boolean
 *    -- that's __sync_bool_compare_and_swap's contract. The codegen
 *    used to return the sete-based bool for both builtins.
 */
#include <stdio.h>

int main(void) {
    /* (1) oldval as a plain literal -- must not crash, and the swap
     * must actually happen when the literal matches *ptr. */
    int seed = -1;
    int prev = __sync_val_compare_and_swap(&seed, -1, 42);
    if (prev != -1) return 1;
    if (seed != 42) return 2;

    /* (2) __sync_val_compare_and_swap returns the OLD value, always --
     * on both the success and the failure path. */
    int x = 100;
    int old_ok = __sync_val_compare_and_swap(&x, 100, 200);
    if (old_ok != 100) return 3;
    if (x != 200) return 4;

    int y = 100;
    int old_fail = __sync_val_compare_and_swap(&y, 999, 200);
    if (old_fail != 100) return 5; /* NOT 0/1 -- the actual current value */
    if (y != 100) return 6; /* no swap: mismatch */

    /* __sync_bool_compare_and_swap: still returns a plain 0/1, and
     * still tolerates a literal oldval (same address-of-rvalue bug). */
    int z = -1;
    int ok1 = __sync_bool_compare_and_swap(&z, -1, 5);
    if (ok1 != 1) return 7;
    if (z != 5) return 8;

    int w = 100;
    int ok2 = __sync_bool_compare_and_swap(&w, 999, 200);
    if (ok2 != 0) return 9;
    if (w != 100) return 10;

    printf("OK\n");
    return 0;
}
