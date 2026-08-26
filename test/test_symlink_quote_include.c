/* Regression test: `#include "..."` from a SYMLINKED source file
 * resolved the "same directory as the including file" search relative
 * to the symlink's REAL (readlink-resolved) target directory, instead
 * of the symlink's own invocation directory -- the opposite of real
 * gcc's behavior.
 *
 * `resolve_include()` (preprocess.c) used to take a separate
 * `curr_file`/`curr_display` pair and compute the quote-include lookup
 * directory via `path_dirname(curr_file)`, where `curr_file` was
 * `lvl->fpath` -- `full_path()`'s `realpath()`-resolved, symlink-
 * following identity of the current file. A build convention like
 * `ln -s real/impl.c wrapper.c` (compiled by naming the symlink
 * directly -- e.g. slimcc's own `platform.c -> platform/linux-glibc-
 * generic.c`) then had `#include "slimcc.h"` (living next to
 * wrapper.c, NOT inside real/) resolve relative to real/'s directory
 * instead, producing a spurious "include file not found" for a header
 * genuinely sitting right next to the compiled file. Fixed by
 * collapsing `resolve_include()` to a single `curr_file` parameter
 * (always the as-invoked path, `lvl->filename` -- never symlink-
 * resolved) used consistently for both the directory search and the
 * `#line`-marker display name, matching `__has_include`'s own call
 * site, which already passed the as-invoked path for this purpose.
 *
 * This test self-drives rcc: creates a small directory tree with a
 * symlinked .c file whose real target lives in a different
 * subdirectory than a header it `#include`s by name, and confirms
 * `rcc -o out link.c` compiles and links successfully (fails with the
 * old bug: "include file 'sym_header.h' not found").
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600], sub[620];
    snprintf(dir, sizeof(dir), "%s/test_symlink_inc_%d.d", td, pid);
    snprintf(sub, sizeof(sub), "%s/sub", dir);
#if defined(_WIN32)
    /* Symlinks require elevated privileges by default on Windows CI runners. */
    (void)rcc; (void)td; (void)dir; (void)sub;
    return 0;
#else
    if (test_mkdir(dir) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }
    if (test_mkdir(sub) != 0) { printf("FAIL: mkdir %s\n", sub); return 1; }

    char header[700], real_c[700], link_c[700];
    snprintf(header, sizeof(header), "%s/sym_header.h", dir);
    snprintf(real_c, sizeof(real_c), "%s/real_impl.c", sub);
    snprintf(link_c, sizeof(link_c), "%s/link.c", dir);

    FILE *fp = fopen(header, "w");
    if (!fp) { printf("FAIL: create %s\n", header); return 1; }
    fprintf(fp, "#define SYM_VALUE 4242\n");
    fclose(fp);

    fp = fopen(real_c, "w");
    if (!fp) { printf("FAIL: create %s\n", real_c); return 1; }
    fprintf(fp,
            "#include <stdio.h>\n"
            "#include \"sym_header.h\"\n"
            "int main(void) { printf(\"%%d\\n\", SYM_VALUE); return 0; }\n");
    fclose(fp);

    /* link.c (in `dir`, next to sym_header.h) -> sub/real_impl.c
     * (in `sub`, where sym_header.h does NOT exist). */
    if (symlink("sub/real_impl.c", link_c) != 0) {
        printf("FAIL: symlink %s -> sub/real_impl.c\n", link_c);
        return 1;
    }

    char out[700], cmd[2600];
    snprintf(out, sizeof(out), "%s/out", dir);
    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, out, link_c);
    int rc = system(cmd);
    remove(header);
    remove(real_c);
    remove(link_c);
    if (rc != 0) {
        printf("FAIL: rcc failed to compile a symlinked source file whose "
               "quote-include lives next to the symlink itself (exit %d)\n", rc);
        rmdir(sub);
        rmdir(dir);
        return 1;
    }

    rc = system(out);
    remove(out);
    rmdir(sub);
    rmdir(dir);
    if (rc != 0) { printf("FAIL: compiled binary exited %d\n", rc); return 1; }
    printf("OK\n");
    return 0;
#endif
}
