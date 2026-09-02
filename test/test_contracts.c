/* GH issue #45: C contracts, following Jens Gustedt's "Contracts for C"
 * proposal (https://gustedt.wordpress.com/2025/03/10/contracts-for-c/),
 * minus the pre()/post() *statement* forms (contract_assert/contract_assume):
 * only the declarator-trailing `pre(EXPR)` / `post([NAME:] EXPR)`
 * contract-specifiers are supported, so the whole feature is hideable
 * behind a feature-test macro for other compilers with e.g.
 *   #ifndef __RCC__
 *   #define pre(...)
 *   #define post(...)
 *   #endif
 * (rcc already predefines __RCC__; see test_c23_features.c).
 *
 * A satisfied contract has zero *observable* effect: this file directly
 * exercises the happy paths (compiled and executed as ordinary rcc
 * output, exactly like every other test/test_*.c). A violated contract
 * must print a diagnostic to stderr and abort() the process, and an
 * always-false constant condition must be a compile error (like
 * static_assert) -- both are only observable from a subprocess, so
 * those cases shell out to a fresh `rcc` invocation, mirroring
 * test_c29_octal_obsolescent.c's compile_capture() helper.
 */
#include "test_common.h"
#include <string.h>
#include <signal.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif

// cmd.exe (system()/popen() on Windows go through it regardless of the
// launching shell) has no /dev/null; see test_common.h's NULL_REDIRECT
// comment for the full story (a bare "/dev/null" there aborts the whole
// redirection setup before the child even runs).
#ifdef _WIN32
#define STDOUT_NULL_REDIRECT "1>NUL"
#define DISCARD_ALL_REDIRECT "1>NUL 2>NUL"
#else
#define STDOUT_NULL_REDIRECT "1>/dev/null"
#define DISCARD_ALL_REDIRECT "1>/dev/null 2>/dev/null"
#endif

/* precondition + named postcondition binding, matching the blog's own
 * `void* my_malloc(size_t size) pre(size) post(r: r);` example. */
static char malloc_buf[64];
static void *my_malloc(long size) pre(size > 0 && size <= 64) post(r: r != 0) {
    return malloc_buf;
}

/* void function: unnamed postcondition, checked at the implicit
 * fallthrough exit (no explicit `return;`). */
static int side_effect_seen;
static void bump(int *p) pre(p != 0) post(side_effect_seen == 1) {
    side_effect_seen = 1;
    *p = *p + 1;
}

/* void function: unnamed postcondition, checked at an explicit early
 * `return;` as well as at fallthrough. */
static int early_exit_taken;
static void maybe_bump(int *p, int skip) pre(p != 0) post(side_effect_seen == 1) {
    if (skip) {
        early_exit_taken = 1;
        side_effect_seen = 1; /* still satisfy the postcondition on early exit */
        return;
    }
    side_effect_seen = 1;
    *p = *p + 1;
}

/* a constant-true precondition must compile away to nothing (Gustedt's
 * proposal: an integer-constant condition behaves like static_assert)
 * -- exercised here only for "doesn't miscompile", the elision itself
 * is verified indirectly by test_constant_false_precondition_is_error()
 * below (the constant-false twin errors out, proving both are actually
 * evaluated at compile time). */
static int always_ok(int x) pre(1) post(2) {
    return x;
}

/* prototype-only declaration with contracts: nothing to instrument (no
 * body here), must not error or affect the later definition (which
 * intentionally repeats its own, independent contracts -- rcc does not
 * merge contracts across declarations, see parser.c's
 * activate_function_contracts()). */
static int no_merge(int x) pre(x >= 0);
static int no_merge(int x) pre(x >= 0) {
    return x;
}

static int compile_capture(const char *rcc, const char *srcf, const char *objf,
                            const char *extra_flags, char *out, size_t outsz) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s -o %s %s 2>&1", rcc, extra_flags, objf, srcf);
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    size_t n = fread(out, 1, outsz - 1, p);
    out[n] = '\0';
    return pclose(p);
}

static int write_src(const char *path, const char *src) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(src, f);
    fclose(f);
    return 0;
}

#ifndef _WIN32
// system()'s status reflects the WRAPPING /bin/sh's own exit, not a
// signal delivered to sh itself: when the child it ran was killed by a
// signal, sh reports that as its own NORMAL exit with code 128+signum
// (the convention test_kr_param_list_eof.c also documents).
static int aborted_via_shell(int status) {
    return status >= 0 && WIFEXITED(status) && WEXITSTATUS(status) == 128 + SIGABRT;
}
#else
// Windows system() returns the child's exit code directly, without
// POSIX's signal-number encoding to check exactly; abort() there just
// needs to have terminated the process abnormally (nonzero).
static int aborted_via_shell(int status) {
    return status != 0;
}
#endif

/* Compile `src` (expected to compile cleanly) to `binf`, run it, and
 * return the raw popen()/pclose() wait status. */
static int run_capture(const char *rcc, const char *td, int pid, const char *src, int *compiled_ok) {
    char srcf[256], binf[256], out[512];
    snprintf(srcf, sizeof(srcf), "%s/test_contracts_run_%d.c", td, pid);
    snprintf(binf, sizeof(binf), "%s/test_contracts_run_%d.bin", td, pid);
    write_src(srcf, src);
    int rc = compile_capture(rcc, srcf, binf, "", out, sizeof(out));
    *compiled_ok = (rc == 0);
    int status = -1;
    if (rc == 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s " DISCARD_ALL_REDIRECT, binf);
        status = system(cmd);
    }
    remove(srcf);
    remove(binf);
    return status;
}

