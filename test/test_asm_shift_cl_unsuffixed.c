/* `shr %cl, %r8` (no size suffix) sized the operation from the FIRST
 * register operand, the 1-byte %cl count, and assembled `shr %cl, %r8b`.
 * The operand size comes from the destination. Seen in nettle's memxor3.asm.
 */
#include <stdio.h>
#if defined(__x86_64__) || defined(_M_X64)

int main(void) {
    unsigned long long a = 0x8000000000000000ull, b = 0x1ull, c = 0xf000000000000000ull;
    unsigned d = 0x80000000u;
    __asm__("mov %0, %%r8\n\tshr %%cl, %%r8\n\tmov %%r8, %0" : "+r"(a) : "c"(4) : "r8");
    __asm__("mov %0, %%r12\n\tshl %%cl, %%r12\n\tmov %%r12, %0" : "+r"(b) : "c"(40) : "r12");
    __asm__("mov %0, %%r13\n\tsar %%cl, %%r13\n\tmov %%r13, %0" : "+r"(c) : "c"(8) : "r13");
    __asm__("mov %0, %%r14d\n\tshr %%cl, %%r14d\n\tmov %%r14d, %0" : "+r"(d) : "c"(8) : "r14");
    if (a != 0x0800000000000000ull || b != 0x10000000000ull || c != 0xfff0000000000000ull ||
        d != 0x00800000u) {
        printf("FAIL: %llx %llx %llx %x\n", a, b, c, d);
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
