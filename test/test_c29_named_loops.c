/* C29 (WG14 N3355): named/labeled loops -- an ordinary label placed
 * directly before a loop or switch lets `break label;`/`continue label;`
 * target that specific enclosing statement instead of only the
 * innermost one, skipping past any intervening nested loops/switches.
 */
#include "test_common.h"

/* break out of the OUTER loop from inside a doubly-nested inner loop. */
static int test_labeled_break_for(void)
{
    int hits = 0;
    outer: for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            hits++;
            if (i == 2 && j == 2)
                break outer;
        }
    }
    /* i=0,1,2 each contribute j=0..4 (5 hits) except i==2 which stops at
     * j==2 (3 hits: j=0,1,2) -> 5 + 5 + 3 = 13 */
    return hits;
}

/* continue the OUTER loop from inside a nested loop, skipping the rest
 * of both the inner loop and the outer loop's own remaining body. */
static int test_labeled_continue_for(void)
{
    int outer_iters = 0, inner_hits = 0;
    top: for (int i = 0; i < 4; i++) {
        outer_iters++;
        for (int j = 0; j < 4; j++) {
            if (j == 1)
                continue top;
            inner_hits++;
        }
        inner_hits += 1000; /* must never run: continue top skips this */
    }
    return outer_iters * 100 + inner_hits;
}

/* labeled break out of a switch nested inside a loop, and out of a loop
 * nested inside a switch (labels work on both statement kinds). */
static int test_labeled_break_switch(void)
{
    int total = 0;
    lp: for (int i = 0; i < 3; i++) {
        sw: switch (i) {
        case 1:
            total += 10;
            break sw; /* ordinary innermost break would do the same here */
            /* not reached */
        case 2:
            total += 100;
            break lp; /* exits the FOR, not just the switch */
        default:
            total += 1;
        }
        total += 1000; /* must not run for i==2 (break lp skips it) */
    }
    return total;
}

/* while and do-while loops can be labeled too. */
static int test_labeled_while(void)
{
    int n = 0;
    w: while (1) {
        for (int j = 0; j < 10; j++) {
            n++;
            if (n == 7)
                break w;
        }
    }
    return n;
}

static int test_labeled_do_while(void)
{
    int n = 0, passes = 0, skipped = 0;
    d: do {
        passes++;
        for (int j = 0; j < 3; j++) {
            if (passes == 2 && j == 1) {
                skipped++;
                continue d; /* skips the "n += 1" below for this pass only */
            }
        }
        n += 1;
    } while (passes < 3);
    return passes * 100 + n * 10 + skipped;
}

int main(void)
{
    if (test_labeled_break_for() != 13) {
        printf("FAIL: test_labeled_break_for() = %d, want 13\n", test_labeled_break_for());
        return 1;
    }
    int r = test_labeled_continue_for();
    /* 4 outer iterations, each inner loop stops at j==1 (1 hit: j==0) */
    if (r != 404) {
        printf("FAIL: test_labeled_continue_for() = %d, want 404\n", r);
        return 2;
    }
    /* i=0: default (+1), then total+=1000 (+1000) = 1001
     * i=1: case 1 (+10), break sw, then total+=1000 (+1000) = 2011
     * i=2: case 2 (+100), break lp -- skips the trailing total+=1000
     * and the rest of the loop entirely: final = 2111 */
    if (test_labeled_break_switch() != 2111) {
        printf("FAIL: test_labeled_break_switch() = %d, want 2111\n",
               test_labeled_break_switch());
        return 3;
    }
    if (test_labeled_while() != 7) {
        printf("FAIL: test_labeled_while() = %d, want 7\n", test_labeled_while());
        return 4;
    }
    int dw = test_labeled_do_while();
    if (dw != 321) {
        printf("FAIL: test_labeled_do_while() = %d, want 321\n", dw);
        return 5;
    }

    return 0;
}
