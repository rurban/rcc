/* GAS's label-difference idiom "A - B" already worked for numeric local
 * labels (see test_label_diff.c: "772b-771b"), but silently evaluated to
 * 0 for a pair of *named* (non-numeric) labels instead — anywhere in the
 * codebase, not tied to any one construct.
 *
 * Root cause: an internal "pre-fold complete assembler-time expressions"
 * optimization in the macro-expansion pass (meant for macro-parameter
 * arithmetic like "\type + (.Lregnr << 8)") evaluates a bare identifier
 * against the table of .set/.equ assembler-time variables — but couldn't
 * tell "a genuine .set variable, value legitimately 0" apart from "an
 * as-yet-undefined name that might be a real label" and silently treated
 * the latter as 0 too, without ever signaling failure. "myB - myA" (two
 * ordinary labels, neither ever .set) then "successfully" folded to
 * "0 - 0 = 0" and got baked in as a wrong literal *before* the real
 * label-difference-fixup logic — which is otherwise completely correct —
 * ever got a chance to run.
 *
 * Found via a real Linux kernel build: usr/initramfs_data.S computes its
 * embedded initramfs blob's size as "__irf_end - __irf_start", both
 * ordinary (non-numeric) labels.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int check_section_bytes(const char *rcc, const char *td, int pid,
                               const char *tag, const char *src,
                               const char *section, const char *expect_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_nld_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_nld_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j %s %s " NULL_REDIRECT, section, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed to run\n", tag); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    if (!strstr(out, expect_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" not found in %s:\n%s\n",
               tag, expect_hex, section, out);
        return 0;
    }
    return 1;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    /* Both labels already defined ("backward" references) — the common
     * case (a size computed right after the block it measures), same as
     * test_label_diff.c's numeric-label back-reference case but with
     * ordinary names instead. */
    static const char src_back[] =
        "int main(void) {\n"
        "    __asm__ volatile (\n"
        "        \"myA:\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"myB:\\n\\t\"\n"
        "        \".pushsection .rodata\\n\\t\"\n"
        "        \".byte 0xAA\\n\\t\"\n"
        "        \".byte myB - myA\\n\\t\"\n"
        "        \".byte 0xBB\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "    );\n"
        "    return 0;\n"
        "}\n";
    ok &= check_section_bytes(rcc, td, pid, "back", src_back, rodata_section_name(), "aa03bb");

    /* Both labels still forward-referenced when the directive is
     * processed, resolved once assemble_inline finishes. */
    static const char src_fwd[] =
        "int main(void) {\n"
        "    __asm__ volatile (\n"
        "        \".pushsection .rodata\\n\\t\"\n"
        "        \".byte 0xAA\\n\\t\"\n"
        "        \".byte laterB - laterA\\n\\t\"\n"
        "        \".byte 0xBB\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "        \"laterA:\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"nop\\n\\t\"\n"
        "        \"laterB:\\n\\t\"\n"
        "    );\n"
        "    return 0;\n"
        "}\n";
    ok &= check_section_bytes(rcc, td, pid, "fwd", src_fwd, rodata_section_name(), "aa02bb");

    if (!ok) return 1;
    printf("OK named (non-numeric) label-difference resolves correctly, "
           "not silently 0\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
