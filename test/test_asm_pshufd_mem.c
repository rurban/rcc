/* `pshufd $imm, mem, %xmm` (memory source) was assembled as if the memory
 * operand were a register (`pshufd $0x1b, %xmm0, %xmm1`), no load at all.
 * Also covers the RIP-relative symbol form, whose PC32 relocation addend
 * must account for the trailing imm8.
 */
#include <stdio.h>
#if defined(__x86_64__) || defined(_M_X64)

unsigned pshufd_src[4] = {1, 2, 3, 4};

int main(void) {
    unsigned out[4] = {0}, out2[4] = {0};
    __asm__ volatile("pshufd $0x1b, (%0), %%xmm1\n\t"
                     "movdqu %%xmm1, (%1)\n\t"
                     "pshufd $0x1b, pshufd_src(%%rip), %%xmm2\n\t"
                     "movdqu %%xmm2, (%2)\n\t"
                     :: "r"(pshufd_src), "r"(out), "r"(out2) : "xmm1", "xmm2", "memory");
    if (out[0] != 4 || out[1] != 3 || out[2] != 2 || out[3] != 1 || out2[0] != 4 || out2[3] != 1) {
        printf("FAIL: %u %u %u %u | %u %u\n", out[0], out[1], out[2], out[3], out2[0], out2[3]);
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
