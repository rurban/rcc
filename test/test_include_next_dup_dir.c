/* A duplicate `-I` flag pointing at the SAME directory twice (extremely
 * common in autotools-generated build commands -- e.g. wget2's
 * libwget/Makefile: `-I../lib -I../lib`) broke `#include_next`: after
 * finding the search-list entry that supplied the current file,
 * resolve_include_next() advanced past only the FIRST occurrence of
 * that directory, landing back on the SECOND (duplicate) occurrence --
 * the identical physical file the current header's own #include_next
 * came from. gnulib-style wrapper headers deliberately use a
 * re-enterable "split double-inclusion guard" (no ordinary header
 * guard blocking a second pass), so re-finding the same file recursed
 * through its own #include_next again, forever, until rcc's
 * include-depth limit tripped ("Include depth exceeded"). Broke
 * wget2's libwget/base64.c (and every other TU including gnulib's
 * lib/stddef.h) with `-I../lib -I../lib`.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_inc_next_dup_%d.d", td, pid);
#if defined(_WIN32)
    (void)rcc; (void)dir;
    return 0;
#else
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }

    /* wrapdir/wrap.h: a gnulib-style wrapper that chains onward via
     * #include_next -- reachable via a directory listed TWICE on the
     * command line. */
    char wrapdir[650], realdir[650];
    snprintf(wrapdir, sizeof(wrapdir), "%s/wrap", dir);
    snprintf(realdir, sizeof(realdir), "%s/real", dir);
    if (mkdir(wrapdir, 0755) != 0) { printf("FAIL: mkdir wrapdir\n"); return 1; }
    if (mkdir(realdir, 0755) != 0) { printf("FAIL: mkdir realdir\n"); return 1; }

    char wraphdr[700], realhdr[700], src[700];
    snprintf(wraphdr, sizeof(wraphdr), "%s/wrap.h", wrapdir);
    snprintf(realhdr, sizeof(realhdr), "%s/wrap.h", realdir);
    snprintf(src, sizeof(src), "%s/main.c", dir);

    FILE *fp = fopen(wraphdr, "w");
    if (!fp) { printf("FAIL: open wraphdr\n"); return 1; }
    fputs("#ifndef WRAP_SEEN_ONCE\n#define WRAP_SEEN_ONCE\n#endif\n"
          "#include_next <wrap.h>\n", fp);
    fclose(fp);

    fp = fopen(realhdr, "w");
    if (!fp) { printf("FAIL: open realhdr\n"); return 1; }
    fputs("#define WRAP_REACHED_REAL 1\n", fp);
    fclose(fp);

    fp = fopen(src, "w");
    if (!fp) { printf("FAIL: open src\n"); return 1; }
    fputs("#include <wrap.h>\n"
          "#if !defined(WRAP_REACHED_REAL)\n"
          "#error include_next did not reach the real header\n"
          "#endif\n"
          "int main(void) { return 0; }\n", fp);
    fclose(fp);

    char out[700], cmd[2600];
    snprintf(out, sizeof(out), "%s/a.out", dir);
    /* -I wrapdir -I wrapdir (duplicate) -I realdir: the duplicate must
     * not make #include_next loop back on wrap/wrap.h forever. */
    snprintf(cmd, sizeof(cmd),
             "%s -I%s -I%s -I%s -o %s %s " NULL_REDIRECT,
             rcc, wrapdir, wrapdir, realdir, out, src);
    int rc = system(cmd);
    if (rc != 0) { printf("FAIL: compile with duplicate -I failed (exit %d)\n", rc); return 1; }

    printf("ALL INCLUDE_NEXT DUP-DIR TESTS PASSED\n");
    return 0;
#endif
}
