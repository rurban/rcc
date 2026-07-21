/* write_dep_file() used to write the raw input path into the generated
 * Make dependency file's prerequisite list unconditionally, including the
 * literal "-" when compiling from stdin. kbuild's fixdep then tries to
 * open a file actually named "-" and fails ("error opening file: -: No
 * such file or directory"), breaking scripts/checksyscalls.sh, which
 * pipes a generated probe source through `$(CC) ... -x c -`. GCC omits
 * stdin from its own -M output for the same reason; verify rcc does too.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    char cmd[512], obj[64], dep[64];
    snprintf(obj, sizeof(obj), "/tmp/test_dep_stdin_%d.o", (int)getpid());
    snprintf(dep, sizeof(dep), "/tmp/test_dep_stdin_%d.d", (int)getpid());

    snprintf(cmd, sizeof(cmd),
             "echo 'int main(void){return 0;}' | %s -Wp,-MMD,%s -x c -c -o %s -",
             rcc, dep, obj);
    int rc = system(cmd);
    remove(obj);
    if (rc != 0) {
        printf("FAIL: compile from stdin with -Wp,-MMD failed (rc=%d)\n", rc);
        return 1;
    }

    FILE *f = fopen(dep, "r");
    if (!f) {
        printf("FAIL: no dependency file generated\n");
        return 2;
    }
    char content[4096];
    size_t n = fread(content, 1, sizeof(content) - 1, f);
    content[n] = '\0';
    fclose(f);
    remove(dep);

    /* Scan for a bare "-" token (stdin) among the whitespace-separated
     * prerequisites — it must never appear. */
    int bad = 0;
    char *p = content;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        char *start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
        if (p - start == 1 && start[0] == '-') bad = 1;
    }

    if (bad) {
        printf("FAIL: dependency file lists stdin (\"-\") as a prerequisite: %s\n", content);
        return 3;
    }
    printf("OK\n");
    return 0;
}
