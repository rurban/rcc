/* resolve_include()'s "skip a match that's already active on the include
 * stack" guard (added to fix ast/ksh93's `#include <../include/X.h>`
 * relative-escape idiom colliding with rcc's own bundled headers) only
 * applied when the found search directory was rcc's OWN bundled include
 * dir (RCC_INCDIR, or its "include" source-tree fallback alias) -- but
 * it matched by comparing the search dir's *string value* to the
 * literal "include", not by tracking which dirs[] slot rcc itself
 * inserted. Any project using the extremely common `-Iinclude`
 * convention for its OWN include/ directory collided: a legitimate,
 * standard circular header pair (A.h currently being processed
 * #include "B.h", and B.h -- protected by its own include guard --
 * #include "A.h" back) was wrongly treated as "the same bundled-header
 * self-reference" and skipped entirely, so `#include "A.h"` from inside
 * B.h resolved to nothing: "include file 'A.h' not found" even though
 * A.h plainly exists right there in the -Iinclude directory.
 *
 * Confirmed via test/third_party's test_chibischeme: chibi-scheme's own
 * `-Iinclude`, with include/chibi/eval.h currently active including
 * (transitively) include/chibi/bignum.h, which `#include "chibi/eval.h"`
 * back -- a completely standard include-guarded circular header pair.
 *
 * Fixed by having build_search_dirs() report the RCC_INCDIR/"include"-
 * fallback pair's [lo, hi) index range directly (it's the one inserting
 * them, so it already knows exactly where), and checking `i` against
 * that range in resolve_include()/resolve_include_next() instead of
 * comparing dirs[i]'s string value -- so a same-named user -I directory
 * is never mistaken for rcc's own bundled headers.
 *
 * The reproducer needs an actual `include/` directory relative to the
 * compiler's own cwd (rcc's fallback dir is a bare relative string, not
 * a resolved path) -- so this writes into a scratch subdirectory of the
 * test's own working directory rather than an absolute -I path, and
 * cleans up afterward, instead of `cd`-ing into a temp dir (which
 * needlessly complicates resolving the rcc binary's own path across
 * platforms for no benefit -- the bug is in shared, target-independent
 * preprocessor code).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char incdir[256], a[300], b[300], main_c[300], bin[300], cmd[900];

    snprintf(incdir, sizeof(incdir), "qinc_%d_include", pid);
    snprintf(cmd, sizeof(cmd), "mkdir \"%s\"", incdir);
    system(cmd); /* return code not checked: fopen() below is the real check */

    /* A.h: currently-active file that (transitively) reaches B.h. */
    snprintf(a, sizeof(a), "%s/a.h", incdir);
    FILE *fa = fopen(a, "w");
    if (!fa) { printf("FAIL: cannot write a.h\n"); return 1; }
    fputs("#ifndef A_H\n#define A_H\n#include \"b.h\"\nstatic int a_marker = 1;\n#endif\n", fa);
    fclose(fa);

    /* B.h: included from A.h, self-references A.h back (guard-protected,
     * standard circular-header idiom) before using a_marker. */
    snprintf(b, sizeof(b), "%s/b.h", incdir);
    FILE *fb = fopen(b, "w");
    if (!fb) { printf("FAIL: cannot write b.h\n"); return 2; }
    fputs("#ifndef B_H\n#define B_H\n#include \"a.h\"\nstatic int b_marker = 2;\n#endif\n", fb);
    fclose(fb);

    snprintf(main_c, sizeof(main_c), "qinc_%d_main.c", pid);
    FILE *fm = fopen(main_c, "w");
    if (!fm) { printf("FAIL: cannot write main.c\n"); return 3; }
    fputs("#include \"a.h\"\nint main(void){return a_marker + b_marker - 3;}\n", fm);
    fclose(fm);

#ifdef _WIN32
    snprintf(bin, sizeof(bin), "qinc_%d_bin.exe", pid);
#else
    snprintf(bin, sizeof(bin), "qinc_%d_bin", pid);
#endif
    snprintf(cmd, sizeof(cmd), "%s -I%s %s -o %s " NULL_REDIRECT, rcc, incdir, main_c, bin);
    int rc = system(cmd);
    remove(a);
    remove(b);
    remove(main_c);
    snprintf(cmd, sizeof(cmd), "rmdir \"%s\"", incdir);
    system(cmd);
    if (rc != 0) {
        printf("FAIL: circular quote-include pair via -I%s failed to compile\n", incdir);
        return 4;
    }

    char run_cmd[350];
#ifdef _WIN32
    snprintf(run_cmd, sizeof(run_cmd), "%s", bin);
#else
    snprintf(run_cmd, sizeof(run_cmd), "./%s", bin);
#endif
    rc = system(run_cmd);
    remove(bin);
    if (rc != 0) {
        printf("FAIL: circular quote-include pair ran with wrong result (rc=%d)\n", rc);
        return 5;
    }

    printf("OK\n");
    return 0;
}
