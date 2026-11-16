/*
 * -fsanitize=undefined: a shift by a runtime count keeps later stack slots
 * of its function inside the frame.
 *
 * The shift check stashes its value and count in two struct-ret-area slots.
 * The check is generated in Pass 2 only, so Pass 1, which sizes the frame,
 * has to reserve those slots as well. Without that, every later slot of the
 * function sits 16 bytes per shift further down than the frame allows: here
 * the first mk() result lands below the stack pointer, and the second mk()
 * call overwrites it with its scratch array before sum2() reads it.
 *
 * No shift is out of range, so UBSan reports nothing; only the sum can go
 * wrong. The accumulator is unsigned so the adds need no overflow check.
 */
#include <stdio.h>

struct big {
    long v[6];
};

__attribute__((noinline)) struct big mk(long x) {
    struct big b;
    volatile long scratch[256];
    for (int i = 0; i < 256; i++) scratch[i] = -1;
    for (int i = 0; i < 6; i++) b.v[i] = x * 10 + i;
    return b;
}

__attribute__((noinline)) long sum2(struct big a, struct big b) {
    long s = 0;
    for (int i = 0; i < 6; i++) s += a.v[i] + b.v[i];
    return s;
}

#define SH r += (unsigned long)(1L << s);
#define SH10 SH SH SH SH SH SH SH SH SH SH

__attribute__((noinline)) long shifts_then_calls(int s) {
    unsigned long r = 0;
    SH10 SH10 SH10 SH10
    return (long)r + sum2(mk(1), mk(2));
}

int main(void) {
    long got = shifts_then_calls(3);
    long want = 40 * 8 + (10 + 11 + 12 + 13 + 14 + 15) + (20 + 21 + 22 + 23 + 24 + 25);
    if (got != want) {
        printf("got %ld, want %ld\n", got, want);
        return 1;
    }
    printf("OK\n");
    return 0;
}
