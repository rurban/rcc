/* C29 (WG14 N3353): the traditional leading-zero octal spelling (`0123`)
 * is marked obsolescent -- rcc keeps accepting it (there are no plans to
 * remove it), but under `-pedantic` warns to nudge toward `0o123`,
 * gated by the same `-Wno-c23-c2y-compat` flag as the other C2Y-vs-C23
 * compat diagnostics. This must all happen via a subprocess: the
 * diagnostic is stderr text, not something observable from inside the
 * compiled program itself.
 */
#include "test_common.h"
#include <string.h>

static int compile_capture(const char *rcc, const char *srcf, const char *objf,
                            const char *extra_flags, char *out, size_t outsz)
{
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s -c -o %s %s 2>&1", rcc, extra_flags, objf, srcf);
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    size_t n = fread(out, 1, outsz - 1, p);
    out[n] = '\0';
    return pclose(p);
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char srcf[256], objf[256];
    snprintf(srcf, sizeof(srcf), "%s/test_c29_octal_obs_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_c29_octal_obs_%d.o", td, pid);

    static const char src[] = "int main(void) { int x = 0123; return x; }\n";
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    char out[2048];

    /* Default (no -pedantic): legacy octal accepted silently. */
    int rc = compile_capture(rcc, srcf, objf, "", out, sizeof(out));
    remove(objf);
    if (rc != 0) {
        printf("FAIL: plain '0123' should still compile: %s\n", out);
        remove(srcf);
        return 2;
    }
    if (strstr(out, "obsolescent")) {
        printf("FAIL: unexpected obsolescence warning without -pedantic: %s\n", out);
        remove(srcf);
        return 3;
    }

    /* -pedantic: must warn, but still compile (obsolescence is not a
     * constraint violation). */
    rc = compile_capture(rcc, srcf, objf, "-pedantic", out, sizeof(out));
    remove(objf);
    if (rc != 0) {
        printf("FAIL: -pedantic '0123' should still compile: %s\n", out);
        remove(srcf);
        return 4;
    }
    if (!strstr(out, "obsolescent")) {
        printf("FAIL: -pedantic did not warn about legacy octal: %s\n", out);
        remove(srcf);
        return 5;
    }

    /* -pedantic -Wno-c23-c2y-compat: warning suppressed. */
    rc = compile_capture(rcc, srcf, objf, "-pedantic -Wno-c23-c2y-compat", out, sizeof(out));
    remove(objf);
    if (rc != 0) {
        printf("FAIL: -pedantic -Wno-c23-c2y-compat '0123' should still compile: %s\n", out);
        remove(srcf);
        return 6;
    }
    if (strstr(out, "obsolescent")) {
        printf("FAIL: -Wno-c23-c2y-compat did not suppress the warning: %s\n", out);
        remove(srcf);
        return 7;
    }

    /* Plain "0" is never obsolescent (it's the only spelling of zero). */
    static const char src0[] = "int main(void) { int x = 0; return x; }\n";
    f = fopen(srcf, "w");
    fputs(src0, f);
    fclose(f);
    rc = compile_capture(rcc, srcf, objf, "-pedantic", out, sizeof(out));
    remove(objf);
    remove(srcf);
    if (rc != 0 || strstr(out, "obsolescent")) {
        printf("FAIL: plain '0' wrongly flagged as obsolescent: %s\n", out);
        return 8;
    }

    return 0;
}
