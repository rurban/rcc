/* An object-like macro invoked bare at file/top scope (not inside another
 * macro's argument list or body) that is immediately followed, with no
 * source whitespace, by a literal token must preserve that tightness in
 * `-E` output, exactly like real GCC/Clang: "MACRO)" with MACRO expanding
 * to "(1)" must render as "(1))", never "(1) )".
 *
 * Root cause: expand_token()'s object-like-macro branch determined this
 * tightness by comparing `t` (the invocation token) against `t->next` via
 * str_needs_space() — but `t->next` is only ever populated for tokens
 * pulled from an already-built list (a macro body or an already-collected
 * argument list). A macro invoked at the top level is read ONE TOKEN AT A
 * TIME straight from the file via pp_next_raw(): its sibling token hasn't
 * even been lexed yet when expand_token() runs, so `t->next` was always
 * NULL there, and the check silently defaulted to "needs a space".
 *
 * Found via micropython's py/qstrdefs.h: `QCFG(BYTES_IN_LEN,
 * MICROPY_QSTR_BYTES_IN_LEN)` where `MICROPY_QSTR_BYTES_IN_LEN` expands to
 * `(1)` — QCFG itself is plain text (parsed by makeqstrdata.py, not a C
 * macro), so the whole invocation sits at top scope. rcc's -E output came
 * out "QCFG(BYTES_IN_LEN, (1) )" (spurious space before the final ')'),
 * which makeqstrdata.py's `^QCFG\((.+), (.+)\)` regex parses into a value
 * of "(1) " (trailing space) instead of "(1)" — its paren-stripping check
 * (`value[-1] == ')'`) then fails, and `int("(1) ")` throws, aborting the
 * whole micropython build.
 *
 * Fixed via tok_tight_after(): a direct source-buffer check (does the byte
 * immediately following `t`'s own spelling exist and is it non-
 * whitespace?) that needs no `->next` link at all, so it works uniformly
 * whether `t` came from a pre-built list or a fresh top-level raw read.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

static char *run_pp(const char *rcc, const char *srcf, int pid) {
    char cmd[512];
    char outf[160];
    snprintf(outf, sizeof(outf), "%s/test_ppoma_out_%d.txt", get_tmpdir(), pid);
    snprintf(cmd, sizeof(cmd), "%s -E %s > %s " NULL_REDIRECT, rcc, srcf, outf);
    int rc = system(cmd);
    if (rc != 0) { remove(outf); return NULL; }
    FILE *f = fopen(outf, "r");
    remove(outf);
    if (!f) return NULL;
    static char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[160];
    snprintf(srcf, sizeof(srcf), "%s/test_ppoma_%d.c", td, pid);

    static const char src[] =
        "#define MICROPY_QSTR_BYTES_IN_LEN (1)\n"
        "QCFG(BYTES_IN_LEN, MICROPY_QSTR_BYTES_IN_LEN)\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    char *out = run_pp(rcc, srcf, pid);
    remove(srcf);
    if (!out) { printf("FAIL: -E failed\n"); return 1; }

    if (strstr(out, "(1) )")) {
        printf("FAIL: spurious space before closing paren; -E output:\n%s\n", out);
        return 1;
    }
    if (!strstr(out, "QCFG(BYTES_IN_LEN, (1))")) {
        printf("FAIL: expected \"QCFG(BYTES_IN_LEN, (1))\" in -E output:\n%s\n", out);
        return 1;
    }

    printf("OK an object-like macro invoked bare at top scope, immediately "
           "followed by a literal token with no source whitespace, keeps "
           "that tightness through -E\n");
    return 0;
}
