/* Function-call argument-count checking (C11 6.5.2.2p6/p7).
 *
 * rcc previously accepted calls with fewer or more arguments than a
 * prototyped function declared -- only the implicit conversion casts in
 * cast_funcall_args() were checked, never the arity. Real GCC/Clang
 * hard-error on both. The leniency silently corrupted configure feature
 * probes that rely on the compile error to detect an ABI: GNU Emacs's
 * `pthread_setname_np takes a single argument` test compiled the one-arg
 * call `pthread_setname_np ("a")` against glibc's two-parameter
 * prototype, defined HAVE_PTHREAD_SETNAME_NP_1ARG, and every
 * make-thread then called `pthread_setname_np (p_name)` with the missing
 * pthread_t argument read from a garbage register -- SIGSEGV inside
 * glibc's strlen on the name pointer (seen in emacs test/thread-tests,
 * process-tests, emacs-module-tests and lisp/thread-tests).
 *
 * Fixed by counting declared parameters vs. call arguments in
 * cast_funcall_args() and erroring on a mismatch unless the function is
 * variadic (which accepts extra trailing arguments) or old-style K&R
 * (which has no prototype to check against).
 */
#include <stdio.h>
#include "test_common.h"

static int run(const char *rcc, const char *src_name, const char *src) {
    char path[1200], cmd[1600];
    snprintf(path, sizeof(path), "%s/%s", get_tmpdir(), src_name);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(src, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s/%s.o " NULL_REDIRECT,
             rcc, path, get_tmpdir(), src_name);
    int rc = system(cmd);
    remove(path);
    return rc;
}

int main(void) {
    const char *rcc = find_rcc();

    /* Control: correct arity against a full prototype must compile. */
    if (run(rcc, "a_ok.c", "int f(int a, char *b);\nint main(void){ return f(1, 0); }\n") != 0) {
        printf("FAIL: correct-arity call wrongly rejected\n");
        return 2;
    }

    /* Too few arguments must error (the pthread_setname_np case). */
    if (run(rcc, "a_few.c", "int f(int a, char *b);\nint main(void){ return f(1); }\n") == 0) {
        printf("FAIL: too-few-arguments call wrongly accepted\n");
        return 3;
    }

    /* Too many arguments must error too. */
    if (run(rcc, "a_many.c", "int f(int a);\nint main(void){ return f(1, 2); }\n") == 0) {
        printf("FAIL: too-many-arguments call wrongly accepted\n");
        return 4;
    }

    /* Variadic functions accept extra trailing arguments. */
    if (run(rcc, "a_var.c", "int f(int a, ...);\nint main(void){ return f(1, 2, 3); }\n") != 0) {
        printf("FAIL: variadic call with extra args wrongly rejected\n");
        return 5;
    }

    /* Variadic functions still need their named parameters. */
    if (run(rcc, "a_varfew.c", "int f(int a, ...);\nint main(void){ return f(); }\n") == 0) {
        printf("FAIL: variadic call missing named params wrongly accepted\n");
        return 6;
    }

    /* Old-style (no-prototype) calls keep their traditional leniency. */
    if (run(rcc, "a_old.c", "int f();\nint main(void){ return f(1, 2, 3); }\n") != 0) {
        printf("FAIL: old-style call with extra args wrongly rejected\n");
        return 7;
    }

    printf("OK\n");
    return 0;
}
