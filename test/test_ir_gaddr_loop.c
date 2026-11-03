// Global addresses used inside loops: the IR backend hoists them out of the
// loop and keeps them in registers (or rematerializes them when the register
// allocator runs out). Covers plain/static globals, extern and weak symbols
// (GOT-loaded addresses), many live globals (register pressure), calls inside
// loops, nested loops and address identity.
#include <stddef.h>

static char sieve_buf[1001];
static int sa[64], sb[64], sc[64], sd[64], se[64], sf[64], sg[64], sh[64];
int ga[64], gb[64];
long gsum;
extern int ext_arr[16];
int ext_arr[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
#ifdef _WIN32
int weak_arr[16]; // PE has no weak data symbols
#else
__attribute__((weak)) int weak_arr[16];
#endif

__attribute__((noinline)) int bump(int x) { return x + 1; }

int sieve(int limit) {
    int count = 0;
    for (int i = 0; i <= limit; i++) sieve_buf[i] = 1;
    sieve_buf[0] = sieve_buf[1] = 0;
    for (int i = 2; i * i <= limit; i++)
        if (sieve_buf[i])
            for (int j = i * i; j <= limit; j += i) sieve_buf[j] = 0;
    for (int i = 2; i <= limit; i++)
        if (sieve_buf[i]) count++;
    return count;
}

long many_globals(void) { // eight static arrays live across the loop: pressure
    for (int i = 0; i < 64; i++) {
        sa[i] = i;
        sb[i] = i * 2;
        sc[i] = i * 3;
        sd[i] = i * 4;
        se[i] = i * 5;
        sf[i] = i * 6;
        sg[i] = i * 7;
        sh[i] = i * 8;
    }
    long s = 0;
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 64; i++) s += sa[i] + sb[i] + sc[i] + sd[i] + se[i] + sf[i] + sg[i] + sh[i] + k;
    return s;
}

long with_calls(void) { // a call inside the loop: the address must survive it
    long s = 0;
    for (int i = 0; i < 64; i++) {
        ga[i] = bump(i);
        gb[i] = ga[i] * 2;
        s += gb[i];
    }
    return s;
}

long got_arrays(void) { // extern and weak symbols: addresses come from the GOT
    long s = 0;
    for (int i = 0; i < 16; i++) {
        weak_arr[i] = ext_arr[i] * 3;
        s += weak_arr[i] + ext_arr[i];
    }
    return s;
}

int addr_identity(void) { // hoisted address equals the address taken outside
    int *p = &ga[5];
    int n = 0;
    for (int i = 0; i < 64; i++)
        if (&ga[i] == p) n += i;
    return n;
}

long scalar_global(const int *a, int n) { // read-modify-write global in a loop
    gsum = 0;
    for (int i = 0; i < n; i++) gsum += a[i];
    return gsum;
}

long nested(void) { // inner and outer loop both use the same globals
    long s = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) {
            ga[i * 8 + j] = i + j;
            s += ga[i * 8 + j] + sa[i * 8 + j];
        }
    return s;
}

int main(void) {
    int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    if (sieve(1000) != 168) return 1;
    long mg = 0;
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 64; i++) mg += (long)i * 36 + k;
    if (many_globals() != mg) return 2;
    long wc = 0;
    for (int i = 0; i < 64; i++) wc += (long)(i + 1) * 2;
    if (with_calls() != wc) return 3;
    long gt = 0;
    for (int i = 0; i < 16; i++) gt += (long)(i + 1) * 3 + (i + 1);
    if (got_arrays() != gt) return 4;
    if (addr_identity() != 5) return 5;
    if (scalar_global(a, 10) != 55) return 6;
    long ns = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) ns += (i + j) + (long)(i * 8 + j);
    if (nested() != ns) return 7;
    return 0;
}
