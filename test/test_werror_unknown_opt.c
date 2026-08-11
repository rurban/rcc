/* rcc deliberately tolerates command-line flags it doesn't implement (many
 * third-party Makefiles pass compiler-specific flags unconditionally) by
 * warning and continuing rather than erroring. But real GCC/Clang always
 * hard-error on a genuinely unrecognized *non-warning* flag (-f.../-m...,
 * as opposed to -W...) -- unconditionally for -f/-m flags, and rcc already
 * matched that behavior for -W flags specifically gated on
 * -Werror=unknown-warning-option. What was missing: a bare -Werror
 * combined with an unrecognized non-W flag (e.g. `-Werror -fiambroken`)
 * silently kept compiling instead of erroring, unlike real GCC/Clang
 * (verified: `gcc -c t.c -fiambroken` errors even *without* -Werror at
 * all).
 *
 * Found via test_muon's own `common/104 has arg` capability probe, which
 * checks "does `-Werror -fiambroken` correctly fail to compile" as its
 * mechanism for confirming -Werror can promote an unsupported argument
 * probe into a hard failure.
 *
 * Fixed by promoting an unrecognized flag to a hard error when bare
 * -Werror is present, UNLESS the flag looks like a warning flag (-W...) --
 * those keep the pre-existing clang-style leniency (warn unless the
 * caller specifically passed -Werror=unknown-warning-option), matching
 * how meson/muon's own warning-flag-support probes expect a bare -Werror
 * to NOT itself promote an unknown -W name to an error. Without -Werror
 * at all, an unknown flag of any shape still just warns and continues,
 * preserving rcc's existing tolerance for flags it doesn't implement.
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

static int run(const char *rcc, const char *extra_args) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -c %s/test_werror_unknown_opt_src.c -o %s/test_werror_unknown_opt_src.o %s " NULL_REDIRECT,
             rcc, get_tmpdir(), get_tmpdir(), extra_args);
    return system(cmd);
}

int main(void) {
    const char *rcc = find_rcc();
    char src[600];
    snprintf(src, sizeof(src), "%s/test_werror_unknown_opt_src.c", get_tmpdir());
    FILE *f = fopen(src, "w");
    if (!f) return 1;
    fputs("int main(void) { return 0; }\n", f);
    fclose(f);

    /* Case 1: an unrecognized non-W flag WITHOUT -Werror must still just
     * warn and succeed (rcc's existing tolerance must not regress). */
    int rc1 = run(rcc, "-fiambroken");
    if (rc1 != 0) {
        printf("FAIL: -fiambroken without -Werror wrongly failed to compile (rc=%d)\n", rc1);
        return 2;
    }

    /* Case 2: the same unrecognized non-W flag WITH -Werror must now
     * fail, matching real GCC/Clang. */
    int rc2 = run(rcc, "-Werror -fiambroken");
    if (rc2 == 0) {
        printf("FAIL: -Werror -fiambroken wrongly compiled cleanly\n");
        return 3;
    }

    /* Case 3: an unrecognized -W flag WITH bare -Werror must still only
     * warn (clang-style leniency for warning-flag probes; -Werror alone
     * must not promote it -- only -Werror=unknown-warning-option does). */
    int rc3 = run(rcc, "-Werror -Wtotallyfakewarning123");
    if (rc3 != 0) {
        printf("FAIL: -Werror -Wtotallyfakewarning123 wrongly failed to compile (rc=%d)\n", rc3);
        return 4;
    }

    /* Case 4: an unrecognized -W flag WITH -Werror=unknown-warning-option
     * must fail (the pre-existing, unchanged behavior). */
    int rc4 = run(rcc, "-Werror=unknown-warning-option -Wtotallyfakewarning123");
    if (rc4 == 0) {
        printf("FAIL: -Werror=unknown-warning-option -Wtotallyfakewarning123 wrongly compiled cleanly\n");
        return 5;
    }

    /* Case 5: a genuinely supported flag combined with -Werror must still
     * compile cleanly (the fix must not reject valid flags). */
    int rc5 = run(rcc, "-Werror -O2");
    if (rc5 != 0) {
        printf("FAIL: -Werror -O2 (a real, supported flag) wrongly failed to compile (rc=%d)\n", rc5);
        return 6;
    }

    remove(src);
    printf("OK\n");
    return 0;
}
