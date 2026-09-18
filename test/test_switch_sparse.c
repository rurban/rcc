/* -O1 sparse switch lowering (codegen.c's switch_emit_bsearch /
 * switch_emit_hash_dispatch, added alongside the dense small-int jump
 * table): a switch with enough non-range cases to skip the dense jump
 * table (sparse values, or too many/too spread out) lowers to a sorted
 * binary search (>= 8 cases) or, above a higher threshold, a compile-time
 * perfect hash (>= 17 cases) instead of a linear cmp/jcc chain.
 *
 * Also exercises a real bug found while building this: ARM64's
 * SUBS-based `cmp` immediate only encodes an unsigned 12-bit value,
 * optionally left-shifted by 12 (a multiple of 4096 up to 0xffffff).
 * asm_cmp_imm() didn't check this and always encoded with shift 0, so
 * a case value like 4096 silently truncated to `cmp w, #0` -- wrong on
 * every input, including on -O0's plain linear chain (predates this
 * pass entirely). See asm_cmp_imm() in codegen_asm.h.
 */
#include <stdlib.h>

/* 10 sparse cases (>= 8 => binary search), spanning negative, small,
 * and values ARM64's immediate compare can't encode directly (5000,
 * 100000: neither <= 4095 nor a multiple of 4096). */
static int bsearch10(int x) {
    switch (x) {
    case -50: return 1;
    case 3: return 2;
    case 17: return 3;
    case 42: return 4;
    case 99: return 5;
    case 150: return 6;
    case 777: return 7;
    case 1000: return 8;
    case 5000: return 9;
    case 100000: return 10;
    default: return -1;
    }
}

/* 20 sparse int cases (>= 17 => perfect hash attempted), including
 * several exact multiples of 4096 (4096, 8192, 16384, ...) that
 * previously mis-encoded as `cmp w, #0` on ARM64. */
static int hash20(int x) {
    switch (x) {
    case 2: return 100;
    case 9: return 101;
    case 23: return 102;
    case 47: return 103;
    case 88: return 104;
    case 133: return 105;
    case 256: return 106;
    case 512: return 107;
    case 1024: return 108;
    case 2048: return 109;
    case 4096: return 110;
    case 8192: return 111;
    case 16384: return 112;
    case 32768: return 113;
    case 65536: return 114;
    case 131072: return 115;
    case 262144: return 116;
    case 524288: return 117;
    case 1048576: return 118;
    case 2097152: return 119;
    default: return -1;
    }
}

/* 18 sparse 8-byte cases: exercises the 64-bit sign-extension path
 * (sz == 8) in both the hash key extension and the binary-search
 * fallback's scratch-register compare. */
static long hashlong(long x) {
    switch (x) {
    case 1LL: return 1;
    case 1000000000000LL: return 2;
    case -1000000000000LL: return 3;
    case 2000000000000LL: return 4;
    case 3000000000000LL: return 5;
    case 4000000000000LL: return 6;
    case 5000000000000LL: return 7;
    case 6000000000000LL: return 8;
    case 7000000000000LL: return 9;
    case 8000000000000LL: return 10;
    case 9000000000000LL: return 11;
    case 10000000000000LL: return 12;
    case 11000000000000LL: return 13;
    case 12000000000000LL: return 14;
    case 13000000000000LL: return 15;
    case 14000000000000LL: return 16;
    case 15000000000000LL: return 17;
    case 16000000000000LL: return 18;
    default: return -1;
    }
}

/* 17 sparse unsigned cases: the zero-extension path (is_uns), including
 * a case value above INT32_MAX. */
static unsigned hashuns(unsigned x) {
    switch (x) {
    case 4000000000u: return 1;
    case 1u: return 2;
    case 2u: return 3;
    case 3u: return 4;
    case 4u: return 5;
    case 5u: return 6;
    case 6u: return 7;
    case 7u: return 8;
    case 8u: return 9;
    case 9u: return 10;
    case 10u: return 11;
    case 11u: return 12;
    case 12u: return 13;
    case 13u: return 14;
    case 14u: return 15;
    case 15u: return 16;
    case 16u: return 17;
    default: return 0;
    }
}

