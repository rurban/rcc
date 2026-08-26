/* Real GCC/Clang predefine __STRICT_ANSI__ for an EXPLICIT strict
 * (non-gnu) `-std=cNN`/`-std=iso9899:*` request, never for the GNU-dialect
 * default (no -std= given at all) nor `-std=gnuNN`. rcc never defined it
 * at all, so a portable library header's `#if !defined(__STRICT_ANSI__)`
 * gate around GNU-only content (e.g. bare `typeof`, only ever a keyword in
 * GNU dialects or C23+) always evaluated true even under a strict -std=c11
 * request -- and since rcc (matching real GCC) also correctly REJECTS
 * bare `typeof` outside GNU/C23 mode, that combination hard-failed to
 * parse content the header itself believed it had already excluded.
 *
 * Found via CPython's Include/pymacro.h:
 *   #if (defined(__GNUC__) && !defined(__STRICT_ANSI__) && ...)
 *   #define Py_ARRAY_LENGTH(array) \
 *       (sizeof(array) / sizeof((array)[0]) + Py_BUILD_ASSERT_EXPR(
 *           !__builtin_types_compatible_p(typeof(array), typeof(&(array)[0]))))
 *   #else
 *   #define Py_ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))
 *   #endif
 * compiled with `-std=c11` (CPython's own real build flag): rcc entered
 * the typeof branch it should have skipped, then correctly rejected the
 * bare `typeof` it contains ("expected specific operator"), aborting the
 * whole Objects/call.c compile.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

static char *run_pp(const char *rcc, const char *extra_flags, const char *srcf, int pid, int tag) {
    char cmd[512];
    char outf[160];
    snprintf(outf, sizeof(outf), "%s/test_sat_out_%d_%d.txt", get_tmpdir(), pid, tag);
    snprintf(cmd, sizeof(cmd), "%s %s -E %s > %s " NULL_REDIRECT, rcc, extra_flags, srcf, outf);
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
    snprintf(srcf, sizeof(srcf), "%s/test_sat_%d.c", td, pid);

    static const char src[] =
        "#if defined(__STRICT_ANSI__)\n"
        "strict_ansi_defined\n"
        "#else\n"
        "strict_ansi_not_defined\n"
        "#endif\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    /* Explicit strict -std=c11: __STRICT_ANSI__ must be defined. */
    char *out1 = run_pp(rcc, "-std=c11", srcf, pid, 1);
    if (!out1 || !strstr(out1, "strict_ansi_defined") || strstr(out1, "strict_ansi_not_defined")) {
        printf("FAIL: -std=c11 should define __STRICT_ANSI__; got:\n%s\n", out1 ? out1 : "(pp failed)");
        remove(srcf);
        return 1;
    }

    /* -std=gnu11: GNU dialect, __STRICT_ANSI__ must NOT be defined. */
    char *out2 = run_pp(rcc, "-std=gnu11", srcf, pid, 2);
    if (!out2 || !strstr(out2, "strict_ansi_not_defined")) {
        printf("FAIL: -std=gnu11 should NOT define __STRICT_ANSI__; got:\n%s\n", out2 ? out2 : "(pp failed)");
        remove(srcf);
        return 2;
    }

    /* No -std= at all: GNU-dialect default, __STRICT_ANSI__ must NOT be
     * defined either (matches real GCC/Clang's default gnu17-family mode). */
    char *out3 = run_pp(rcc, "", srcf, pid, 3);
    if (!out3 || !strstr(out3, "strict_ansi_not_defined")) {
        printf("FAIL: default (no -std=) should NOT define __STRICT_ANSI__; got:\n%s\n", out3 ? out3 : "(pp failed)");
        remove(srcf);
        return 3;
    }

    remove(srcf);

    /* End-to-end: CPython's exact Py_ARRAY_LENGTH gate must take the
     * portable, typeof-free branch under -std=c11 and compile cleanly. */
    char srcf2[160];
    snprintf(srcf2, sizeof(srcf2), "%s/test_sat2_%d.c", td, pid);
    static const char src2[] =
        "#if (defined(__GNUC__) && !defined(__STRICT_ANSI__))\n"
        "#define ARRLEN(a) (sizeof(a)/sizeof((a)[0]) + (int)(!__builtin_types_compatible_p(typeof(a), typeof(&(a)[0]))))\n"
        "#else\n"
        "#define ARRLEN(a) (sizeof(a)/sizeof((a)[0]))\n"
        "#endif\n"
        "int main(void) { int arr[4]; return (int)ARRLEN(arr) == 4 ? 0 : 1; }\n";
    f = fopen(srcf2, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf2); return 1; }
    fputs(src2, f);
    fclose(f);

    char exef[160], cmd[512];
    snprintf(exef, sizeof(exef), "%s/test_sat2_%d", td, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif
    snprintf(cmd, sizeof(cmd), "%s -std=c11 -o %s %s " NULL_REDIRECT, rcc, exef, srcf2);
    int rc = system(cmd);
    remove(srcf2);
    if (rc != 0) {
        printf("FAIL: CPython-style Py_ARRAY_LENGTH gate under -std=c11 failed to compile\n");
        return 1;
    }
    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exef);
    int status = system(cmd);
    remove(exef);
#ifndef _WIN32
    int exit_code = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#else
    int exit_code = status;
#endif
    if (exit_code != 0) {
        printf("FAIL: CPython-style Py_ARRAY_LENGTH gate ran but gave wrong result (exit=%d)\n", exit_code);
        return 1;
    }

    printf("OK __STRICT_ANSI__ is defined only for an explicit strict "
           "-std=cNN, never for -std=gnuNN or the GNU-dialect default\n");
    return 0;
}
