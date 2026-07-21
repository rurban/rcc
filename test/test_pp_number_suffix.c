/* C11 6.4.8: a pp-number is a digit sequence optionally followed by
 * identifier characters. Tokens like "1b"/"2b" — used as macro arguments
 * in kernel constructs like _ASM_EXTABLE_TYPE(1b, 2b, ...), where GAS
 * numeric-local-label references are passed through textually rather
 * than evaluated as numbers — must lex as a single pp-number token
 * instead of erroring as an invalid integer-constant suffix. */

#define STRINGIFY(x) #x
#define TOSTR(x) STRINGIFY(x)

int main(void)
{
    const char *s = TOSTR(1b);
    if (s[0] != '1' || s[1] != 'b' || s[2] != '\0') return 1;

    const char *s2 = TOSTR(2b);
    if (s2[0] != '2' || s2[1] != 'b' || s2[2] != '\0') return 2;

    return 0;
}
