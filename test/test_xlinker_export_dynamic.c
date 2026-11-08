/* `-Xlinker ARG` is `-Wl,ARG`. rcc ignored it with a warning, so CPython's
 * LINKFORSHARED (`-Xlinker -export-dynamic`) left the `python` executable
 * without exported Py* symbols and every extension module failed to import
 * with "undefined symbol: _Py_NoneStruct".
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

#ifndef __linux__ /* -export-dynamic + dlsym(RTLD_DEFAULT) is the ELF/glibc idiom */
int main(void) { printf("OK (skipped)\n"); return 0; }
#else
#include <unistd.h>
int main(void) {
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char src[160], exe[160], cmd[640];
    snprintf(src, sizeof(src), "%s/test_xl_%d.c", get_tmpdir(), pid);
    snprintf(exe, sizeof(exe), "%s/test_xl_%d", get_tmpdir(), pid);
    FILE *f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); return 1; }
    fputs("#define _GNU_SOURCE\n#include <dlfcn.h>\n"
          "int xl_exported(void) { return 7; }\n"
          "int main(void) { return dlsym(RTLD_DEFAULT, \"xl_exported\") ? 0 : 1; }\n", f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s -Xlinker -export-dynamic %s -o %s -ldl " NULL_REDIRECT, rcc, src, exe);
    int rc = system(cmd);
    if (rc == 0) rc = system(exe);
    remove(src);
    remove(exe);
    if (rc != 0) { printf("FAIL: -Xlinker -export-dynamic did not export xl_exported\n"); return 1; }
    printf("OK\n");
    return 0;
}
#endif
