/* A newline embedded inside a still-open "/ * ... * /" block comment was
 * being treated as the end of the enclosing preprocessor directive line,
 * same as a real (uncommented) newline. Per the standard's phase ordering
 * (comments, including any newlines inside them, are removed in phase 3
 * before directive lines are delimited in phase 4), a comment-embedded
 * newline must never terminate a #define/#if/etc. body - only a genuine
 * newline outside any comment may.
 *
 * collect_directive_tokens() in src/preprocess.c called lex_one() in a
 * loop and broke out of it on either TK_NL (a real newline) or TK_CNL (one
 * emitted per newline embedded in a block comment being skipped) alike.
 * Any #define whose replacement list contained a multi-line comment - an
 * extremely common pattern (see fs/nfs/callback_xdr.c's
 * CB_OP_GETATTR_RES_MAXSZ in the Linux kernel) - had its body silently
 * truncated at the comment's first embedded newline, and the remainder of
 * the intended body leaked out as bogus top-level tokens instead.
 *
 * Fixed by having collect_directive_tokens() treat TK_CNL as "keep
 * scanning, just bump the line counter" and reserve early-exit for TK_NL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ppmc_%d.c", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_ppmc_%d", td, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif

    /* Two shapes of the bug: a multi-line comment on its own (no
     * backslash needed at all, since phase 3 comment removal precedes
     * phase 4 directive splitting), and the harder case from the real
     * kernel macro - a multi-line comment immediately followed by an
     * explicit line-continuation backslash right after the closing
     * "* /", continuing the #define onto yet another physical line. */
    static const char src[] =
        "#define A (1 + 2 + /* comment\n"
        "                     spanning lines */ 3)\n"
        "#define B (10 + \\\n"
        "            20 + \\\n"
        "            /* another\n"
        "             * multi-line comment */\\\n"
        "            30)\n"
        "int main(void) { return (A == 6 && B == 60) ? 0 : 1; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile failed (rc=%d) for #define bodies containing "
               "multi-line comments\n", rc);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exef);
    int status = system(cmd);
    remove(exef);
#ifndef _WIN32
    int exit_code = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#else
    int exit_code = status; /* Windows system() returns the exit code directly */
#endif
    if (exit_code != 0) {
        printf("FAIL: expected exit 0 (A==6 && B==60), got %d - a "
               "comment-embedded newline truncated a #define body\n",
               exit_code);
        return 1;
    }

    printf("OK a newline inside a multi-line block comment no longer "
           "terminates the enclosing #define body\n");
    return 0;
}
