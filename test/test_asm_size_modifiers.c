/* GCC extended-asm operand size-override modifiers: %b/%w/%k force a
 * specific sub-register width (8/16/32-bit) for an operand regardless of
 * its natural size. Used throughout the Linux kernel's uaccess/atomic/MSR
 * inline asm (e.g. arch/x86/include/asm/segment.h's `%k0`, gsseg.h's
 * `%k[sel]`) to pick the right register name without the caller tracking
 * operand widths itself.
 *
 * (%h, the legacy high-byte modifier, needs a hardware-pinned a/b/c/d
 * register via a single-letter constraint to be meaningful — that pinning
 * is a separate, still-open gap in the inline-asm template substitution,
 * not covered here.)
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>

int main(void)
{
    long wide = 0x1122334455667788L;

    /* %k: 32-bit view of a 64-bit operand — writes only the low dword. */
    {
        long x = wide;
        unsigned r;
        __asm__("movl %k1, %0" : "=r"(r) : "r"(x));
        if (r != 0x55667788u) return 1;
    }

    /* %w: 16-bit view. */
    {
        long x = wide;
        unsigned short r;
        __asm__("movw %w1, %0" : "=r"(r) : "r"(x));
        if (r != 0x7788) return 2;
    }

    /* %b: 8-bit low-byte view. */
    {
        long x = wide;
        unsigned char r;
        __asm__("movb %b1, %0" : "=r"(r) : "r"(x));
        if (r != 0x88) return 3;
    }

    printf("OK asm size modifiers\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
