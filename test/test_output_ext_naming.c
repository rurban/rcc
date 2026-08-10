/* replace_ext() (src/main.c), which computes rcc's default `-c`/`-S`
 * output filename when no explicit `-o` is given, only stripped the
 * source's trailing extension for .c/.i/.s - any other accepted input
 * extension (rcc treats every input as compilable C regardless of
 * extension, so .C/.cc/.cxx/.cpp are all accepted) fell through to
 * literally appending ".o" to the WHOLE filename instead of replacing
 * the extension: `rcc -c foo.cxx` produced `foo.cxx.o`, not the
 * conventional `foo.o` every real compiler driver (gcc, clang, tcc)
 * produces regardless of which extension it dispatches on.
 *
 * Found via ksh93's own build-time C-compiler probe (src/cmd/INIT/C+probe):
 * it compiles a trial `test.cxx` (probing whether the extension is
 * accepted at all before falling back to `.c`), globs `test.*` to find
 * the produced object file, and requires it to be named exactly
 * `test.<ext>.o`-stripped, i.e. `test.o` after the probe reverts to
 * plain C - the malformed `test.cxx.o` name broke that glob-based
 * bookkeeping and made the probe misreport rcc as "not a C compiler".
 */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* rcc derives its default `-c` output name from the source's own
 * basename (stripping any directory component), so with no `-o` the
 * object always lands in the process's CWD - chdir into td once at
 * startup so every check below, and the plain-basename source/object
 * names it uses, land where we look for them without needing a shell
 * `cd &&` (whose quoting/path semantics differ between POSIX sh and
 * Windows cmd.exe). */
static int compile_check(const char *rcc, const char *ext) {
    char src[128], expect_obj[128], stray_obj[160], cmd[600];
    snprintf(src, sizeof(src), "test_extname.%s", ext);
    snprintf(expect_obj, sizeof(expect_obj), "test_extname.o");
    snprintf(stray_obj, sizeof(stray_obj), "test_extname.%s.o", ext);

    FILE *f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); return 1; }
    /* .s is fed to the assembler directly, not preprocessed as C - a
     * blank line is valid (empty) GAS input, unlike every other
     * extension here which needs real C source. */
    fputs(strcmp(ext, "s") == 0 ? "\n" : "int main(void){return 0;}\n", f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c %s " NULL_REDIRECT, rcc, src);
    int rc = system(cmd);
    remove(src);
    if (rc != 0) {
        printf("FAIL: -c %s failed (rc=%d)\n", ext, rc);
        remove(expect_obj);
        remove(stray_obj);
        return 2;
    }

    int has_expected = access(expect_obj, F_OK) == 0;
    int has_stray = access(stray_obj, F_OK) == 0;
    remove(expect_obj);
    remove(stray_obj);

    if (has_stray) {
        printf("FAIL: ext=%s produced foo.%s.o instead of foo.o\n", ext, ext);
        return 3;
    }
    if (!has_expected) {
        printf("FAIL: ext=%s produced no foo.o at all\n", ext);
        return 4;
    }
    return 0;
}

int main(void) {
    const char *rcc_raw = find_rcc();
    char rcc_abs[4096];
    /* find_rcc() may return a CWD-relative path ("./rcc"); resolve to
     * an absolute path first, since main() below chdir()s away from
     * wherever that relative path was rooted. */
#ifdef _WIN32
    const char *rcc = _fullpath(rcc_abs, rcc_raw, sizeof(rcc_abs)) ? rcc_abs : rcc_raw;
#else
    const char *rcc = realpath(rcc_raw, rcc_abs) ? rcc_abs : rcc_raw;
#endif
    const char *td = get_tmpdir();
    if (chdir(td) != 0) {
        printf("FAIL: cannot chdir to %s\n", td);
        return 5;
    }
    const char *exts[] = {"c", "C", "cc", "cxx", "cpp", "i", "s"};
    for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); i++) {
        int rc = compile_check(rcc, exts[i]);
        if (rc != 0) return rc;
    }
    printf("OK\n");
    return 0;
}
