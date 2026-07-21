/* GNU "asm goto" and the kernel's jump-label/static-key infrastructure
 * (arch/x86/include/asm/jump_label.h), hit while pushing further into the
 * Linux kernel build (kernel/exec_domain.o's objtool failures):
 *
 * - Basic "asm goto" with a computed label (%l[name]) referencing a C
 *   label declared in the trailing goto-label list.
 *
 * - The kernel's JUMP_TABLE_ENTRY macro emits, inside a
 *   ".pushsection __jump_table"/".popsection" block, three GAS
 *   PC-relative-to-here directives in a row:
 *     .long  1b            - .
 *     .long  %l[label]      - .
 *     .quad  %c0 + %c1      - .
 *   ("1b" = the patch-site code address, "%l[label]" = the taken-branch
 *   target — a forward reference to a C label defined by regular codegen,
 *   not a GAS-local label inside this same asm block — and "%c0 + %c1" =
 *   the static_key's address plus a 0/1 branch-polarity constant, using
 *   the "c" modifier to emit a bare constant/symbol with no "$" prefix
 *   since it's a data-directive expression, not an instruction operand).
 *   This exercised three separate gaps:
 *     1. %l[label] substitution followed by "- ." was pre-evaluated by
 *        the GAS macro-assembler's expression folder (meant for
 *        \type + (.Lregnr << 8)-style compile-time arithmetic) treating
 *        the bare "." as an unknown-but-zero identifier instead of
 *        bailing out, corrupting the directive to ".long 0" before it
 *        ever reached the real symbol/relocation logic.
 *     2. The "c" modifier and address-of-global "i" operands weren't
 *        substituted at all.
 *     3. The "SYM - ." directive handler didn't split a trailing
 *        "+ addend" off the symbol name, so "key + 0 - ." tried to
 *        create a relocation against a symbol literally named
 *        "key + 0" instead of "key" with addend 0.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>

static int basic_asm_goto(int key)
{
    asm goto ("cmpl $1, %0\n\t"
              "je %l[l_yes]"
              : : "r"(key) : : l_yes);
    return 0;
l_yes:
    return 1;
}

struct static_key { int enabled; };
static struct static_key g_my_key = {0};

int main(void)
{
    if (basic_asm_goto(1) != 1) return 1;
    if (basic_asm_goto(0) != 0) return 2;

    /* Mirrors arch_static_branch()'s JUMP_TABLE_ENTRY shape exactly
     * (minus the ANNOTATE_DATA_SPECIAL/.balign noise, which don't affect
     * what's being tested here). The 5-byte NOP is what a not-yet-patched
     * static key falls through as; jumping here would mean the key was
     * (incorrectly) taken. */
    int taken = 0;
    asm goto("1: .byte 0x0f,0x1f,0x44,0x00,0x00\n\t"
             ".pushsection __jump_table,  \"aw\" \n\t"
             ".balign 8 \n\t"
             ".long 1b - . \n\t"
             ".long %l[l_yes] - . \n\t"
             ".quad %c0 + %c1 - . \n\t"
             ".popsection \n\t"
             : : "i" (&g_my_key), "i" (0) : : l_yes);
    goto done;
l_yes:
    taken = 1;
done:
    if (taken) return 3;

    printf("OK jump label / asm goto\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
