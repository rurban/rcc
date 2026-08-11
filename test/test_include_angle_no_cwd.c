/* Angle-bracket #include (and __has_include) must never implicitly search
 * the compiler process's current working directory -- only explicit
 * -I/-iquote directories and the built-in system list. `resolve_include()`
 * had a trailing `if (file_exists(spec)) return canonical_path(spec);`
 * fallback that applied unconditionally after the search-directory loop,
 * regardless of angle vs quote form -- so `#include <name.h>` could
 * silently resolve to an unrelated same-named file sitting in the
 * compiler's cwd even though no search directory (-I or system) actually
 * provided it. Real gcc never does this (verified directly).
 *
 * Found via test_muon's own `common/189 check header` capability probe
 * (meson/muon's `check_header()` compiler method), which deliberately
 * copies a same-named decoy file (`ouagadougou.h`, a placeholder name
 * chosen to be obviously nonexistent) into the build directory next to
 * the compiled test file specifically to confirm the compiler does NOT
 * accidentally find it via cwd search when using angle brackets.
 *
 * Fixed by only reaching that trailing cwd-relative fallback for quote
 * includes (`#include "..."`, where C's own search rules already permit
 * a directory-independent fallback) or for #embed (a separate construct
 * with its own distinct search-path semantics in real GCC --embed-dir=,
 * unrelated to -I, that rcc doesn't implement; rcc pragmatically reuses
 * its #include search machinery for #embed and intentionally keeps the
 * cwd fallback there -- see test_embed.c's own angle-bracket case).
 *
 * On _WIN32, this test writes into the process's own inherited cwd with
 * plain relative filenames rather than get_tmpdir() -- matching
 * test_embed.c's own established workaround for this harness's wine
 * environment, where TEMP/TMP aren't reliably set.
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

static int compile(const char *rcc, const char *src, const char *obj) {
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s -c %s -o %s " NULL_REDIRECT, rcc, src, obj);
    return system(cmd);
}

int main(void) {
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char srcname[80], objname[80], decoyname[80];
#ifdef _WIN32
    /* Plain relative filenames in the inherited cwd -- see file comment. */
    snprintf(srcname, sizeof(srcname), "test_incang_%d.c", pid);
    snprintf(objname, sizeof(objname), "test_incang_%d.o", pid);
    snprintf(decoyname, sizeof(decoyname), "decoy_header_xyz_%d.h", pid);
#else
    const char *td = get_tmpdir();
    snprintf(srcname, sizeof(srcname), "%s/test_incang_%d.c", td, pid);
    snprintf(objname, sizeof(objname), "%s/test_incang_%d.o", td, pid);
    snprintf(decoyname, sizeof(decoyname), "%s/decoy_header_xyz_%d.h", td, pid);
#endif

    /* Both the compiled source and the decoy header sit in the same
     * directory -- mirroring muon's own check_header() shape (the decoy
     * copied into the build directory alongside the test source). */
    if (!write_file(decoyname, "int this_should_never_be_seen;\n")) return 2;
    char src_content_angle[200];
    snprintf(src_content_angle, sizeof(src_content_angle),
             "#include <%s>\nint main(void) { return 0; }\n", decoyname);
    if (!write_file(srcname, src_content_angle)) return 3;

    int rc = compile(rcc, srcname, objname);
    remove(srcname);
    remove(objname);
    remove(decoyname);
    if (rc == 0) {
        printf("FAIL: #include <decoy_header_xyz.h> wrongly found the cwd-adjacent "
               "decoy header via angle-bracket search\n");
        return 4;
    }

    /* The equivalent quote-include form must still find a same-named
     * decoy sitting next to the source (quote includes correctly search
     * the compiled file's own directory) -- this leniency must be
     * unaffected by the angle-bracket fix. */
    if (!write_file(decoyname, "int fine;\n")) return 5;
    char src_content_quote[200];
    snprintf(src_content_quote, sizeof(src_content_quote),
             "#include \"%s\"\nint main(void) { return 0; }\n", decoyname);
    if (!write_file(srcname, src_content_quote)) return 6;
    int rc2 = compile(rcc, srcname, objname);
    remove(srcname);
    remove(objname);
    remove(decoyname);
    if (rc2 != 0) {
        printf("FAIL: #include \"decoy_header_xyz.h\" (quote form, source-relative "
               "search) wrongly failed to compile (rc=%d)\n", rc2);
        return 7;
    }

    printf("OK\n");
    return 0;
}
