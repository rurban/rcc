// SPDX-License-Identifier: LGPL-2.1-or-later
// -lutil (also -ldl/-lpthread/-lrt) on glibc >= 2.34 names an empty
// "!<arch>\n" stub archive and no lib<name>.so at all. rcc's native ELF
// linker treated "no .so found" as unresolvable and handed the whole link to
// gcc -- which, unlike rcc, does not rescan archives, so two static archives
// given in dependency-wrong order then failed with "undefined reference".
// openrc's muon build links exactly like this (librc.a libshared.a ... -lutil,
// where libshared.a needs librc.a).
//
// Black box: link  main.o  libb.a  liba.a  -lutil  where main -> a -> b.
// gcc's single pass rejects that order (libb.a is scanned before anything
// needs b); rcc's native linker rescans until nothing new resolves, so the
// link only succeeds if -lutil does NOT force the gcc fallback.
#define _DEFAULT_SOURCE
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__) && !defined(__aarch64__)

static const char *host_cc(void) {
    const char *cc = getenv("GCC_FOR_TESTS");
    return (cc && *cc) ? cc : "cc";
}

static int write_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(text, f);
    fclose(f);
    return 0;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    const char *cc = host_cc();
    int pid = (int)getpid();
    char buf[2048];

    char mainc[512], ac[512], bc[512], ao[512], bo[512], liba[512], libb[512], probe[512], exe[512];
    snprintf(mainc, sizeof mainc, "%s/test_lsao_main_%d.c", td, pid);
    snprintf(ac, sizeof ac, "%s/test_lsao_a_%d.c", td, pid);
    snprintf(bc, sizeof bc, "%s/test_lsao_b_%d.c", td, pid);
    snprintf(ao, sizeof ao, "%s/test_lsao_a_%d.o", td, pid);
    snprintf(bo, sizeof bo, "%s/test_lsao_b_%d.o", td, pid);
    snprintf(liba, sizeof liba, "%s/test_lsao_liba_%d.a", td, pid);
    snprintf(libb, sizeof libb, "%s/test_lsao_libb_%d.a", td, pid);
    snprintf(probe, sizeof probe, "%s/test_lsao_probe_%d.c", td, pid);
    snprintf(exe, sizeof exe, "%s/test_lsao_exe_%d", td, pid);

    // Skip where the host has no -lutil at all (musl): nothing to test.
    write_file(probe, "int main(void) { return 0; }\n");
    snprintf(buf, sizeof buf, "%s -o %s %s -lutil " NULL_REDIRECT, cc, exe, probe);
    int have_util = system(buf) == 0;
    remove(probe);
    remove(exe);
    if (!have_util) {
        printf("SKIP: host cannot link -lutil\n");
        return 0;
    }

    if (write_file(mainc, "int a_fn(void);\nint main(void) { return a_fn() == 42 ? 0 : 1; }\n") ||
        write_file(ac, "int b_fn(void);\nint a_fn(void) { return b_fn() + 1; }\n") ||
        write_file(bc, "int b_fn(void) { return 41; }\n")) {
        printf("FAIL: cannot write fixtures\n");
        return 1;
    }

    int ok = 1;
    snprintf(buf, sizeof buf, "%s -c -o %s %s " NULL_REDIRECT, rcc, ao, ac);
    if (system(buf) != 0) ok = 0;
    snprintf(buf, sizeof buf, "%s -c -o %s %s " NULL_REDIRECT, rcc, bo, bc);
    if (system(buf) != 0) ok = 0;
    snprintf(buf, sizeof buf, "ar rcs %s %s " NULL_REDIRECT, liba, ao);
    if (system(buf) != 0) ok = 0;
    snprintf(buf, sizeof buf, "ar rcs %s %s " NULL_REDIRECT, libb, bo);
    if (system(buf) != 0) ok = 0;
    if (!ok) {
        printf("FAIL: cannot build fixture archives\n");
    } else {
        // libb.a first, liba.a second: wrong order for a one-pass linker.
        snprintf(buf, sizeof buf, "%s -o %s %s %s %s -lutil " NULL_REDIRECT, rcc, exe, mainc, libb, liba);
        int rc = system(buf);
        if (rc != 0) {
            printf("FAIL: link failed (rc=%d)\n", rc);
            ok = 0;
        } else {
            rc = system(exe);
            if (rc != 0) {
                printf("FAIL: run failed (rc=%d)\n", rc);
                ok = 0;
            }
        }
    }

    remove(mainc);
    remove(ac);
    remove(bc);
    remove(ao);
    remove(bo);
    remove(liba);
    remove(libb);
    remove(exe);
    if (ok) printf("OK\n");
    return ok ? 0 : 1;
}

#else
int main(void) {
    printf("SKIP: Linux x86-64 only\n");
    return 0;
}
#endif
