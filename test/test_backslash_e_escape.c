/* GCC/Clang extension: `\e` in a char or string literal means ESC
 * (0x1B), the same as `\x1b`/`\033`. Not standard C, but a long-
 * standing, widely-relied-upon extension both compilers support.
 *
 * Regression: rcc's get_escape_char() handled `a`, `b`, `f`, `n`, `r`,
 * `t`, `v`, `0` but not `e` -- an unrecognized escape character falls
 * through to `default: return c;`, which is correct for escapes that
 * mean "the literal character" (`\\`, `\'`, `\"`, `\?`) but wrong for
 * `\e`: it silently produced the ASCII letter 'e' (0x65) instead of
 * ESC (0x1B), with no diagnostic at all.
 *
 * Found via a real PHP build: Zend/zend_language_scanner.c's own
 * double-quoted-string lexer uses `*t++ = '\e';` to emit the ESC byte
 * for PHP source containing `"\e"`. Every PHP string literal
 * containing `\e` decoded to `e` instead of ESC (0x1B), corrupting the
 * array-sorting order of PHP's own escape-sequence test (comparing
 * ESC as the byte 0x1B, sorted before all the plain-letter escapes,
 * against 'e', sorted among them).
 */
#include <stdio.h>

int main(void)
{
    char c = '\e';
    if (c != 0x1b) {
        printf("FAIL: '\\e' == %d, want 27\n", (int)c);
        return 1;
    }

    const char *s = "\e[0m";
    if ((unsigned char)s[0] != 0x1b || s[1] != '[' || s[2] != '0' || s[3] != 'm' || s[4] != '\0') {
        printf("FAIL: \"\\e[0m\"[0] == %d, want 27\n", (unsigned char)s[0]);
        return 2;
    }

    printf("OK backslash-e escape\n");
    return 0;
}
