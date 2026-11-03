// Values whose only use is the `return` right after their definition: the IR
// backend produces them directly in rax. Each producer kind is covered,
// including remainder (finishes in rdx), calls, compares, shifts, loads,
// extensions and multi-return shapes with different producers per path.
#include <stddef.h>

__attribute__((noinline)) int ext(int x) { return x * 3 + 1; }
__attribute__((noinline)) long extl(long x) { return x - 7; }

int g_arr[8] = {10, 20, 30, 40, 50, 60, 70, 80};
long g_l = 123456789012L;
short g_s = -1234;
unsigned char g_c = 200;

int r_add(int a, int b) { return a + b; }
int r_sub(int a, int b) { return a - b; }
int r_mul(int a, int b) { return a * b; }
int r_and(int a, int b) { return a & b; }
int r_or(int a, int b) { return a | b; }
int r_xor(int a, int b) { return a ^ b; }
int r_shl(int a, int b) { return a << (b & 15); }
int r_sar(int a, int b) { return a >> (b & 15); }
unsigned r_shr(unsigned a, int b) { return a >> (b & 15); }
int r_neg(int a) { return -a; }
int r_not(int a) { return ~a; }
int r_div(int a, int b) { return a / b; }
int r_rem(int a, int b) { return a % b; }
unsigned r_udiv(unsigned a, unsigned b) { return a / b; }
unsigned r_urem(unsigned a, unsigned b) { return a % b; }
long r_lrem(long a, long b) { return a % b; }
int r_cmp(int a, int b) { return a < b; }
int r_load(int i) { return g_arr[i & 7]; }
long r_loadl(void) { return g_l; }
int r_sext16(void) { return g_s; }
int r_zext8(void) { return g_c; }
int r_trunc(long x) { return (int)x; }
int r_sext32(int x) { return (long)x > 5 ? 1 : (int)(long)x; }
long r_widen(int x) { return (long)x; }
int r_call(int x) { return ext(x); }
long r_calll(long x) { return extl(x); }
int r_call_args(int a, int b, int c) { return ext(a + b + c); }

int r_multi(int k, int a, int b) { // different producer per return path
    if (k == 0) return a + b;
    if (k == 1) return a % (b | 1);
    if (k == 2) return ext(a);
    if (k == 3) return g_arr[b & 7];
    if (k == 4) return a < b;
    return -a;
}

int r_after_loop(int n) { // producer is a phi-free arithmetic op after a loop
    int s = 0;
    for (int i = 0; i < n; i++) s += i * i;
    return s ^ n;
}

int r_pressure(int a, int b, int c, int d, int e, int f, int g, int h) { // many live values
    int x = a + b, y = c + d, z = e + f, w = g + h;
    return (x * y) % (z + w + 1);
}

int main(void) {
    if (r_add(3, 4) != 7) return 1;
    if (r_sub(3, 10) != -7) return 2;
    if (r_mul(-6, 7) != -42) return 3;
    if (r_and(0xf0f0, 0xff00) != 0xf000) return 4;
    if (r_or(0xf0, 0x0f) != 0xff) return 5;
    if (r_xor(0xff, 0x0f) != 0xf0) return 6;
    if (r_shl(3, 4) != 48) return 7;
    if (r_sar(-64, 3) != -8) return 8;
    if (r_shr(0x80000000u, 4) != 0x08000000u) return 9;
    if (r_neg(5) != -5) return 10;
    if (r_not(5) != -6) return 11;
    if (r_div(-100, 7) != -14) return 12;
    if (r_rem(-100, 7) != -2) return 13;
    if (r_udiv(4000000000u, 7) != 571428571u) return 14;
    if (r_urem(4000000000u, 7) != 3u) return 15;
    if (r_lrem(-123456789012L, 1000003L) != -123456789012L % 1000003L) return 16;
    if (r_cmp(1, 2) != 1 || r_cmp(2, 1) != 0) return 17;
    if (r_load(2) != 30 || r_load(11) != 40) return 18;
    if (r_loadl() != 123456789012L) return 19;
    if (r_sext16() != -1234) return 20;
    if (r_zext8() != 200) return 21;
    if (r_trunc(0x1ffffffffL) != -1) return 22;
    if (r_sext32(-9) != -9 || r_sext32(9) != 1) return 23;
    if (r_widen(-5) != -5L) return 24;
    if (r_call(4) != 13) return 25;
    if (r_calll(100) != 93) return 26;
    if (r_call_args(1, 2, 3) != 19) return 27;
    if (r_multi(0, 5, 6) != 11) return 28;
    if (r_multi(1, 100, 6) != 100 % 7) return 29;
    if (r_multi(2, 5, 0) != 16) return 30;
    if (r_multi(3, 0, 5) != 60) return 31;
    if (r_multi(4, 1, 2) != 1) return 32;
    if (r_multi(5, 9, 0) != -9) return 33;
    if (r_after_loop(10) != (285 ^ 10)) return 34;
    if (r_pressure(1, 2, 3, 4, 5, 6, 7, 8) != (3 * 7) % (11 + 15 + 1)) return 35;
    return 0;
}
