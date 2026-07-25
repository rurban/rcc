/* A same-section forward "jmp"/"call" whose target label comes after an
 * intervening ALTERNATIVE()-style deferred .skip padding insertion got its
 * PC-relative displacement computed and baked into the object buffer too
 * early: define_label() (src/asm.c) resolved and byte-patched every
 * FIXUP_REL32 targeting the label it was defining immediately, during the
 * single left-to-right pass over the source text. But a .skip's own
 * padding amount depends on labels that may not be known until later in
 * the same buffer (the kernel's ALTERNATIVE() macro computes it as
 * max(0, replacement_len - original_len), where replacement_len lives in
 * a forward-referenced .pushsection'd .altinstr_replacement block) - so
 * every such insertion is itself deferred to a *separate* pass, run only
 * after the whole file has been scanned once.
 *
 * That ordering is exactly backwards for a forward jmp/call whose target
 * sits on the far side of one of these deferred insertions: the jmp's
 * relative displacement gets computed and written into the buffer using
 * the *pre-insertion* distance between call site and target, and by the
 * time the .skip's padding is actually inserted (shifting the target
 * forward, since it fell after the insertion point but the call site
 * didn't), nothing goes back to fix the already-baked-in displacement -
 * it silently stays wrong, off by exactly the inserted byte count.
 *
 * Found via a real Linux kernel build: arch/x86/entry/entry_64.S's
 * "jmp target1" (near entry_SYSCALL_64's start) spans an
 * ALTERNATIVE("", "verw x86_verw_sel(%rip)", ...) whose replacement is
 * longer than its (empty) original, inserting real padding between the
 * jmp and its target.
 *
 * Fixed by deferring the byte patch itself (not just noticing the
 * problem) until after every FIXUP_ALIGN/FIXUP_SKIP_MAXDIFF in the file
 * has resolved and shifted both the call site and the target's own
 * locals[] entry into their final positions - see FIXUP_REL32_DEFERRED
 * in src/obj.h.
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
    snprintf(srcf, sizeof(srcf), "%s/test_ars_%d.S", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_ars_%d.o", td, pid);

    /* A forward "jmp local_target" immediately followed by an
     * ALTERNATIVE() whose replacement ("nop" x8, 8 bytes) is longer than
     * its (empty) original — inserting 8 bytes of 0x90 padding between
     * the jmp and local_target, which the jmp's own displacement must
     * account for. */
    static const char src[] =
        ".code64\n.text\n"
        ".macro altinstr_entry orig, alt, ft_flags, orig_len, alt_len\n"
        "\t.long \\orig-.\n"
        "\t.long \\alt-.\n"
        "\t.4byte \\ft_flags\n"
        "\t.byte \\orig_len\n"
        "\t.byte \\alt_len\n"
        ".endm\n"
        ".macro ALTERNATIVE oldinstr, newinstr, ft_flags\n"
        "740:\n"
        "\t\\oldinstr\n"
        "741:\n"
        "\t.skip -(((744f-743f)-(741b-740b)) > 0) * ((744f-743f)-(741b-740b)),0x90\n"
        "742:\n"
        "\t.pushsection .altinstructions,\"a\"\n"
        "\taltinstr_entry 740b,743f,\\ft_flags,742b-740b,744f-743f\n"
        "\t.popsection\n"
        "\t.pushsection .altinstr_replacement,\"ax\"\n"
        "743:\n"
        "\t\\newinstr\n"
        "744:\n"
        "\t.popsection\n"
        ".endm\n"
        ".globl start_fn\n"
        "start_fn:\n"
        "\tmovl $1, %eax\n"
        "\tjmp local_target\n"
        "\tALTERNATIVE \"\", \"nop; nop; nop; nop; nop; nop; nop; nop\", 5\n"
        "\tmovl $99, %eax\n" /* must be skipped by the jmp - 99 is never seen */
        "local_target:\n"
        "\tmovl $42, %eax\n" /* jmp must land exactly here */
        "\tret\n";

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
    char out[8192];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[8192];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = (char)tolower((unsigned char)*s);
    collapsed[cn] = '\0';

    /* "movl $1,%eax" (b801000000) then "jmp local_target" (e9 + rel32).
     * The jmp is at buffer offset 5, 5 bytes long, so its own PC-relative
     * base is 10; local_target sits 8 bytes of ALTERNATIVE padding plus
     * the 5-byte skipped "movl $99,%eax" (b863000000) past that, i.e. at
     * 10+8+5=23=0x17, giving rel32=0x17-10=13=0x0d. A stale, pre-padding
     * displacement would instead show 0x00000000 (jmp target computed
     * before the 8-byte insertion, landing back on the movl $99 that
     * immediately follows the jmp itself). */
    if (!strstr(collapsed, "b801000000e90d000000")) {
        printf("FAIL: expected \"movl $1,%%eax; jmp local_target\" (b801000000 e90d000000) "
               "in .text, got:\n%s\n", out);
        return 1;
    }
    if (strstr(collapsed, "e900000000")) {
        printf("FAIL: jmp's displacement is stale (0x00000000, computed before the "
               "ALTERNATIVE's .skip padding was inserted) in .text:\n%s\n", out);
        return 1;
    }

    printf("OK a forward jmp spanning a deferred ALTERNATIVE .skip insertion "
           "gets its displacement patched after, not before, the insertion\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
