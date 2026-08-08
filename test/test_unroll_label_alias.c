/* Regression test for -funroll (enabled at -O2/-O3): a for-loop whose body
 * defines a label must give each unrolled copy its OWN label, not let every
 * copy share one symbol.
 *
 * Root cause (opt.c's try_unroll()): unrolling clones the loop body N times
 * via clone_expr(), a shallow per-field copy that leaves an ND_LABEL's
 * label_name identical across every copy. codegen.c resolves both `goto`
 * and `&&label` by formatting a single symbolic name,
 * ".L.label.<enclosing-fn>.<label_name>" — since every unrolled copy lives
 * in the *same* enclosing function, every copy's label collided on that one
 * symbol, so a later copy's `goto` bound to an *earlier* copy's already-
 * emitted address instead of its own not-yet-emitted one.
 *
 * Found via httpparser's test_scan(): `for (type_both=0; type_both<2;
 * type_both++) { ...; if (parser.upgrade) goto test; ...; test: ...; }` got
 * unrolled to 2 copies sharing one ".L.label.test_scan.test" symbol. Copy
 * 1's `goto test` bound to copy 0's `test:`, resuming execution mid copy 0
 * with copy 1's live state — which then fed back into copy 0's own loop-
 * continue path, corrupting the enclosing scan's own induction variables
 * and turning a bounded double loop into one that ran roughly 25x too many
 * iterations before the 60s test-harness timeout caught it
 * (test/third_party/test_httpparser hung at -O2/-O3, passed at -O0/-O1
 * where -funroll never fires).
 *
 * `reentries` is a bounded escape hatch (its `goto done` targets a label
 * *outside* the loop, so unrolling never duplicates or renames it) that
 * turns the real bug's runaway loop into a fast, deterministic failure
 * instead of a hang: with the bug, copy 2's `goto mark` binds to an earlier
 * copy's `mark:`/`after:` and the run bounces between copies without ever
 * reaching copy 2's own `mark:` (`last_k` stays 1, `reentries` hits the
 * cap); fixed, copy 2's own `goto mark` lands correctly and the loop runs
 * exactly 3 times.
 */
#include <assert.h>

static int reentries = 0;
static int last_k = -1;

static void run(void) {
    /* try_unroll()'s loop_iteration_count() only recognizes a plain
     * `i = START` assignment as the init clause, not a C99 declaration —
     * `k` must be declared before the loop (matching test_scan()'s own
     * `int i, j, type_both; for (type_both = 0; ...)` shape) for -funroll
     * to actually unroll this loop. */
    int k;
    for (k = 0; k < 3; k++) {
        if (++reentries > 20) goto done; // escape hatch: label lives outside the loop
        if (k == 2) goto mark;
        last_k = k;
        goto after;
    mark:
        last_k = 100 + k;
    after:;
    }
done:;
}

int main(void) {
    run();
    /* Must terminate via 3 normal iterations, never via the escape hatch. */
    assert(reentries == 3);
    /* The k==2 copy's own `goto mark` must land in its own body. */
    assert(last_k == 102);
    return 0;
}
