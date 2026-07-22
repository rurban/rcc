/* AT&T syntax marks an indirect call/jmp target with a leading '*'
 * ("call *%rax", "jmp *(%rax)") to distinguish it from a direct call/jmp
 * to a symbol. rcc's encode_x86() never stripped that '*' before
 * classifying the operand — is_reg()/is_mem() check the operand's first
 * character (must be '%' or contain '(' respectively), and "*%rax"/
 * "*(%rax)" both start with '*', so neither ever matched. Every indirect
 * call/jmp silently fell through to the direct/symbol-based path instead:
 * the operand text itself (e.g. "*(%r10)") got used as if it were a
 * symbol name, producing a bogus direct call/jmp to a nonsense "symbol"
 * (or, for a register operand with no parens, effectively a jump to
 * itself/next instruction) instead of actually branching through the
 * register or memory operand.
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/
 * paravirt_types.h's PARAVIRT_CALL ("call *%[paravirt_opptr]", used for
 * every paravirt-patchable operation) and plain "jmp *%reg" (used by
 * retpoline thunks and elsewhere) both hit this — objtool caught the
 * paravirt case as "retpoline_safe hint not an indirect jump/call/ret/nop"
 * because the annotated instruction wasn't actually an indirect branch
 * at all.
 *
 * This is a correctness bug, not just an objtool-annotation mismatch:
 * verified at runtime here, not just by checking the encoded bytes.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>

static int hit;
static void target(void) { hit = 42; }

int main(void)
{
    /* call *reg — indirect call through a register operand. */
    {
        hit = 0;
        void (*fp)(void) = target;
        __asm__ volatile ("call *%0" : : "r"(fp) : "memory", "cc");
        if (hit != 42) return 1;
    }

    /* call *mem — indirect call through a memory operand (the
     * paravirt-style "m" constraint case that triggered this). */
    {
        hit = 0;
        void (*fp)(void) = target;
        __asm__ volatile ("call *%0" : : "m"(fp) : "memory", "cc");
        if (hit != 42) return 2;
    }

    /* jmp *reg — indirect jump through a register operand, landing on a
     * computed-goto label. */
    {
        int taken = 0;
        void *dest = &&l_target;
        __asm__ volatile ("jmp *%0" : : "r"(dest));
        taken = 0; /* skipped if the jmp actually landed on l_target */
        goto l_done;
    l_target:
        taken = 1;
    l_done:
        if (!taken) return 3;
    }

    printf("OK indirect call/jmp through register and memory operands\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
