// Unit test for -finline (enabled at -O2/-O3). Every function below is a
// candidate the inliner may replace at its call sites; inlining must never
// change the observed result. The asserts hold at any optimization level,
// so the test is meaningful whether or not inlining actually fires.
#include <assert.h>

static int add(int a, int b) { return a + b; }
static inline int sq(int x) { return x * x; }
int maxi(int a, int b) { return a > b ? a : b; }
static int getx(int *p) { return *p; }
static int usemul(int a) { return a + a + a; } // param used 3x
static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int deref2(int *p) { return *p + *p; } // read-through-pointer twice
static unsigned char trunc8(int x) { return (unsigned char)x; } // return-type cast
static double fscale(double x, double k) { return x * k + 1.0; }

// Directly recursive: must NOT be inlined, but must still compute correctly.
static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

// Writes its by-value parameter: must NOT be unsoundly inlined (the caller's
// argument must be unaffected).
static int inc(int x) { return ++x; }

int main(void) {
    int y = 3, z = 4, v = 5;
    int arr[3] = {10, 20, 30};

    assert(add(y, z) == 7);
    assert(add(y, z) * 2 == 14);
    assert(sq(6) == 36);
    assert(maxi(y, z) == 4);
    assert(getx(&v) == 5);
    assert(usemul(y) == 9);
    assert(clampi(y, 0, 5) == 3);
    assert(clampi(9, 0, 5) == 5);
    assert(deref2(&arr[2]) == 60);
    assert(trunc8(300) == 44); // 300 & 0xff
    assert(fscale(2.0, 3.0) == 7.0);
    assert(fib(10) == 55);

    // inc() reads a copy; y must remain 3 after the call.
    assert(inc(y) == 4);
    assert(y == 3);

    return 0;
}
