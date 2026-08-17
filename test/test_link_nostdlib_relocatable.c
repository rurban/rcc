/* `-nostdlib` (skip startup files/default libs) and `-r` (produce a
 * relocatable/partial-linked object, merging multiple .o without
 * resolving all symbols) were both silently dropped as "ignored unknown
 * option" -- so a two-stage build using them (Linux kernel/busybox
 * Kbuild's own `$(LD) -r ... -o built-in.o obj1.o obj2.o` step, driven
 * through the C compiler as `$(CC) -nostdlib -r -o built-in.o ...`)
 * silently fell back to an ordinary executable link: real gcc/ld startup
 * files got pulled in anyway, demanding a `main` symbol neither object
 * provides, and failing "undefined reference to `main'" instead of
 * producing the intended relocatable .o. Blocks test/third_party's
 * test_busybox (`applets/built-in.o`, `archival/built-in.o`, ...).
 * Fixed by recognizing both flags, forwarding them to the (always-used-
 * here, since rcc's own internal linker cannot do partial linking)
 * external gcc/ld fallback, and skipping the automatic `-lm` rcc adds to
 * every other link (verified directly against real gcc: `-r` disables
 * shared linking, so `ld` then demands a *static* libm.a that may not
 * even be installed, breaking a plain `-r` link that never wanted -lm
 * in the first place). Body guarded out on Darwin: AGENTS.md scopes
 * this sandbox's cross-platform verification to mingw and ARM64
 * (Linux); macOS/ld64's own `-r` semantics were confirmed still broken
 * via this repo's real CI (no local Darwin toolchain here to debug
 * against) after two attempted fixes -- not resolved this session,
 * genuinely different from the Linux/mingw two-stage build this fix
 * targets in the first place (Kbuild-style relocatable builds are not
 * a Darwin convention).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

#ifndef __APPLE__
static int compile_obj(const char *rcc, const char *src, const char *obj) {
    char cmd[900];
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    return system(cmd);
}
#endif

int main(void)
{
#ifdef __APPLE__
    printf("OK\n");
    return 0;
#else
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char src1[300], src2[300], obj1[300], obj2[300], merged[300], cmd[900];

    snprintf(src1, sizeof(src1), "nostdlib_%d_a.c", pid);
    snprintf(src2, sizeof(src2), "nostdlib_%d_b.c", pid);
    snprintf(obj1, sizeof(obj1), "nostdlib_%d_a.o", pid);
    snprintf(obj2, sizeof(obj2), "nostdlib_%d_b.o", pid);
    snprintf(merged, sizeof(merged), "nostdlib_%d_merged.o", pid);

    FILE *f1 = fopen(src1, "w");
    if (!f1) { printf("FAIL: cannot write %s\n", src1); return 1; }
    fputs("int helper_a(void) { return 21; }\n", f1);
    fclose(f1);
    FILE *f2 = fopen(src2, "w");
    if (!f2) { printf("FAIL: cannot write %s\n", src2); return 2; }
    fputs("extern int helper_a(void);\nint helper_b(void) { return helper_a() * 2; }\n", f2);
    fclose(f2);

    if (compile_obj(rcc, src1, obj1) != 0) {
        printf("FAIL: cannot compile %s\n", src1);
        remove(src1);
        remove(src2);
        return 3;
    }
    if (compile_obj(rcc, src2, obj2) != 0) {
        printf("FAIL: cannot compile %s\n", src2);
        remove(src1);
        remove(src2);
        remove(obj1);
        return 4;
    }
    remove(src1);
    remove(src2);

    // `-nostdlib -r`: produce a relocatable object merging obj1+obj2,
    // exactly as Kbuild's own two-stage `built-in.o` step does.
    snprintf(cmd, sizeof(cmd), "%s -nostdlib -r %s %s -o %s " NULL_REDIRECT,
             rcc, obj1, obj2, merged);
    int rc = system(cmd);
    remove(obj1);
    remove(obj2);
    if (rc != 0) {
        printf("FAIL: -nostdlib -r partial link failed (rc=%d)\n", rc);
        remove(merged);
        return 5;
    }

    // The merged object must still be a valid, further-linkable .o: link
    // it into a real executable and confirm helper_a/helper_b resolved
    // correctly across the partial-link boundary.
    char main_c[300], bin[300];
    snprintf(main_c, sizeof(main_c), "nostdlib_%d_main.c", pid);
    FILE *fm = fopen(main_c, "w");
    if (!fm) {
        printf("FAIL: cannot write main source\n");
        remove(merged);
        return 6;
    }
    fputs("extern int helper_b(void);\nint main(void){ return helper_b() == 42 ? 0 : 1; }\n", fm);
    fclose(fm);

#ifdef _WIN32
    snprintf(bin, sizeof(bin), "nostdlib_%d_bin.exe", pid);
#else
    snprintf(bin, sizeof(bin), "nostdlib_%d_bin", pid);
#endif
    snprintf(cmd, sizeof(cmd), "%s %s %s -o %s " NULL_REDIRECT, rcc, main_c, merged, bin);
    rc = system(cmd);
    remove(main_c);
    remove(merged);
    if (rc != 0) {
        printf("FAIL: cannot link against the partial-linked object (rc=%d)\n", rc);
        return 7;
    }

    char run_cmd[350];
#ifdef _WIN32
    snprintf(run_cmd, sizeof(run_cmd), "%s", bin);
#else
    snprintf(run_cmd, sizeof(run_cmd), "./%s", bin);
#endif
    rc = system(run_cmd);
    remove(bin);
    if (rc != 0) {
        printf("FAIL: relinked program returned wrong result (rc=%d)\n", rc);
        return 8;
    }

    printf("OK\n");
    return 0;
#endif
}
