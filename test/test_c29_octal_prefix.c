/* C29 (WG14 N3353): explicit-prefix octal literals `0o`/`0O`, e.g. `0o52`,
 * alongside the traditional (now obsolescent) leading-zero spelling
 * `052`. Both must produce the same value; the new prefix must also
 * compose correctly with C23 digit separators and u/l/wb suffixes, and
 * with a following `.`/identifier so `0o17` inside `0o17u` or similar
 * isn't accidentally mis-lexed.
 */
#include "test_common.h"

int main(void)
{
    /* Basic equivalence with decimal/legacy-octal spellings. */
    if (0o52 != 42) { printf("FAIL: 0o52 != 42 (got %d)\n", 0o52); return 1; }
    if (0O52 != 42) { printf("FAIL: 0O52 != 42 (got %d)\n", 0O52); return 2; }
    if (0o52 != 052) { printf("FAIL: 0o52 != 052\n"); return 3; }

    /* Zero and single-digit forms. */
    if (0o0 != 0) { printf("FAIL: 0o0 != 0\n"); return 4; }
    if (0o7 != 7) { printf("FAIL: 0o7 != 7\n"); return 5; }

    /* Full digit range (0-7) and a multi-digit value. */
    if (0o777 != 511) { printf("FAIL: 0o777 != 511 (got %d)\n", 0o777); return 6; }
    if (0O1234567 != 342391) { printf("FAIL: 0O1234567 mismatch (got %d)\n", 0O1234567); return 7; }

    /* Suffixes: u, l, ul, ull. */
    unsigned u = 0o17u;
    if (u != 15u) { printf("FAIL: 0o17u != 15\n"); return 8; }
    long l = 0o20L;
    if (l != 16L) { printf("FAIL: 0o20L != 16\n"); return 9; }
    unsigned long long ull = 0o37ull;
    if (ull != 31ull) { printf("FAIL: 0o37ull != 31\n"); return 10; }

    /* Negative via unary minus (the literal itself is always non-negative). */
    if (-0o10 != -8) { printf("FAIL: -0o10 != -8\n"); return 11; }

    /* Used directly as an array size / enum value (must be a genuine
     * integer constant expression, foldable at compile time). */
    int arr[0o10];
    if ((int)(sizeof(arr) / sizeof(arr[0])) != 8) {
        printf("FAIL: 0o10 not usable as array bound\n");
        return 12;
    }
    enum { E = 0o100 };
    if (E != 64) { printf("FAIL: 0o100 enum value != 64\n"); return 13; }

#if 0o11 == 9
    /* Must also fold in preprocessor #if constant-expression context. */
#else
    printf("FAIL: 0o11 wrong in #if\n");
    return 14;
#endif

    /* C23 digit separators inside a 0o literal. */
    if (0o1'2'3 != 0o123) { printf("FAIL: digit-separated 0o1'2'3\n"); return 15; }

    return 0;
}
