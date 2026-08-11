/* alloca() gets special codegen (codegen.c, gen_funcall(): call_target ==
 * bi_s_alloca) without needing any declaration in scope -- unlike an
 * ordinary function, there's no prototype for the normal argument-count
 * checker to validate against. Every specialized alloca codegen path
 * unconditionally read node->args (the size expression) assuming exactly
 * one argument was given; calling `alloca()` with zero arguments (a
 * plain, unprototyped implicit-declaration call -- real code hitting
 * this via e.g. `#elif !defined(alloca) __builtin_alloca; #endif`-style
 * feature-probe macros, or simply a typo) left `gen(node->args)` reading
 * a NULL Node, producing an internal "Invalid register -1" crash deep in
 * codegen instead of a real diagnostic.
 *
 * Found via test_muon's own compiler-capability probes (`common/36 has
 * function`), which compiles `int main(void) { return alloca(); }` to
 * check whether `alloca` exists at all -- real GCC treats alloca as a
 * builtin with a known prototype even without any declaration in scope,
 * so it cleanly reports "too few arguments to function 'alloca';
 * expected 1, have 0" instead of crashing.
 *
 * Fixed by validating the argument count right where alloca's special
 * codegen is recognized, rejecting anything other than exactly one
 * argument with the same GCC-style diagnostic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

static int compile(const char *rcc, const char *src, const char *obj) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    return system(cmd);
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char src[600], obj[700];

    /* Case 1: zero arguments -- the exact test_muon "has function"
     * probe shape -- must be a clean compile error, not a crash. */
    snprintf(src, sizeof(src), "%s/test_alloca_argcount1_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_alloca_argcount1_%d.o", td, pid);
    if (!write_file(src, "int main(void) { return alloca(); }\n"))
        return 1;
    int rc1 = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc1 == 0) {
        printf("FAIL: alloca() with 0 arguments wrongly compiled cleanly\n");
        return 2;
    }

    /* Case 2: too many arguments -- must also be a clean compile error. */
    snprintf(src, sizeof(src), "%s/test_alloca_argcount2_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_alloca_argcount2_%d.o", td, pid);
    if (!write_file(src,
        "int main(void) { return (int)(long)alloca(16, 32); }\n"))
        return 4;
    int rc2 = compile(rcc, src, obj);
    remove(src);
    remove(obj);
    if (rc2 == 0) {
        printf("FAIL: alloca() with 2 arguments wrongly compiled cleanly\n");
        return 5;
    }

    /* Case 3: exactly one argument -- the correct, ordinary usage --
     * must still compile and run correctly (the fix must not reject
     * valid calls). */
    snprintf(src, sizeof(src), "%s/test_alloca_argcount3_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_alloca_argcount3_%d.o", td, pid);
    if (!write_file(src,
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "int main(void) {\n"
        "    char *p = alloca(16);\n"
        "    memset(p, 'x', 16);\n"
        "    return p[0] == 'x' && p[15] == 'x' ? 0 : 1;\n"
        "}\n")) {
        return 6;
    }
    int rc3 = compile(rcc, src, obj);
    if (rc3 != 0) {
        remove(src);
        remove(obj);
        printf("FAIL: alloca() with the correct 1 argument failed to compile "
               "(rc=%d)\n", rc3);
        return 7;
    }
    char exe[750], run_cmd[900];
#ifdef _WIN32
    snprintf(exe, sizeof(exe), "%s\\test_alloca_argcount3_%d.exe", td, pid);
#else
    snprintf(exe, sizeof(exe), "%s/test_alloca_argcount3_%d", td, pid);
#endif
    char link_cmd[1300];
    snprintf(link_cmd, sizeof(link_cmd), "%s %s -o %s " NULL_REDIRECT, rcc, src, exe);
    int link_rc = system(link_cmd);
    remove(src);
    remove(obj);
    if (link_rc != 0) {
        printf("FAIL: alloca() with the correct 1 argument failed to link "
               "(rc=%d)\n", link_rc);
        return 8;
    }
#ifdef _WIN32
    snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", exe);
#else
    snprintf(run_cmd, sizeof(run_cmd), "'%s'", exe);
#endif
    int run_rc = system(run_cmd);
    remove(exe);
    if (run_rc != 0) {
        printf("FAIL: alloca() with the correct 1 argument ran incorrectly "
               "(rc=%d)\n", run_rc);
        return 9;
    }

    printf("OK\n");
    return 0;
}
