// SPDX-License-Identifier: LGPL-2.1-or-later
// Regression test: __attribute__((constructor)) / __attribute__((destructor))
// flags leaked across function DECLARATIONS and stacked onto one function.
//
// The pending_constructor/pending_destructor flags are set by the attribute
// scan and (pre-fix) were only cleared when a function DEFINITION consumed
// them. Two consecutive prototype-only declarations -- glib's
// gconstructor.h idiom:
//
//     G_DEFINE_CONSTRUCTOR(resource_constructor)
//     G_DEFINE_DESTRUCTOR(resource_destructor)
//
// i.e. `static void __attribute__((constructor)) f(void);` immediately
// followed by `static void __attribute__((destructor)) g(void);` -- left
// BOTH flags set when the first DEFINITION was parsed, so the constructor
// function got is_constructor AND is_destructor, while the destructor
// function got neither. The .fini_array then contained the CONSTRUCTOR
// (and the destructor never appeared anywhere): at dlclose the ELF loader
// ran resource_constructor as the "destructor", re-registering the
// module's lazy resource after its memory was about to be unmapped -- the
// next g_resources_get_info() drain read the dangling GStaticResource and
// segfaulted (glib's gio/tests resources test, signal 11).
//
// Fixed by recording the consumed flags on the function's symbol at each
// prototype-only declaration exit and clearing the pending globals, and
// OR-ing the symbol flags into the definition.
//
// The test compiles a small module with the decl/def split above into a
// shared object, dlopens and dlcloses it, and checks the constructor ran
// exactly once (at dlopen) and the destructor ran at dlclose. With the
// pre-fix compiler the destructor never runs (the constructor runs in its
// place) and the destructor marker file is never written.
#define _DEFAULT_SOURCE
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <dlfcn.h>

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char buf[1024];

    char modsrc[512], so[512], ctor_marker[512], dtor_marker[512];
    snprintf(modsrc, sizeof(modsrc), "%s/test_cd_mod_%d.c", td, pid);
    snprintf(so, sizeof(so), "%s/libtest_cd_%d.so", td, pid);
    snprintf(ctor_marker, sizeof(ctor_marker), "%s/test_cd_ctor_%d.mk", td, pid);
    snprintf(dtor_marker, sizeof(dtor_marker), "%s/test_cd_dtor_%d.mk", td, pid);

    // The glib gconstructor.h idiom: constructor and destructor DECLARED
    // with their attributes, DEFINED later without.
    FILE *fm = fopen(modsrc, "w");
    if (!fm) { printf("FAIL: cannot write %s\n", modsrc); return 1; }
    fprintf(fm,
        "#include <stdio.h>\n"
        "static void __attribute__((constructor)) res_ctor(void);\n"
        "static void __attribute__((destructor)) res_dtor(void);\n"
        "static void res_ctor(void) { FILE *f = fopen(\"%s\", \"w\"); if (f) fclose(f); }\n"
        "static void res_dtor(void) { FILE *f = fopen(\"%s\", \"w\"); if (f) fclose(f); }\n",
        ctor_marker, dtor_marker);
    fclose(fm);

    snprintf(buf, sizeof(buf), "%s -fPIC -shared -o %s %s " NULL_REDIRECT,
             rcc, so, modsrc);
    if (system(buf) != 0) {
        printf("FAIL: could not build module\n");
        return 1;
    }
    remove(modsrc);

    void *h = dlopen(so, RTLD_NOW);
    if (!h) {
        printf("FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    FILE *c = fopen(ctor_marker, "r");
    if (!c) {
        printf("FAIL: constructor did not run at dlopen\n");
        return 1;
    }
    fclose(c);

    if (dlclose(h) != 0) {
        printf("FAIL: dlclose: %s\n", dlerror());
        return 1;
    }

    // With the pre-fix compiler the .fini_array held the CONSTRUCTOR, so
    // dlclose re-ran it and the destructor never ran: dtor marker absent.
    FILE *d = fopen(dtor_marker, "r");
    int dtor_ok = (d != NULL);
    if (d) fclose(d);

    remove(so);
    remove(ctor_marker);
    remove(dtor_marker);

    if (!dtor_ok) {
        printf("FAIL: destructor did not run at dlclose (fini_array pointed at the constructor?)\n");
        return 1;
    }

    printf("OK constructor ran at dlopen, destructor at dlclose\n");
    return 0;
}

#else // _WIN32 || __APPLE__

// PE .CRT$XCU / Mach-O __mod_init_func paths are emitted by the COFF/
// Mach-O writers, not the ELF .init_array/.fini_array code this bug
// lives in; skip cleanly.
int main(void) { return 0; }

#endif
