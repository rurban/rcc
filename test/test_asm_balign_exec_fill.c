/* Real GAS defaults ".balign"/".align"/".p2align" (no explicit fill byte
 * given) to NOP-equivalent padding in an executable section - never plain
 * zero there, since zero bytes decode as arbitrary real instructions (x86
 * "00 00" is a valid 2-byte "add %al,(%rax)") rather than harmless no-ops.
 * rcc's own handle_directive() (src/asm.c) always defaulted the omitted
 * fill argument to 0 regardless of section, matching GAS's *data*-section
 * behavior but not its code-section one.
 *
 * Found via a real Linux kernel build: arch/x86/entry/entry_64.S's
 * idtentry_irq macro emits a bare ".p2align 6" (align to 64 bytes, no
 * fill argument) between two idtentry invocations in .entry.text. The
 * resulting zero-filled gap decoded as a run of fake "add" instructions
 * whose length (in bytes) never lined up with any real instruction
 * boundary the surrounding labels expected, so the kernel's own objtool
 * reported "can't find starting instruction" for a label sitting cleanly
 * at the end of that same gap.
 *
 * Fixed by defaulting the fill byte to 0x90 (NOP) whenever the target
 * section carries SHF_EXECINSTR and no fill argument was given; an
 * explicit fill argument (like the kernel's much more common ".balign
 * 16, 0x90") always overrides this regardless of section, unchanged.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
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
    snprintf(srcf, sizeof(srcf), "%s/test_abef_%d.S", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_abef_%d.o", td, pid);

    /* "ret" (1 byte) then ".p2align 3" (no fill) - needs 7 bytes of
     * padding to reach the next 8-byte boundary - then "nop", all inside
     * a single 16-byte objdump row so the row's own address-column text
     * (stripped of whitespace but not itself hex-adjacent to the bytes)
     * can't split the expected run. Every one of those 7 bytes must be
     * 0x90; a single 0x00 among them would still align the trailing
     * "nop" correctly (byte count is unchanged) but decode as a bogus
     * instruction filling the gap. */
    static const char src[] =
        ".code64\n.text\n"
        ".globl start_fn\n"
        "start_fn:\n"
        "\tret\n"
        "\t.p2align 3\n"
        "\tnop\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j .text %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump failed\n"); remove(objf); return 1; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[4096];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = (char)tolower((unsigned char)*s);
    collapsed[cn] = '\0';

    /* c3 (ret) then seven 0x90 padding bytes then 90 (the trailing nop)
     * = eight NOP bytes total after the ret. */
    if (!strstr(collapsed, "c39090909090909090")) {
        printf("FAIL: expected ret (c3) followed by eight 0x90 NOP-fill "
               "bytes in .text (a .p2align 3 with no explicit fill byte "
               "must still pad an executable section with NOPs, not "
               "zeroes), got:\n%s\n", out);
        return 1;
    }

    printf("OK .balign/.align/.p2align with no explicit fill byte pads "
           "an executable section with NOPs (0x90), not zero bytes\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
