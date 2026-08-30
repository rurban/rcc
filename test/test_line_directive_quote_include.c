/* Regression test: `#include "..."` from a file whose reported name was
 * remapped by a `#line N "other-name"` directive (as Ragel/yacc/bison
 * generated sources routinely do, e.g. `#line 1 "char_ref.rl"` at the top
 * of a generated `char_ref.c`) failed to find a header sitting right next
 * to the *real* on-disk file.
 *
 * `resolve_include()` (preprocess.c) computed the quote-include lookup
 * directory from `lvl->filename` -- the file's *reported* name, which
 * `#line` can rewrite to an arbitrary bare string (e.g. "char_ref.rl")
 * unrelated to any real directory. `path_dirname("char_ref.rl")` is "."
 * instead of the file's real directory, so `#include "char_ref.h"`
 * spuriously failed with "include file not found" even though the header
 * genuinely sits next to the real .c file.
 *
 * Fixed by adding a stable `PPLvl.incbase` field, set once from the
 * as-invoked/as-included path when the level is pushed and never touched
 * by `#line`, and using it (instead of the mutable `filename`) for the
 * quote-include directory search. This must not regress the sibling
 * symlink fix (test_symlink_quote_include.c), which relies on the
 * as-invoked (non-realpath-resolved) identity being preserved too --
 * `incbase` uses the exact same as-invoked value that fix depends on.
 *
 * This test self-drives rcc: creates a small directory with a header and
 * a .c file that renames itself via `#line` before `#include`-ing that
 * header by name, and confirms `rcc -o out file.c` compiles and links
 * successfully (fails with the old bug: "include file 'lineinc.h' not
 * found").
 */
#include <stdio.h>
#include <stdlib.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_line_inc_%d.d", td, pid);
    if (test_mkdir(dir) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }

    char header[700], src_c[700];
    snprintf(header, sizeof(header), "%s/lineinc.h", dir);
    snprintf(src_c, sizeof(src_c), "%s/gen.c", dir);

    FILE *fp = fopen(header, "w");
    if (!fp) { printf("FAIL: create %s\n", header); return 1; }
    fprintf(fp, "#define LINE_INC_VALUE 4343\n");
    fclose(fp);

    fp = fopen(src_c, "w");
    if (!fp) { printf("FAIL: create %s\n", src_c); return 1; }
    fprintf(fp,
            "#line 1 \"gen.rl\"\n"
            "#include \"lineinc.h\"\n"
            "#include <stdio.h>\n"
            "int main(void) { printf(\"%%d\\n\", LINE_INC_VALUE); return 0; }\n");
    fclose(fp);

    char out[700], cmd[2600];
    snprintf(out, sizeof(out), "%s/out", dir);
    snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, out, src_c);
    int rc = system(cmd);
    remove(header);
    remove(src_c);
    if (rc != 0) {
        printf("FAIL: rcc failed to compile a #line-renamed source file whose "
               "quote-include lives next to its real path (exit %d)\n", rc);
        rmdir(dir);
        return 1;
    }

    rc = system(out);
    remove(out);
    rmdir(dir);
    if (rc != 0) { printf("FAIL: compiled binary exited %d\n", rc); return 1; }
    printf("OK\n");
    return 0;
}
