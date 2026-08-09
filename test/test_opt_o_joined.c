/* GCC/Clang accept -o's argument either as a separate word ("-o file") or
 * joined directly onto the flag ("-ofile"), exactly like -I/-D/-U/-MF
 * already do in rcc's own driver (src/main.c). -o's own handler instead
 * required an exact "-o" argv element (`!strcmp`) with no joined-form
 * fallback, so any real "-ofile" invocation fell through to the generic
 * "ignored unknown option" catch-all: the compiler silently discarded
 * both the flag and its output path, compiled anyway, and wrote to
 * whatever default output name applied instead of the caller's chosen
 * path (or, with -c, produced no diagnostic at all while still leaving
 * a real .o at the wrong location).
 *
 * Found via test/third_party/test_samba's waf-based configure: every
 * "does this construct compile" probe invokes
 * `rcc ... test.c -c -o<path-to-hashed-tmpdir>/test.c.1.o`, and each one
 * silently failed to produce output at the path waf checked for,
 * cascading into spurious "header not found" / "does not build" results
 * throughout configure (visible symptom: bogus "no such member" errors
 * on perfectly valid code once waf's own dependency-file open then also
 * failed against the never-written object).
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
    snprintf(srcf, sizeof(srcf), "%s/test_ojoined_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_ojoined_%d.o", td, pid);

    static const char src[] = "int main(void) { return 0; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    /* Joined form: -c -o<path>, no space -- must compile and produce
     * exactly the requested object file. */
    remove(objf);
    snprintf(cmd, sizeof(cmd), "%s -c -o%s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: -o%s (joined) should compile, rc=%d\n", objf, rc);
        remove(srcf);
        return 2;
    }
    FILE *chk = fopen(objf, "rb");
    if (!chk) {
        printf("FAIL: -o%s (joined) did not create the requested object file\n", objf);
        remove(srcf);
        return 3;
    }
    fclose(chk);
    remove(objf);

    /* Separate form must keep working identically (regression guard). */
    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: -o %s (separate) should still compile, rc=%d\n", objf, rc);
        remove(srcf);
        return 4;
    }
    chk = fopen(objf, "rb");
    if (!chk) {
        printf("FAIL: -o %s (separate) did not create the requested object file\n", objf);
        remove(srcf);
        return 5;
    }
    fclose(chk);
    remove(objf);
    remove(srcf);

    printf("OK -oFILE (joined) and -o FILE (separate) both work\n");
    return 0;
}
