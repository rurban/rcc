/* glibc's /usr/include/limits.h ends with `#include_next <limits.h>`
 * (guarded on __GNUC__ && !_GCC_LIMITS_H_) to chain to the compiler's
 * own limits.h, and /usr/include/stdlib.h pulls wchar_t via the
 * `#define __need_wchar_t` + `#include <stddef.h>` protocol. Both only
 * fire when the REAL glibc headers are used, i.e. when a user -I dir
 * (like tclConfig.sh's -I/usr/include) precedes RCC_INCDIR — see
 * test_angle_include_precedes_bundled.c for why -I dirs come first.
 * Two rcc bugs broke that chain:
 *
 * 1. resolve_include_next()'s start-scan matched cur_dir against every
 *    dirs[] entry and let the LAST match win: /usr/include appears
 *    both as the -I dir and in sys_include_paths, so the duplicate
 *    match bumped start past RCC_INCDIR entirely -> "include file
 *    'limits.h' not found" and INT_MAX et al. never got defined
 *    (sqlite's shell.c failed on INT_MAX; make sqlite3/tclsqlite3
 *    with rcc reproduced it).
 * 2. rcc's bundled stddef.h inverted the __need_wchar_t protocol:
 *    the typedef was SKIPPED instead of PROVIDED when __need_wchar_t
 *    was set, so wchar_t fell back to int and glibc's
 *    mbtowc()/wctomb()/mbstowcs() declarations failed to parse
 *    ("type defaults to int" / "expected specific operator").
 *
 * Linux/glibc only: neither glibc header exists on macOS/Windows.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

int main(void) {
#if !defined(__linux__)
    (void)0; /* glibc-only: nothing to check elsewhere */
#else
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_glibc_next_%d.d", td, pid);
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
    char src[600], exe[600], cmd[2600];
    snprintf(src, sizeof(src), "%s/t.c", dir);
    snprintf(exe, sizeof(exe), "%s/t", dir);
    FILE *f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); rmdir(dir); return 1; }
    fputs("#include <limits.h>\n"
          "#include <stdlib.h>\n"
          "int main(void) {\n"
          "  wchar_t w = L'x';\n"
          "  return (INT_MAX == 2147483647 && sizeof(wchar_t) == 4\n"
          "          && mbtowc(&w, \"x\", 1) == 1) ? 0 : 1;\n"
          "}\n", f);
    fclose(f);
    /* -I/usr/include must come from the environment, not hardcoded
     * paths: glibc's limits.h/stdlib.h have to be the ones reached. */
    snprintf(cmd, sizeof(cmd), "%s -I/usr/include -o %s %s " NULL_REDIRECT,
             rcc, exe, src);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: rcc -I/usr/include could not compile limits.h/"
               "stdlib.h TU (exit %d)\n", rc);
        remove(src);
        rmdir(dir);
        return 2;
    }
    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exe);
    rc = system(cmd);
    remove(src);
    remove(exe);
    rmdir(dir);
    if (rc != 0) { printf("FAIL: compiled TU misbehaved (exit %d)\n", rc); return 3; }
    printf("OK\n");
#endif
    return 0;
}
