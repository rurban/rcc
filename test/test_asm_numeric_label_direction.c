/* GAS numeric local labels ("1:") are reused by name -- each occurrence
 * gets its own private, uniquely-named symbol internally ("1f"/"1b" pick
 * the nearest not-yet-defined-forward / already-defined-backward
 * occurrence by textual position, never a name lookup). A macro that
 * emits the same "1:" label several times in sequence (e.g. an unrolled
 * bittree decoder invoked back-to-back) relies on each "jae 1f" inside
 * one occurrence resolving to *that occurrence's own* next "1:", not to
 * an already-defined EARLIER "1:" from a prior occurrence.
 *
 * Root cause (src/asm.c, encode_x86): every JMP/Jcc/CALL numeric-local-
 * label site called lookup_local(), which just returns the MOST RECENT
 * definition of the digit seen so far -- correct for a genuine "1b"
 * backward reference, but ALSO wrongly matched a forward "1f" reference
 * once any earlier "1:" of the same digit already existed (lookup_local()
 * has no way to distinguish "not found" from "found the wrong, stale,
 * already-passed occurrence"). A `jae 1f` immediately following an
 * earlier `1:` therefore silently resolved BACKWARD to that earlier
 * label instead of forward to its own next one -- turning an intended
 * forward skip into a backward branch. Found via xz's LZMA1 range
 * decoder: rc_asm_bittree's six-times-unrolled "1:"/"jae 1f" pair
 * resolved every iteration but the first backward into an earlier
 * iteration, corrupting the decoder's symbol register in an unbounded
 * loop.
 *
 * Fixed by capturing each JMP/Jcc/CALL instruction's own operand-0
 * direction suffix ('f' or 'b') BEFORE strip_local_label_suffix()
 * discards it: a "1f" reference now skips lookup_local() entirely
 * (forced to -1, "not yet defined"), always falling through to the
 * forward-fixup path that resolves against the NEXT occurrence's own
 * define_label() call, regardless of how many earlier same-digit "1:"s
 * already exist.
 *
 * This is a pure assembler-level check (like
 * test_asm_forward_local_label_binding.c): compile raw .s source
 * containing two independent numeric-label blocks and verify via
 * objdump disassembly that each `jae 1f` encodes a FORWARD (positive)
 * displacement to its own following "1:", never a backward one.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[160], objf[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_anld_%d.s", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_anld_%d.o", td, pid);

    /* Three occurrences of numeric label "1", two "jae 1f" references --
     * mirrors rc_asm_bittree's own shape of several independent
     * "1:"..."jae 1f"..."1:" blocks emitted back-to-back in one
     * function, each block's forward reference needing to skip to its
     * OWN next "1:", not fall back to an earlier one. */
    static const char src[] =
        "\t.text\n"
        "\t.globl entry\n"
        "entry:\n"
        "\txorl\t%eax, %eax\n"
        "1:\n"
        "\tincl\t%eax\n"
        "\tcmpl\t$0, %eax\n"
        "\tjae\t1f\n"
        "\tmovl\t$999, %eax\n"
        "1:\n"
        "\tincl\t%eax\n"
        "\tcmpl\t$0, %eax\n"
        "\tjae\t1f\n"
        "\tmovl\t$888, %eax\n"
        "1:\n"
        "\tret\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "objdump -d %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump failed\n"); remove(objf); return 1; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* Each disassembled "jae" line ends "jae <hex-target> <....>" --
     * extract the instruction's own address (leading "  N:") and its
     * jump target (the hex word right after "jae"), and require
     * target > own_addr (forward) for every "jae" found. A backward
     * (target <= own_addr) resolution reproduces the exact bug: the
     * unrolled sequence's second/third "jae 1f" branching into an
     * earlier iteration instead of its own next label. */
    int jae_count = 0, forward_count = 0;
    for (const char *ln = out; *ln;) {
        const char *nl = strchr(ln, '\n');
        size_t linelen = nl ? (size_t)(nl - ln) : strlen(ln);
        const char *j = strstr(ln, "jae");
        if (j && j < ln + linelen) {
            long own_addr = strtol(ln, NULL, 16);
            long target = strtol(j + 3, NULL, 16);
            jae_count++;
            if (target > own_addr) forward_count++;
        }
        ln = nl ? nl + 1 : ln + linelen;
    }

    if (jae_count != 2) {
        printf("FAIL: expected 2 \"jae\" instructions, found %d:\n%s\n", jae_count, out);
        return 1;
    }
    if (forward_count != jae_count) {
        printf("FAIL: %d of %d \"jae 1f\" instructions resolved BACKWARD "
               "instead of to their own following \"1:\":\n%s\n",
               jae_count - forward_count, jae_count, out);
        return 1;
    }

    printf("OK: all %d \"jae 1f\" numeric-local-label references resolve "
           "forward to their own next occurrence\n", jae_count);
    return 0;
}
#else
int main(void) { return 0; }
#endif
