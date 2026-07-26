/* A "%l[label]" branch inside a .pushsection'd non-.text region (e.g. the
 * kernel's ALTERNATIVE() macro's .altinstr_replacement, jumping to the
 * taken-branch C label) forward-references a C-level label that's defined
 * *outside* the asm block, by an entirely separate call into the codegen
 * — the label always lives in .text, but the branch referencing it does
 * not. assemble_inline()'s own fixup-resolution pass can't resolve this
 * (the label isn't defined yet, anywhere it knows about), so it delegates
 * to codegen's on_forward callback (cg_inline_fixup_cb), which used to
 * discard which section the branch instruction itself was in and always
 * assumed .text. When the C label was later defined (cg_def_label, always
 * against .text), the pending fixup got patched into .text at the
 * *non-.text* offset it was actually recorded at — silently corrupting
 * whatever real code happened to already be sitting at that (usually very
 * small, since .altinstr_replacement sections tend to be short) offset in
 * .text, rather than patching the intended .altinstr_replacement bytes.
 *
 * This test places an "innocent" function first in .text (sentinel()) —
 * exactly the kind of code that silently got clobbered — then a second
 * function whose ALTERNATIVE()-shaped construct forward-references a C
 * label from inside .altinstr_replacement.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>

static int sentinel(void) {
    return 0x1234;
}

int main(void) {
    if (sentinel() != 0x1234) return 1;

    int taken;
    asm goto("1: .byte 0x0f,0x1f,0x44,0x00,0x00\n\t"
             ".pushsection .altinstructions, \"aM\", @progbits, 12\n\t"
             ".long 1b - . \n\t"
             ".long %l[l_yes] - . \n\t"
             ".4byte 0\n\t"
             ".byte 2f - 1b\n\t"
             ".byte 3f - 2f\n\t"
             ".popsection\n\t"
             ".pushsection .altinstr_replacement, \"ax\"\n\t"
             "2:\n\t"
             "jmp %l[l_yes]\n\t"
             "3:\n\t"
             ".popsection\n\t"
             : : : : l_yes);
    taken = 0;
    goto done;
l_yes:
    taken = 1;
done:
    if (taken) return 2; /* feature flag is 0: replacement never patched in */

    /* The real regression check: sentinel()'s own bytes (the first thing
     * in .text) must still be intact after the asm-goto construct above. */
    if (sentinel() != 0x1234) return 3;

    printf("OK cross-section fixup\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
