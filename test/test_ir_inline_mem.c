// Small callees the -O2 IR inliner splices into callers, and the memory
// hazards the IR load/store optimizer must respect (aliasing stores, narrow
// store + wide load, calls between a store and a load, loop-carried memory).
// Each check returns a distinct nonzero code on mismatch.
#include <stddef.h>

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static long sum_to(int n) { // callee with a loop and a phi
    long s = 0;
    for (int i = 1; i <= n; i++) s += i;
    return s;
}

static int sign3(int x) { // callee with several returns (phi merge of results)
    if (x < 0) return -1;
    if (x == 0) return 0;
    return 1;
}

static void bump(int *p) { *p += 5; } // void callee with a store

static int chain(int x) { return clamp(x * 3, 0, 100) + sign3(x); } // inlined body keeps its own calls

__attribute__((noinline)) static int slow_id(int x) { return x; }

static int use_inlines(int n) {
    int acc = 0;
    for (int i = -3; i < n; i++) {
        acc += clamp(i, 0, 5) * 2 + sign3(i);
        acc += (int)sum_to(i > 0 ? i : 0);
    }
    int v = 1;
    bump(&v);
    bump(&v);
    return acc * 100 + v + chain(7) + slow_id(3);
}

int g;
static int g_other;

static int store_forward(int x) { // same-width store -> load must forward the stored value
    g = x;
    int a = g;
    g = x + 1;
    return a * 10 + g;
}

static int alias_kill(int *p) { // a store through an unknown pointer must reload the global
    g = 7;
    int a = g;
    *p = 9; // may alias g
    int b = g;
    return a * 100 + b;
}

static int no_alias(int *p) { // distinct objects: values stay available
    g = 1;
    g_other = 2;
    int a = g;
    *p = 4;
    return a + g_other * 10;
}

static int narrow_wide(void) { // byte store then int load must observe the new byte
    union { int i; unsigned char c[4]; } u;
    u.i = 0x01020304;
    u.c[1] = 0xff;
    return u.i;
}

static int call_between(int x) { // a call may modify the global between store and load
    extern void clobber_g(void);
    g = x;
    clobber_g();
    return g;
}

void clobber_g(void) { g = 1234; }

static int loop_carried(int n) { // store in the body feeds the next iteration's load
    int arr[1] = {0};
    int *p = arr;
    for (int i = 0; i < n; i++) *p = *p + i;
    return *p;
}

static int dead_store(int x) { // the first store is overwritten before any read
    g = x;
    g = x * 2;
    return g;
}

int main(void) {
    int sink = 0;
    if (use_inlines(6) != 6736) return 1;
    if (store_forward(5) != 56) return 2;
    if (alias_kill(&g) != 709) return 3;
    if (alias_kill(&sink) != 707) return 4;
    if (no_alias(&sink) != 21) return 5;
    if (narrow_wide() != 0x0102ff04) return 6;
    if (call_between(5) != 1234) return 7;
    if (loop_carried(10) != 45) return 8;
    if (dead_store(21) != 42) return 9;
    return 0;
}
