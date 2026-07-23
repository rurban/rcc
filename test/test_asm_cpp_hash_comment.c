/* Three related bugs in how a standalone ".S" file (run through the C
 * preprocessor in "assembler-with-cpp" mode, like GCC's own -x
 * assembler-with-cpp) got tokenized, all found while chasing a real Linux
 * kernel build failure on arch/x86/kernel/verify_cpu.S:
 *
 * 1. A '#' was always lexed as an ordinary C-preprocessor punctuator in
 *    pp_mode, with no notion of GAS's own "# comment to end of line"
 *    convention for a '#' that isn't a preprocessing directive (i.e. isn't
 *    the first token on its line). "xor %di,%di # don't loop" — a real
 *    line from verify_cpu.S — then tokenized "don't" as identifier "don"
 *    followed by a bare "'" that looked like the start of a character
 *    literal, scanned to end of file looking for the closing "'", and
 *    errored "unclosed character literal" instead of just treating
 *    "# don't loop" as a comment the way GAS itself does.
 *
 * 2. The first fix for #1 checked "is this '#' the first token on its
 *    line" using the lexer's own lex_at_line_start/lex_in_directive state,
 *    which is maintained purely as a side effect of lex_one() being called
 *    in strict forward order. The preprocessor's #if/macro-expansion code
 *    doesn't call lex_one() in that strict order across a multi-line
 *    "/* ... *[/]" comment (it drains the comment's embedded-newline TK_CNL
 *    tokens out of band), so that state went stale: a #define whose value
 *    ends in a multi-line trailing comment (a real pattern in
 *    arch/x86/include/asm/msr-index.h's ARCH_CAP_*_NO bit-flag macros) got
 *    its '#' misdetected as "mid-line" on the *next* #define, and the
 *    comment's own body leaked into the token stream as bare code instead
 *    of being stripped, while the #define itself never got its value.
 *
 * 3. The corrected, stateless "scan backward for a real newline" check for
 *    #2 doesn't by itself know the difference between a genuine GAS
 *    mid-line comment and the C preprocessor's OWN '#'/'##' operators
 *    (stringize / token-paste) inside a #define's replacement list, which
 *    are just as "mid-line" syntactically. include/linux/kconfig.h's
 *    IS_ENABLED()/__or()/__and() machinery — used constantly in kernel .S
 *    files via "#if IS_ENABLED(CONFIG_FOO)" — pastes an option name onto
 *    "__ARG_PLACEHOLDER_" with "##"; misreading that "##" as a comment
 *    start silently ate the rest of a macro definition, and the resulting
 *    malformed macro-argument scan for the *next* level of expansion read
 *    off the end of the token list and crashed (NULL dereference) instead
 *    of just erroring cleanly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int compile_ok(const char *rcc, const char *td, int pid,
                      const char *tag, const char *src)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ach_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_ach_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    remove(objf);
    if (rc != 0) {
        printf("FAIL: [%s] compiling standalone .S file failed (rc=%d)\n", tag, rc);
        return 0;
    }
    return 1;
}

/* Checks that the byte payload a macro expands to actually made it into the
 * object file — not just that compilation didn't error — so a macro that
 * silently failed to expand (leaving e.g. a literal, unresolvable "FOO"
 * mnemonic) would still be caught. */
static int compile_and_check_bytes(const char *rcc, const char *td, int pid,
                                   const char *tag, const char *src,
                                   const char *want_hex)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ach_%s_%d.S", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_ach_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s -nostdinc " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compiling standalone .S file failed (rc=%d)\n", tag, rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j .data %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char collapsed[4096];
    size_t cn = 0;
    for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
        if (!isspace((unsigned char)*s)) collapsed[cn++] = *s;
    collapsed[cn] = '\0';

    if (!strstr(collapsed, want_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" in .data, got:\n%s\n",
               tag, want_hex, out);
        return 0;
    }
    return 1;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    /* Bug 1: an apostrophe inside a GAS "# comment" must not be mistaken
     * for the start of a character literal. */
    static const char src_apostrophe[] =
        ".text\n"
        "f:\n"
        "\txor\t%edi,%edi\t\t\t# don't loop\n"
        "\tret\n";
    ok &= compile_ok(rcc, td, pid, "apostrophe", src_apostrophe);

    /* Bug 2: a #define whose value ends in a multi-line "/* ... *[/]"
     * comment must not corrupt the *next* #define's own leading '#'
     * detection (matches ARCH_CAP_MDS_NO's shape in msr-index.h). */
    static const char src_multiline[] =
        "#define FIRST   0x2a   /*\n"
        " * a trailing comment\n"
        " * spanning several lines\n"
        " */\n"
        "#define SECOND  0x37\n"
        ".data\n"
        ".long FIRST\n"
        ".long SECOND\n";
    ok &= compile_and_check_bytes(rcc, td, pid, "multiline", src_multiline,
                                  "2a00000037000000");

    /* Bug 3: '#'/'##' inside a #define's own replacement list (stringize /
     * token-paste) must not be mistaken for a GAS comment, even though
     * they're just as "mid-line" as one (matches kconfig.h's IS_ENABLED()
     * chain: __or(x, y) -> ___or(x, y) -> ____or(__ARG_PLACEHOLDER_##x, y)). */
    static const char src_paste[] =
        "#define PLACEHOLDER_1 0,\n"
        "#define TAKE_SECOND(_ignored, val, ...) val\n"
        "#define OR(x, y)  OR_(x, y)\n"
        "#define OR_(x, y) OR__(PLACEHOLDER_##x, y)\n"
        "#define OR__(arg1_or_junk, y) TAKE_SECOND(arg1_or_junk 1, y)\n"
        "#if OR(1, 0)\n"
        ".data\n"
        ".long 0x99\n"
        "#endif\n";
    ok &= compile_and_check_bytes(rcc, td, pid, "paste", src_paste, "99000000");

    if (!ok) return 1;
    printf("OK standalone .S files handle GAS \"#\" comments, multi-line "
           "comments inside macro values, and '#'/'##' inside macro bodies\n");
    return 0;
}
