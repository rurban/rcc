/* Three related inline-asm codegen bugs, all found via xz's LZMA1 range
 * decoder (src/liblzma/rangecoder/range_decoder.h's hand-tuned x86-64
 * bittree macros: 8 early-clobber "=&r"/"+&r" outputs plus a "r" input --
 * 9 simultaneously-live GP operands, one more than the 8-slot virtual-
 * register pool provides), all rooted in codegen.c's gen(ND_ASM).
 *
 * Bug 1: pool overflow aliasing. When an asm statement needs more
 * simultaneously-live GP registers than the 8-slot pool, a plain
 * alloc_reg() SPILLS an already-committed operand's register to make
 * room, silently aliasing two operands onto the same physical register.
 * Fixed by widening every operand-setup site (output-only "=r", "+r",
 * plain input "r") with a last-resort RAX/RCX/RDX/RDI overflow pool
 * (asm_extra_pool, codegen.c), tried only once the ordinary pool is
 * exhausted (free_reg_count() == 0).
 *
 * Bug 2: try_const_int() (codegen.c) only recognized a bare ND_NUM for
 * an "i"/"n" immediate operand -- any non-trivial-but-still-constant
 * expression (shifts, arithmetic, e.g. RC_TOP_VALUE == (1 << N)) fell
 * back to the runtime-register path, needing a real register for a
 * value that must be a bare assembler immediate. Under the same pool
 * pressure as bug 1, that extra register allocation could itself
 * collide with another live operand. Fixed by falling back to the
 * general compile-time constant evaluator (eval_const_expr()).
 *
 * Bug 3: op_saved capture self-collision. A FIXED-register output
 * (e.g. "=a", forced into %eax) still needs its value captured to a
 * scratch register before the address-register restore may clobber
 * %eax with something else. When the pool was exhausted, capturing via
 * the SAME asm_extra_pool naively picked slot 0 (RAX) without checking
 * whether RAX itself was the very register being captured FROM (or was
 * separately claimed by another operand's own fixed-register
 * constraint) -- silently degenerating to a "mov %eax, %eax" no-op that
 * protected nothing, then corrupting that captured value once a LATER
 * pool-overflow operand's own "first free slot" choice also picked RAX.
 * Fixed by asm_extra_pick() (codegen_asm.h), which skips any pool entry
 * a fixed-register constraint elsewhere in the SAME statement already
 * claims (x86_fixed_claimed bitmask, codegen.c).
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>

/* Bug 1: 8 "+r" read-write outputs (fills the whole 8-slot pool) plus a
 * 9th "r" input, all simultaneously live -- each must land in its OWN
 * distinct physical register. Pre-fix: segfaults (two operands aliased
 * onto the same register corrupt each other's address/value). */
static int test_pool_overflow(void) {
    long a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8, inp = 100;
    __asm__ volatile(
        "addq $10, %0\n\t"
        "addq $20, %1\n\t"
        "addq $30, %2\n\t"
        "addq $40, %3\n\t"
        "addq $50, %4\n\t"
        "addq $60, %5\n\t"
        "addq $70, %6\n\t"
        "addq $80, %7\n\t"
        "addq %8, %0\n\t"
        : "+r"(a), "+r"(b), "+r"(c), "+r"(d), "+r"(e), "+r"(f), "+r"(g), "+r"(h)
        : "r"(inp)
        :
    );
    if (a != 1 + 10 + 100 || b != 22 || c != 33 || d != 44 || e != 55 ||
        f != 66 || g != 77 || h != 88) {
        printf("FAIL: [pool_overflow] a=%ld b=%ld c=%ld d=%ld e=%ld f=%ld "
               "g=%ld h=%ld (want 111 22 33 44 55 66 77 88)\n",
               a, b, c, d, e, f, g, h);
        return 1;
    }
    return 0;
}

/* Bug 2: 8 "=&r" outputs fill the pool exactly; the 9th operand is an
 * "n" immediate with a non-trivial-but-constant expression. Pre-fix:
 * segfaults (try_const_int's runtime-register fallback needs a 9th
 * register, triggering the same aliasing as bug 1). */
static int test_const_int_fold(void) {
    long r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0;
    __asm__ volatile(
        "movq $1, %0\n\t"
        "movq $2, %1\n\t"
        "movq $3, %2\n\t"
        "movq $4, %3\n\t"
        "movq $5, %4\n\t"
        "movq $6, %5\n\t"
        "movq $7, %6\n\t"
        "movq %[imm], %7\n\t"
        : "=&r"(r0), "=&r"(r1), "=&r"(r2), "=&r"(r3), "=&r"(r4), "=&r"(r5), "=&r"(r6), "=&r"(r7)
        : [imm] "n" ((1 << 4) | 1)
        :
    );
    if (r0 != 1 || r1 != 2 || r2 != 3 || r3 != 4 || r4 != 5 || r5 != 6 ||
        r6 != 7 || r7 != 17) {
        printf("FAIL: [const_int_fold] r0=%ld r1=%ld r2=%ld r3=%ld r4=%ld "
               "r5=%ld r6=%ld r7=%ld (want 1 2 3 4 5 6 7 17)\n",
               r0, r1, r2, r3, r4, r5, r6, r7);
        return 1;
    }
    return 0;
}

/* Bug 3: 8 "+&r" operands fill the pool exactly (all still live), plus a
 * 9th operand using the FIXED "=a" (%eax) constraint. Capturing %eax's
 * asm-written value (op_saved) needs a scratch register while the pool
 * is fully committed -- its extra-pool pick must not choose %eax itself
 * (a self-collision no-op) nor any register another operand's own
 * fixed-register constraint separately claims. Pre-fix: the 8th "+&r"
 * operand's own pool-overflow pick landed on %eax (asm_extra_pool's
 * first slot), silently colliding with "=a" once the asm body's
 * `movl $0xdeadbeef, %eax` executed. */
static int test_op_saved_fixedreg(void) {
    long r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0;
    unsigned ax_out = 0;
    __asm__ volatile(
        "movq $1, %0\n\t"
        "movq $2, %1\n\t"
        "movq $3, %2\n\t"
        "movq $4, %3\n\t"
        "movq $5, %4\n\t"
        "movq $6, %5\n\t"
        "movq $7, %6\n\t"
        "movq $8, %7\n\t"
        "movl $0xdeadbeef, %%eax\n\t"
        : "+&r"(r0), "+&r"(r1), "+&r"(r2), "+&r"(r3), "+&r"(r4), "+&r"(r5), "+&r"(r6), "+&r"(r7), "=a"(ax_out)
        :
        :
    );
    if (r0 != 1 || r1 != 2 || r2 != 3 || r3 != 4 || r4 != 5 || r5 != 6 ||
        r6 != 7 || r7 != 8 || ax_out != 0xdeadbeef) {
        printf("FAIL: [op_saved_fixedreg] r0=%ld r1=%ld r2=%ld r3=%ld r4=%ld "
               "r5=%ld r6=%ld r7=%ld ax=%x (want 1 2 3 4 5 6 7 8 deadbeef)\n",
               r0, r1, r2, r3, r4, r5, r6, r7, ax_out);
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_pool_overflow();
    failures += test_const_int_fold();
    failures += test_op_saved_fixedreg();
    if (failures) {
        printf("FAIL: %d sub-case(s) failed\n", failures);
        return 1;
    }
    printf("OK 9+ simultaneously-live inline-asm GP operands (pool overflow, "
           "constant-expression immediates, fixed-register output capture) "
           "all resolve to distinct, uncorrupted registers\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
