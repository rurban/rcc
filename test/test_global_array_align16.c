/* x86-64 psABI 3.1.2: a global array of at least 16 bytes is aligned to at
 * least 16 (GCC does this; smaller arrays keep their element alignment).
 * rcc used the element alignment, so SSE code doing movdqa from such a table
 * faulted: nettle's sha_ni `movdqa (K), MSGK` over
 * `static const uint32_t K[64]` segfaulted when K landed on a 4/8 mod 16
 * address. Covers initialized (.data) and zero-initialized (.bss) arrays.
 */
#include <stdint.h>
#include <stdio.h>

static char pad_a = 1;
static const uint32_t k_init[64] = {1, 2, 3};
static char pad_b = 2;
static uint32_t k_bss[7];
static char pad_c = 3;
static uint64_t k_bss2[2];
static char pad_d = 4;
static uint16_t k_data8[8] = {1};

int main(void) {
    const void *p[] = {k_init, k_bss, k_bss2, k_data8};
    const char *n[] = {"k_init", "k_bss", "k_bss2", "k_data8"};
    for (int i = 0; i < 4; i++)
        if ((uintptr_t)p[i] % 16) {
            printf("FAIL: %s at %p not 16-aligned\n", n[i], p[i]);
            return 1;
        }
    printf("OK %d\n", pad_a + pad_b + pad_c + pad_d);
    return 0;
}
