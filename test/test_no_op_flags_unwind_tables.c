/* -f[no-][asynchronous-]unwind-tables are real GCC flags (whether to emit
 * .eh_frame). rcc's output is always unwindable, so they are accepted as
 * no-ops -- but under -Werror every flag rcc does not know is a hard error,
 * which killed micropython's build at its first preprocess step
 * ("rcc: error: unrecognized command-line option
 * '-fno-asynchronous-unwind-tables'").
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int compile_flags(const char *rcc, const char *td, int pid, const char *flags) {
    char path[600], obj[700], cmd[2600];
    snprintf(path, sizeof(path), "%s/t_unwindflags_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/t_unwindflags_%d.o", td, pid);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs("int main(void) { return 0; }\n", f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s %s -c %s -o %s " NULL_REDIRECT, rcc, flags, path, obj);
    int rc = system(cmd);
    remove(path);
    remove(obj);
    return rc;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    const char *accepted[] = {
        "-Werror -fno-asynchronous-unwind-tables",
        "-Werror -fasynchronous-unwind-tables",
        "-Werror -funwind-tables",
        "-Werror -fno-unwind-tables",
    };
    for (unsigned i = 0; i < sizeof accepted / sizeof accepted[0]; i++) {
        if (compile_flags(rcc, td, pid, accepted[i]) != 0) {
            printf("FAIL: \"%s\" rejected, should be a no-op\n", accepted[i]);
            return 1;
        }
    }
    /* Still strict about flags that really do not exist. */
    if (compile_flags(rcc, td, pid, "-Werror -fno-asynchronous-unwind-tabless") == 0) {
        printf("FAIL: misspelled flag accepted under -Werror\n");
        return 2;
    }
    printf("OK\n");
    return 0;
}
