// Functions the -O2 SSA IR backend compiles, covering the constructs whose
// lowering has gone wrong before: loop-carried values around nested loops
// (an outer counter the inner loop never touches), GNU `?:`, short-circuit
// values, C conversions, pointer arithmetic, and calls with stack arguments.
// Each helper is checked against a value computed by hand/gcc; any mismatch
// returns a distinct nonzero code.
#include <stddef.h>

// The outer counter `i` and the invariant `n` are never written by the inner
// loop; both must survive the inner loop's header.
static int nested(int n, int m) {
    int s = 0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            s += i * 10 + j;
    return s;
}

static int loops_flow(int n) {
    int s = 0, i = 0;
    do {
        i++;
        if (i % 3 == 0) continue;
        if (i > n) break;
        s += i;
    } while (i < 100);
    int k = 0;
    while (k < n) k += 2;
    return s * 100 + k;
}

static long ptr_arith(const int *a, int n) {
    const int *p = a, *end = a + n;
    long s = 0;
    while (p != end) s += *p++;
    return s * 1000 + (end - a);
}

static int narrow(int x) {
    signed char sc = (signed char)x;
    unsigned char uc = (unsigned char)x;
    short sh = (short)x;
    unsigned short us = (unsigned short)x;
    char c2 = (char)(sc + 1);
    return sc + uc * 2 + sh * 3 + us + c2;
}

static long fconv(double d) {
    int i = (int)d;
    long l = (long)d;
    float f = (float)d;
    double back = (double)f;
    long r = i + l * 3 + (long)(back * 4.0);
    if (d >= 0) r += (long)(unsigned)d * 2 + (long)(unsigned long)d * 5; // no out-of-range UB
    return r;
}

static int logic(int a, int b, int c) {
    int r = (a && b) + (a || c) * 2 + !b * 4 + (a ? b : c) * 8;
    int t = a ?: 7;
    return r + t * 16 + ((a > 0) && (b > 0) || c) * 32;
}

static int ucmp(unsigned u, int s) { return (u < 5u) + (s < 5) * 2 + ((long)s < (long)u) * 4 + (u == (unsigned)s) * 8; }

static int add8(int a, int b, int c, int d, int e, int f, int g, double h, int i) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + (int)(h * 8.0) + 9 * i;
}

static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

typedef int (*binop)(int, int);
static int plus(int a, int b) { return a + b; }
static int apply(binop f, int a, int b) { return f(a, b); }

struct pt { int x, y; };

static int fields(void) {
    struct pt p = {0};
    int arr[4] = {0};
    arr[2] = 5;
    p.x = arr[2] + 1;
    p.y = p.x * 2;
    const char *q = "hello";
    int h = 0;
    for (int i = 0; q[i]; i++) h = h * 31 + q[i];
    return p.x + p.y * 10 + arr[0] + arr[2] * 100 + (h & 0xff) * 1000;
}

int main(void) {
    int a[5] = {3, 1, 4, 1, 5};
    if (nested(3, 4) != 138) return 1;
    if (loops_flow(10) != 3710) return 2;
    if (ptr_arith(a, 5) != 14005) return 3;
    if (narrow(0x1ff) != 2553) return 4;
    if (narrow(-129) != 65273) return 5;
    if (fconv(-3.75) != -27L) return 6;
    if (fconv(1234.5) != 18512L) return 7;
    if (logic(0, 5, 2) != 162) return 8;
    if (logic(3, 0, 0) != 54) return 9;
    if (ucmp(3, -1) != 7) return 10;
    if (add8(1, 2, 3, 4, 5, 6, 7, 2.5, 9) != 241) return 11;
    if (fib(15) != 610) return 12;
    if (apply(plus, 20, 22) != 42) return 13;
    if (fields() != 210626) return 14;
    return 0;
}
