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
#include <stdint.h>

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

    /* ADD/SUB/AND/OR/XOR with a memory destination and register source
     * ("addl %reg, mem", AT&T src,dst order) — the mnem/ops splitter's
     * dispatch had no case for is_reg(0)&&is_mem(1), so this form
     * silently encoded zero bytes while still reporting success. Used
     * pervasively by kernel atomics (arch_atomic_add/sub/or/and/xor). */
    {
        int v = 5;
        __asm__ volatile ("addl %1, %0" : "+m"(v) : "r"(3) : "memory");
        if (v != 8) return 12;
        __asm__ volatile ("subl %1, %0" : "+m"(v) : "r"(2) : "memory");
        if (v != 6) return 13;
        __asm__ volatile ("andl %1, %0" : "+m"(v) : "r"(0xc) : "memory");
        if (v != 4) return 14;
        __asm__ volatile ("orl %1, %0" : "+m"(v) : "r"(0x10) : "memory");
        if (v != 0x14) return 15;
        __asm__ volatile ("xorl %1, %0" : "+m"(v) : "r"(0xff) : "memory");
        if (v != 0xeb) return 16;
    }

    /* "lock" (and rep/repe/repne) is a prefix byte, not a standalone
     * instruction — mnem/ops splitting upstream cuts at the first
     * whitespace, so "lock xaddl %0, %1" arrives as mnem="lock",
     * ops_str="xaddl %0, %1". Returning right after emitting the prefix
     * byte discarded that operand string (and the real instruction in
     * it) entirely, leaving a dangling lock prefix with no instruction
     * — this broke every lock-prefixed atomic in the kernel. */
    {
        int val = 5, add = 3;
        __asm__ volatile ("lock xaddl %0, %1" : "+r"(add), "+m"(val) : : "memory");
        if (val != 8 || add != 5) return 17;
    }

    /* lock bts/btr/btc mem,reg — kernel bitops' non-constant-index path
     * (arch_set_bit/clear_bit/change_bit). */
    {
        unsigned long word = 0;
        long bit = 5;
        __asm__ volatile ("lock btsq %1,%0" : "+m"(word) : "Ir"(bit) : "memory");
        if (word != (1UL << 5)) return 18;
        __asm__ volatile ("lock btrq %1,%0" : "+m"(word) : "Ir"(bit) : "memory");
        if (word != 0) return 19;
        __asm__ volatile ("lock btcq %1,%0" : "+m"(word) : "Ir"(bit) : "memory");
        if (word != (1UL << 5)) return 20;
    }

    /* lock cmpxchg mem,reg — atomic_cmpxchg/try_cmpxchg and friends. */
    {
        int64_t val = 10, old = 10, newv = 20, result;
        __asm__ volatile ("lock cmpxchgq %2,%1"
                           : "=a"(result), "+m"(val)
                           : "r"(newv), "0"(old)
                           : "memory");
        if (val != 20 || result != 10) return 21;
    }

    /* ud2 (0F 0B) — kernel BUG()/WARN_ON() traps compile to this in every
     * translation unit; must at least assemble even when never executed. */
    {
        int x = 1;
        if (x != 1)
            __asm__ volatile ("ud2");
    }

    /* ADD/SUB/AND/OR/XOR with an immediate source and memory destination
     * ("addl $imm, mem", AT&T src,dst order) — the ALU_OP dispatch had no
     * case for is_imm(0)&&is_mem(1) either, matching no branch at all.
     * This is x86_64's __smp_mb() ("lock addl $0,-4(%rsp)"), used by
     * every smp_mb() in the kernel (wq_has_sleeper() and friends). */
    {
        int v = 20;
        __asm__ volatile ("addl $5, %0" : "+m"(v) :: "memory");
        if (v != 25) return 22;
        __asm__ volatile ("subl $3, %0" : "+m"(v) :: "memory");
        if (v != 22) return 23;
        __asm__ volatile ("andl $0xf, %0" : "+m"(v) :: "memory");
        if (v != 6) return 24;
        __asm__ volatile ("orl $0x10, %0" : "+m"(v) :: "memory");
        if (v != 0x16) return 25;
        __asm__ volatile ("xorl $0xff, %0" : "+m"(v) :: "memory");
        if (v != 0xe9) return 26;
        /* the exact __smp_mb() form: no output, just the side effect of
         * executing (correctness here is "doesn't crash / drop bytes"). */
        __asm__ volatile ("lock addl $0,-4(%%rsp)" ::: "memory", "cc");
    }

    /* String instructions (movs/stos/cmps/scas) — no operands, implicit
     * rsi/rdi/rcx, always rep-prefixed in practice. copy_user_generic's
     * "rep movsb" is used by every raw_copy_{to,from}_user() call. */
    {
        char src[16] = "hello world!!!!";
        char dst[16] = {0};
        unsigned long n = 16, to = (unsigned long)dst, from = (unsigned long)src;
        __asm__ volatile ("rep movsb" : "+c"(n), "+D"(to), "+S"(from) :: "memory");
        for (int i = 0; i < 16; i++)
            if (dst[i] != src[i]) return 27;
    }
    {
        char buf[8];
        unsigned long n = 8, to = (unsigned long)buf;
        __asm__ volatile ("rep stosb" : "+c"(n), "+D"(to) : "a"(0x7a) : "memory");
        for (int i = 0; i < 8; i++)
            if (buf[i] != 0x7a) return 28;
    }

   return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
