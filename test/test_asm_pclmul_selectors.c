/* GAS: pclmulhqlqdq = pclmulqdq $0x01, pclmullqhqdq = $0x10 (AT&T operand
 * order). rcc had the two imm8 values swapped, which broke nettle's GCM
 * ghash (ghash-update-2.asm). Needs a CPU with PCLMULQDQ.
 */
#include <stdio.h>
#include <stdint.h>
#if defined(__x86_64__) || defined(_M_X64)

static int check(uint64_t out[2], uint64_t a0, uint64_t a1, uint64_t b0, uint64_t b1, int sel_dst_hi,
                 int sel_src_hi) {
    /* carry-less multiply of the selected qwords, low bits only: use
     * operands whose product is distinguishable per selection */
    uint64_t x = sel_dst_hi ? a1 : a0, y = sel_src_hi ? b1 : b0;
    uint64_t lo = 0, hi = 0;
    for (int i = 0; i < 64; i++)
        if ((y >> i) & 1) {
            lo ^= x << i;
            if (i) hi ^= x >> (64 - i);
        }
    return out[0] == lo && out[1] == hi;
}

int main(void) {
    unsigned eax = 1, ebx, ecx = 0, edx;
    __asm__("cpuid" : "+a"(eax), "=b"(ebx), "+c"(ecx), "=d"(edx));
    if (!(ecx & 2)) { /* CPUID.1:ECX.PCLMULQDQ */
        printf("OK (no pclmul)\n");
        return 0;
    }
    uint64_t a[2] = {0x3, 0x5}, b[2] = {0x7, 0x11}, o[2];
    __asm__("movdqu (%1), %%xmm1\n\tmovdqu (%2), %%xmm2\n\tpclmulhqlqdq %%xmm2, %%xmm1\n\tmovdqu %%xmm1, (%0)"
            :: "r"(o), "r"(a), "r"(b) : "xmm1", "xmm2", "memory");
    if (!check(o, a[0], a[1], b[0], b[1], 1, 0)) { printf("FAIL: pclmulhqlqdq\n"); return 1; }
    __asm__("movdqu (%1), %%xmm1\n\tmovdqu (%2), %%xmm2\n\tpclmullqhqdq %%xmm2, %%xmm1\n\tmovdqu %%xmm1, (%0)"
            :: "r"(o), "r"(a), "r"(b) : "xmm1", "xmm2", "memory");
    if (!check(o, a[0], a[1], b[0], b[1], 0, 1)) { printf("FAIL: pclmullqhqdq\n"); return 1; }
    printf("OK\n");
    return 0;
}
#else
int main(void) {
    printf("OK (not x86-64)\n");
    return 0;
}
#endif
