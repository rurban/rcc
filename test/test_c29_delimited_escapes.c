/* C29 (WG14 N2785/N3353): "delimited escape sequences" -- `\x{...}` and
 * the brand-new `\o{...}` (octal has never had a bare `\o` form) give
 * hex/octal character escapes an unambiguous end delimiter, so they
 * can't accidentally swallow a following hex/octal-looking character
 * (`"\x1b[0m"` vs. the always-correct `"\x{1b}[0m"`). `\u{...}` does
 * the same for universal-character-name escapes, and additionally
 * drops the fixed 4/8 hex digit width of `\uXXXX`/`\UXXXXXXXX`,
 * accepting any number of digits for any codepoint.
 */
#include "test_common.h"
#include <string.h>
#include <wchar.h>

int main(void)
{
    /* \x{...}: narrow char and string, values matching the equivalent
     * bare \x form. */
    char c1 = '\x{41}';
    if (c1 != 'A') { printf("FAIL: '\\x{41}' != 'A' (got %d)\n", c1); return 1; }
    char c2 = '\x41';
    if (c1 != c2) { printf("FAIL: '\\x{41}' != '\\x41'\n"); return 2; }

    const char *s1 = "\x{48}\x{65}\x{6c}\x{6c}\x{6f}";
    if (strcmp(s1, "Hello") != 0) {
        printf("FAIL: delimited hex string != \"Hello\" (got \"%s\")\n", s1);
        return 3;
    }

    /* The whole point: a delimited escape doesn't swallow a following
     * hex digit the way the bare form would. */
    const char *s2 = "\x{1}23";
    if (s2[0] != 1 || s2[1] != '2' || s2[2] != '3' || s2[3] != '\0') {
        printf("FAIL: '\\x{1}23' mis-parsed (bytes %d %d %d %d)\n",
               (int)s2[0], (int)s2[1], (int)s2[2], (int)s2[3]);
        return 4;
    }

    /* \o{...}: octal escape, no length limit (unlike bare \NNN's 3-digit
     * cap), and no unbraced \o form exists at all. */
    char c3 = '\o{101}'; /* octal 101 = 65 = 'A' */
    if (c3 != 'A') { printf("FAIL: '\\o{101}' != 'A' (got %d)\n", c3); return 5; }
    char c4 = '\101';
    if (c3 != c4) { printf("FAIL: '\\o{101}' != '\\101'\n"); return 6; }

    const char *s3 = "\o{110}\o{145}\o{154}\o{154}\o{157}";
    if (strcmp(s3, "Hello") != 0) {
        printf("FAIL: delimited octal string != \"Hello\" (got \"%s\")\n", s3);
        return 7;
    }

    /* \u{...}: universal character name, variable-width, in a wide
     * string/char. Must match the fixed-width \uXXXX spelling for the
     * same codepoint, and also accept a value that needs more than 4
     * hex digits (\U-only range) without a separate \U{} spelling.
     * Codepoints below are all outside the basic source character set
     * (C11 6.4.3p2 forbids a UCN from designating a basic-charset
     * character like plain 'H' -- real gcc/clang reject '\u0048'). */
    wchar_t wc1 = L'\u{e9}'; /* U+00E9 LATIN SMALL LETTER E WITH ACUTE */
    wchar_t wc2 = L'\u00e9';
    if (wc1 != wc2 || wc1 != 0xe9) {
        printf("FAIL: L'\\u{e9}' != L'\\u00e9' (got 0x%x vs 0x%x)\n",
               (unsigned)wc1, (unsigned)wc2);
        return 8;
    }

    const wchar_t *ws = L"\u{e9}\u{e8}\u{ea}";
    if (ws[0] != 0xe9 || ws[1] != 0xe8 || ws[2] != 0xea || ws[3] != 0) {
        printf("FAIL: delimited unicode wide string mismatch (0x%x 0x%x 0x%x)\n",
               (unsigned)ws[0], (unsigned)ws[1], (unsigned)ws[2]);
        return 9;
    }

    /* Supplementary-plane codepoint (needs > 4 hex digits): U+1F600.
     * Uses a U'' (char32_t) constant, not L'' (wchar_t): wchar_t is only
     * 16 bits on Windows (UTF-16), which can't hold an astral codepoint
     * without a surrogate pair -- char32_t is always 32 bits everywhere,
     * which is what this checks (the escape's hex-digit-count parsing,
     * not platform wchar_t/surrogate-pair encoding). */
    unsigned int emoji = U'\u{1F600}';
    if (emoji != 0x1F600) {
        printf("FAIL: U'\\u{1F600}' != 0x1F600 (got 0x%x)\n", emoji);
        return 10;
    }

    /* Malformed forms (missing closing '}') must be a compile error, not
     * silently accepted or crash -- checked via a subprocess since this
     * translation unit itself must compile cleanly. */
    {
        const char *rcc = find_rcc();
        const char *td = get_tmpdir();
        int pid = (int)getpid();
        char srcf[256], objf[256], cmd[768];
        snprintf(srcf, sizeof(srcf), "%s/test_c29_esc_bad_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_c29_esc_bad_%d.o", td, pid);

        static const char src[] =
            "int main(void) { char c = '\\x{41'; return c; }\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: cannot write %s\n", srcf); return 11; }
        fputs(src, f);
        fclose(f);

        snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        int wrc = system(cmd);
        remove(objf);
        remove(srcf);
        if (wrc == 0) {
            printf("FAIL: unterminated '\\x{41' must be a compile error\n");
            return 12;
        }
    }

    return 0;
}
