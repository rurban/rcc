/* Every compiler input file went through the full C pipeline
 * (preprocess -> parse -> typecheck -> codegen) unconditionally, with no
 * branch anywhere for a standalone assembly file (".S", needing C
 * preprocessing, or ".s", raw) passed directly as a compiler input — as
 * opposed to inline asm inside a .c file, which the C parser routes
 * through the same assembler already. A raw ".S" file's leading
 * directives (".section foo,\"a\"", a bare "label:") aren't valid C at
 * file scope, so the C parser choked on them with "type defaults to int"
 * / "expected specific operator" — several steps removed from the real
 * cause (this file was never supposed to reach the C parser at all).
 *
 * Found via a real Linux kernel build: usr/initramfs_data.S (wraps the
 * compressed initramfs cpio archive for early userspace) is exactly this
 * shape — .section/.incbin/.globl/label-difference, no C at all.
 *
 * Fixing this exposed two more, more fundamental bugs along the way,
 * both covered here:
 *
 * 1. main.c's new .S handling re-renders the token stream (after full
 *    macro/#ifdef preprocessing) back to flat text via a new
 *    pp_tokens_to_text(), since the assembler only ever consumes plain
 *    text. The C lexer splits assembly's dot-containing names
 *    (".section", ".init.ramfs", "label:") into separate punctuation +
 *    identifier tokens, same as it would "a.b.c" in real C — naively
 *    space-joining them back turned ".section" into ". section" and
 *    "label:" into "label :", and the assembler's own label detection
 *    requires zero whitespace before the colon. Fixed by re-gluing
 *    tokens that were byte-adjacent (no whitespace at all) in whichever
 *    buffer they were lexed from.
 *
 * 2. The GAS label-difference idiom "A - B" (e.g. computing a data
 *    blob's size as "end_label - start_label") silently evaluated to 0
 *    for any pair of *named* (non-numeric) labels, anywhere in the
 *    codebase, not just via a .S file: an internal "pre-fold complete
 *    assembler-time expressions" optimization (meant for macro-parameter
 *    arithmetic like "\type + (.Lregnr << 8)") couldn't tell "a genuine
 *    .set variable" apart from "an as-yet-undefined real label", so it
 *    silently treated the latter as 0 too and *successfully* folded
 *    "0 - 0" into a wrong literal "0" instead of leaving the expression
 *    for the real label-difference-fixup logic (already correct) to
 *    resolve once both labels are actually defined.
 *
 * .incbin (embedding another file's raw bytes verbatim, used by
 * initramfs_data.S for the actual cpio payload) wasn't implemented at
 * all; covered by test_incbin.c instead of duplicated here.
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
    int ok = 1;

    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_saf_%d.S", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_saf_%d.o", td, pid);

    /* The real-world shape: a named-label difference sized field, a
     * custom section whose name shares a substring with a built-in one
     * (".init.ramfs" contains no built-in name, but exercises the same
     * dot-splitting token-reconstruction path .init.ramfs.info does in
     * the real file), and an #ifdef exactly like the real header. */
    static const char src[] =
        "#define WORDSIZE 8\n"
        ".section .init.ramfs,\"a\"\n"
        "start_label:\n"
        ".byte 1,2,3,4,5\n"
        "end_label:\n"
        ".section .init.ramfs.info,\"a\"\n"
        ".globl blob_size\n"
        "blob_size:\n"
        "#if WORDSIZE == 8\n"
        "\t.quad end_label - start_label\n"
        "#else\n"
        "\t.long end_label - start_label\n"
        "#endif\n";

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
        return 2;
    }

    snprintf(cmd, sizeof(cmd), "objdump -h %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump -h failed\n"); remove(objf); return 3; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);

    if (!strstr(out, ".init.ramfs") || !strstr(out, ".init.ramfs.info")) {
        printf("FAIL: expected both custom sections, got:\n%s\n", out);
        ok = 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j .init.ramfs.info %s " NULL_REDIRECT, objf);
    p = popen(cmd, "r");
    if (!p) { printf("FAIL: objdump -s failed\n"); remove(objf); return 4; }
    n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* end_label - start_label == 5, little-endian .quad: "05000000 00000000". */
    if (!strstr(out, "05000000 00000000")) {
        printf("FAIL: expected blob_size == 5 (little-endian quad), got:\n%s\n", out);
        ok = 0;
    }

    if (!ok) return 1;
    printf("OK standalone .S file compiles through the assembler, "
           "not the C parser\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
