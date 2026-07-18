// Unit test for -funroll (enabled at -O2/-O3). Every for-loop below has a
// constant iteration count and should be a candidate for unrolling. Unrolling
// must never change the observed result. The asserts hold at any optimization
// level, so the test is meaningful whether or not unrolling actually fires.
#include <assert.h>

// Basic const-sized loops
static int sum_to(int n) {
    int s = 0;
    for (int i = 0; i < 5; i++)
        s += i;
    return s;
}

static int prod_range(void) {
    int p = 1;
    for (int i = 1; i <= 4; i++)
        p *= i;
    return p;
}

// Loop body is a compound statement
static int compound_body(void) {
    int x = 0;
    for (int i = 0; i < 3; i++) {
        x += i;
        x += 1;
    }
    return x;
}

// Loop with a larger but still bounded count
static int larger_count(void) {
    int s = 0;
    for (int i = 0; i < 10; i++)
        s += i;
    return s;
}

// Loop with constant start > 0
static int nonzero_start(void) {
    int s = 0;
    for (int i = 3; i < 7; i++)
        s += i;
    return s;
}

// Loop using <= with exactly one iteration
static int single_iter_le(void) {
    int r = 0;
    for (int i = 0; i <= 0; i++)
        r = 42;
    return r;
}

// Loop using < with exactly one iteration
static int single_iter_lt(void) {
    int r = 0;
    for (int i = 7; i < 8; i++)
        r = 99;
    return r;
}

int main(void) {
    // sum 0..4 = 10
    assert(sum_to(0) == 10);

    // product 1*2*3*4 = 24
    assert(prod_range() == 24);

    // (0+1)+(1+1)+(2+1) = 1+2+3 = 6
    assert(compound_body() == 6);

    // sum 0..9 = 45
    assert(larger_count() == 45);

    // sum 3+4+5+6 = 18
    assert(nonzero_start() == 18);

    // <= 0: single iteration, r = 42
    assert(single_iter_le() == 42);

    // < 8 starting at 7: single iteration, r = 99
    assert(single_iter_lt() == 99);

    return 0;
}
