/* parse_kr_param_list() (parser.c) parses an old-style (K&R) function's
 * parameter-name list and its following declaration-list, looping
 * `while (!equalc(tok, "{"))` to consume each `type name;` declaration
 * until the function body's opening brace. Malformed top-level input
 * that gets misdetected as a K&R-style function head (e.g. an
 * unrecognized leading identifier followed by `(...)`, then more tokens
 * that never actually reach a `{`) ran this loop straight into the
 * token stream's end: declspec()/declarator() called on the trailing
 * EOF sentinel token silently fail to consume it (EOF isn't a valid
 * type-specifier token), so `tok` stayed pinned at EOF forever while the
 * loop kept iterating -- and declarator() unconditionally reads
 * `tok->next` a few lines in, which is NULL for the lexer's genuine
 * end-of-list EOF token, segfaulting several calls deeper inside
 * skip_attributes()/read_type_attrs().
 *
 * Found while fixing __declspec(...) to no longer be silently accepted
 * on native Linux (see test_declspec_native_reject.c): once __declspec
 * stopped being consumed as a recognized attribute, `__declspec(dllimport)
 * int foo(void);` got misparsed as a bogus K&R function named
 * `__declspec` with one old-style parameter `dllimport` — and the
 * trailing `int foo(void);` (not a `{}` body) drove this exact crash.
 * Reduced to a minimal repro independent of __declspec entirely: any
 * unrecognized identifier followed by a K&R-shaped parameter list and a
 * declaration that never reaches `{` triggers it.
 *
 * Fixed by diagnosing a clean "expected '{' before end of input" error
 * the moment the declaration-list loop reaches TK_EOF, instead of
 * looping into it.
 */
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

// Returns the real exit code on a clean process exit, or -1 if the
// process died from a signal (crash) -- distinguishing "compiled with a
// clean diagnostic" from "crashed internally", which a raw nonzero
// system() status alone can't tell apart (both are nonzero).
static int compile(const char *rcc, const char *src, const char *obj) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
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

    /* `bogus(name)` is not a recognized keyword, not a typedef, so it's
     * misread as an implicit-int K&R function head with one old-style
     * parameter `name`. The following `int foo(void);` never provides a
     * `{` body for it -- must be a clean syntax error, not a crash. */
    snprintf(src, sizeof(src), "%s/test_kr_eof_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_kr_eof_%d.o", td, pid);
    if (!write_file(src, "bogus(name)\nint foo(void);\n"))
        return 1;
    int rc = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc == 0) {
        printf("FAIL: malformed K&R-shaped input wrongly compiled cleanly\n");
        return 2;
    }
    if (rc == -1 || rc >= 128) {
        // -1: the intermediate /bin/sh wrapper itself was signal-killed
        // (rare). >=128: system()'s underlying /bin/sh -c reports its own
        // child (rcc) dying from a signal via the standard shell
        // convention of exiting normally with status 128+signum (e.g. 139
        // for SIGSEGV) -- WIFEXITED/WEXITSTATUS alone can't tell that
        // apart from a genuine "exit(139)" call, but rcc's own
        // error_tok()-driven diagnostic exit codes are always small
        // (1), so >=128 unambiguously means the process died from a
        // signal, not a clean diagnostic.
        printf("FAIL: malformed K&R-shaped input crashed instead of a clean "
               "diagnostic (rc=%d)\n", rc);
        return 3;
    }

    /* A genuine, well-formed K&R (old-style) function definition must
     * still compile, link, and run correctly (the fix must not reject
     * valid code). */
    snprintf(src, sizeof(src), "%s/test_kr_valid_%d.c", td, pid);
    if (!write_file(src,
        "int add(a, b)\n"
        "int a, b;\n"
        "{\n"
        "    return a + b;\n"
        "}\n"
        "int main(void) { return add(1, 2) == 3 ? 0 : 1; }\n")) {
        return 4;
    }
    char exe[750], run_cmd[900], link_cmd[1300];
#ifdef _WIN32
    snprintf(exe, sizeof(exe), "%s\\test_kr_valid_%d.exe", td, pid);
#else
    snprintf(exe, sizeof(exe), "%s/test_kr_valid_%d", td, pid);
#endif
    snprintf(link_cmd, sizeof(link_cmd), "%s %s -o %s " NULL_REDIRECT, rcc, src, exe);
    int link_rc = system(link_cmd);
    remove(src);
    if (link_rc != 0) {
        printf("FAIL: a genuine K&R function definition failed to compile/link "
               "(rc=%d)\n", link_rc);
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
        printf("FAIL: a genuine K&R function definition ran incorrectly (rc=%d)\n", run_rc);
        return 6;
    }

    printf("OK\n");
    return 0;
}
