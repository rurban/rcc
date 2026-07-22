/* A %l[label] branch inside a .pushsection'd non-.text region (e.g. the
 * kernel's ALTERNATIVE_TERNARY()-based static_cpu_has(), whose replacement
 * "jmp %l[t_no]" lands in .altinstr_replacement) forward-referencing a C
 * label that's defined *outside* the asm block, by an entirely separate
 * later call into codegen (the label always lives in .text). A previous
 * fix (cross-section forward-reference fixups always assumed .text, see
 * git log) made sure the patched bytes land in the *branch's* section
 * instead of clobbering .text at the same offset — but didn't address a
 * second, independent problem: once the label is later defined, the
 * fixup was still resolved as a same-buffer relative displacement
 * (target_off - instr_off), even when the branch and the label live in
 * completely unrelated sections. That's not a PC-relative displacement
 * at all — it's the difference between two independent sections' byte
 * counters, a meaningless number that happens to look like a plausible
 * (if wildly wrong) jump target. Confirmed via a real kernel build:
 * kernel/exec_domain.c's _static_cpu_has() produced a jmp inside
 * .altinstr_replacement (147 bytes total) whose baked-in displacement
 * pointed at a "target" over 150KB away — objtool caught it as "can't
 * find jump dest instruction".
 *
 * Fixed by tracking which section the branch instruction's fixup
 * belongs to (already recorded, from the prior fix) and, when it
 * differs from the label's own section (always .text for a C-level
 * label), emitting a real R_X86_64_PC32 relocation instead of a
 * same-buffer byte patch — mirroring the identical cross-section case
 * assemble_inline() already handles for fixups resolved entirely within
 * one inline-asm block.
 *
 * Verified via objdump: the jmp must carry a real relocation against the
 * label's symbol (not a baked-in same-buffer displacement, which would
 * show up as raw E9-prefixed bytes with *no* relocation line at all).
 *
 * Test not working on cross or non-x86.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_xsjr_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_xsjr_%d.o", td, pid);

    static const char src[] =
        "int main(void) {\n"
        "    int taken;\n"
        "    asm goto(\n"
        "        \".pushsection .altinstr_replacement, \\\"ax\\\"\\n\\t\"\n"
        "        \"jmp %l[l_yes]\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "        : : : : l_yes);\n"
        "    taken = 0;\n"
        "    goto done;\n"
        "l_yes:\n"
        "    taken = 1;\n"
        "done:\n"
        "    return taken;\n"
        "}\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s 2>/dev/null", rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "objdump -dr -j .altinstr_replacement %s 2>/dev/null", objf);
    FILE *p = popen(cmd, "r");
    if (!p) {
        printf("FAIL: objdump failed to run\n");
        remove(objf);
        return 1;
    }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* A real relocation, not a baked-in same-buffer displacement: must
     * see the PC32 reloc type against the l_yes label's symbol name. */
    if (!strstr(out, pc32_reloc_name()) || !strstr(out, "l_yes")) {
        printf("FAIL: expected a %s relocation against l_yes, "
               "got:\n%s\n", pc32_reloc_name(), out);
        return 1;
    }

    printf("OK cross-section jmp fixup uses a real relocation\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
