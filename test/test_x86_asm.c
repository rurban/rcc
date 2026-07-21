/* x86-64 GCC extended-asm features used pervasively in the Linux kernel's
 * inline asm (uaccess/atomic/MSR templates):
 *
 * - Operand size-override modifiers %b/%w/%k force a specific
 *   sub-register width (8/16/32-bit) for an operand regardless of its
 *   natural size, e.g. arch/x86/include/asm/segment.h's `%k0`.
 *   (%h, the legacy high-byte modifier, needs a hardware-pinned a/b/c/d
 *   register via a single-letter constraint to be meaningful — that
 *   pinning is a separate, still-open gap in the inline-asm template
 *   substitution, not covered here.)
 *
 * - Named operands (`[name] "constraint" (expr)`) let a template
 *   reference an operand as %[name] instead of a positional %N, so
 *   inserting/reordering operands doesn't require renumbering every
 *   reference, e.g. `[errout] "+r" (err)` referenced as %[errout]. A
 *   size modifier and a name combine as %k[sel].
 *
 * - GAS allows ';' as a statement separator on one physical line, and a
 *   label may be followed directly by a directive on the same line. Both
 *   are rare in compiler-generated code but used by hand-written kernel
 *   objtool/exception-table annotations, e.g.
 *     912: .pushsection .discard.annotate_data,"M",@progbits,8; .long 912b - .,1 ; .popsection
 *   and
 *     2:  .pushsection __ex_table, "aM", @progbits, 12
 *   Without support for either, the section switch never happens and the
 *   .pushsection/.long/.popsection content leaks straight into .text,
 *   corrupting the surrounding code.
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

    /* Named operands. */
    {
        int out = 0;
        int in = 42;
        __asm__ volatile ("movl %[in], %[out]"
                           : [out] "=r"(out)
                           : [in] "r"(in));
        if (out != 42) return 4;
    }

    /* Named operand + size modifier combined (kernel style, %k[sel]). */
    {
        unsigned r = 0;
        __asm__ volatile ("movl %k[val], %[dst]"
                           : [dst] "=r"(r)
                           : [val] "r"(wide));
        if (r != 0x55667788u) return 5;
    }

    /* Semicolon-joined pushsection/long/popsection on one line, followed
     * by a normal (newline-separated) exception-table block. */
    {
        int result;
        __asm__ volatile (
            "1:\n\t"
            "movl $5, %0\n\t"
            "2:\n\t"
            "912: .pushsection .discard.annotate_data,\"M\",@progbits,8; .long 912b - .,1 ; .popsection\n\t"
            ".pushsection __ex_table, \"aM\", @progbits, 12\n\t"
            ".balign 4\n\t"
            ".long (1b) - .\n\t"
            ".long (2b) - .\n\t"
            ".long 7\n\t"
            ".popsection\n\t"
            : "=r"(result)
        );
        if (result != 5) return 6;
    }

    /* Label immediately followed by a directive with no semicolon at
     * all (just whitespace). */
    {
        int result;
        __asm__ volatile (
            "1:\n\t"
            "movl $8, %0\n\t"
            "2:  .pushsection __ex_table, \"aM\", @progbits, 12\n\t"
            " .balign 4\n\t"
            " .long (1b) - .\n\t"
            " .long (2b) - .\n\t"
            " .long 8 \n\t"
            " .popsection\n\t"
            : "=r"(result)
        );
        if (result != 8) return 7;
    }

    /* GAS numeric local labels ("1b"/"1f") as actual jump targets, not
     * just data references — a retry-loop pattern used constantly in
     * kernel inline asm (x86 jmp/jcc/call/lea operand resolution must
     * strip the b/f direction suffix before looking up the label). */
    {
        long n = 5;
        __asm__ volatile (
            "1:\n\t"
            "decl %k0\n\t"
            "jnz 1b\n\t"
            : "=r"(n)
            : "0"(n)
        );
        if (n != 0) return 8;
    }
    {
        int result = 1;
        __asm__ volatile (
            "jmp 1f\n\t"
            "movl $0, %0\n\t"
            "1:\n\t"
            : "=r"(result)
            : "0"(result)
        );
        if (result != 1) return 9;
    }

    /* "i"/"n" immediate operand constraints must emit the numeric value
     * ($42) rather than falling back to a register name. */
    {
        int result;
        __asm__ volatile ("movl %1, %0" : "=r"(result) : "i"(42));
        if (result != 42) return 10;
    }

    /* A numeric-local-label retry loop referencing TWO memory operands
     * (the common kernel shape: bump a counter, decrement the loop
     * variable, loop while nonzero) — the decrement must be the last
     * flag-setting instruction before the jnz, since x86 conditional
     * jumps test the flags left by whichever instruction executed last,
     * not by whichever operand "looks" like the loop variable. */
    {
        static int loop_n = 5, loop_count = 0;
        __asm__ volatile (
            "1:\n\t"
            "incl %1\n\t"
            "decl %0\n\t"
            "jnz 1b\n\t"
            : "+m"(loop_n), "+m"(loop_count)
        );
        if (loop_n != 0 || loop_count != 5) return 11;
    }

    printf("OK asm x86 operand modifiers/named operands/semicolon stmts\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
