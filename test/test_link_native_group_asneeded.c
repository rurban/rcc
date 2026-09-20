/* Native-linker archive-group and --as-needed handling (src/link_elf.c).
 *
 * Two more -Wl, capabilities the native ELF linker used to force a fall
 * back to the external (gcc) linker for (see test_link_wl_fallback.c's
 * own header comment for the general history):
 *
 * 1. -Wl,--start-group/--end-group: real ld only re-scans an archive for
 *    a newly-satisfiable member within an explicit group bracket, so two
 *    archives with a genuine circular dependency (A's only definition of
 *    some symbol needs B, and vice versa) fail a single left-to-right
 *    pass unless bracketed. resolve_archives() in link_elf.c now always
 *    re-scans every archive to a fixed point (a strict superset of what
 *    --start-group buys you), so recognizing the flag as a no-op and
 *    exercising the circular case is enough to prove it works.
 * 2. -Wl,--as-needed: a linked-but-unreferenced shared library must not
 *    get a DT_NEEDED entry -- otherwise a program that merely happens to
 *    pass `-lfoo` on its link line (a common autotools LIBS artifact)
 *    carries a runtime dependency on libfoo.so it never actually calls
 *    into.
 *
 * Both scenarios assert the native path was actually taken (RCC_LINK_DEBUG
 * output must not mention falling back), not merely that *some* linker
 * eventually produced a working binary -- an external-linker fallback
 * would also pass a purely functional check. ELF/Linux-only: DT_NEEDED
 * introspection and RCC_LINK_DEBUG's own "falling back" trace are both
 * specific to link_elf.c; on other platforms this is a clean skip.
 */
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

static int run_capture(const char *cmd, char *buf, size_t bufsz, int *rc_out) {
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    size_t n = fread(buf, 1, bufsz - 1, p);
    buf[n] = 0;
    *rc_out = pclose(p);
    return 0;
}

