// SPDX-License-Identifier: LGPL-2.1-or-later
// When rcc's native ELF linker cannot handle a program (here: a PC-relative
// reference to a libc *data* symbol, `optarg`, which needs a copy relocation
// it does not implement) the link is handed to gcc. rcc's own linker rescans
// archives until nothing new resolves; a one-pass gcc/ld does not, so the same
// command line failed with "undefined reference" when archive X needs a
// symbol from archive Y listed BEFORE it. openrc's muon build is exactly that:
//   librc.a libeinfo.a libshared.a   (libshared.a -> librc.a), no group.
// The fallback now brackets archive inputs in --start-group/--end-group.
//
// Black box: main -> a_fn (liba.a) -> b_fn (libb.a), given as  libb.a liba.a
// plus an `optarg` reference to force the fallback.
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

    char mainc[512], ac[512], bc[512], ao[512], bo[512], liba[512], libb[512], exe[512];
    snprintf(mainc, sizeof mainc, "%s/test_lfag_main_%d.c", td, pid);
    snprintf(ac, sizeof ac, "%s/test_lfag_a_%d.c", td, pid);
    snprintf(bc, sizeof bc, "%s/test_lfag_b_%d.c", td, pid);
    snprintf(ao, sizeof ao, "%s/test_lfag_a_%d.o", td, pid);
    snprintf(bo, sizeof bo, "%s/test_lfag_b_%d.o", td, pid);
    snprintf(liba, sizeof liba, "%s/test_lfag_liba_%d.a", td, pid);
    snprintf(libb, sizeof libb, "%s/test_lfag_libb_%d.a", td, pid);
    snprintf(exe, sizeof exe, "%s/test_lfag_exe_%d", td, pid);

    if (write_file(mainc,
                   "extern char *optarg;\n"
                   "int a_fn(void);\n"
                   "int main(void) { return (a_fn() == 42 && optarg == 0) ? 0 : 1; }\n") ||
        write_file(ac, "int b_fn(void);\nint a_fn(void) { return b_fn() + 1; }\n") ||
        write_file(bc, "int b_fn(void) { return 41; }\n")) {
        printf("FAIL: cannot write fixtures\n");
        return 1;
    }

    // Objects from the host compiler: what matters is only the .a ordering.
    int ok = 1;
    snprintf(buf, sizeof buf, "%s -c -o %s %s " NULL_REDIRECT, cc, ao, ac);
    if (system(buf) != 0) ok = 0;
    snprintf(buf, sizeof buf, "%s -c -o %s %s " NULL_REDIRECT, cc, bo, bc);
    if (system(buf) != 0) ok = 0;
    snprintf(buf, sizeof buf, "ar rcs %s %s " NULL_REDIRECT, liba, ao);
    if (system(buf) != 0) ok = 0;
    snprintf(buf, sizeof buf, "ar rcs %s %s " NULL_REDIRECT, libb, bo);
    if (system(buf) != 0) ok = 0;
    if (!ok) {
        printf("FAIL: cannot build fixture archives\n");
    } else {
        snprintf(buf, sizeof buf, "%s -o %s %s %s %s " NULL_REDIRECT, rcc, exe, mainc, libb, liba);
        int rc = system(buf);
        if (rc != 0) {
            printf("FAIL: link failed (rc=%d)\n", rc);
            ok = 0;
        } else if ((rc = system(exe)) != 0) {
            printf("FAIL: run failed (rc=%d)\n", rc);
            ok = 0;
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
