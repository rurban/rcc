/* movlps/movhps/movlpd/movhpd (xmm <-> m64) had no assembler handler; the
 * generic "mov" prefix dispatch assembled them as integer moves
 * (`movlpd %xmm13, 16(%rdi)` -> `mov %rdi, 16(%rdi)`), corrupting the
 * pointer. Seen in nettle's umac-nh-n.asm.
 */
#include <stdio.h>
#if defined(__x86_64__) || defined(_M_X64)

int main(void) {
    double src[2] = {1.5, 2.5}, lo[2] = {0, 0}, hi[2] = {0, 0}, lo2[2] = {0, 0};
    float fsrc[4] = {1, 2, 3, 4}, flo[4] = {0}, fhi[4] = {0};
    __asm__ volatile("movdqu (%0), %%xmm1\n\t"
                     "movlpd %%xmm1, (%1)\n\t"
                     "movhpd %%xmm1, (%2)\n\t"
                     "pxor %%xmm2, %%xmm2\n\t"
                     "movlpd (%0), %%xmm2\n\t"
                     "movhpd 8(%0), %%xmm2\n\t"
                     "movdqu %%xmm2, (%3)\n\t"
                     :: "r"(src), "r"(lo), "r"(hi), "r"(lo2) : "xmm1", "xmm2", "memory");
    __asm__ volatile("movups (%0), %%xmm3\n\t"
                     "movlps %%xmm3, (%1)\n\t"
                     "movhps %%xmm3, (%2)\n\t"
                     :: "r"(fsrc), "r"(flo), "r"(fhi) : "xmm3", "memory");
    if (lo[0] != 1.5 || hi[0] != 2.5 || lo2[0] != 1.5 || lo2[1] != 2.5 || flo[0] != 1 || flo[1] != 2 ||
        fhi[0] != 3 || fhi[1] != 4) {
        printf("FAIL: %g %g %g %g | %g %g %g %g\n", lo[0], hi[0], lo2[0], lo2[1], flo[0], flo[1], fhi[0], fhi[1]);
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
