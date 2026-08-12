/* Real GCC promotes `#warning` to a hard compile error under `-Werror`
 * (`error: #warning ... [-Werror=cpp]`, verified directly) -- but NOT
 * under `-pedantic-errors` alone (verified: gcc -pedantic-errors still
 * only warns). rcc's `#warning` directive unconditionally just printed
 * the warning and continued, with no mechanism at all to promote it to
 * an error even when the caller explicitly opted in via -Werror.
 *
 * Found via test_muon's own `common/28 try compile` capability probe
 * (meson/muon's `compiler.compiles(werror: true)` method), which
 * specifically checks that code containing `#warning` fails to compile
 * under -Werror.
 *
 * Fixed by promoting #warning to a clean compile error when
 * opt_werror_flag (set only by the literal bare -Werror token,
 * deliberately distinct from opt_Werror which -pedantic-errors also
 * sets -- see main.c's own comment on that split) is set.
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

static int compile(const char *rcc, const char *src, const char *obj, const char *extra) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s %s -c %s -o %s " NULL_REDIRECT, rcc, extra, src, obj);
    return system(cmd);
}

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char src[600], obj[700];
    snprintf(src, sizeof(src), "%s/test_warn_werror_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_warn_werror_%d.o", td, pid);
    if (!write_file(src, "#warning This is a warning\nint main(void) { return 0; }\n"))
        return 1;

    /* Case 1: without -Werror, #warning must not block compilation. */
    int rc1 = compile(rcc, src, obj, "");
    if (rc1 != 0) {
        printf("FAIL: #warning without -Werror wrongly failed to compile (rc=%d)\n", rc1);
        return 2;
    }

    /* Case 2: with bare -Werror, #warning must promote to a compile
     * error, matching real gcc's -Werror=cpp behavior. */
    int rc2 = compile(rcc, src, obj, "-Werror");
    if (rc2 == 0) {
        printf("FAIL: #warning -Werror wrongly compiled cleanly\n");
        return 3;
    }

    /* Case 3: -pedantic-errors alone must NOT promote #warning (matches
     * real gcc exactly: -pedantic-errors promotes pedantic diagnostics,
     * not -Wcpp ones). */
    int rc3 = compile(rcc, src, obj, "-pedantic-errors");
    if (rc3 != 0) {
        printf("FAIL: #warning -pedantic-errors (without bare -Werror) wrongly "
               "failed to compile (rc=%d)\n", rc3);
        return 4;
    }

    remove(src);
    remove(obj);
    printf("OK\n");
    return 0;
}
