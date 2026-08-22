/* rcc's multi-file driver used a fixed 64-slot input_files[] array:
 * every source file past the 64th was SILENTLY dropped (no error, no
 * warning), and the resulting link died with undefined references.
 * sqlite's `make testfixture` links 86 sources in a single invocation
 * (sqlite3.c + tclsqlite-ex.c landed past the cap, so every
 * sqlite3_* symbol was undefined). Regression: 70 files must all
 * compile AND link into one executable.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

#define NFILES 70

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_many_inputs_%d.d", td, pid);
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }

    /* 70 small files defining f0()..f69() */
    for (int i = 0; i < NFILES; i++) {
        char p[700], body[128];
        snprintf(p, sizeof(p), "%s/f%d.c", dir, i);
        snprintf(body, sizeof(body), "int f%d(void){return %d;}\n", i, i);
        FILE *f = fopen(p, "w");
        if (!f) { printf("FAIL: cannot write %s\n", p); rmdir(dir); return 1; }
        fputs(body, f);
        fclose(f);
    }
    /* one main that calls them all */
    char main_p[700], main_body[8192], *w = main_body;
    snprintf(main_p, sizeof(main_p), "%s/main.c", dir);
    w += sprintf(w, "extern int ");
    for (int i = 0; i < NFILES; i++) w += sprintf(w, "f%d(),", i);
    w += sprintf(w, ";\nint main(void){return %d - (", (NFILES * (NFILES - 1)) / 2);
    for (int i = 0; i < NFILES; i++) w += sprintf(w, "f%d()+", i);
    w += sprintf(w, "0);}\n");
    FILE *f = fopen(main_p, "w");
    if (!f) { printf("FAIL: cannot write %s\n", main_p); rmdir(dir); return 1; }
    fputs(main_body, f);
    fclose(f);

    char cmd[12000], *c = cmd;
    c += sprintf(c, "%s -o %s/main %s/main.c", rcc, dir, dir);
    for (int i = 0; i < NFILES; i++) c += sprintf(c, " %s/f%d.c", dir, i);
    strcat(c, " " NULL_REDIRECT);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: rcc could not compile+link %d files (exit %d)\n", NFILES + 1, rc);
        rc = 2;
        goto out;
    }
    snprintf(cmd, sizeof(cmd), "%s/main " NULL_REDIRECT, dir);
    rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: linked %d-file program misbehaved (exit %d)\n", NFILES + 1, rc);
        rc = 3;
        goto out;
    }
    printf("OK\n");
    rc = 0;
out:
    for (int i = 0; i < NFILES; i++) {
        char p[700];
        snprintf(p, sizeof(p), "%s/f%d.c", dir, i);
        remove(p);
    }
    remove(main_p);
    snprintf(cmd, sizeof(cmd), "%s/main", dir);
    remove(cmd);
    rmdir(dir);
    return rc;
}
