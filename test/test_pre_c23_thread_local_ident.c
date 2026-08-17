/* "thread_local"/"constexpr" are keywords only from C23 onward -- C11/C17
 * provide no such identifier reservation (only "_Thread_local" is a real
 * pre-C23 keyword; "thread_local" is merely a <threads.h> macro, and
 * "constexpr" has no pre-C23 equivalent at all). rcc's keyword table
 * unconditionally classified both bare spellings as storage-class
 * keywords regardless of -std=, breaking any pre-C23 declaration that
 * uses one as a plain identifier: "int thread_local;" mis-parsed as a
 * duplicate/incomplete storage-class specifier ("expected variable
 * name"), and even where a parameter declaration `int f(int
 * thread_local)` did not hard-error, is_typename() -- used to
 * disambiguate a parenthesized cast/compound-literal from a plain
 * parenthesized expression -- still treated a bare `(thread_local)` as
 * introducing a type-name, silently misparsing `if (thread_local) ...`
 * into nonsense that always evaluated as if the parameter were 0. Fixed
 * by gating both the declaration-specifier scanner and is_typename()'s
 * KW_STORAGE check on C23 for exactly these two spellings, matching the
 * codebase's existing typeof/alignas/bool C23-gating pattern. Found via
 * kefir (a real, actively maintained C11 compiler): source/ast/local_context.c's
 * `require_global_ordinary_object(..., kefir_bool_t thread_local, ...)`
 * parameter, referenced later as `if (thread_local) { ... }`.
 *
 * Must build under an explicit pre-C23 -std=: rcc's own default is C23
 * (where these spellings ARE real keywords, correctly rejected as
 * identifiers -- see the second half of this test), so this can't be a
 * plain default-flags unit test.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static const char *SRC =
    "extern int printf(const char *, ...);\n"
    "static int use_thread_local_param(int thread_local) {\n"
    "    if (thread_local) return 1;\n"
    "    return 0;\n"
    "}\n"
    "static int use_constexpr_param(int constexpr) {\n"
    "    if (constexpr) return 1;\n"
    "    return 0;\n"
    "}\n"
    "int main(void) {\n"
    "    int thread_local = 5;\n"
    "    int constexpr = 7;\n"
    "    if (thread_local != 5) return 1;\n"
    "    if (constexpr != 7) return 2;\n"
    "    if (use_thread_local_param(0) != 0) return 3;\n"
    "    if (use_thread_local_param(1) != 1) return 4;\n"
    "    if (use_thread_local_param(42) != 1) return 5;\n"
    "    if (use_constexpr_param(0) != 0) return 6;\n"
    "    if (use_constexpr_param(1) != 1) return 7;\n"
    "    if (use_constexpr_param(9) != 1) return 8;\n"
    "    printf(\"OK\\n\");\n"
    "    return 0;\n"
    "}\n";

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char src[600], bin[620], cmd[2600];
    int pid = (int)getpid();

    snprintf(src, sizeof(src), "%s/test_tlconstexpr_%d.c", td, pid);
    snprintf(bin, sizeof(bin), "%s/test_tlconstexpr_%d", td, pid);

    FILE *sf = fopen(src, "w");
    if (!sf) { printf("FAIL: cannot write %s\n", src); return 1; }
    fputs(SRC, sf);
    fclose(sf);

    /* Pre-C23: both spellings are plain identifiers. Must compile, link,
     * and run correctly -- the whole point is that `if (thread_local)`
     * reads the real parameter value instead of silently evaluating as
     * a bogus cast/compound-literal that always reads as 0. */
    snprintf(cmd, sizeof(cmd), "%s -std=c11 -o %s %s", rcc, bin, src);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: -std=c11 compile of thread_local/constexpr-as-identifier failed (rc=%d)\n", rc);
        remove(src);
        return 2;
    }
    rc = system(bin);
    remove(bin);
    if (rc != 0) {
        printf("FAIL: -std=c11 binary exited %d, expected 0 (thread_local/constexpr "
               "param misread as always-false)\n", rc);
        remove(src);
        return 3;
    }

    /* C23 (rcc's own default): both ARE real keywords, so the exact same
     * source using them as identifiers must be REJECTED, not silently
     * miscompiled. */
    snprintf(cmd, sizeof(cmd), "%s -std=c23 -o %s %s 2>/dev/null", rcc, bin, src);
    rc = system(cmd);
    remove(src);
    if (rc == 0) {
        printf("FAIL: -std=c23 accepted thread_local/constexpr as plain identifiers "
               "(should be rejected: they are real C23 keywords)\n");
        remove(bin);
        return 4;
    }

    printf("OK\n");
    return 0;
}
