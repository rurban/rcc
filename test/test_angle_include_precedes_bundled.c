/* GCC precedence for an angle-form `#include <file>`: -I dirs, then
 * gcc's own private freestanding-header dir (e.g. .../gcc/16/include,
 * holding stddef.h/stdint.h/limits.h/stdbool.h/float.h/stdarg.h), then
 * the real system dirs -- confirmed directly against real gcc (a
 * `-Idir` with `dir/stddef.h` present, chaining onward via its own
 * `#include_next <stddef.h>`, satisfies `#include <stddef.h>` ahead of
 * gcc's own bundled one).
 *
 * build_search_dirs() (src/preprocess.c) used to check RCC_INCDIR
 * before every -I directory for the angle form specifically (unlike
 * the quote form, fixed earlier -- see test_iquote_precedes_bundled.c),
 * on the theory that ast/ksh93's own relative-escape `#include
 * <../include/wchar.h>` idiom needed it. It didn't: that idiom is
 * driven by resolve_include()'s own "skip a match already active on
 * the include stack" guard and resolve_include_next()'s
 * is_noop_forward_to_active() skip, neither of which depends on
 * RCC_INCDIR's position in the search list (confirmed against ksh93's
 * own third_party build after the reorder: unchanged, still passing).
 *
 * Keeping RCC_INCDIR ahead of -I broke every project shipping its own
 * gnulib-style "-I override + #include_next onward" replacement header
 * sharing a name with something rcc bundles as a fully self-contained
 * header (no #include_next of its own): findutils' gl/lib/stddef.h
 * (providing gl_unreachable(), relying on being the FIRST responder to
 * every `#include <stddef.h>` in the TU to track and clean up the
 * __need_size_t/__need_NULL/... "extract just this one type" protocol
 * glibc's own deep internal headers use, e.g. bits/types/struct_iovec.h)
 * was silently unreachable -- an undiagnosed dead end: "undefined
 * reference to `gl_unreachable'" only at link time, no compile-time
 * signal at all. Reproduced directly here with the same shape: a
 * generated -I override for each of the six affected headers, each
 * also defining something else that must be reached, not shadowed by
 * rcc's own bundled copy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

static int write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(contents, f);
    fclose(f);
    return 1;
}

static int check_header(const char *rcc, const char *dir, const char *name,
                         const char *marker) {
    char override[700], src[700], obj[700], cmd[2600], body[400], srcbody[600];
    snprintf(override, sizeof(override), "%s/%s", dir, name);
    snprintf(src, sizeof(src), "%s/t_%s.c", dir, marker);
    snprintf(obj, sizeof(obj), "%s/t_%s.o", dir, marker);
    snprintf(body, sizeof(body),
        "#define %s 1\n#include_next <%s>\n", marker, name);
    if (!write_file(override, body)) return 0;
    snprintf(srcbody, sizeof(srcbody),
        "#include <%s>\n#ifndef %s\n#error override was not reached\n#endif\n"
        "int main(void){return 0;}\n",
        name, marker);
    if (!write_file(src, srcbody)) { remove(override); return 0; }
    snprintf(cmd, sizeof(cmd), "%s -I%s -c %s -o %s " NULL_REDIRECT,
             rcc, dir, src, obj);
    int rc = system(cmd);
    remove(override);
    remove(src);
    remove(obj);
    return rc == 0;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600];
    snprintf(dir, sizeof(dir), "%s/test_angle_prec_%d.d", td, pid);
    if (mkdir(dir, 0755) != 0) { printf("FAIL: mkdir %s\n", dir); return 1; }

    static const struct { const char *name, *marker; } headers[] = {
        {"stddef.h", "ANGLE_STDDEF_OVERRIDE"},
        {"stdint.h", "ANGLE_STDINT_OVERRIDE"},
        {"limits.h", "ANGLE_LIMITS_OVERRIDE"},
        {"stdbool.h", "ANGLE_STDBOOL_OVERRIDE"},
        {"float.h", "ANGLE_FLOAT_OVERRIDE"},
        {"stdarg.h", "ANGLE_STDARG_OVERRIDE"},
    };
    int failed = 0;
    for (size_t i = 0; i < sizeof(headers) / sizeof(headers[0]); i++) {
        if (!check_header(rcc, dir, headers[i].name, headers[i].marker)) {
            printf("FAIL: <%s> -I override with #include_next was not reached\n",
                   headers[i].name);
            failed = 1;
        }
    }
    rmdir(dir);
    if (failed) return 2;

    printf("OK\n");
    return 0;
}
