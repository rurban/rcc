/* -E output must emit a linemarker for a header that produces no tokens
 * of its own: glibc's bits/signum-generic.h is all #defines, and zsh's
 * configure greps the preprocessor's linemarkers for the header that
 * defines SIG* macros ("where signal.h is located"). rcc only emitted a
 * marker when a TOKEN's filename changed, so macro-only headers never
 * appeared and zsh's configure died with "SIGNAL MACROS NOT FOUND".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_linemarker_%d.d", td, pid);
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
    char hdr[600], src[600], out[600], cmd[2600];
    snprintf(hdr, sizeof(hdr), "%s/sigs.h", dir);
    snprintf(src, sizeof(src), "%s/t.c", dir);
    snprintf(out, sizeof(out), "%s/out.i", dir);
    FILE *f = fopen(hdr, "w");
    if (!f) { printf("FAIL: cannot write %s\n", hdr); rmdir(dir); return 1; }
    fputs("#ifndef SIGS_H\n#define SIGS_H\n#define SIG_TST1 1\n#define SIG_TST2 2\n#endif\n", f);
    fclose(f);
    f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); rmdir(dir); return 1; }
    fputs("#include \"sigs.h\"\nint main(void){return SIG_TST2;}\n", f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -I%s -E %s -o %s " NULL_REDIRECT, rcc, dir, src, out);
    int rc = system(cmd);
    if (rc != 0) { printf("FAIL: rcc -E failed (exit %d)\n", rc); goto out; }
    char *buf = NULL;
    size_t len = 0;
    FILE *fp = fopen(out, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = (size_t)ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = malloc(len + 1);
        if (fread(buf, 1, len, fp) != len) { buf[len] = '\0'; }
        buf[len] = '\0';
        fclose(fp);
    }
    if (!buf || !strstr(buf, "sigs.h")) {
        printf("FAIL: -E output lacks a linemarker for the macro-only header\n");
        rc = 2;
    } else {
        printf("OK\n");
        rc = 0;
    }
    free(buf);
out:
    remove(hdr);
    remove(src);
    remove(out);
    rmdir(dir);
    return rc;
}
