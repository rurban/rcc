// SPDX-License-Identifier: LGPL-2.1-or-later
// Verify rcc recognizes a *versioned* shared-library filename passed
// directly as a link input (e.g. "libz.so.1.3.2", "libcjson.so.1.7.19"),
// not just a bare ".so"/".dylib" suffix.
//
// The dynamic linker's SONAME convention appends numeric version
// components after ".so" (glibc's own libz.so.1.3.2, cjson's
// libcjson.so.1.7.19, ...); real build systems (zlib's `make test`,
// cJSON's CMake `check` target) pass that exact versioned path as a
// positional link argument. rcc's input-file classifier used
// strrchr(path, '.') to find the extension, which on
// "libz.so.1.3.2" finds the LAST dot (".2") -- missing ".so"
// entirely -- so rcc tried to compile the binary .so as C source
// ("libz.so.1.3.2:1: error: invalid token \x7fELF").
#define _DEFAULT_SOURCE
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>

#if !defined(_WIN32) && !defined(__APPLE__)

static const char *host_cc(void) {
    const char *cc = getenv("GCC_FOR_TESTS");
    return (cc && *cc) ? cc : "cc";
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    const char *cc = host_cc();
    int pid = (int)getpid();
    char buf[1024];

    char libsrc[512], libso_versioned[512], mainsrc[512], exe[512];
    snprintf(libsrc, sizeof(libsrc), "%s/test_vso_lib_%d.c", td, pid);
    // Mimic a real SONAME: libfoo.so.<major>.<minor>.<patch>.
    snprintf(libso_versioned, sizeof(libso_versioned), "%s/libtestvso_%d.so.1.3.2", td, pid);
    snprintf(mainsrc, sizeof(mainsrc), "%s/test_vso_main_%d.c", td, pid);
    snprintf(exe, sizeof(exe), "%s/test_vso_exe_%d", td, pid);

    FILE *f = fopen(libsrc, "w");
    if (!f) { printf("FAIL: cannot write %s\n", libsrc); return 1; }
    fputs("int vso_add(int a, int b) { return a + b + 200; }\n", f);
    fclose(f);

    snprintf(buf, sizeof(buf), "%s -shared -fPIC -o %s %s " NULL_REDIRECT,
             cc, libso_versioned, libsrc);
    if (system(buf) != 0) {
        printf("FAIL: host cc -shared compile failed\n");
        remove(libsrc);
        return 1;
    }
    remove(libsrc);

    FILE *mf = fopen(mainsrc, "w");
    if (!mf) { printf("FAIL: cannot write %s\n", mainsrc); return 1; }
    fputs(
        "int vso_add(int a, int b);\n"
        "int main(void) { return vso_add(2, 3) == 205 ? 0 : 1; }\n",
        mf);
    fclose(mf);

    // Pass the versioned .so path directly as a positional link input
    // (not via -l/-L), exactly like zlib's `libz.so.1.3.2` / cJSON's
    // `libcjson.so.1.7.19` on the real link command line.
    int ok = 1;
    setenv("LD_LIBRARY_PATH", td, 1);
    snprintf(buf, sizeof(buf), "%s -o %s %s %s " NULL_REDIRECT,
             rcc, exe, mainsrc, libso_versioned);
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
        remove(exe);
    }

    remove(mainsrc);
    remove(libso_versioned);

    if (ok) printf("OK versioned .so direct-path linking\n");
    return ok ? 0 : 1;
}

#else // _WIN32 || __APPLE__

// Windows (.lib/.dll import-library model) doesn't use SONAME-versioned
// filenames; macOS's ".N.dylib" versioning scheme differs from ELF's
// ".so.N.N.N" and needs its own test. Skip cleanly here.
int main(void) { return 0; }

#endif
