/* #include_next: resolve_include_next() (src/preprocess.c) continues the
 * search using build_search_dirs()'s combined list (RCC_INCDIR, then
 * every user -I directory, then the real system include dirs), starting
 * right after wherever the *current* file was found.
 *
 * Case 1: a trivial one-line forwarding wrapper in a user -I directory
 * must be skipped, not mistaken for the real next header. Found via
 * ksh93's own libast headers: ast_wchar.h's `#include <../include/
 * wchar.h>` resolves (through RCC_INCDIR) to rcc's own include/wchar.h,
 * whose own `#include_next <wchar.h>` must reach glibc's <wchar.h> for
 * wint_t/mbstate_t - but ksh93's own build passes `-Istd
 * -I.../src/lib/libast/std`, and that directory contains its own
 * libast wchar.h wrapper (itself just `#include <ast_wchar.h>`, which
 * -- since ast_wchar.h is already open, mid #include_next -- is a
 * no-op due to its own include guard). Naively accepting the first
 * match left wint_t permanently undeclared. Fixed by having
 * resolve_include_next() recognize this exact shape (a header whose
 * entire content, once comments/blank lines are stripped, is a single
 * #include resolving to a file *already active* on the include stack)
 * and skip past it to keep searching.
 *
 * Case 2 (the fix's own regression, caught fixing test_ksh93's __FILE
 * gap): that recognition must be narrow. A user -I directory can also
 * legitimately *replace* a bundled header with real, non-forwarding
 * content of its own -- e.g. ksh93's own std/stdio.h forwards to its
 * sfio-based ast_stdio.h, which is *not* already open and defines real
 * new content (__FILE, among others). A blanket "skip every user -I
 * dir when continuing from RCC_INCDIR" (an earlier, overly broad
 * version of this fix) silently threw that away too, leaving __FILE
 * undeclared in glibc's own <wchar.h> and breaking test_ksh93 further
 * down the line. This case reproduces it directly: a decoy "limits.h"
 * that provides real (non-forwarding) content of its own must still be
 * found and used, not skipped just because it lives in a user -I dir.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600], decoy[700], src[600], obj[700], cmd[2400];

    snprintf(dir, sizeof(dir), "%s/test_incnext_%d", td, pid);
    snprintf(decoy, sizeof(decoy), "%s/limits.h", dir);
    snprintf(src, sizeof(src), "%s/test_incnext_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_incnext_%d.o", td, pid);

    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        printf("FAIL: cannot create %s\n", dir);
        return 1;
    }

    /* Case 1: a trivial forwarder back to the already-open bundled
     * <limits.h> - #include_next must see straight through it to the
     * real system <limits.h>, not stop here (which would just re-open
     * rcc's own bundled header a second time). */
    if (!write_file(decoy, "#include <limits.h>\n")) {
        printf("FAIL: cannot write %s\n", decoy);
        return 2;
    }
    if (!write_file(src, "#include <limits.h>\nint main(void){return INT_MAX > 0 ? 0 : 1;}\n")) {
        printf("FAIL: cannot write %s\n", src);
        return 3;
    }
    snprintf(cmd, sizeof(cmd), "%s -I%s -c %s -o %s " NULL_REDIRECT,
             rcc, dir, src, obj);
    int rc = system(cmd);
    remove(src);
    remove(obj);
    if (rc != 0) {
        printf("FAIL: #include_next <limits.h> did not skip a trivial forwarder (rc=%d)\n", rc);
        remove(decoy);
        remove(dir);
        return 4;
    }

    /* Case 2: a *real*, non-forwarding replacement in the same -I
     * directory must still be found and used, not skipped just because
     * case 1's heuristic lives in the same function. */
    if (!write_file(decoy, "#define INCNEXT_REAL_OVERRIDE 1\n")) {
        printf("FAIL: cannot write %s\n", decoy);
        return 5;
    }
    if (!write_file(src,
        "#include <limits.h>\n"
        "#ifndef INCNEXT_REAL_OVERRIDE\n"
        "#error real -I limits.h override was skipped\n"
        "#endif\n"
        "int main(void){return 0;}\n")) {
        printf("FAIL: cannot write %s\n", src);
        remove(decoy);
        remove(dir);
        return 6;
    }
    snprintf(cmd, sizeof(cmd), "%s -I%s -c %s -o %s " NULL_REDIRECT,
             rcc, dir, src, obj);
    rc = system(cmd);

    remove(src);
    remove(decoy);
    remove(obj);
    remove(dir);

    if (rc != 0) {
        printf("FAIL: #include_next <limits.h> skipped a real -I override (rc=%d)\n", rc);
        return 7;
    }
    printf("OK\n");
    return 0;
}
