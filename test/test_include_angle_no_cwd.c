/* Neither angle-bracket nor quote-form #include (nor __has_include) may
 * ever implicitly search the compiler process's current working
 * directory -- only the including file's own directory (quote form
 * only), explicit -I/-iquote directories, and the built-in system list.
 * `resolve_include()` had a trailing
 * `if (file_exists(spec)) return canonical_path(spec);` fallback that
 * applied unconditionally after the search-directory loop, for BOTH
 * angle and quote forms -- so `#include <name.h>` or `#include "name.h"`
 * could silently resolve to an unrelated same-named file sitting in the
 * compiler's cwd even though no search directory (source file's own
 * directory, -I, or system) actually provided it. Real gcc never does
 * this for either form (verified directly for both).
 *
 * Found via test_muon's own `common/189 check header` and
 * `common/203 find_library and headers` capability probes:
 * - `common/189` (meson/muon's `check_header()` compiler method)
 *   deliberately copies a same-named decoy file (`ouagadougou.h`, a
 *   placeholder name chosen to be obviously nonexistent) into the build
 *   directory next to the compiled test file specifically to confirm
 *   the compiler does NOT accidentally find it via cwd search when
 *   using angle brackets.
 * - `common/203` (meson/muon's `find_library(has_headers:)` method)
 *   checks `__has_include("foo.h")` (quote form) from a compiled file
 *   one directory level *below* a real `foo.h`, with the compiler's own
 *   cwd set to that parent directory (where `foo.h` genuinely sits) but
 *   no explicit `-I`/`header_include_directories` for it -- must NOT be
 *   found either, since it's neither in the source's own directory nor
 *   any search directory, only in the unrelated cwd.
 *
 * Fixed by removing the cwd fallback for both #include forms entirely,
 * except for #embed (a separate construct with its own distinct
 * search-path semantics in real GCC --embed-dir=, unrelated to -I, that
 * rcc doesn't implement; rcc pragmatically reuses its #include search
 * machinery for #embed and intentionally keeps the cwd fallback there
 * -- see test_embed.c's own angle-bracket case).
 *
 * On _WIN32, this test writes into the process's own inherited cwd with
 * plain relative filenames rather than get_tmpdir() -- matching
 * test_embed.c's own established workaround for this harness's wine
 * environment, where TEMP/TMP aren't reliably set.
 */
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
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
    int pid = (int)getpid();
    char srcname[80], objname[80], decoy_bare[80], decoy_path[600];
#ifdef _WIN32
    /* Plain relative filenames in the inherited cwd -- see file comment.
     * decoy_bare and decoy_path coincide here (no directory prefix). */
    snprintf(srcname, sizeof(srcname), "test_incang_%d.c", pid);
    snprintf(objname, sizeof(objname), "test_incang_%d.o", pid);
    snprintf(decoy_bare, sizeof(decoy_bare), "decoy_header_xyz_%d.h", pid);
    snprintf(decoy_path, sizeof(decoy_path), "%s", decoy_bare);
#else
    const char *td = get_tmpdir();
    snprintf(srcname, sizeof(srcname), "%s/test_incang_%d.c", td, pid);
    snprintf(objname, sizeof(objname), "%s/test_incang_%d.o", td, pid);
    /* decoy_bare is the *bare* filename embedded literally in the
     * #include directive text (a genuine relative reference, resolved
     * against the source file's own directory) -- decoy_path is where
     * the file actually gets written (same directory as srcname). */
    snprintf(decoy_bare, sizeof(decoy_bare), "decoy_header_xyz_%d.h", pid);
    snprintf(decoy_path, sizeof(decoy_path), "%s/%s", td, decoy_bare);
