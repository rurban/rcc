/* rcc unconditionally recognized MSVC's `__declspec(...)` syntax and
 * silently accepted (swallowed, no-op) it on every target, including
 * native Linux -- but real GCC never recognizes `__declspec` at all on
 * native Linux (only MinGW-targeted GCC does, as a genuine extension for
 * Windows DLL export/import). `__declspec(dllexport) int foo(void) {
 * return 0; }` compiled cleanly under rcc; under real gcc on Linux it's
 * a syntax error (`__declspec` parsed as an ordinary, implicitly-`int`
 * identifier, then `(dllexport)` collides with the following `int foo`).
 *
 * Found via test_muon's own `common/197 function attributes` capability
 * probe, which specifically checks that `__declspec(dllexport)` reports
 * NOT supported on posix (only on `_WIN32`).
 *
 * Fixed by gating `__declspec` keyword recognition (both the
 * is_typename() lookahead and read_type_attrs()'s actual consumption) to
 * _WIN32 builds only, matching real GCC's own target-specific behavior.
 *
 * A second, more serious bug surfaced while verifying this fix: once
 * `__declspec` is no longer consumed as a recognized attribute on Linux,
 * `__declspec(dllimport) int foo(void);` (a bodyless prototype -- no
 * following K&R declaration-list ever reaching a `{`) crashed rcc
 * internally (SIGSEGV) instead of producing a clean diagnostic --
 * parse_kr_param_list()'s declaration-list loop ran off the end of the
 * token stream into a NULL dereference. See test_kr_param_list_eof.c for
 * that regression test.
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

static int compile(const char *rcc, const char *src, const char *obj) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    return system(cmd);
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

#ifdef _WIN32
    /* On the mingw target, __declspec(dllexport) is a genuine, long-
     * standing GNU extension (unrelated to MSVC compat) -- must keep
     * compiling cleanly here, unlike native Linux below. */
    snprintf(src, sizeof(src), "%s\\test_declspec_win_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s\\test_declspec_win_%d.o", td, pid);
    if (!write_file(src, "__declspec(dllexport) int foo(void) { return 0; }\n"))
        return 1;
    int rc = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc != 0) {
        printf("FAIL: __declspec(dllexport) wrongly rejected on _WIN32 (rc=%d)\n", rc);
        return 2;
    }
#else
    /* On native Linux, __declspec is not a recognized keyword at all
     * (matching real gcc) -- must be a clean compile error, not silently
     * accepted. */
    snprintf(src, sizeof(src), "%s/test_declspec_posix_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_declspec_posix_%d.o", td, pid);
    if (!write_file(src, "__declspec(dllexport) int foo(void) { return 0; }\n"))
        return 1;
    int rc = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc == 0) {
        printf("FAIL: __declspec(dllexport) wrongly compiled cleanly on native Linux\n");
        return 3;
    }
#endif

    /* Ordinary __attribute__ usage (a completely separate, real GNU
     * extension) must be unaffected by the __declspec gating, on every
     * target. */
    snprintf(src, sizeof(src), "%s%stest_declspec_attr_%d.c", td,
#ifdef _WIN32
             "\\",
#else
             "/",
#endif
             pid);
    snprintf(obj, sizeof(obj), "%s.o", src);
    if (!write_file(src,
        "int foo(void) __attribute__((weak));\n"
        "int foo(void) { return 0; }\n"
        "int main(void) { return foo(); }\n")) {
        return 4;
    }
    int rc2 = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc2 != 0) {
        printf("FAIL: ordinary __attribute__((weak)) usage wrongly rejected "
               "after the __declspec fix (rc=%d)\n", rc2);
        return 5;
    }

    printf("OK\n");
    return 0;
}
