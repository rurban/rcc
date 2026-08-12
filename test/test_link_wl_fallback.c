/* Internal-linker capability fallback (src/main.c).
 *
 * rcc's own native ELF/PE/Mach-O linker only understands -l/-L/-static
 * inputs plus bare .a/.so positionals and the -pie/-pic/-shared/-static/
 * -export-dynamic mode flags. Linker commands it cannot honor used to be
 * silently dropped - every -Wl, option (rpath, soname, --start-group/
 * --end-group, --as-needed, --no-undefined, -v, -z, ...) produced a link
 * that "succeeded" with the wrong semantics (no DT_RUNPATH/DT_SONAME, or
 * an archive whose --start-group closure never resolved) - and a bare
 * `rcc -Wl,-v` with no input files died at "no input files" instead of
 * running the linker-version probe that build tools like muon use to
 * detect the linker type. Both are now routed straight to the external
 * (gcc) linker.
 *
 * The observable contracts here are ELF-specific: the "GNU ld version"
 * banner only exists on GNU-ld platforms, and the runtime-finds-the-
 * library check relies on DT_RUNPATH (there is no equivalent on PE/COFF,
 * where DLL lookup is PATH-based, or on Mach-O's @rpath/install_name
 * scheme). So the test is Linux/ELF-only; on other platforms it is a
 * clean skip.
 */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

#if !defined(__linux__)
int main(void) {
    printf("OK\n");
    return 0;
}
#else

int main(void) {
    char rcc_abs[1024];
    const char *rcc_raw = find_rcc();
    const char *rcc = realpath(rcc_raw, rcc_abs) ? rcc_abs : rcc_raw;
    const char *td = get_tmpdir();
    if (chdir(td) != 0) {
        printf("FAIL: cannot chdir to %s\n", td);
        return 5;
    }

    /* 1. A bare -Wl,-v with no inputs is a link-only probe: it must run
     * the external linker (collect2/ld print their version banners),
     * not die with "no input files". */
    {
        char cmd[800];
        snprintf(cmd, sizeof(cmd), "%s -Wl,-v " NULL_REDIRECT " 2>&1", rcc);
        FILE *p = popen(cmd, "r");
        if (!p) { printf("FAIL: popen\n"); return 1; }
        char buf[4096] = {0};
        size_t n = fread(buf, 1, sizeof(buf) - 1, p);
        buf[n] = 0;
        int rc = pclose(p);
        if (!strstr(buf, "GNU ld version") && !strstr(buf, "collect2")) {
            printf("FAIL: bare -Wl,-v produced no linker version output (rc=%d)\n%s\n", rc, buf);
            return 2;
        }
    }

    /* 2. A -Wl,-rpath link must reach the external linker and record
     * DT_RUNPATH, or the produced binary cannot find its shared library
     * at runtime. Build librcc_rt.so into a non-standard dir and link
     * prog against it with -Wl,-rpath,<that dir>. */
    {
        char sub[700];
        snprintf(sub, sizeof(sub), "%s/rcc_rpath_%d", td, (int)getpid());
        char mk[700];
        snprintf(mk, sizeof(mk), "mkdir -p '%s'", sub);
        if (system(mk) != 0) { printf("FAIL: mkdir\n"); return 3; }

        FILE *f = fopen("rcc_rt_lib.c", "w");
        if (!f) { printf("FAIL: cannot write lib source\n"); return 3; }
        fprintf(f, "int rcc_rt_fun(void) { return 42; }\n");
        fclose(f);

        char cmd[900];
        snprintf(cmd, sizeof(cmd), "%s -shared -fPIC rcc_rt_lib.c -o '%s/librcc_rt.so' "
                 "-Wl,-soname,librcc_rt.so " NULL_REDIRECT, rcc, sub);
        if (system(cmd) != 0) { printf("FAIL: shared lib link\n"); return 4; }

        f = fopen("rcc_rt_main.c", "w");
        if (!f) { printf("FAIL: cannot write main source\n"); return 4; }
        fprintf(f, "extern int rcc_rt_fun(void);\nint main(void) { return rcc_rt_fun() == 42 ? 0 : 1; }\n");
        fclose(f);

        snprintf(cmd, sizeof(cmd), "%s -o rcc_rt_prog rcc_rt_main.c -L'%s' -lrcc_rt "
                 "-Wl,-rpath,'%s' " NULL_REDIRECT, rcc, sub, sub);
        if (system(cmd) != 0) { printf("FAIL: prog link with -Wl,-rpath\n"); return 5; }

        int rc = system("./rcc_rt_prog " NULL_REDIRECT);
        remove("rcc_rt_prog");
        if (rc != 0) {
            printf("FAIL: prog with DT_RUNPATH cannot find its library at runtime\n");
            { char rm[700]; snprintf(rm, sizeof(rm), "rm -rf '%s'", sub); system(rm); }
            return 6;
        }
        { char rm[700]; snprintf(rm, sizeof(rm), "rm -rf '%s'", sub); system(rm); }
    }
    remove("rcc_rt_lib.c");
    remove("rcc_rt_main.c");

    printf("OK\n");
    return 0;
}
#endif
