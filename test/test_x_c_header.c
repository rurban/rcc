/* `-x c-header` (CMake's PRECOMPILE_HEADERS feature, e.g. SDL3's
 * cmake_pch.h.gch build step) was rejected outright: main.c's `-x`
 * dispatch only recognized `c`/`none`, erroring "unsupported -x
 * c-header, only C is supported" for anything else -- unconditionally
 * failing the whole build the first time CMake tried to generate a
 * precompiled header, long before any real source got compiled.
 *
 * rcc has no serialized-PCH backend, but it doesn't need one: `-include`
 * (src/main.c) always source-includes the named header's text on every
 * translation unit, never consulting a sibling ".gch"/".pch" file. So a
 * `.gch` produced by treating the header as an ordinary C compile (like
 * `-x c`) is simply an unused build artifact that satisfies the
 * Makefile dependency -- no different from what real GCC/Clang tcc-style
 * minimal compilers already do when PCH isn't implemented.
 *
 * Fixed by accepting "-x c-header" (both "-x c-header" and "-xc-header"
 * spellings, matching the existing "-x c"/"-xc" handling) as a synonym
 * for "-x c", falling through to the ordinary compile path.
 *
 * Found via SDL3's CMake build: `rcc ... -x c-header -include
 * cmake_pch.h -o cmake_pch.h.gch -c cmake_pch.h.c` (an empty wrapper
 * TU forcing the real header through `-include`) hard-failed the
 * cmake_pch.h.gch build target before any of SDL3's own source built.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int run(const char *cmd) {
    return system(cmd);
}

int main(void) {
    const char *rcc_raw = find_rcc();
    char rcc_abs[4096];
#ifdef _WIN32
    const char *rcc = _fullpath(rcc_abs, rcc_raw, sizeof(rcc_abs)) ? rcc_abs : rcc_raw;
#else
    const char *rcc = realpath(rcc_raw, rcc_abs) ? rcc_abs : rcc_raw;
#endif
    const char *td = get_tmpdir();
    if (chdir(td) != 0) {
        printf("FAIL: cannot chdir to %s\n", td);
        return 1;
    }

    /* A header with real, checkable content -- a declaration only valid
     * once (no include guard needed here since it's compiled standalone),
     * mirroring CMake's generated cmake_pch.h shape. */
    FILE *f = fopen("t_pch.h", "w");
    if (!f) { printf("FAIL: cannot write t_pch.h\n"); return 2; }
    fputs("#pragma GCC system_header\n"
          "static inline int t_pch_probe(int x) { return x + 1; }\n",
          f);
    fclose(f);

    /* Empty wrapper TU, exactly as CMake generates for cmake_pch.h.c;
     * the real content only arrives via -include. */
    f = fopen("t_pch_wrap.c", "w");
    if (!f) { printf("FAIL: cannot write t_pch_wrap.c\n"); return 3; }
    fputs("/* generated */\n", f);
    fclose(f);

    char cmd[1024];
    /* Both spellings CMake/GCC accept: "-x c-header" (space) and the
     * "-xc-header" (joined) form. */
    snprintf(cmd, sizeof(cmd),
             "%s -x c-header -include t_pch.h -o t_pch.h.gch -c t_pch_wrap.c " NULL_REDIRECT,
             rcc);
    int rc = run(cmd);
    if (rc != 0) {
        printf("FAIL: -x c-header rejected (rc=%d)\n", rc);
        remove("t_pch.h");
        remove("t_pch_wrap.c");
        remove("t_pch.h.gch");
        return 4;
    }
    int has_gch = access("t_pch.h.gch", F_OK) == 0;
    remove("t_pch.h.gch");

    snprintf(cmd, sizeof(cmd),
             "%s -xc-header -include t_pch.h -o t_pch.h.gch -c t_pch_wrap.c " NULL_REDIRECT,
             rcc);
    rc = run(cmd);
    remove("t_pch.h");
    remove("t_pch_wrap.c");
    int has_gch2 = access("t_pch.h.gch", F_OK) == 0;
    remove("t_pch.h.gch");

    if (rc != 0) {
        printf("FAIL: -xc-header rejected (rc=%d)\n", rc);
        return 5;
    }
    if (!has_gch || !has_gch2) {
        printf("FAIL: -x c-header compile succeeded but produced no output file\n");
        return 6;
    }

    /* A genuinely unsupported -x language must still be rejected. */
    f = fopen("t_pch_wrap2.c", "w");
    if (!f) { printf("FAIL: cannot write t_pch_wrap2.c\n"); return 7; }
    fputs("/* generated */\n", f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -x fortran -c t_pch_wrap2.c " NULL_REDIRECT, rcc);
    rc = run(cmd);
    remove("t_pch_wrap2.c");
    remove("t_pch_wrap2.o");
    if (rc == 0) {
        printf("FAIL: -x fortran should have been rejected\n");
        return 8;
    }

    printf("OK\n");
    return 0;
}
