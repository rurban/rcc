/* #include_next (src/preprocess.c's resolve_include_next()) continues the
 * search using build_search_dirs()'s list: RCC_INCDIR, then a relative
 * "include" fallback (added only when it differs from RCC_INCDIR), then
 * every user -I directory, then the real system include dirs.
 *
 * RCC_INCDIR and the "include" fallback are meant as two alternate
 * *physical* locations for the SAME logical bundled-header set (an
 * installed copy vs. a source checkout's own include/) -- exactly the
 * shape you get running an *installed* rcc (RCC_INCDIR baked to e.g.
 * /usr/local/include/rcc) with the current directory sitting inside a
 * tree that also happens to have its own "include/" subdirectory with a
 * byte-identical copy of rcc's bundled headers (a source checkout, or
 * here, a synthetic duplicate built for this test).
 *
 * resolve_include_next() used to stop advancing past the search list as
 * soon as it found *any* entry whose canonicalized path matched the
 * current file's directory -- but that only escapes ONE of the two
 * physical bundled-header locations. The original #include <stdio.h>
 * resolves through RCC_INCDIR (checked first); #include_next from
 * *inside* that file then advanced past only RCC_INCDIR and landed on
 * the OTHER physical copy at the "include" fallback -- a full,
 * non-trivial file (not a one-line forwarder, so
 * is_noop_forward_to_active() doesn't catch it), but its own include
 * guard is *already defined* by the first copy, so its entire body --
 * including its own #include_next <stdio.h> that would reach the real
 * system header -- is silently skipped. #include_next then resolves
 * "successfully" to a file that contributes nothing, leaving FILE (and
 * everything else the real system stdio.h defines) undeclared, with no
 * error at all.
 *
 * Fixed by having resolve_include_next() skip past BOTH bundled-header
 * slots together whenever the current file was found in either one, not
 * just whichever slot's physical path happened to match.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

static char *read_file_alloc(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

int main(void) {
#ifdef _WIN32
    /* RCC_INCDIR / relative-"include" fallback duplication is a POSIX
     * search-path shape (see build_search_dirs()); not exercised on the
     * Windows target. */
    printf("OK\n");
    return 0;
#else
    const char *rcc_rel = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600], inc_dir[700], src[700], obj[700], cmd[3200];
    char cwd[1024], abs_rcc[1536];

    if (!getcwd(cwd, sizeof(cwd))) {
        printf("FAIL: getcwd failed\n");
        return 1;
    }
    /* find_rcc() returns a path relative to the repo root (e.g. "./rcc")
     * unless RCC=/abs/path overrides it; make it absolute so it still
     * resolves after the subprocess below cd's elsewhere. */
    if (rcc_rel[0] == '/')
        snprintf(abs_rcc, sizeof(abs_rcc), "%s", rcc_rel);
    else
        snprintf(abs_rcc, sizeof(abs_rcc), "%s/%s", cwd, rcc_rel);

    char *stdio_content = read_file_alloc("include/stdio.h");
    if (!stdio_content) {
        printf("FAIL: cannot read include/stdio.h from repo root (cwd=%s)\n", cwd);
        return 2;
    }

    snprintf(dir, sizeof(dir), "%s/test_incnext_dup_%d", td, pid);
    snprintf(inc_dir, sizeof(inc_dir), "%s/include", dir);
    snprintf(src, sizeof(src), "%s/src.c", dir);
    snprintf(obj, sizeof(obj), "%s/src.o", dir);

    if (test_mkdir(dir) != 0 && errno != EEXIST) {
        printf("FAIL: cannot create %s\n", dir);
        free(stdio_content);
        return 3;
    }
    if (test_mkdir(inc_dir) != 0 && errno != EEXIST) {
        printf("FAIL: cannot create %s\n", inc_dir);
        free(stdio_content);
        return 4;
    }

    /* A byte-identical duplicate of rcc's own bundled include/stdio.h,
     * sitting where the "include" search-path fallback will find it
     * once the subprocess below runs with `dir` as its cwd. */
    char dup_stdio[900];
    snprintf(dup_stdio, sizeof(dup_stdio), "%s/stdio.h", inc_dir);
    if (!write_file(dup_stdio, stdio_content)) {
        printf("FAIL: cannot write %s\n", dup_stdio);
        free(stdio_content);
        return 5;
    }
    free(stdio_content);

    if (!write_file(src,
        "#include <stdio.h>\n"
        "int main(void){ FILE *f; (void)f; return 0; }\n")) {
        printf("FAIL: cannot write %s\n", src);
        return 6;
    }

    /* Run with cwd = `dir` so the relative "include" search-path entry
     * resolves to the duplicate created above, while RCC_INCDIR (an
     * absolute path baked into the binary) still resolves to the real
     * bundled headers -- reproducing the two-physical-copies shape. */
    snprintf(cmd, sizeof(cmd), "cd %s && %s -c src.c -o src.o " NULL_REDIRECT,
             dir, abs_rcc);
    int rc = system(cmd);

    remove(obj);
    remove(src);
    remove(dup_stdio);
    rmdir(inc_dir);
    rmdir(dir);

    if (rc != 0) {
        printf("FAIL: #include_next <stdio.h> was swallowed by a duplicate "
               "bundled-header copy at the \"include\" fallback (rc=%d)\n", rc);
        return 7;
    }
    printf("OK\n");
    return 0;
#endif
}
