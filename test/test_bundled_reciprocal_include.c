/* resolve_include()'s "skip a match already active on the include stack"
 * guard exists only for ast/ksh93's `#include <../include/X.h>` relative-
 * escape idiom (a bundled header deliberately jumping past itself to reach
 * the real system one). It used to fire unconditionally for ANY spec
 * landing in RCC_INCDIR's [bundled_lo, bundled_hi) range, including an
 * ordinary, non-escaping reciprocal include between two bundled headers
 * (e.g. rcc's own include/xmmintrin.h ending in `#include <emmintrin.h>`,
 * whose own first line is the reciprocal `#include <xmmintrin.h>`). That
 * ordinary case is a harmless no-op via the reciprocal header's own
 * `#ifndef` guard (already defined from the still-open outer file) --
 * exactly as xmmintrin.h's own comment documents -- but the self-active
 * skip forced it PAST the entire bundled range instead, landing on
 * whatever (if anything) matched further down the system search path. With
 * no such system match, the include failed outright: "include file
 * '...' not found". Under a real clang-built rcc, clang's own system
 * <xmmintrin.h> DOES match there, so instead of failing the compile got
 * clang's SSE intrinics grafted in, redeclaring rcc's own with
 * conflicting attributes ("conflicting types for '_mm_add_ss'").
 *
 * Fixed by gating the self-active skip on the spec containing ".." (the
 * relative-escape shape), so an ordinary reciprocal same-name include
 * resolves back to itself and relies on the guard macro, matching real
 * GCC/Clang precedent for circular header pairs.
 *
 * Reproduced here without needing clang installed: rcc's "include"
 * source-tree fallback (build_search_dirs()) is a bare relative path
 * resolved against the process's cwd, occupying the same [bundled_lo,
 * bundled_hi) range as the real RCC_INCDIR. Running rcc from a scratch
 * directory with its own ./include/ subdirectory lets two synthetic
 * headers exercise the exact bundled-range reciprocal-include shape,
 * with no third file anywhere to accidentally satisfy the buggy skip's
 * fallthrough -- so pre-fix this deterministically errors "not found"
 * on every platform, not just where a conflicting system header exists. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
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
    if (test_mkdir("include") != 0 && errno != EEXIST) {
        printf("FAIL: cannot mkdir include\n");
        return 2;
    }

    /* A.h: like xmmintrin.h, reciprocally includes B.h before finishing
     * its own guarded body. */
    if (!write_file("include/t_recip_a.h",
        "#ifndef T_RECIP_A_H\n#define T_RECIP_A_H\n"
        "#include <t_recip_b.h>\n"
        "static int t_recip_a_marker = 1;\n"
        "#endif\n")) {
        printf("FAIL: cannot write t_recip_a.h\n");
        return 3;
    }
    /* B.h: like emmintrin.h, its first line is the reciprocal include
     * back to A.h -- which is still open (self-active) at this point. */
    if (!write_file("include/t_recip_b.h",
        "#ifndef T_RECIP_B_H\n#define T_RECIP_B_H\n"
        "#include <t_recip_a.h>\n"
        "static int t_recip_b_marker = 2;\n"
        "#endif\n")) {
        printf("FAIL: cannot write t_recip_b.h\n");
        remove("include/t_recip_a.h");
        return 4;
    }
    if (!write_file("t_recip_main.c",
        "#include <t_recip_a.h>\n"
        "int main(void){ return t_recip_a_marker + t_recip_b_marker - 3; }\n")) {
        printf("FAIL: cannot write t_recip_main.c\n");
        remove("include/t_recip_a.h");
        remove("include/t_recip_b.h");
        return 5;
    }

    char cmd[600];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "%s t_recip_main.c -o t_recip_bin.exe " NULL_REDIRECT, rcc);
#else
    snprintf(cmd, sizeof(cmd), "%s t_recip_main.c -o t_recip_bin " NULL_REDIRECT, rcc);
#endif
    int rc = system(cmd);
    remove("include/t_recip_a.h");
    remove("include/t_recip_b.h");
    remove("t_recip_main.c");
    if (rc != 0) {
        printf("FAIL: reciprocal bundled-header include failed to compile (rc=%d)\n", rc);
        return 6;
    }

#ifdef _WIN32
    rc = system("t_recip_bin.exe");
    remove("t_recip_bin.exe");
#else
    rc = system("./t_recip_bin");
    remove("t_recip_bin");
#endif
    if (rc != 0) {
        printf("FAIL: reciprocal bundled-header binary ran with wrong result (rc=%d)\n", rc);
        return 7;
    }

    printf("OK\n");
    return 0;
}
