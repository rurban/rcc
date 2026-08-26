/* rcc's bundled include/iconv.h used to be entirely self-contained on
 * every platform (just iconv_t/iconv_open/iconv/iconv_close, declared
 * from scratch, no #include_next) -- unlike stdio.h/wchar.h/limits.h/
 * math.h, which all chain onward to the platform's real header. Since
 * RCC_INCDIR is always searched before every -I directory (see
 * build_search_dirs()), that made rcc's own iconv.h win outright for
 * every `#include <iconv.h>`, permanently hiding a project's own -I
 * iconv.h override -- and whatever else that project's own header
 * happened to also provide.
 *
 * Found via ksh93/ast: its own src/lib/libast/std/iconv.h wraps
 * ast_iconv.h, which (as an ast-internal side effect unrelated to
 * iconv itself) also pulls in ccode.h's CC_bel/CC_esc/CC_vt constants
 * that string/chresc.c relies on getting via `#include <iconv.h>`.
 * With rcc's own iconv.h winning outright, ast's own std/iconv.h was
 * never reached, leaving CC_bel undeclared.
 *
 * Fixed by having rcc's own iconv.h #include_next <iconv.h> on every
 * target with a reliable native header to chain to (everywhere except
 * Windows, which has none, and macOS, whose libiconv-provided one
 * isn't guaranteed present) -- matching stdio.h's own established
 * pattern exactly.
 *
 * Functionality alone (iconv_open/iconv/iconv_close actually working)
 * can't distinguish the fix: rcc's own from-scratch declarations are
 * ABI-identical to the real header's, so the real libc implementation
 * works fine either way. What must be verified is that #include_next
 * actually reaches a project's own -I override -- reproduced directly,
 * mirroring ast's std/iconv.h -> ast_iconv.h -> ccode.h shape: a
 * generated -I iconv.h wrapper that also defines an unrelated marker
 * macro, which only becomes visible if the chain is followed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

int main(void) {
#if defined(_WIN32) || defined(__APPLE__)
    /* Neither target has a reliable native <iconv.h> to chain to --
     * rcc's own iconv.h stays self-contained on both by design (see
     * its own #if defined(_WIN32) || defined(__APPLE__) branch);
     * nothing to verify. */
    printf("OK\n");
    return 0;
#else
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char dir[600], override[700], src[600], obj[700], cmd[2400];

    snprintf(dir, sizeof(dir), "%s/test_iconv_%d", td, pid);
    snprintf(override, sizeof(override), "%s/iconv.h", dir);
    snprintf(src, sizeof(src), "%s/test_iconv_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_iconv_%d.o", td, pid);

    if (test_mkdir(dir) != 0 && errno != EEXIST) {
        printf("FAIL: cannot create %s\n", dir);
        return 1;
    }

    /* Mirrors ast's std/iconv.h -> ast_iconv.h -> ccode.h shape: a real,
     * non-forwarding -I override that (as an unrelated side effect)
     * also defines something else -- must be reached, not shadowed by
     * rcc's own bundled iconv.h. */
    if (!write_file(override,
        "#define ICONV_CHAIN_REACHED 1\n"
        "typedef void *iconv_t;\n"
        "iconv_t iconv_open(const char *tocode, const char *fromcode);\n"
        "unsigned long iconv(iconv_t cd, char **inbuf, unsigned long *inbytesleft,\n"
        "                     char **outbuf, unsigned long *outbytesleft);\n"
        "int iconv_close(iconv_t cd);\n")) {
        printf("FAIL: cannot write %s\n", override);
        return 2;
    }
    if (!write_file(src,
        "#include <iconv.h>\n"
        "#ifndef ICONV_CHAIN_REACHED\n"
        "#error -I iconv.h override was not reached\n"
        "#endif\n"
        "int main(void){return 0;}\n")) {
        printf("FAIL: cannot write %s\n", src);
        remove(override);
        remove(dir);
        return 3;
    }

    snprintf(cmd, sizeof(cmd), "%s -I%s -c %s -o %s " NULL_REDIRECT,
             rcc, dir, src, obj);
    int rc = system(cmd);

    remove(src);
    remove(override);
    remove(obj);
    remove(dir);

    if (rc != 0) {
        printf("FAIL: #include_next <iconv.h> did not reach the -I override (rc=%d)\n", rc);
        return 4;
    }
    printf("OK\n");
    return 0;
#endif
}
