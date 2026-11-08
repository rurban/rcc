/* `.quad 0x8000000080008008` (any literal above INT64_MAX) was parsed with
 * strtoll() and saturated to 0x7fffffffffffffff. nettle's sha3-permute.asm
 * round-constant table (.rc) came out wrong, so every sha3 test failed.
 */
#include <stdio.h>
#include <stdint.h>

extern const uint64_t quad_u64_tab[4];
__asm__(".section .rodata\n"
        ".balign 8\n"
        ".globl quad_u64_tab\n"
        "quad_u64_tab:\n"
        ".quad 0x8000000080008008\n"
        ".quad 0xffffffffffffffff\n"
        ".quad 18446744073709551615\n"
        ".quad -2\n"
        ".text\n");

int main(void) {
    if (quad_u64_tab[0] != 0x8000000080008008ull || quad_u64_tab[1] != ~0ull || quad_u64_tab[2] != ~0ull ||
        quad_u64_tab[3] != (uint64_t)-2) {
        printf("FAIL: %llx %llx %llx %llx\n", (unsigned long long)quad_u64_tab[0],
               (unsigned long long)quad_u64_tab[1], (unsigned long long)quad_u64_tab[2],
               (unsigned long long)quad_u64_tab[3]);
        return 1;
    }
    printf("OK\n");
    return 0;
}
