/* GCC PR104764: `sizeof (TYPE [ dim` with a malformed/unbracketed VLA
 * dimension expression that never closes the array bracket or the
 * sizeof's own paren before running into unrelated tokens (or EOF).
 *
 * unary()'s (src/parser.c) `sizeof (type) {...}` compound-literal
 * lookahead scans forward counting only `(`/`)` depth to find the
 * matching close paren, with no TK_EOF check. Malformed input whose
 * parens never balance (e.g. a stray `[` swallowing the sizeof's own
 * `)` into an array-dimension expression, or literally running past
 * the end of the token list) walked the loop straight through the
 * EOF sentinel and off the end of the token list, segfaulting on the
 * next `t->next` dereference or `equalc(NULL, ...)` call.
 *
 * Old gcc famously hung indefinitely on this exact shape (PR104764);
 * modern gcc reports a clean syntax error. rcc crashed instead of
 * either -- must report a clean diagnostic, not a crash.
 */
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

// See test_kr_param_list_eof.c's `compile()` for the rc encoding this
// mirrors: -1 or >=128 means the process died from a signal (crash).
static int compile(const char *rcc, const char *src, const char *obj) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -std=gnu89 -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    int status = system(cmd);
#ifndef _WIN32
    return (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#else
    return status; // Windows system() returns the exit code directly
#endif
}

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
    char src[600], obj[700];

    /* Reduced from PR104764's mutant.c: `sizeof ( int [ a}` never
     * closes the sizeof's own `(` (the `[` opens an array-dimension
     * expression that itself never sees a matching `]`) before the
     * enclosing function body's `}` -- and then the file ends without
     * ever balancing anything. Must be a clean syntax error. */
    snprintf(src, sizeof(src), "%s/test_sizeof_unbal_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_sizeof_unbal_%d.o", td, pid);
    if (!write_file(src,
        "static a();\n"
        "b(void) {sizeof ( int [ a}\n"
        "static c();\n"
        "d(void) {sizeof((int[c\n"))
        return 1;
    int rc = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc == 0) {
        printf("FAIL: malformed unbalanced sizeof(...) input wrongly compiled cleanly\n");
        return 2;
    }
    if (rc == -1 || rc >= 128) {
        printf("FAIL: malformed unbalanced sizeof(...) input crashed instead of "
               "a clean diagnostic (rc=%d)\n", rc);
        return 3;
    }

    /* A genuine, well-balanced `sizeof(TYPE[dim])` (VLA sizeof) must
     * still compile, link, and run correctly. */
    char exe[750], run_cmd[900], link_cmd[1300];
    const char *valid_src_text =
        "int f(int n) { return (int)sizeof(int[n]); }\n"
        "int main(void) { return f(3) == 12 ? 0 : 1; }\n";
    snprintf(src, sizeof(src), "%s/test_sizeof_valid_%d.c", td, pid);
#ifdef _WIN32
    snprintf(exe, sizeof(exe), "%s\\test_sizeof_valid_%d.exe", td, pid);
#else
    snprintf(exe, sizeof(exe), "%s/test_sizeof_valid_%d", td, pid);
#endif
    if (!write_file(src, valid_src_text)) return 4;
    snprintf(link_cmd, sizeof(link_cmd), "%s %s -o %s " NULL_REDIRECT, rcc, src, exe);
    int link_rc = system(link_cmd);
    remove(src);
    if (link_rc != 0) {
        printf("FAIL: a genuine sizeof(TYPE[dim]) VLA expression failed to "
               "compile/link (rc=%d)\n", link_rc);
        return 5;
    }
#ifdef _WIN32
    snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", exe);
#else
    snprintf(run_cmd, sizeof(run_cmd), "'%s'", exe);
#endif
    int run_rc = system(run_cmd);
    remove(exe);
    if (run_rc != 0) {
        printf("FAIL: a genuine sizeof(TYPE[dim]) VLA expression ran incorrectly "
               "(rc=%d)\n", run_rc);
        return 6;
    }

    printf("OK\n");
    return 0;
}
