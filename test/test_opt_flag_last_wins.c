/* Optimization-level flags (-O0/-O1/-O2/-O3) are not additive: like every
 * real C compiler, whichever "-On" appears LAST on the command line is the
 * one that takes effect, overriding any earlier one. rcc instead treated
 * opt_O1 (and the inlining/unrolling it implies) as a sticky flag that,
 * once set by an earlier -O2/-O3, was never cleared by a later -O0 — so
 * __OPTIMIZE__ stayed defined regardless of command-line order.
 *
 * Found via a real Linux kernel build: crypto/jitterentropy.c has
 *   #ifdef __OPTIMIZE__
 *   #error "The CPU Jitter random number generator must not be compiled
 *            with optimizations. ... Use the compiler switch -O0 ..."
 *   #endif
 * and crypto/Makefile sets "CFLAGS_jitterentropy.o = -O0" specifically to
 * satisfy this — kbuild's per-file CFLAGS mechanism appends that -O0
 * AFTER the whole-build "-O2" already on the command line, giving rcc
 * "... -O2 ... -O0 -c -o jitterentropy.o jitterentropy.c". Since -O2 had
 * already flipped opt_O1 on and rcc's -O0 handler never cleared it,
 * __OPTIMIZE__ stayed defined and the file's own safety #error fired.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_optlast_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_optlast_%d.o", td, pid);

    static const char src[] =
        "#ifdef __OPTIMIZE__\n"
        "#error \"__OPTIMIZE__ must not be defined when -O0 is the last -O flag\"\n"
        "#endif\n"
        "int main(void) { return 0; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    /* -O2 then -O0, matching kbuild's real ordering (whole-build -O2
     * followed by a per-file CFLAGS_x.o = -O0 override): must compile
     * clean, __OPTIMIZE__ must not be defined. */
    snprintf(cmd, sizeof(cmd), "%s -O2 -O0 -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(objf);
    if (rc != 0) {
        printf("FAIL: -O2 -O0 (O0 last) should compile without __OPTIMIZE__, rc=%d\n", rc);
        remove(srcf);
        return 2;
    }

    /* Sanity check the other direction: -O0 then -O2 (O2 last) must still
     * define __OPTIMIZE__ (the #error must fire, i.e. compile must fail). */
    snprintf(cmd, sizeof(cmd), "%s -O0 -O2 -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    rc = system(cmd);
    remove(objf);
    remove(srcf);
    if (rc == 0) {
        printf("FAIL: -O0 -O2 (O2 last) should define __OPTIMIZE__ and fail to compile\n");
        return 3;
    }

    printf("OK last -O flag on the command line wins\n");
    return 0;
}
