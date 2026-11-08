/* A RIP-relative memory operand naming a symbol ("sym(%rip)") got its
 * PC32 relocation only for lea/movaps/movups; every other mnemonic (movdqa,
 * mov, addl, ...) was encoded with a bare disp32 of 0, i.e. a load relative
 * to itself, with no relocation. nettle's sha_ni
 * `movdqa .Lswap_mask(%rip), %xmm9` segfaulted on it. The relocation is now
 * attached for every mnemonic, with the addend adjusted for trailing
 * immediates.
 */
#include <stdio.h>
#if defined(__x86_64__) || defined(_M_X64)

long long rip_sym_gv = 0x1122334455667788LL;
int rip_sym_cnt = 40;

int main(void) {
    unsigned char out[16];
    long long v;
    __asm__ volatile(
        "jmp .Lrip_test_go\n"
        ".p2align 4\n"
        ".Lrip_test_mask: .byte 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0\n"
        ".Lrip_test_go: movdqa .Lrip_test_mask(%%rip), %%xmm9\n"
        "movdqu %%xmm9, (%0)\n"
        :
        : "r"(out)
        : "xmm9", "memory");
    __asm__ volatile("movq rip_sym_gv(%%rip), %0" : "=r"(v));
    __asm__ volatile("addl $2, rip_sym_cnt(%%rip)" ::: "memory");
    for (int i = 0; i < 16; i++)
        if (out[i] != 15 - i) {
            printf("FAIL: movdqa mask byte %d = %d\n", i, out[i]);
            return 1;
        }
    if (v != 0x1122334455667788LL) {
        printf("FAIL: movq sym(%%rip) = %llx\n", v);
        return 1;
    }
    if (rip_sym_cnt != 42) {
        printf("FAIL: addl $2, sym(%%rip): %d\n", rip_sym_cnt);
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
