/* The fallback GCC-linker invocation (main.c, reached whenever the native
 * linker declines -- e.g. any program that needs dynamic libc symbols
 * like printf/fprintf) built its `system()` command line by substituting
 * paths (the -o output path, each object file, the bundled mingw/darwin
 * runtime object) via a bare, unquoted `%s`. A path containing a space
 * split into extra shell words: `ld` then reported "cannot find
 * <tail-after-the-space>: No such file or directory" instead of ever
 * seeing the single, intended path.
 *
 * Real-world trigger: the muon build-system's own test harness
 * (test_muon third-party target) names one of its native test
 * subdirectories literally "4 tryrun" (a space in the directory name);
 * compiling and linking a trivial printf-using probe program into
 * ".../native/4 tryrun/.muon/compiler_check_exe" hit this exact bug and
 * broke every one of muon's own compiler-capability probes that used it.
 *
 * Fixed by double-quoting every path substituted into the linker command
 * string (matching the existing path_is_shell_safe()-gated double-quote
 * convention the -S disassembly invocation already uses) and rejecting
 * -- rather than attempting to escape -- any path containing a real
 * shell metacharacter first.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "test_common.h"

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600], src[700], exe[750], cmd[2400], run_cmd[900];

#ifdef _WIN32
    snprintf(dir, sizeof(dir), "%s\\test_link_space_%d dir", td, pid);
#else
    snprintf(dir, sizeof(dir), "%s/test_link_space_%d dir", td, pid);
#endif
    if (test_mkdir(dir) != 0 && errno != EEXIST) {
        printf("FAIL: cannot create %s\n", dir);
        return 1;
    }
#ifdef _WIN32
    snprintf(src, sizeof(src), "%s\\t.c", dir);
    snprintf(exe, sizeof(exe), "%s\\t out.exe", dir);
#else
    snprintf(src, sizeof(src), "%s/t.c", dir);
    snprintf(exe, sizeof(exe), "%s/t out", dir);
#endif

    /* Use printf/fprintf so this reaches the fallback GCC linker path
     * (the native linker handles simple self-contained programs itself
     * and never hits the buggy code path at all -- see main.c's
     * "Try the native linker first" comment). */
    if (!write_file(src,
        "#include <stdio.h>\n"
        "int main(void) {\n"
        "    printf(\"%s\\n\", \"stdout\");\n"
        "    fprintf(stderr, \"%s\\n\", \"stderr\");\n"
        "    return 0;\n"
        "}\n")) {
        printf("FAIL: cannot write %s\n", src);
        return 2;
    }

    snprintf(cmd, sizeof(cmd), "%s \"%s\" -o \"%s\" " NULL_REDIRECT, rcc, src, exe);
    int rc = system(cmd);
    if (rc != 0) {
        remove(src);
        printf("FAIL: compile+link into a path with a space failed (rc=%d)\n", rc);
        return 3;
    }

#ifdef _WIN32
    snprintf(run_cmd, sizeof(run_cmd), "\"%s\" " NULL_REDIRECT, exe);
#else
    snprintf(run_cmd, sizeof(run_cmd), "'%s' " NULL_REDIRECT, exe);
#endif
    int run_rc = system(run_cmd);

    remove(src);
    remove(exe);
    rmdir(dir);

    if (run_rc != 0) {
        printf("FAIL: linked binary at a path with a space didn't run "
               "correctly (rc=%d)\n", run_rc);
        return 4;
    }

    printf("OK\n");
    return 0;
}