/* 9 exact cases plus one case-range: must still take the linear/range
 * chain (case-ranges disqualify both sparse strategies), and the range
 * must still work correctly. */
static int mixedrange(int x) {
    switch (x) {
    case 1: return 1;
    case 2: return 2;
    case 3: return 3;
    case 4: return 4;
    case 5: return 5;
    case 6: return 6;
    case 7: return 7;
    case 8: return 8;
    case 9: return 9;
    case 100 ... 200: return 100;
    default: return -1;
    }
}

int main(void) {
    if (bsearch10(-50) != 1) abort();
    if (bsearch10(3) != 2) abort();
    if (bsearch10(17) != 3) abort();
    if (bsearch10(42) != 4) abort();
    if (bsearch10(99) != 5) abort();
    if (bsearch10(150) != 6) abort();
    if (bsearch10(777) != 7) abort();
    if (bsearch10(1000) != 8) abort();
    if (bsearch10(5000) != 9) abort();
    if (bsearch10(100000) != 10) abort();
    if (bsearch10(0) != -1) abort();
    if (bsearch10(-51) != -1) abort();
    if (bsearch10(-49) != -1) abort();
    if (bsearch10(2) != -1) abort();
    if (bsearch10(1001) != -1) abort();
    if (bsearch10(99999) != -1) abort();
    if (bsearch10(100001) != -1) abort();
    if (bsearch10(-1000000) != -1) abort();
    if (bsearch10(1000000) != -1) abort();

    {
        int hvals[20] = {2, 9, 23, 47, 88, 133, 256, 512, 1024, 2048,
                         4096, 8192, 16384, 32768, 65536, 131072, 262144,
                         524288, 1048576, 2097152};
        int i, j, expect;
        for (i = 0; i < 20; i++)
            if (hash20(hvals[i]) != 100 + i) abort();
        if (hash20(0) != -1) abort();
        if (hash20(1) != -1) abort();
        if (hash20(3) != -1) abort();
        if (hash20(255) != -1) abort();
        if (hash20(257) != -1) abort();
        if (hash20(1023) != -1) abort();
        if (hash20(1025) != -1) abort();
        if (hash20(-2) != -1) abort();
        if (hash20(2097153) != -1) abort();
        if (hash20(-2097152) != -1) abort();
        for (i = -100; i < 100; i++) {
            expect = -1;
            for (j = 0; j < 20; j++)
                if (hvals[j] == i) expect = 100 + j;
            if (hash20(i) != expect) abort();
        }
    }

    if (hashlong(1LL) != 1) abort();
    if (hashlong(-1000000000000LL) != 3) abort();
    if (hashlong(16000000000000LL) != 18) abort();
    if (hashlong(0LL) != -1) abort();
    if (hashlong(-1LL) != -1) abort();
    if (hashlong(1000000000001LL) != -1) abort();
    if (hashlong(17000000000000LL) != -1) abort();

    if (hashuns(1u) != 2) abort();
    if (hashuns(16u) != 17) abort();
    if (hashuns(4000000000u) != 1) abort();
    if (hashuns(0u) != 0) abort();
    if (hashuns(17u) != 0) abort();
    if (hashuns(4000000001u) != 0) abort();
    if (hashuns(3999999999u) != 0) abort();

    {
        int i;
        for (i = 1; i <= 9; i++)
            if (mixedrange(i) != i) abort();
        if (mixedrange(100) != 100) abort();
        if (mixedrange(150) != 100) abort();
        if (mixedrange(200) != 100) abort();
        if (mixedrange(0) != -1) abort();
        if (mixedrange(10) != -1) abort();
        if (mixedrange(99) != -1) abort();
        if (mixedrange(201) != -1) abort();
        if (mixedrange(-5) != -1) abort();
    }

    return 0;
}
