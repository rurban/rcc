/* Taking the address of a function defined in a shared library, from a
 * plain (no -fPIE/-fPIC) executable that only declares it `extern`, must
 * yield the same value as taking the address of that same function from
 * *inside* the shared library itself. C requires function-pointer
 * equality to be well-defined for the same function regardless of where
 * the pointer was formed; real GCC/Clang guarantee this across a shared
 * library boundary by always loading an externally-linked function's
 * address through the GOT, never baking in a direct RIP-relative offset
 * to some local PLT stub.
 *
 * Regression: codegen's "function used as a value" path (as opposed to
 * a direct call, which already went through the PLT) gated GOT-indirected
 * addressing purely on `opt_pic` (true only under -fPIC/-fPIE). A plain
 * executable (opt_pic == false) that merely *declares* an external
 * function and takes its address got a direct `lea` to that function's
 * PLT-style stub inside the executable, while code compiled into the
 * *defining* shared library (opt_pic == true there) correctly computed
 * the function's real address via the GOT. The two values differed,
 * breaking function-pointer identity across the module boundary.
 *
 * Found via jq (jqlang/jq): main.c registers &jq_util_input_next_input_cb
 * (defined in libjq.so's util.c) as a callback; util.c's own
 * jq_util_input_get_position() later asserts the stored pointer equals
 * its own &jq_util_input_next_input_cb -- the assertion aborted
 * (SIGABRT) because the two addresses disagreed. Fixed by also using
 * GOT-indirected addressing whenever the function has no local
 * definition in this translation unit (LVar.is_extern), independent of
 * opt_pic.
 *
 * ELF/dlfcn-specific (POSIX shared-library ABI concept); skipped on
 * Windows/PE and macOS/Mach-O, which have no directly analogous
 * cross-module function-pointer-identity requirement exercised the same
 * way here.
 */
#if !defined(_WIN32) && !defined(__APPLE__)
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char buf[1200];

    char libsrc[512], mainsrc[512], so[512], exe[512];
    snprintf(libsrc, sizeof(libsrc), "%s/test_fpid_lib_%d.c", td, pid);
    snprintf(mainsrc, sizeof(mainsrc), "%s/test_fpid_main_%d.c", td, pid);
    snprintf(so, sizeof(so), "%s/libtest_fpid_%d.so", td, pid);
    snprintf(exe, sizeof(exe), "%s/test_fpid_%d", td, pid);

    // Library defines `cb` and hands back its own view of &cb.
    FILE *fl = fopen(libsrc, "w");
    if (!fl) { printf("FAIL: cannot write %s\n", libsrc); return 1; }
    fputs("void cb(void) {}\n"
          "void *get_cb_from_lib(void) { return (void *)cb; }\n",
          fl);
    fclose(fl);

    // Plain (no -fPIE/-fPIC) executable only *declares* cb, takes its
    // address, and compares against the library's own view.
    FILE *fm = fopen(mainsrc, "w");
    if (!fm) { printf("FAIL: cannot write %s\n", mainsrc); remove(libsrc); return 1; }
    fputs("extern void cb(void);\n"
          "void *get_cb_from_lib(void);\n"
          "int main(void) {\n"
          "    void *a = (void *)cb;\n"
          "    void *b = get_cb_from_lib();\n"
          "    return a != b;\n"
          "}\n",
          fm);
    fclose(fm);

    snprintf(buf, sizeof(buf), "%s -fPIC -shared -o %s %s " NULL_REDIRECT, rcc, so, libsrc);
    if (system(buf) != 0) {
        printf("FAIL: could not build shared library\n");
        remove(libsrc);
        remove(mainsrc);
        return 1;
    }

    snprintf(buf, sizeof(buf), "%s -o %s %s %s -Wl,-rpath,%s " NULL_REDIRECT, rcc, exe, mainsrc, so, td);
    int rc = system(buf);
    remove(libsrc);
    remove(mainsrc);
    if (rc != 0) {
        printf("FAIL: could not build executable against the shared library\n");
        remove(so);
        return 1;
    }

    rc = system(exe);
    remove(so);
    remove(exe);
    if (rc != 0) {
        printf("FAIL: &cb taken from the executable != &cb taken from inside its "
               "defining shared library (exit %d)\n", rc);
        return 1;
    }

    printf("OK function pointer identity holds across the shared library boundary\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
