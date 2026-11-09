/*
 * Conditional branches, forward and backward, over more code than an arm64
 * B.cond can reach.
 *
 * B.cond holds a 19-bit word offset, so it reaches +-1MB. rcc patched larger
 * displacements into the field anyway, and the branch wrapped around into
 * unrelated code. Large functions now branch around an unconditional B, and
 * this one compiles to about 1.4MB.
 */
#include <stdio.h>

#define X2(a) a a
#define X10(a) a a a a a a a a a a
#define X20K(a) X2(X10(X10(X10(X10(a)))))

volatile int sink;

static int branches(int c, int n) {
    int loops = 0;
    do {
        if (c) { // forward over the block when c is 0
            X20K(sink += c;)
        }
        loops++;
    } while (--n); // backward over the block when n is still non-zero
    return loops;
}

int main(void) {
    if (branches(0, 1) != 1 || branches(1, 1) != 1) {
        printf("FAIL: long forward branch\n");
        return 1;
    }
    if (branches(0, 2) != 2 || branches(1, 2) != 2) {
        printf("FAIL: long backward branch\n");
        return 1;
    }
    return 0;
}