int main(void) {
    char rcc_abs[1024];
    const char *rcc_raw = find_rcc();
    const char *rcc = realpath(rcc_raw, rcc_abs) ? rcc_abs : rcc_raw;
    const char *td = get_tmpdir();
    if (chdir(td) != 0) {
        printf("FAIL: cannot chdir to %s\n", td);
        return 9;
    }
    int pid = (int)getpid();
    char buf[8192];
    int rc;

    /* 1. --start-group: liba's ga_call_b() needs gb_fn() from libb, and
     * libb's gb_call_a() needs ga_fn() from liba -- a genuine circular
     * archive dependency neither side can resolve alone. */
    {
        char name_a[32], name_b[32];
        snprintf(name_a, sizeof(name_a), "grp%d_a", pid);
        snprintf(name_b, sizeof(name_b), "grp%d_b", pid);
        char ga_c[64], gb_c[64], ga_o[64], gb_o[64], liba[64], libb[64], gmain_c[64], gprog[64];
        snprintf(ga_c, sizeof(ga_c), "%s.c", name_a);
        snprintf(gb_c, sizeof(gb_c), "%s.c", name_b);
        snprintf(ga_o, sizeof(ga_o), "%s.o", name_a);
        snprintf(gb_o, sizeof(gb_o), "%s.o", name_b);
        snprintf(liba, sizeof(liba), "lib%s.a", name_a);
        snprintf(libb, sizeof(libb), "lib%s.a", name_b);
        snprintf(gmain_c, sizeof(gmain_c), "grp%d_main.c", pid);
        snprintf(gprog, sizeof(gprog), "grp%d_prog", pid);

        FILE *f = fopen(ga_c, "w");
        if (!f) { printf("FAIL: write %s\n", ga_c); return 1; }
        fputs("extern int gb_fn(void);\nint ga_fn(void){return 1;}\n"
              "int ga_call_b(void){return gb_fn()+1;}\n", f);
        fclose(f);
        f = fopen(gb_c, "w");
        if (!f) { printf("FAIL: write %s\n", gb_c); return 1; }
        fputs("extern int ga_fn(void);\nint gb_fn(void){return 2;}\n"
              "int gb_call_a(void){return ga_fn()+2;}\n", f);
        fclose(f);

        char cmd[900];
        snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, ga_c, ga_o);
        if (system(cmd) != 0) { printf("FAIL: compile %s\n", ga_c); return 1; }
        snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, gb_c, gb_o);
        if (system(cmd) != 0) { printf("FAIL: compile %s\n", gb_c); return 1; }
        remove(ga_c);
        remove(gb_c);

        snprintf(cmd, sizeof(cmd), "ar rcs %s %s " NULL_REDIRECT, liba, ga_o);
        if (system(cmd) != 0) { printf("FAIL: ar %s\n", liba); return 1; }
        snprintf(cmd, sizeof(cmd), "ar rcs %s %s " NULL_REDIRECT, libb, gb_o);
        if (system(cmd) != 0) { printf("FAIL: ar %s\n", libb); return 1; }
        remove(ga_o);
        remove(gb_o);

        f = fopen(gmain_c, "w");
        if (!f) { printf("FAIL: write %s\n", gmain_c); return 1; }
        fputs("extern int ga_call_b(void);\nint main(void){return ga_call_b()==3?0:1;}\n", f);
        fclose(f);

        snprintf(cmd, sizeof(cmd),
                 "RCC_LINK_DEBUG=1 %s %s -L. -Wl,--start-group -l%s -l%s -Wl,--end-group -o %s 2>&1",
                 rcc, gmain_c, name_a, name_b, gprog);
        if (run_capture(cmd, buf, sizeof(buf), &rc) != 0) { printf("FAIL: popen\n"); return 2; }
        remove(gmain_c);
        remove(liba);
        remove(libb);
        if (rc != 0) {
            printf("FAIL: --start-group circular archive link failed (rc=%d)\n%s\n", rc, buf);
            return 3;
        }
        if (strstr(buf, "falling back")) {
            printf("FAIL: --start-group link used the external-linker fallback, not the native path\n%s\n", buf);
            remove(gprog);
            return 4;
        }
        char run_cmd[128];
        snprintf(run_cmd, sizeof(run_cmd), "./%s " NULL_REDIRECT, gprog);
        int run_rc = system(run_cmd);
        remove(gprog);
        if (run_rc != 0) {
            printf("FAIL: --start-group linked program returned wrong result (rc=%d)\n", run_rc);
            return 5;
        }
    }

    /* 2. --as-needed: libunused.so is on the link line but nothing calls
     * into it -- it must not appear in the output's DT_NEEDED list. */
    {
        char libname[32];
        snprintf(libname, sizeof(libname), "asn%d_unused", pid);
        char unused_c[64], unused_so[64], amain_c[64], aprog[64];
        snprintf(unused_c, sizeof(unused_c), "%s.c", libname);
        snprintf(unused_so, sizeof(unused_so), "lib%s.so", libname);
        snprintf(amain_c, sizeof(amain_c), "asn%d_main.c", pid);
        snprintf(aprog, sizeof(aprog), "asn%d_prog", pid);

        FILE *f = fopen(unused_c, "w");
        if (!f) { printf("FAIL: write %s\n", unused_c); return 6; }
        fputs("int asn_unused_fn(void){return 7;}\n", f);
        fclose(f);
        char cmd[900];
        snprintf(cmd, sizeof(cmd), "%s -shared -fPIC %s -o %s " NULL_REDIRECT, rcc, unused_c, unused_so);
        if (system(cmd) != 0) { printf("FAIL: build %s\n", unused_so); return 6; }
        remove(unused_c);

        f = fopen(amain_c, "w");
        if (!f) { printf("FAIL: write %s\n", amain_c); return 6; }
        fputs("int main(void){return 0;}\n", f);
        fclose(f);

        snprintf(cmd, sizeof(cmd),
                 "RCC_LINK_DEBUG=1 %s %s -L. -Wl,--as-needed -l%s -o %s 2>&1",
                 rcc, amain_c, libname, aprog);
        if (run_capture(cmd, buf, sizeof(buf), &rc) != 0) { printf("FAIL: popen\n"); return 7; }
        remove(amain_c);
        remove(unused_so);
        if (rc != 0) {
            printf("FAIL: --as-needed link failed (rc=%d)\n%s\n", rc, buf);
            return 8;
        }
        if (strstr(buf, "falling back")) {
            printf("FAIL: --as-needed link used the external-linker fallback, not the native path\n%s\n", buf);
            remove(aprog);
            return 9;
        }

        snprintf(cmd, sizeof(cmd), "readelf -d %s " NULL_REDIRECT, aprog);
        FILE *p = popen(cmd, "r");
        remove(aprog);
        if (!p) { printf("FAIL: popen readelf\n"); return 10; }
        char dbuf[4096] = {0};
        size_t n = fread(dbuf, 1, sizeof(dbuf) - 1, p);
        dbuf[n] = 0;
        pclose(p);
        if (strstr(dbuf, libname)) {
            printf("FAIL: --as-needed still recorded a DT_NEEDED entry for the unreferenced library\n%s\n", dbuf);
            return 11;
        }
    }

    printf("OK\n");
    return 0;
}
#endif
