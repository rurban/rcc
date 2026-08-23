/* `rcc -E -` (stdin input) must preserve line breaks in its output:
 * pp_print_tokens() discarded the lineno of any token whose filename
 * starts with '<' — "<stdin>" included — so every stdin token fell
 * back to cur_line, merging the whole output onto one line. Broke
 * toybox's build (scripts/make.sh pipes the NEWTOY flag scan through
 * `$CC -E -` and line-splits the result) and any other -E - consumer.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_stdin_line_%d.d", td, pid);
#if defined(_WIN32)
    /* stdin via system() pipe is not exercised on Windows CI */
    (void)rcc; (void)dir;
    return 0;
#else
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
#endif
    char out[600], cmd[2600];
    snprintf(out, sizeof(out), "%s/out.i", dir);
    snprintf(cmd, sizeof(cmd),
             "printf 'int a;\\nint b;\\n' | %s -E - > %s " NULL_REDIRECT,
             rcc, out);
    int rc = system(cmd);
    if (rc != 0) { printf("FAIL: rcc -E - failed (exit %d)\n", rc); goto out; }
    char *buf = NULL;
    FILE *fp = fopen(out, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buf = malloc((size_t)len + 1);
        if (fread(buf, 1, (size_t)len, fp) != (size_t)len) { buf[len] = '\0'; }
        buf[len] = '\0';
        fclose(fp);
    }
    /* both declarations must be on separate output lines */
    if (!buf || !strstr(buf, "int a;\nint b;")) {
        printf("FAIL: -E - output merged lines\n%s\n", buf ? buf : "(empty)");
        rc = 2;
    } else {
        printf("OK\n");
        rc = 0;
    }
    free(buf);
out:
    remove(out);
    rmdir(dir);
    return rc;
}