int main(void) {
    /* --- happy paths: run in-process, self-hosted by rcc --- */
    void *p = my_malloc(16);
    if (p != malloc_buf) { printf("FAIL: my_malloc precondition/postcondition rejected valid call\n"); return 1; }

    int v = 5;
    bump(&v);
    if (v != 6 || !side_effect_seen) { printf("FAIL: bump() postcondition rejected valid call\n"); return 2; }

    side_effect_seen = 0;
    int v2 = 5;
    maybe_bump(&v2, 1);
    if (!early_exit_taken || !side_effect_seen || v2 != 5) { printf("FAIL: maybe_bump() early-exit path broken\n"); return 3; }

    side_effect_seen = 0;
    maybe_bump(&v2, 0);
    if (v2 != 6 || !side_effect_seen) { printf("FAIL: maybe_bump() fallthrough path broken\n"); return 4; }

    if (always_ok(7) != 7) { printf("FAIL: always_ok() with constant-true contracts broken\n"); return 5; }
    if (no_merge(3) != 3) { printf("FAIL: no_merge() broken\n"); return 6; }

    /* --- violations: must abort() with a diagnostic naming the kind,
     * the condition, and the function --- */
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    int compiled_ok;
    int status = run_capture(rcc, td, pid,
        "int f(int x) pre(x > 0) { return x * 2; }\n"
        "int main(void) { return f(-1); }\n",
        &compiled_ok);
    if (!compiled_ok) { printf("FAIL: precondition-violation test source failed to compile\n"); return 10; }
    if (!aborted_via_shell(status)) {
        printf("FAIL: violated precondition did not abort() (status=%d)\n", status);
        return 11;
    }

    status = run_capture(rcc, td, pid,
        "int f(int x) post(r: r > 0) { return x; }\n"
        "int main(void) { return f(-1); }\n",
        &compiled_ok);
    if (!compiled_ok) { printf("FAIL: postcondition-violation test source failed to compile\n"); return 12; }
    if (!aborted_via_shell(status)) {
        printf("FAIL: violated postcondition did not abort() (status=%d)\n", status);
        return 13;
    }

    status = run_capture(rcc, td, pid,
        "int side = 0;\n"
        "void f(void) post(side == 1) { }\n"
        "int main(void) { f(); return 0; }\n",
        &compiled_ok);
    if (!compiled_ok) { printf("FAIL: void-fallthrough postcondition test source failed to compile\n"); return 14; }
    if (!aborted_via_shell(status)) {
        printf("FAIL: violated void-fallthrough postcondition did not abort() (status=%d)\n", status);
        return 15;
    }

    /* the diagnostic itself: function name, kind, and condition text */
    {
        char srcf[256], binf[256], out[512];
        snprintf(srcf, sizeof(srcf), "%s/test_contracts_diag_%d.c", td, pid);
        snprintf(binf, sizeof(binf), "%s/test_contracts_diag_%d.bin", td, pid);
        write_src(srcf, "int shrink(int n) pre(n > 0) { return n - 1; }\n"
                        "int main(void) { return shrink(0); }\n");
        char cbuf[512];
        int rc = compile_capture(rcc, srcf, binf, "", cbuf, sizeof(cbuf));
        remove(srcf);
        if (rc != 0) { printf("FAIL: diagnostic-text test source failed to compile: %s\n", cbuf); return 16; }
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s 2>&1 " STDOUT_NULL_REDIRECT, binf);
        FILE *rp = popen(cmd, "r");
        char msg[512] = {0};
        if (rp) {
            size_t n = fread(msg, 1, sizeof(msg) - 1, rp);
            msg[n] = '\0';
            pclose(rp);
        }
        remove(binf);
        if (!strstr(msg, "precondition") || !strstr(msg, "n > 0") || !strstr(msg, "shrink")) {
            printf("FAIL: precondition violation message missing details: %s\n", msg);
            return 17;
        }
    }

    /* --- constant conditions are discharged like static_assert: a
     * provably-false one is a compile error, not a runtime check --- */
    {
        char srcf[256], objf[256], out[512];
        snprintf(srcf, sizeof(srcf), "%s/test_contracts_const_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_contracts_const_%d.o", td, pid);
        write_src(srcf, "int f(int x) pre(0) { return x; }\nint main(void){ return f(1); }\n");
        int rc = compile_capture(rcc, srcf, objf, "-c", out, sizeof(out));
        remove(srcf);
        remove(objf);
        if (rc == 0) { printf("FAIL: 'pre(0)' should be a compile error\n"); return 20; }
        if (!strstr(out, "never satisfied")) {
            printf("FAIL: 'pre(0)' compile error missing expected diagnostic: %s\n", out);
            return 21;
        }
    }

    /* post(NAME: ...) on a void-returning function has nothing to bind
     * NAME to -- must be a compile error. */
    {
        char srcf[256], objf[256], out[512];
        snprintf(srcf, sizeof(srcf), "%s/test_contracts_voidbind_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_contracts_voidbind_%d.o", td, pid);
        write_src(srcf, "void f(int x) post(r: r > 0) { (void)x; }\nint main(void){ f(1); return 0; }\n");
        int rc = compile_capture(rcc, srcf, objf, "-c", out, sizeof(out));
        remove(srcf);
        remove(objf);
        if (rc == 0) { printf("FAIL: 'post(r: ...)' on a void function should be a compile error\n"); return 22; }
        if (!strstr(out, "void")) {
            printf("FAIL: void-binding compile error missing expected diagnostic: %s\n", out);
            return 23;
        }
    }

    return 0;
}
