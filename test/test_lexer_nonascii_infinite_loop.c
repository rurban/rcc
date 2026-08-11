/* lex_one() (src/lexer.c) dispatches any byte >= 0x80 by decoding one UTF-8
 * codepoint and checking is32_ident1() (is this a valid identifier-START
 * character?). When it wasn't (e.g. U+00B7 MIDDLE DOT -- General Category
 * Po, punctuation, not a letter -- appearing bare, not as part of an
 * adjacent ASCII identifier's own continuation scan), the code did a bare
 * `continue` without ever advancing `p`. The outer dispatch loop then
 * re-examined the exact same byte position, re-decoded the same
 * codepoint, and reached the exact same "not identifier-start" verdict
 * again -- forever. Any source byte sequence >= 0x80 that decodes to a
 * codepoint outside is32_ident1()'s tables hung rcc indefinitely (a
 * genuine DoS-class bug: it doesn't require malformed UTF-8, just an
 * ordinary punctuation-class Unicode character sitting outside a string/
 * char literal and outside an identifier's own continuation scan).
 *
 * Real-world trigger: golang/go's go1.4 bootstrap sources (test_go),
 * include/runtime/funcdata.h's NO_LOCAL_POINTERS macro body:
 *   #define NO_LOCAL_POINTERS ... runtime\xc2\xb7no_pointers_stackmap(SB)
 * -- a Plan9/Go-toolchain "package\xc2\xb7symbol" assembly-name
 * convention. Compiling any .c file that #includes this header (whether
 * or not the macro is ever expanded -- a #define's replacement list is
 * tokenized unconditionally) hung rcc forever tokenizing the middle dot.
 *
 * Fixed by always advancing `p` on this path: past the decoded
 * codepoint's bytes when decode_utf8() made progress, or past one byte
 * otherwise (a malformed sequence that didn't decode at all) -- and by
 * emitting the skipped bytes as their own preprocessing token (C11
 * 6.4p3: any leftover non-white-space character forms its own pp-token),
 * so a macro body that merely stores this text still reproduces it
 * byte-for-byte if the macro is ever expanded/stringized.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test_common.h"

static int write_file(const char *path, const char *contents, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fwrite(contents, 1, len, f);
    fclose(f);
    return 1;
}

// POSIX `timeout N cmd...` has no equivalent Windows cmd.exe syntax
// (its own built-in `timeout` takes `/T <seconds>`, not a command to
// run) -- only prefix the defense-in-depth wall-clock guard on
// platforms where it actually works; the overall test harness has its
// own per-test hang detection regardless.
#ifdef _WIN32
#define TIMEOUT_PREFIX ""
#else
#define TIMEOUT_PREFIX "timeout 10 "
#endif

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    char src[600], obj[700], cmd[2400];

#ifdef _WIN32
    snprintf(src, sizeof(src), "%s\\test_lex_middot_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s\\test_lex_middot_%d.obj", td, pid);
#else
    snprintf(src, sizeof(src), "%s/test_lex_middot_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_lex_middot_%d.o", td, pid);
#endif

    /* Exact minimal shape of the real trigger: a #define whose
     * replacement list contains a bare U+00B7 (UTF-8: 0xC2 0xB7) between
     * two ASCII identifier fragments, never itself invoked. */
    const char src_text[] =
        "#define X foo\xc2\xb7test_lex_middotbar\n"
        "int main(void){return 0;}\n";
    if (!write_file(src, src_text, sizeof(src_text) - 1)) {
        printf("FAIL: cannot write %s\n", src);
        return 1;
    }

    /* Defense in depth: even if this exact fix regresses, `timeout`
     * guarantees this test fails cleanly (nonzero rc) rather than
     * hanging the whole suite run forever. */
    snprintf(cmd, sizeof(cmd), TIMEOUT_PREFIX "%s -c %s -o %s " NULL_REDIRECT,
             rcc, src, obj);
    int rc = system(cmd);
    remove(src);
    remove(obj);

    if (rc != 0) {
        printf("FAIL: compiling a bare U+00B7 in a #define body hung or "
               "failed (rc=%d)\n", rc);
        return 2;
    }

    /* A genuinely valid Unicode identifier (accented Latin, a real
     * XID_Start character) must still lex correctly -- the fix must not
     * have broken the normal "valid identifier-start" path while making
     * the "not identifier-start" path terminate. */
#ifdef _WIN32
    snprintf(src, sizeof(src), "%s\\test_lex_valid_uid_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s\\test_lex_valid_uid_%d.obj", td, pid);
#else
    snprintf(src, sizeof(src), "%s/test_lex_valid_uid_%d.c", td, pid);
    snprintf(obj, sizeof(obj), "%s/test_lex_valid_uid_%d.o", td, pid);
#endif
    const char valid_src[] =
        "int caf\xc3\xa9 = 1;\n" /* café: U+00E9, a valid ident char */
        "int main(void){return caf\xc3\xa9 - 1;}\n";
    if (!write_file(src, valid_src, sizeof(valid_src) - 1)) {
        printf("FAIL: cannot write %s\n", src);
        return 3;
    }
    snprintf(cmd, sizeof(cmd), TIMEOUT_PREFIX "%s -c %s -o %s " NULL_REDIRECT,
             rcc, src, obj);
    rc = system(cmd);
    remove(src);
    remove(obj);
    if (rc != 0) {
        printf("FAIL: a genuinely valid Unicode identifier no longer "
               "compiles (rc=%d)\n", rc);
        return 4;
    }

    printf("OK\n");
    return 0;
}
