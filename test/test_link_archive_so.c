// SPDX-License-Identifier: LGPL-2.1-or-later
// Verify rcc can consume both static (.a) and dynamic (.so) libraries
// via -L/-l, not just synthesize its own dynamic linking against a
// fixed set of known system libraries (libc, libm, ...).
//
// The native ELF linker's -l handling only ever looks up hardcoded
// system libraries (see find_shared_lib()'s one caller in
// link_elf.c, resolving "libc.so.6" and nothing else); an unresolved
// symbol from an arbitrary -l<name> falls through to rcc's own
// mingw/gcc fallback linker instead, which does understand real -L/-l
// archive and shared-object resolution. resolve_archives() in
// link_elf.c is also a no-op stub -- its only caller -- even though
// load_archive() right above it is a complete, working GNU-ar symbol
// walker that's simply never invoked. This test doesn't care which
// path (native or fallback) rcc takes; it's a black-box regression
// guard that -l against a real .a and a real .so both link and run
// correctly, so it keeps working unchanged once the native linker's
// own archive/shared-object resolution gets wired up.
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
    int ok = 1;
    char buf[1024];

    char libsrc[512], libobj[512], liba[512], libso[512];
    char mainsrc[512], exea[512], exeso[512], libname[64];
    snprintf(libname, sizeof(libname), "testlas_%d", pid);
    snprintf(libsrc, sizeof(libsrc), "%s/test_las_lib_%d.c", td, pid);
    snprintf(libobj, sizeof(libobj), "%s/test_las_lib_%d.o", td, pid);
    snprintf(liba, sizeof(liba), "%s/lib%s.a", td, libname);
    snprintf(libso, sizeof(libso), "%s/lib%s.so", td, libname);
    snprintf(mainsrc, sizeof(mainsrc), "%s/test_las_main_%d.c", td, pid);
    snprintf(exea, sizeof(exea), "%s/test_las_exea_%d", td, pid);
    snprintf(exeso, sizeof(exeso), "%s/test_las_exeso_%d", td, pid);

    FILE *f = fopen(libsrc, "w");
    if (!f) { printf("FAIL: cannot write %s\n", libsrc); return 1; }
    fputs("int mylib_add(int a, int b) { return a + b + 100; }\n", f);
    fclose(f);

    // Build fixture .a via the host toolchain: a real-world static
    // archive is exactly what -l needs to resolve against, and rcc
    // itself has no -shared/archive-producing mode of its own to
    // build one with.
    snprintf(buf, sizeof(buf), "%s -c -o %s %s " NULL_REDIRECT, cc, libobj, libsrc);
    if (system(buf) != 0) {
        printf("FAIL: host cc compile failed\n");
        remove(libsrc);
        return 1;
    }
    snprintf(buf, sizeof(buf), "ar rcs %s %s " NULL_REDIRECT, liba, libobj);
    if (system(buf) != 0) {
        printf("FAIL: ar failed\n");
        remove(libsrc);
        remove(libobj);
        return 1;
    }

    // Build fixture .so from the same source.
    snprintf(buf, sizeof(buf), "%s -shared -fPIC -o %s %s " NULL_REDIRECT, cc, libso, libsrc);
    if (system(buf) != 0) {
        printf("FAIL: host cc -shared compile failed\n");
        remove(libsrc);
        remove(libobj);
        remove(liba);
        return 1;
    }
    remove(libsrc);
    remove(libobj);

    FILE *mf = fopen(mainsrc, "w");
    if (!mf) { printf("FAIL: cannot write %s\n", mainsrc); return 1; }
    fputs(
        "int mylib_add(int a, int b);\n"
        "int main(void) { return mylib_add(2, 3) == 105 ? 0 : 1; }\n",
        mf);
    fclose(mf);

    // Static archive: -l<name> must resolve mylib_add from the .a.
    // -L/-Wl,-Bstatic isn't needed: with only a .a present (no .so of
    // the same name in the same -L dir yet at link time) there is
    // nothing else for -l to find.
    {
        char tmpa[512];
        snprintf(tmpa, sizeof(tmpa), "%s/lib%s_a_only.a", td, libname);
        rename(liba, tmpa);
        char anamelib[80];
        snprintf(anamelib, sizeof(anamelib), "%s_a_only", libname);
        snprintf(buf, sizeof(buf), "%s -o %s %s -L%s -l%s " NULL_REDIRECT,
                 rcc, exea, mainsrc, td, anamelib);
        int rc = system(buf);
        if (rc != 0) {
            printf("FAIL [static]: link failed (rc=%d)\n", rc);
            ok = 0;
        } else {
            snprintf(buf, sizeof(buf), "%s", exea);
            rc = system(buf);
            if (rc != 0) {
                printf("FAIL [static]: run failed (rc=%d)\n", rc);
                ok = 0;
            }
            remove(exea);
        }
        remove(tmpa);
    }

    // Shared object: -l<name> must resolve to the .so, and the result
    // must actually load and run it at runtime (LD_LIBRARY_PATH here
    // stands in for whatever rpath/search-path mechanism the linker
    // that actually produced the binary relies on).
    {
        setenv("LD_LIBRARY_PATH", td, 1);
        snprintf(buf, sizeof(buf), "%s -o %s %s -L%s -l%s " NULL_REDIRECT,
                 rcc, exeso, mainsrc, td, libname);
        int rc = system(buf);
        if (rc != 0) {
            printf("FAIL [shared]: link failed (rc=%d)\n", rc);
            ok = 0;
        } else {
            snprintf(buf, sizeof(buf), "%s", exeso);
            rc = system(buf);
            if (rc != 0) {
                printf("FAIL [shared]: run failed (rc=%d)\n", rc);
                ok = 0;
            }
            remove(exeso);
        }
    }

    remove(mainsrc);
    remove(libso);

    if (ok) printf("OK static archive + shared object -l linking\n");
    return ok ? 0 : 1;
}

#else // _WIN32 || __APPLE__

// Windows (.lib/.dll import-library model) and macOS (.dylib, no .a
// archive symbol-table format in common use) need their own,
// format-specific version of this test; skip cleanly here rather than
// force ELF-specific assumptions onto a different linker model.
int main(void) { return 0; }

#endif
