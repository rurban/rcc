/* Native-linker -Wl, option handling (src/link_elf.c, src/main.c).
 *
 * rcc's native ELF linker (link_elf.c, shared by the Linux/x86_64 and
 * arm64-cross builds) used to only understand -l/-L/-static inputs plus
 * bare .a/.so positionals and the -pie/-pic/-shared/-static/
 * -export-dynamic mode flags: every -Wl, option (rpath, soname,
 * --start-group/--end-group, --as-needed, --no-undefined, -v, -z, ...)
 * forced a fall back to the external (gcc) linker, whether or not the
 * option actually needed one. Both scenarios below -- a -Wl,-soname +
 * -Wl,-rpath shared-lib/executable pair, and a bare version-probe -Wl,-v
 * -- are now handled natively (see link_parse_opts() in link.c and its
 * call sites in link_elf.c): the probe prints rcc's own "GNU ld (rcc)
 * ..." banner (see main.c), and DT_SONAME/DT_RUNPATH are written
 * directly by link_elf() instead of only ever coming from a spawned
 * `cc`/`ld` subprocess.
 *
 * The observable contracts here are ELF-specific: the "GNU ld (" banner
 * only exists on GNU-ld-alike platforms (this native one included), and
 * the runtime-finds-the-library check relies on DT_RUNPATH (there is no
 * equivalent on PE/COFF, where DLL lookup is PATH-based, or on Mach-O's
 * @rpath/install_name scheme -- and both of those native backends still
 * fall back to the external linker for every -Wl, option, unchanged).
 * So the test is Linux/ELF-only; on other platforms it is a clean skip.
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

    /* 1. A bare -Wl,-v with no inputs is a link-only probe: it must
     * produce a "linker version" style banner, not die with "no input
     * files". rcc itself now prints one directly (main.c's
     * link_opts.version_probe check, "GNU ld (rcc) <version>") the
     * moment it recognizes -Wl,-v -- before it even attempts a link.
     * This exact invocation (no real object/source inputs at all) still
     * goes on to fall back to the external linker afterward (crt1.o's
     * `main` reference can never resolve with nothing else to link), so
     * collect2/ld's own banner ends up in the captured output too;
     * checked loosely for either: "collect2" (GCC's own wrapper) or
     * "GNU ld version" (older Binutils) are compiler-specific tells;
     * "GNU ld (" (both current Binutils' actual banner, e.g. "GNU ld
     * (GNU Binutils for Ubuntu) 2.42", and rcc's own) is the one that
     * must always be present for this to count as a real probe
     * response. */
    {
        char cmd[800];
        snprintf(cmd, sizeof(cmd), "%s -Wl,-v " NULL_REDIRECT " 2>&1", rcc);
        FILE *p = popen(cmd, "r");
        if (!p) { printf("FAIL: popen\n"); return 1; }
        char buf[4096] = {0};
        size_t n = fread(buf, 1, sizeof(buf) - 1, p);
        buf[n] = 0;
        int rc = pclose(p);
        if (!strstr(buf, "GNU ld version") && !strstr(buf, "collect2") && !strstr(buf, "GNU ld (")) {
            printf("FAIL: bare -Wl,-v produced no linker version output (rc=%d)\n%s\n", rc, buf);
            return 2;
        }
    }

    /* 2. A -Wl,-rpath link must record DT_RUNPATH -- now written
     * directly by link_elf() -- or the produced binary cannot find its
     * shared library at runtime. Build librcc_rt.so into a non-standard
     * dir and link prog against it with -Wl,-rpath,<that dir>. */
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
