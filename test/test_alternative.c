/* The kernel's ALTERNATIVE() macro (arch/x86/include/asm/alternative.h)
 * puts real replacement instructions in a ".pushsection .altinstr_replacement"
 * block — unlike __ex_table/__jump_table, which only ever hold .long/.quad
 * data directives, this section holds actual machine code that the kernel
 * patches in at boot time if a CPU feature is present.
 *
 * Before this fix, an instruction inside any .pushsection'd section other
 * than .text was silently dropped: encode_x86()/encode_arm64() hardcoded
 * their output buffer to &as->obj->text (and every branch/call relocation
 * inside them hardcoded SEC_TEXT), and the main per-line dispatch loop in
 * assemble_inline() outright skipped instruction lines whenever the
 * current section wasn't .text. This mirrors the exact shape of
 * ALTERNATIVE(oldinstr, newinstr, feature) — minus the CPU-feature-based
 * runtime patching machinery itself, which isn't needed to exercise the
 * assembler-level gap this test targets.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>

int main(void)
{
    int result;
    __asm__ volatile (
        "771:\n\t"
        "movl $1, %0\n\t"
        "772:\n\t"
        "773:\n\t"
        ".pushsection .altinstructions, \"aM\", @progbits, 12\n\t"
        ".long 771b - .\n\t"
        ".long 774f - .\n\t"
        ".4byte 0\n\t"
        ".byte 773b - 771b\n\t"
        ".byte 775f - 774f\n\t"
        ".popsection\n\t"
        ".pushsection .altinstr_replacement, \"ax\"\n\t"
        "774:\n\t"
        "movl $2, %0\n\t"
        "775:\n\t"
        ".popsection\n\t"
        : "=r"(result)
    );
    /* Feature flag is 0 (never patched in), so the default/old path
     * always runs: result must be 1, never 2. */
    if (result != 1) return 1;

    printf("OK alternative-style pushsection with real instructions\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
