/* #include_next: resolve_include_next() (src/preprocess.c) continues the
 * search using build_search_dirs()'s combined list (RCC_INCDIR, then
 * every user -I directory, then the real system include dirs), starting
 * right after wherever the *current* file was found. For a header
 * reached through RCC_INCDIR (rcc's own bundled headers, e.g.
 * include/limits.h forwarding to the platform's real <limits.h> for
 * POSIX/XSI macros and platform internals it deliberately doesn't
 * define itself), that used to mean walking straight through every
 * user -I directory before ever reaching the system dirs - so a project
 * that happens to place a same-named header of its own in one of those
 * -I directories (a legitimate, unrelated file, e.g. a build-system
 * wrapper) got picked up INSTEAD of the real system header, and
 * #include_next's whole point - reaching past rcc's own header to the
 * platform's genuine one - silently failed.
 *
 * Fixed by having resolve_include_next() skip every user -I directory
 * entirely when continuing from RCC_INCDIR (or its "include" fallback),
 * landing directly in the real system include chain - matching how GCC
 * never lets an unrelated project header shadow its own private
 * "fixed" includes.
 *
 * Found via ksh93's own libast headers: ast_wchar.h's
 * `#include <../include/wchar.h>` resolves (through RCC_INCDIR, since
 * `RCC_INCDIR/../include/wchar.h` collapses right back to
 * RCC_INCDIR/wchar.h) to rcc's own include/wchar.h, whose own
 * `#include_next <wchar.h>` must reach glibc's <wchar.h> for wint_t -
 * but ksh93's own build passes `-Istd -I.../src/lib/libast/std`, and
 * that directory happens to contain its own libast wchar.h wrapper
 * (itself just `#include <ast_wchar.h>`, guarded right back into a
 * no-op by ast_wchar.h's own include guard) - previously found and
 * used FIRST, leaving wint_t permanently undeclared. Reproduced here
 * with rcc's own bundled <limits.h> (unconditionally chains onward via
 * `#include_next <limits.h>` on every platform, unlike <wchar.h> whose
 * downstream typedefs have their own unrelated platform-specific gaps)
 * and a decoy "limits.h" of our own in a plain -I directory, standing
 * in for ksh93's std/wchar.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

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

    /* A decoy "limits.h" that -I finds ahead of the system dirs. If
     * #include_next inside rcc's own bundled <limits.h> ever stops
     * here instead of reaching the real platform <limits.h>, this
     * fires and the compile below fails. */
    FILE *f = fopen(decoy, "w");
    if (!f) { printf("FAIL: cannot write %s\n", decoy); return 2; }
    fputs("#error decoy limits.h must never be reached\n", f);
    fclose(f);

    f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); return 3; }
    fputs("#include <limits.h>\nint main(void){return INT_MAX > 0 ? 0 : 1;}\n", f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -I%s -c %s -o %s " NULL_REDIRECT,
             rcc, dir, src, obj);
    int rc = system(cmd);

    remove(src);
    remove(decoy);
    remove(obj);
    remove(dir);

    if (rc != 0) {
        printf("FAIL: #include_next <limits.h> did not reach the real system header (rc=%d)\n", rc);
        return 4;
    }
    printf("OK\n");
    return 0;
}
