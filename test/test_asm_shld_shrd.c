/* shld/shrd (double-precision shifts) fell into the shl/shr handler's
 * 3-character prefix match: `shrd $0x20, %rcx, %rbx` assembled as
 * `shr $0x20, %rcx`. Seen in nettle's ecc-curve448-modp.asm.
 */
#include <stdio.h>
#include <stdint.h>
#if defined(__x86_64__) || defined(_M_X64)

int main(void) {
    uint64_t hi = 0x1122334455667788ull, lo = 0xaabbccdd99887766ull;
    uint64_t r1 = hi, r2 = hi, r3 = hi, r4 = hi;
    uint32_t w1 = 0x12345678u, w2 = 0x9abcdef0u;
    __asm__("mov %1, %%r8\n\tshrd $16, %%r8, %0" : "+r"(r1) : "r"(lo) : "r8", "cc");
    __asm__("mov %1, %%r9\n\tshld $8, %%r9, %0" : "+r"(r2) : "r"(lo) : "r9", "cc");
    __asm__("shrd %%cl, %1, %0" : "+r"(r3) : "r"(lo), "c"(12) : "cc");
    __asm__("shld %%cl, %1, %0" : "+r"(r4) : "r"(lo), "c"(4) : "cc");
    __asm__("shld $12, %1, %0" : "+r"(w1) : "r"(w2) : "cc");
    uint64_t e1 = (hi >> 16) | (lo << 48), e2 = (hi << 8) | (lo >> 56), e3 = (hi >> 12) | (lo << 52),
             e4 = (hi << 4) | (lo >> 60);
    uint32_t e5 = (0x12345678u << 12) | (0x9abcdef0u >> 20);
    if (r1 != e1 || r2 != e2 || r3 != e3 || r4 != e4 || w1 != e5) {
        printf("FAIL: %llx %llx %llx %llx %x\n", (unsigned long long)r1, (unsigned long long)r2,
               (unsigned long long)r3, (unsigned long long)r4, w1);
        return 1;
    }
    printf("OK\n");
    return 0;
}
#else
int main(void) {
    printf("OK (not x86-64)\n");
    return 0;
}
#endif
