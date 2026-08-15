/* GCC precedence for a quote-form `#include "file"`: the including file's
 * own directory, then -iquote dirs, then -I dirs, then rcc's own bundled
 * include dir (RCC_INCDIR), then the real system dirs -- confirmed
 * directly against real gcc (a `-iquote dir` with `dir/stdarg.h` present
 * satisfies `#include "stdarg.h"` ahead of gcc's own bundled one).
 * build_search_dirs() (src/preprocess.c) used to check RCC_INCDIR before
 * EVERY -iquote/-I directory, for both include forms, so a quote-form
 * include of a file that happens to share a name with something rcc
 * bundles (e.g. a project's own "string.h", colliding with rcc's
 * <string.h> compatibility shim) always resolved to rcc's bundled copy
 * instead of the caller's own -iquote-supplied file -- found via
 * noplate's own `#include "string.h"` (a file outside its `-iquote
 * ./src/` root, needing to reach its own src/string.h).
 *
 * -I/-isystem/-idirafter and the angle form both deliberately keep
 * RCC_INCDIR searched FIRST (unaffected by this fix -- see
 * test_include_next_skips_user_dirs.c, which relies on exactly that for
 * ast/ksh93's own #include_next escape idiom), so this test only
 * exercises -iquote specifically.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    char dir[600], hdr[700], src[600], obj[600], cmd[3200];
    int pid = (int)getpid();

    snprintf(dir, sizeof(dir), "%s/test_iquote_prec_%d.d", td, pid);
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
    /* "limits.h" collides with rcc's own bundled include/limits.h. */
    snprintf(hdr, sizeof(hdr), "%s/limits.h", dir);
    snprintf(src, sizeof(src), "%s/test_iquote_prec_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_iquote_prec_%d.o", td, pid);

    FILE *hf = fopen(hdr, "w");
    if (!hf) { printf("FAIL: write header\n"); return 2; }
    fputs("#define IQ_DECOY_WINS 1\n", hf);
    fclose(hf);

    FILE *sf = fopen(src, "w");
    if (!sf) { printf("FAIL: write src\n"); return 3; }
    /* Quote form: must resolve to the -iquote decoy, not rcc's bundled
     * limits.h (which has no IQ_DECOY_WINS macro and does define
     * INT_MAX, letting the two forms be told apart unambiguously). */
    fputs("#include \"limits.h\"\n"
          "#ifndef IQ_DECOY_WINS\n#error decoy_not_found\n#endif\n"
          "int main(void){return 0;}\n", sf);
    fclose(sf);

    snprintf(cmd, sizeof(cmd), "%s -iquote %s -c -o %s %s " NULL_REDIRECT,
             rcc, dir, obj, src);
    int rc = system(cmd);
    remove(obj);

    if (rc != 0) {
        printf("FAIL: quote-form \"limits.h\" did not prefer the -iquote decoy over rcc's bundled copy (rc=%d)\n", rc);
        remove(hdr);
        remove(src);
        rmdir(dir);
        return 4;
    }

    /* Angle form must NOT pick up the same -iquote-only decoy: it still
     * needs rcc's bundled INT_MAX-providing limits.h (or the real system
     * one), unaffected by this fix. */
    sf = fopen(src, "w");
    if (!sf) { printf("FAIL: rewrite src\n"); remove(hdr); rmdir(dir); return 5; }
    fputs("#include <limits.h>\n"
          "int main(void){return INT_MAX > 0 ? 0 : 1;}\n", sf);
    fclose(sf);

    snprintf(cmd, sizeof(cmd), "%s -iquote %s -c -o %s %s " NULL_REDIRECT,
             rcc, dir, obj, src);
    rc = system(cmd);

    remove(obj);
    remove(hdr);
    remove(src);
    rmdir(dir);

    if (rc != 0) {
        printf("FAIL: angle-form <limits.h> was wrongly diverted to the -iquote-only decoy (rc=%d)\n", rc);
        return 6;
    }

    printf("OK\n");
    return 0;
}
