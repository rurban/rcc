/* GCC predefines the non-reserved `unix` and `linux` macros only in GNU
 * dialects; under a strict -std=cNN they are absent. rcc defined them
 * always, so OpenRC's `struct sockaddr_un unix;` (src/shared/misc.c, built
 * with meson c_std=c99) expanded to `struct sockaddr_un 1;`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

static char *run_pp(const char *rcc, const char *flags, const char *srcf, int pid, int tag) {
    char cmd[512], outf[160];
    snprintf(outf, sizeof(outf), "%s/test_sul_out_%d_%d.txt", get_tmpdir(), pid, tag);
    snprintf(cmd, sizeof(cmd), "%s %s -E %s > %s " NULL_REDIRECT, rcc, flags, srcf, outf);
    if (system(cmd) != 0) { remove(outf); return NULL; }
    FILE *f = fopen(outf, "r");
    remove(outf);
    if (!f) return NULL;
    static char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

#ifdef _WIN32 /* GCC/mingw predefines neither `unix` nor `linux` at all */
int main(void) {
    printf("OK (windows)\n");
    return 0;
}
#else
int main(void) {
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char srcf[160];
    snprintf(srcf, sizeof(srcf), "%s/test_sul_%d.c", get_tmpdir(), pid);
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs("unix_is unix linux_is linux\n", f);
    fclose(f);

    static const struct { const char *flags, *want; } cases[] = {
        {"-std=c99",   "unix_is unix linux_is linux"},
        {"-std=c11",   "unix_is unix linux_is linux"},
        {"-std=gnu99", "unix_is 1 linux_is 1"},
        {"",           "unix_is 1 linux_is 1"},
    };
    for (int i = 0; i < 4; i++) {
        char *out = run_pp(rcc, cases[i].flags, srcf, pid, i);
        if (!out || !strstr(out, cases[i].want)) {
            printf("FAIL: [%s] want '%s', got '%s'\n", cases[i].flags, cases[i].want,
                   out ? out : "(pp failed)");
            remove(srcf);
            return 1;
        }
    }
    remove(srcf);
    printf("OK\n");
    return 0;
}
#endif
