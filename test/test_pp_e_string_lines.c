/* Regression: rcc -E must not concatenate adjacent string literals that
 * are separated by source newlines. Downstream line-oriented consumers
 * (micropython's makeqstrdata.py, etc.) rely on each literal staying on
 * its own output line. */
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *src =
    "\"Q(*)\"\n"
    "\"Q(_)\"\n"
    "\"Q(/)\"\n";

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char srcf[256];
    char outf[256];
    char cmd[1024];
    snprintf(srcf, sizeof(srcf), "%s/test_e_strings_src_%d.txt", td, pid);
    snprintf(outf, sizeof(outf), "%s/test_e_strings_out_%d.txt", td, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) return 1;
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -E %s > %s " NULL_REDIRECT, rcc, srcf, outf);
    int st = system(cmd);
    remove(srcf);
    if (st != 0) {
        remove(outf);
        printf("FAIL: rcc -E exited with status %d\n", st);
        return 2;
    }

    f = fopen(outf, "r");
    if (!f) return 3;
    static char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    remove(outf);

    if (strstr(buf, "\"Q(*)\"\n") == NULL ||
        strstr(buf, "\"Q(_)\"\n") == NULL ||
        strstr(buf, "\"Q(/)\"\n") == NULL) {
        printf("FAIL: -E output lost separate string lines:\n%s\n", buf);
        return 4;
    }

    printf("OK -E preserves separate string literal lines\n");
    return 0;
}
