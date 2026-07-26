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
    const char *td = get_tmpdir();
    char src[600], obj[600], dep[600], cmd[2048];
    int pid = (int)getpid();

    snprintf(src, sizeof(src), "%s/test_dep_stdin_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_dep_stdin_%d.o", td, pid);
    snprintf(dep, sizeof(dep), "%s/test_dep_stdin_%d.d", td, pid);

    FILE *sf = fopen(src, "w");
    if (!sf) {
        printf("FAIL: cannot write %s\n", src);
        return 1;
    }
    fputs("int main(void){return 0;}\n", sf);
    fclose(sf);

    /* Feed the source via stdin redirection ("<") rather than a shell
     * pipe — works identically under cmd.exe and a POSIX shell, unlike
     * `echo ... | ...` whose quoting rules differ across the two. */
    snprintf(cmd, sizeof(cmd),
             "%s -Wp,-MMD,%s -x c -c -o %s - < %s",
             rcc, dep, obj, src);
    int rc = system(cmd);
    remove(src);
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
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        char *start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        if (p - start == 1 && start[0] == '-') bad = 1;
    }

    if (bad) {
        printf("FAIL: dependency file lists stdin (\"-\") as a prerequisite: %s\n", content);
        return 3;
    }
    printf("OK\n");
    return 0;
}
