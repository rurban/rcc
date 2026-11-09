// SPDX-License-Identifier: LGPL-2.1-or-later
// A shared library symbol exported only under a NON-default version
// (`name@VER`, versym bit 15 set -- a "compat symbol") cannot be bound by a
// new link; `ld` reports "undefined reference". glibc >= 2.34 keeps
// __malloc_hook that way. rcc's genuine-undefined-symbol gate matched on the
// bare .dynsym name only, so autoconf's AC_CHECK_FUNCS([__malloc_hook])
// succeeded and libopus then failed to compile its malloc-hook test.
//
// Black box: build libhv.so with the host toolchain exporting
//   hv_default@@VER_1   (default version: linkable)
//   hv_compat@VER_1     (non-default:     not linkable)
// then rcc must link a program using hv_default and must refuse one using
// hv_compat.
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
    char libc_[512], map[512], lib[512], good[512], bad[512], exe[512];
    snprintf(libc_, sizeof libc_, "%s/test_lhv_lib_%d.c", td, pid);
    snprintf(map, sizeof map, "%s/test_lhv_%d.map", td, pid);
    snprintf(lib, sizeof lib, "%s/libtestlhv%d.so", td, pid);
    snprintf(good, sizeof good, "%s/test_lhv_good_%d.c", td, pid);
    snprintf(bad, sizeof bad, "%s/test_lhv_bad_%d.c", td, pid);
    snprintf(exe, sizeof exe, "%s/test_lhv_exe_%d", td, pid);

    if (write_file(libc_,
                   "int hv_default_impl(void) { return 7; }\n"
                   "int hv_compat_impl(void) { return 8; }\n"
                   "__asm__(\".symver hv_default_impl,hv_default@@VER_1\");\n"
                   "__asm__(\".symver hv_compat_impl,hv_compat@VER_1\");\n") ||
        write_file(map, "VER_1 { global: hv_default; hv_compat; };\n") ||
        write_file(good, "int hv_default(void);\nint main(void) { return hv_default() == 7 ? 0 : 1; }\n") ||
        write_file(bad, "int hv_compat(void);\nint main(void) { return hv_compat() == 8 ? 0 : 1; }\n")) {
        printf("FAIL: cannot write fixtures\n");
        return 1;
    }

    snprintf(buf, sizeof buf, "%s -shared -fPIC -Wl,--version-script=%s -o %s %s " NULL_REDIRECT, cc, map, lib, libc_);
    if (system(buf) != 0) {
        printf("SKIP: host toolchain cannot build a versioned .so\n");
        remove(libc_);
        remove(map);
        remove(good);
        remove(bad);
        return 0;
    }

    setenv("LD_LIBRARY_PATH", td, 1);
    int ok = 1;
    char name[64];
    snprintf(name, sizeof name, "testlhv%d", pid);

    snprintf(buf, sizeof buf, "%s -o %s %s -L%s -l%s " NULL_REDIRECT, rcc, exe, good, td, name);
    if (system(buf) != 0) {
        printf("FAIL: default-version symbol did not link\n");
        ok = 0;
    } else if (system(exe) != 0) {
        printf("FAIL: default-version program did not run\n");
        ok = 0;
    }
    remove(exe);

    snprintf(buf, sizeof buf, "%s -o %s %s -L%s -l%s " NULL_REDIRECT, rcc, exe, bad, td, name);
    if (system(buf) == 0) {
        printf("FAIL: non-default (compat) version symbol linked, ld rejects it\n");
        ok = 0;
    }

    remove(exe);
    remove(libc_);
    remove(map);
    remove(lib);
    remove(good);
    remove(bad);
    if (ok) printf("OK\n");
    return ok ? 0 : 1;
}

#else
int main(void) {
    printf("SKIP: Linux x86-64 only\n");
    return 0;
}
#endif
