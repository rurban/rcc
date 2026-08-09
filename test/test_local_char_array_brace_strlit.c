/* `{ STRLIT }` for a local (function-scope, non-static, non-constexpr)
 * char/wide-char array target is a superfluous-but-legal single-element
 * brace (C11 6.7.9p14) -- exactly equivalent to the bare STRLIT form.
 * global_init_one() (used for globals/statics/constexpr locals) and
 * infer_array_type() (used for sizing any array declarator) both already
 * unwrapped this shape correctly, but local_init_one() -- the separate
 * codegen path that builds runtime ND_ASSIGN statements for an ordinary
 * local's initializer -- did not: it fell into the generic "array with
 * braces" per-element loop, saw exactly one initializer-list element (the
 * string literal), and treated it as a single SCALAR initializer for the
 * array's char element type, assigning the string's decayed `char*`
 * address (silently truncated to its low byte by the char-typed store)
 * into element 0 -- every other element was left at its zero-initialized
 * default, and element 0 held an arbitrary, non-deterministic byte
 * depending on where the string literal's storage happened to land.
 *
 * Found via test/third_party/test_liblz4's programs/lz4io.c:
 *   static const char* LZ4IO_toHuman(long double size, char* buf) {
 *       const char units[] = {"\0KMGTPEZY"};
 *       size_t i = 0;
 *       for (; size >= 1024; i++) size /= 1024;
 *       sprintf(buf, "%.2Lf%c", size, units[i]);
 *       ...
 *   }
 * `lz4 -l` (list mode) reported an uncompressed size like "3.00" instead
 * of "3.00M" for any file needing a KB/MB/GB unit suffix, since
 * `units[i]` (i>0) read a zeroed byte instead of the real unit letter --
 * failing lz4's own `test-lz4-list.py` test suite.
 */
#include <stdio.h>
#include <string.h>
#include <wchar.h>

int main(void)
{
    /* The exact bug shape: a NUL-prefixed string with an unwritten byte
     * at every non-zero index if the array-content copy stops after one
     * corrupted element. */
    const char units[] = {"\0KMGTPEZY"};
    if (sizeof(units) != 10) {
        printf("FAIL: sizeof(units) = %d, expected 10\n", (int)sizeof(units));
        return 1;
    }
    static const char expect[10] = "\0KMGTPEZY";
    if (memcmp(units, expect, 10) != 0) {
        printf("FAIL: units[] content mismatch:");
        for (int i = 0; i < 10; i++) printf(" %d", (unsigned char)units[i]);
        printf("\n");
        return 2;
    }

    /* Non-const local, plain char array. */
    char plain[] = {"hello"};
    if (strcmp(plain, "hello") != 0 || sizeof(plain) != 6) {
        printf("FAIL: plain = \"%s\" sizeof=%d, expected \"hello\"/6\n",
               plain, (int)sizeof(plain));
        return 3;
    }

    /* Trailing comma inside the superfluous brace: `{ STRLIT, }`. */
    const char trailing[] = {"world",};
    if (strcmp(trailing, "world") != 0) {
        printf("FAIL: trailing = \"%s\", expected \"world\"\n", trailing);
        return 4;
    }

    /* Wide string in braces must still decode via the wide-string path,
     * not the byte-oriented one. */
    const wchar_t wide[] = {L"wide"};
    if (wide[0] != L'w' || wide[1] != L'i' || wide[2] != L'd' ||
        wide[3] != L'e' || wide[4] != L'\0' ||
        sizeof(wide) / sizeof(wide[0]) != 5) {
        printf("FAIL: wide[] mismatch, len=%d\n",
               (int)(sizeof(wide) / sizeof(wide[0])));
        return 5;
    }

    /* Regression guard: a genuine one-pointer array of char* initialized
     * BY a string literal (not sized BY one) must still work -- this is
     * the TY_PTR-excluded case local_init_one's existing bare-STR branch
     * (and the array-of-pointers case entirely) must not regress. */
    const char *ptrs[] = {"vec_"};
    if (sizeof(ptrs) / sizeof(ptrs[0]) != 1 || strcmp(ptrs[0], "vec_") != 0) {
        printf("FAIL: ptrs[] mismatch\n");
        return 6;
    }

    printf("ok\n");
    return 0;
}
