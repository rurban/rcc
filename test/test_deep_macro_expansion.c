/* rcc's macro expander capped frame nesting at 600; anything deeper was
 * SILENTLY left unexpanded (a 700-deep linear chain stopped at ~M148,
 * and metalang99/datatype99's eval machine — which nests ~700 frames in
 * a single datatype() expansion — stalled mid-recursion, leaving
 * ML99_PRIV_REC_NEXT_* residue that failed to parse).
 *
 * Regression: a 750-deep linear macro chain must fully expand (and
 * compile, and run) — each M_N(x) -> M_{N-1}(x) keeps the caller's
 * frame on the stack while the callee expands, so the depth is exactly
 * the chain length.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

#define NCHAIN 750

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_deep_macro_%d.d", td, pid);
    if (test_mkdir(dir) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
    char src[600], exe[600], cmd[2600], body[64 * 1024], *w = body;
    snprintf(src, sizeof(src), "%s/t.c", dir);
    snprintf(exe, sizeof(exe), "%s/t", dir);
    w += sprintf(w, "#define M000(x) 0\n");
    for (int i = 1; i < NCHAIN; i++)
        w += sprintf(w, "#define M%03d(x) M%03d(x)\n", i, i - 1);
    w += sprintf(w, "int main(void){return M%03d(42);}\n", NCHAIN - 1);
    FILE *f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); rmdir(dir); return 1; }
    fputs(body, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, exe, src);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: %d-deep macro chain did not expand fully (exit %d)\n", NCHAIN, rc);
        remove(src);
        rmdir(dir);
        return 2;
    }
    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exe);
    rc = system(cmd);
    remove(src);
    remove(exe);
    rmdir(dir);
    if (rc != 0) { printf("FAIL: expanded chain misbehaved (exit %d)\n", rc); return 3; }
    printf("OK\n");
    return 0;
}
