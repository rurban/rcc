/* Three real GCC driver flags rcc unconditionally tolerates as no-ops
 * (matching the existing -m64/-fno-common precedent) got hard-rejected
 * once combined with -Werror, hard-failing real third-party builds at
 * the very first compile step:
 *
 * - "-s" (link-time strip: drop the symbol table and relocation info
 *   from the output). rcc's native linker has no strip pass, but the
 *   flag only affects binary size/debuggability, never program
 *   behavior. Found via jerryscript's CMake build (`-Werror ... -s` on
 *   its unit-doc/API-reference link steps).
 *
 * - "-fdata-sections"/"-ffunction-sections" (place each global/function
 *   in its own ELF section, enabling a later `ld --gc-sections` link to
 *   drop unreferenced ones). rcc's native linker never garbage-collects
 *   sections -- every symbol it emits is always kept -- so per-symbol
 *   section splitting has nothing to implement. Found via micropython's
 *   `-Werror ... -fdata-sections -ffunction-sections` build, which
 *   hard-failed at the very first preprocess step
 *   ("rcc: error: unrecognized command-line option '-fdata-sections'").
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int compile_flags(const char *rcc, const char *td, int pid,
                         const char *flags, const char *src) {
    char path[600], obj[700], cmd[2600];
    snprintf(path, sizeof(path), "%s/t_noopflags_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/t_noopflags_%d.o", td, pid);
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(src, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "%s %s -c %s -o %s " NULL_REDIRECT,
             rcc, flags, path, obj);
    int rc = system(cmd);
    remove(path);
    remove(obj);
    return rc;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    const char *src = "int main(void) { return 0; }\n";

    if (compile_flags(rcc, td, pid, "-Werror -s", src) != 0) {
        printf("FAIL: -s rejected under -Werror, should be accepted as a no-op\n");
        return 1;
    }
    if (compile_flags(rcc, td, pid, "-Werror -fdata-sections", src) != 0) {
        printf("FAIL: -fdata-sections rejected under -Werror, should be a no-op\n");
        return 2;
    }
    if (compile_flags(rcc, td, pid, "-Werror -ffunction-sections", src) != 0) {
        printf("FAIL: -ffunction-sections rejected under -Werror, should be a no-op\n");
        return 3;
    }
    if (compile_flags(rcc, td, pid, "-Werror -fdata-sections -ffunction-sections -s", src) != 0) {
        printf("FAIL: all three combined rejected under -Werror\n");
        return 4;
    }

    /* A genuinely unrecognized flag must still be rejected under -Werror
     * (guards against these fixes over-broadening the accept list). */
    if (compile_flags(rcc, td, pid, "-Werror -fnot-a-real-flag", src) == 0) {
        printf("FAIL: -fnot-a-real-flag accepted, should still be rejected under -Werror\n");
        return 5;
    }

    printf("OK\n");
    return 0;
}
