/* check_type()'s ND_FUNCALL case (type.c, add_type_internal) has its own
 * implicit-argument-cast loop, separate from parser.c's cast_funcall_args
 * -- it skips a cast only when same_type(arg_ty, param_ty) holds. For a
 * struct/union argument, same_type() falls back to a plain size compare
 * (no struct-tag-identity special case), so when a struct tag is forward-
 * declared (typedef'd and used in an earlier prototype visible only via a
 * header, from a callee DEFINED in a different translation unit) and only
 * later completed in the caller's own TU, the prototype's parameter Type
 * object is frozen at the incomplete size (0) while the local variable's
 * own type observes the real completed size. same_type() then (correctly)
 * says "not the same type", and the loop wraps the struct ARGUMENT in a
 * cast to that stale zero-size type -- discarding the real size at every
 * later ABI decision (register/stack classification byte-copy count),
 * corrupting the call. parser.c's own cast_funcall_args() already
 * deliberately excludes struct/union from its narrower is_integer/
 * is_flonum check for the same reason; type.c's independent loop didn't.
 *
 * Found via zstd 1.5.7: zstd_compress.c calls ZSTDMT_initCStream_internal
 * (defined only in zstdmt_compress.c) with a `ZSTD_CCtx_params` argument
 * (28 bytes, > 16 -> memory-class ABI arg) whose type is forward-declared
 * in zstd.h and only completed later, in the same file, via
 * zstd_compress_internal.h -- every multi-threaded compression call
 * (`zstd` CLI's default stdin/stdout path) silently passed a garbage
 * struct, tripping `ZSTD_checkCParams()`'s validation and aborting.
 *
 * This needs two genuinely separate translation units: a single-file
 * repro (callee defined in the same file) doesn't reproduce, because
 * parsing the callee's own definition re-derives its function type from
 * the (by-then-complete) struct, masking the stale prototype.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[512], a[600], b[600], exe[600], cmd[2000];
    snprintf(dir, sizeof(dir), "%s/test_incomplete_struct_%d.d", td, pid);
    if (test_mkdir(dir) != 0 && errno != EEXIST) {
        printf("FAIL: mkdir %s\n", dir);
        return 1;
    }
    snprintf(a, sizeof(a), "%s/main.c", dir);
    snprintf(b, sizeof(b), "%s/callee.c", dir);
    snprintf(exe, sizeof(exe), "%s/prog", dir);

    /* main.c: prototype declared while Params is still incomplete
     * (forward typedef only); Params is completed LATER in this same
     * file (needed to declare/init the local var), but check()'s real
     * definition lives only in callee.c -- this file's own function
     * type for check() is never refreshed past the stale prototype. */
    FILE *f = fopen(a, "w");
    if (!f) { printf("FAIL: write %s\n", a); return 1; }
    fputs(
        "#include <stdio.h>\n"
        "typedef struct Params_s Params;\n"
        "int check(Params p, int tail);\n"
        "struct Params_s { int a, b, c, d, e, f, g; };\n"
        "int main(void) {\n"
        "    Params p; p.a=1; p.b=2; p.c=3; p.d=4; p.e=5; p.f=6; p.g=7;\n"
        "    int r = check(p, 100);\n"
        "    printf(\"%d\\n\", r);\n"
        "    return r == 128 ? 0 : 1;\n"
        "}\n",
        f);
    fclose(f);

    f = fopen(b, "w");
    if (!f) { printf("FAIL: write %s\n", b); return 1; }
    fputs(
        "typedef struct Params_s Params;\n"
        "struct Params_s { int a, b, c, d, e, f, g; };\n"
        "int check(Params p, int tail) {\n"
        "    return p.a + p.b + p.c + p.d + p.e + p.f + p.g + tail;\n"
        "}\n",
        f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -O2 -o %s %s %s " NULL_REDIRECT, rcc, exe, a, b);
    if (system(cmd) != 0) {
        printf("FAIL: compile\n");
        return 1;
    }
    snprintf(cmd, sizeof(cmd), "%s", exe);
    int rc = system(cmd);
    remove(a);
    remove(b);
    remove(exe);
    rmdir(dir);
    if (rc != 0) {
        printf("FAIL: exit code %d (expected 0 -- sum should be 128)\n", rc);
        return 1;
    }
    printf("OK\n");
    return 0;
}
