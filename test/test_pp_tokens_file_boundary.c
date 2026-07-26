/* pp_tokens_to_text() (which reconstitutes a standalone ".S" file's
 * preprocessed token stream back into plain assembly text for the built-in
 * assembler) tracked line numbers per source file to decide where to insert
 * a newline between tokens, but on a *file boundary* — returning from one
 * #include to another, or into a new one — it reset the tracked line
 * number without ever emitting a newline first. The last token from the
 * old file and the first token of the new one then landed on the exact
 * same reconstructed text line.
 *
 * Found via a real Linux kernel build: arch/x86/entry/entry.S pulls in
 * include/linux/objtool.h (via unwind_hints.h), which ends a ".macro
 * UNWIND_HINT ... .endm" block right at its own EOF/include boundary.
 * annotate.h's own last macro's ".endm" and objtool.h's "UNWIND_HINT"
 * macro header ended up glued onto one line — ".endm .macro UNWIND_HINT
 * ...", which then doesn't match ".macro" as a directive at all (the line
 * literally starts with ".endm", not ".macro"). The whole UNWIND_HINT
 * definition silently vanished: never registered, so every later
 * invocation fell through to "unknown x86 instruction: unwind_hint" and
 * emitted nothing, desyncing every following byte offset.
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

    char h1[128], h2[128], srcf[128], objf[128], cmd[512];
    snprintf(h1, sizeof(h1), "%s/test_ptfb_h1_%d.h", td, pid);
    snprintf(h2, sizeof(h2), "%s/test_ptfb_h2_%d.h", td, pid);
    snprintf(srcf, sizeof(srcf), "%s/test_ptfb_%d.S", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_ptfb_%d.o", td, pid);

    /* h1's macro ends right at EOF — no trailing blank line, no further
     * content after ".endm" — matching exactly how one #include's last
     * line hands off to the next file. */
    static const char h1_src[] =
        ".macro FROM_HEADER_ONE\n"
        "\t.long 0xaaaaaaaa\n"
        ".endm";
    FILE *f1 = fopen(h1, "w");
    if (!f1) { printf("FAIL: cannot write %s\n", h1); return 1; }
    fputs(h1_src, f1);
    fclose(f1);

    static const char h2_src[] =
        ".macro FROM_HEADER_TWO\n"
        "\t.long 0xbbbbbbbb\n"
        ".endm\n";
    FILE *f2 = fopen(h2, "w");
    if (!f2) { printf("FAIL: cannot write %s\n", h2); remove(h1); return 1; }
    fputs(h2_src, f2);
    fclose(f2);

    /* #include's h-char-sequence filename isn't a regular string literal
     * (no escape processing per the standard), but embedding get_tmpdir()'s
     * raw Windows path (backslash separators) is still asking for trouble —
     * same lesson as test_incbin.c's ".incbin" path earlier. Windows
     * accepts '/' just as well, so sidestep the question entirely. */
    char h1_inc[128], h2_inc[128];
    snprintf(h1_inc, sizeof(h1_inc), "%s", h1);
    snprintf(h2_inc, sizeof(h2_inc), "%s", h2);
    for (char *p = h1_inc; *p; p++) if (*p == '\\') *p = '/';
    for (char *p = h2_inc; *p; p++) if (*p == '\\') *p = '/';

    char src[512];
    snprintf(src, sizeof(src),
             "#include \"%s\"\n"
             "#include \"%s\"\n"
             ".data\n"
             "FROM_HEADER_ONE\n"
             "FROM_HEADER_TWO\n",
             h1_inc, h2_inc);
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); remove(h1); remove(h2); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc -D__ASSEMBLY__ " NULL_REDIRECT,
             rcc, objf, srcf);
    int rc = system(cmd);
    remove(h1);
    remove(h2);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d)\n", rc);
        remove(objf);
        return 1;
    }

    char dcmd[512];
    snprintf(dcmd, sizeof(dcmd), "objdump -s -j .data %s " NULL_REDIRECT, objf);
    FILE *p = popen(dcmd, "r");
    if (!p) { printf("FAIL: objdump failed\n"); remove(objf); return 1; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[4096];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = *s;
    collapsed[cn] = '\0';

    /* Both macros must have registered and expanded: a macro whose
     * definition got glued onto the previous file's last line never makes
     * it into the macro table, so its invocation ("FROM_HEADER_TWO") falls
     * through unexpanded and emits nothing instead of its .long value. */
    if (!strstr(collapsed, "aaaaaaaa") || !strstr(collapsed, "bbbbbbbb")) {
        printf("FAIL: expected both 0xaaaaaaaa and 0xbbbbbbbb in .data, got:\n%s\n", out);
        return 1;
    }

    printf("OK a #include file boundary doesn't glue the last line of one "
           "file onto the first line of the next\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
