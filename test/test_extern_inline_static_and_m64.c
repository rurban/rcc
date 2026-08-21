/* Two real rcc bugs found compiling mjansson/rpmalloc 2.0.1:
 *
 * 1. `extern inline` functions were wrongly treated the same as bare
 *    `inline` by the "static object used in inline function" diagnostic
 *    (C11 6.7.4p3). A bare, non-static, non-gnu_inline `inline` function
 *    might be emitted as *the* external definition in some OTHER
 *    translation unit, where a referenced `static` object isn't visible
 *    -- real gcc warns there (confirmed: "'sv' is static but used in
 *    inline function 'j' which is not static", present even with no
 *    flags at all). But `extern inline` unambiguously provides the
 *    external definition right here, in THIS translation unit, where any
 *    `static` object it references is visible by definition -- gcc never
 *    warns there. rcc's `current_fn_is_inline` flag failed to exclude
 *    `attr.is_extern`, so it warned on `extern inline` too. rpmalloc's
 *    malloc.c declares every override (rpvalloc, rppvalloc, malloc, ...)
 *    as `extern inline ... { ... static_thing ... }`, so under rpmalloc's
 *    own `-Werror -pedantic`, this false positive was a hard build
 *    failure.
 *
 * 2. `-m64` (a legitimate, common GCC flag meaning "64-bit ABI", which
 *    rcc's own generated object files always are, being native-only) was
 *    rejected outright as an unrecognized command-line option, promoted
 *    to a hard `-Werror` build failure by rpmalloc's own build
 *    (mjansson/rpmalloc's `configure.py`-generated ninja file always
 *    passes `-m64` on this architecture). `-m32` remains correctly
 *    rejected -- rcc is native-only, no cross-compilation to a different
 *    word width in one binary (see AGENTS.md).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int compile_flags(const char *rcc, const char *td, int pid,
                          const char *flags, const char *src) {
    char path[600], obj[700], cmd[2600];
    snprintf(path, sizeof(path), "%s/t_exin64_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/t_exin64_%d.o", td, pid);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(src, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s %s -c %s -o %s " NULL_REDIRECT,
             rcc, flags, path, obj);
    int rc = system(cmd);
    remove(path);
    remove(obj);
    return rc;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    /* 1a. A bare, non-static `inline` function referencing a file-scope
     * `static` object: still warned about (non-fatal without -Werror). */
    if (compile_flags(rcc, td, pid, "",
            "static int sv;\n"
            "inline void j(void) { (void)sv; }\n") != 0) {
        printf("FAIL: bare inline referencing static object rejected without -Werror\n");
        return 1;
    }

    /* 1b. ...and promoted to a hard error under -Werror (still a real
     * C11 constraint violation gcc itself diagnoses unconditionally). */
    if (compile_flags(rcc, td, pid, "-Werror",
            "static int sv;\n"
            "inline void j(void) { (void)sv; }\n") == 0) {
        printf("FAIL: bare inline referencing static object accepted under -Werror\n");
        return 1;
    }

    /* 1c. The rpmalloc bug: `extern inline` referencing a static object
     * must compile clean even under -Werror -- this IS the external
     * definition, right here, where the static object is visible. */
    if (compile_flags(rcc, td, pid, "-Werror",
            "static int sv;\n"
            "extern inline void j(void) { (void)sv; }\n") != 0) {
        printf("FAIL: extern inline referencing static object rejected under -Werror\n");
        return 1;
    }

    /* 1d. static inline (internal linkage function itself) referencing a
     * static object was never diagnosed either -- must stay that way. */
    if (compile_flags(rcc, td, pid, "-Werror",
            "static int sv;\n"
            "static inline void j(void) { (void)sv; }\n") != 0) {
        printf("FAIL: static inline referencing static object rejected under -Werror\n");
        return 1;
    }

    /* 2a. -m64: accepted as a no-op (rcc's native target is already
     * 64-bit on x86-64/ARM64/mingw64). */
    if (compile_flags(rcc, td, pid, "-Werror -m64",
            "int main(void) { return 0; }\n") != 0) {
        printf("FAIL: -m64 rejected, should be accepted as a no-op\n");
        return 1;
    }

    /* 2b. -m32: still correctly rejected -- no cross-compilation to a
     * different word width in one binary. */
    if (compile_flags(rcc, td, pid, "-m32",
            "int main(void) { return 0; }\n") == 0) {
        printf("FAIL: -m32 accepted, should be rejected (no cross-compile in one binary)\n");
        return 1;
    }

    printf("OK\n");
    return 0;
}
