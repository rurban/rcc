/* path_join(dir, file) unconditionally concatenated dir + "/" + file, never
 * checking whether `file` was already an absolute path. A quote-form
 * #include naming an absolute path (`#include "/tmp/foo.h"`) then got
 * "joined" onto the including source file's own directory instead of used
 * as-is -- e.g. source at "/tmp/build/t.c" including "/tmp/foo.h" produced
 * the literal, almost-certainly-nonexistent candidate path
 * "/tmp/build//tmp/foo.h" (dir + separator + already-absolute file), which
 * failed to resolve even though the named file genuinely exists.
 *
 * Every path-joining utility (POSIX os.path.join, Python's os.path.join,
 * std::filesystem::path::append, ...) treats joining an absolute path onto
 * any base as returning that absolute path unchanged -- and so does real
 * GCC for #include "/abs/path" (confirmed empirically). Found via
 * test_pp_tokens_file_boundary.c, an existing, unrelated regression test
 * that happened to embed absolute header paths in quote-form #include
 * directives and started failing once a separate fix (angle-bracket/
 * quote-form cwd-search tightening) removed the trailing bare-filename
 * fallback that had been silently masking this bug all along.
 */
#include <stdio.h>
#include <stdlib.h>
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

    /* Header and source deliberately live in *different* subdirectories so
     * a "dir + file" naive concatenation (rather than "file used as-is")
     * cannot accidentally still resolve to the right place. */
    char hdir[300], sdir[300];
    snprintf(hdir, sizeof(hdir), "%s/absinc_h_%d", td, pid);
    snprintf(sdir, sizeof(sdir), "%s/absinc_s_%d", td, pid);
    { char mk[700]; snprintf(mk, sizeof(mk), "mkdir -p \"%s\" \"%s\"", hdir, sdir); system(mk); }

    char hdr[400], src[400], obj[400], cmd[1400];
    snprintf(hdr, sizeof(hdr), "%s/absinc.h", hdir);
    snprintf(src, sizeof(src), "%s/absinc.c", sdir);
    snprintf(obj, sizeof(obj), "%s/absinc.o", sdir);

    char hdr_inc[400];
    snprintf(hdr_inc, sizeof(hdr_inc), "%s", hdr);
    for (char *p = hdr_inc; *p; p++) if (*p == '\\') *p = '/';

    if (!write_file(hdr, "int abs_include_marker;\n")) { printf("FAIL: write header\n"); return 1; }

    char srcbuf[600];
    snprintf(srcbuf, sizeof(srcbuf),
             "#include \"%s\"\n"
             "int main(void) { return abs_include_marker; }\n",
             hdr_inc);
    if (!write_file(src, srcbuf)) { printf("FAIL: write source\n"); remove(hdr); return 1; }

    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    int rc = system(cmd);
    remove(hdr);
    remove(src);
    remove(obj);
    { char rm[700]; snprintf(rm, sizeof(rm), "rm -rf \"%s\" \"%s\"", hdir, sdir); system(rm); }

    if (rc != 0) {
        printf("FAIL: #include \"<absolute path>\" failed to resolve when the "
               "header lives in a directory unrelated to the including "
               "source file's own directory (rc=%d)\n", rc);
        return 2;
    }

    printf("OK\n");
    return 0;
}
