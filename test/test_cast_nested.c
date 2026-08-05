/* Nested integer casts must truncate at each step.
 * (u64)(u32)x where x is a u64 must clear the upper 32 bits of the
 * register; when the intermediate u32 truncation is skipped, the
 * full 64-bit value leaks into the enclosing multiply, producing
 * a wrong high half (found via xxHash XXH_mult32to64). */
#include <stdio.h>
#include <stdint.h>

static uint64_t mult32to64(uint64_t x, uint64_t y) {
    return (uint64_t)(uint32_t)x * (uint64_t)(uint32_t)y;
}

int main(void) {
    uint64_t x = 0xca057f7396d53f59ULL;
    uint64_t y = 0x85EBCA76ULL;

    /* Via intermediate variable (worked before the fix). */
    uint32_t x32 = (uint32_t)x;
    uint32_t y32 = (uint32_t)y;
    uint64_t r_vars = (uint64_t)x32 * (uint64_t)y32;
    if (r_vars != 0x4ee7b6f42dfa6d06ULL) {
        printf("FAIL vars: %016llx\n", (unsigned long long)r_vars);
        return 1;
    }

    /* Inline nested cast — the u32 truncation was previously lost. */
    uint64_t r_direct = (uint64_t)(uint32_t)x * (uint64_t)(uint32_t)y;
    if (r_direct != 0x4ee7b6f42dfa6d06ULL) {
        printf("FAIL direct: %016llx\n", (unsigned long long)r_direct);
        return 2;
    }

    /* Through a function that hides the cast from constant folding. */
    uint64_t r_fn = mult32to64(x, y);
    if (r_fn != 0x4ee7b6f42dfa6d06ULL) {
        printf("FAIL fn: %016llx\n", (unsigned long long)r_fn);
        return 3;
    }

    /* Signed truncation: (int)(long)v should sign-extend back. */
    long sx = 0xFFFFFFFF80000001L;
    long r_sx = (long)(int)sx;
    if (r_sx != -2147483647L) {
        printf("FAIL signed: %ld\n", r_sx);
        return 4;
    }

    printf("OK\n");
    return 0;
}