#endif

    /* Both the compiled source and the decoy header sit in the same
     * directory -- mirroring muon's own check_header() shape (the decoy
     * copied into the build directory alongside the test source). */
    if (!write_file(decoy_path, "int this_should_never_be_seen;\n")) return 2;
    char src_content_angle[200];
    snprintf(src_content_angle, sizeof(src_content_angle),
             "#include <%s>\nint main(void) { return 0; }\n", decoy_bare);
    if (!write_file(srcname, src_content_angle)) return 3;

    int rc = compile(rcc, srcname, objname);
    remove(srcname);
    remove(objname);
    remove(decoy_path);
    if (rc == 0) {
        printf("FAIL: #include <decoy_header_xyz.h> wrongly found the cwd-adjacent "
               "decoy header via angle-bracket search\n");
        return 4;
    }

    /* The equivalent quote-include form must still find a same-named
     * decoy sitting next to the source (quote includes correctly search
     * the compiled file's own directory) -- this leniency must be
     * unaffected by the angle-bracket fix. Uses the bare filename in
     * the #include directive, exactly like the angle-bracket case
     * above, so this genuinely exercises the source-relative search
     * path rather than an absolute-path #include (which would work
     * regardless of any search-directory logic). */
    if (!write_file(decoy_path, "int fine;\n")) return 5;
    char src_content_quote[200];
    snprintf(src_content_quote, sizeof(src_content_quote),
             "#include \"%s\"\nint main(void) { return 0; }\n", decoy_bare);
    if (!write_file(srcname, src_content_quote)) return 6;
    int rc2 = compile(rcc, srcname, objname);
    remove(srcname);
    remove(objname);
    remove(decoy_path);
    if (rc2 != 0) {
        printf("FAIL: #include \"decoy_header_xyz.h\" (quote form, source-relative "
               "search) wrongly failed to compile (rc=%d)\n", rc2);
        return 7;
    }

#ifndef _WIN32
    /* Quote form must also NOT find a same-named file that exists ONLY
     * in the compiler process's own cwd, when the compiled source lives
     * in an unrelated subdirectory (one level below cwd) and no search
     * directory covers it -- exactly the common/203 shape (muon
     * compiles a relative ".muon/test.c" with cwd set to the parent
     * build directory, where a same-named header genuinely sits, but
     * .muon/ itself does not, and no -I covers either). Uses chdir()
     * (inherited by the system()-spawned rcc child) so the decoy's
     * directory genuinely becomes the compiler's cwd while the source
     * is compiled via a relative path pointing down into a
     * subdirectory -- unlike cases 1+2 above, which never distinguish
     * "found via cwd" from "found via the source's own directory"
     * whenever those two happen to coincide. Non-Windows only:
     * subdirectory creation under this harness's wine environment has
     * proven unreliable (see test_embed.c's own get_tmpdir()
     * workaround); cases 1+2 above already cover the core fix on every
     * target.
     */
    char orig_cwd[600];
    if (!getcwd(orig_cwd, sizeof(orig_cwd))) return 8;
    char rcc_abs[600];
    const char *rcc3 = rcc;
    if (rcc3[0] == '.') {
        const char *rel = (rcc3[1] == '/') ? rcc3 + 2 : rcc3;
        snprintf(rcc_abs, sizeof(rcc_abs), "%s/%s", orig_cwd, rel);
        rcc3 = rcc_abs;
    }
    char subname[32];
    snprintf(subname, sizeof(subname), "sub_%d", pid);
    char subdir[700];
    snprintf(subdir, sizeof(subdir), "%s/%s", td, subname);
    { char mk[700]; snprintf(mk, sizeof(mk), "mkdir -p '%s'", subdir); system(mk); }
    if (!write_file(decoy_path, "int not_here;\n")) return 9;
    char sub_src_full[700];
    snprintf(sub_src_full, sizeof(sub_src_full), "%s/t.c", subdir);
    if (!write_file(sub_src_full, src_content_quote)) return 10;
    char sub_src_rel[64], sub_obj_rel[64];
    snprintf(sub_src_rel, sizeof(sub_src_rel), "%s/t.c", subname);
    snprintf(sub_obj_rel, sizeof(sub_obj_rel), "%s/t.o", subname);
    int chdir_rc = chdir(td);
    int rc3 = chdir_rc == 0 ? compile(rcc3, sub_src_rel, sub_obj_rel) : -1;
    chdir(orig_cwd);
    remove(sub_src_full);
    { char objp[750]; snprintf(objp, sizeof(objp), "%s/t.o", subdir); remove(objp); }
    remove(decoy_path);
    { char rm[700]; snprintf(rm, sizeof(rm), "rm -rf '%s'", subdir); system(rm); }
    if (chdir_rc != 0) return 11;
    if (rc3 == 0) {
        printf("FAIL: #include \"decoy_header_xyz.h\" wrongly found a same-named "
               "file sitting only in the compiler's cwd, not the source's own "
               "directory\n");
        return 12;
    }
#endif

    printf("OK\n");
    return 0;
}
