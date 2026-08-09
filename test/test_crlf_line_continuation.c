/* A backslash-newline macro continuation must splice correctly even when
 * the source file uses CRLF line endings (this file itself is CRLF --
 * verify with `file test/test_crlf_line_continuation.c`). splice_lines_with_counts()
 * (preprocess.c) only recognized a backslash immediately followed by'\n',
 * missing the CRLF form '\' '\r' '\n' used throughout Windows-authored
 * third-party sources (e.g. unqlite.c's amalgamated source, discovered
 * via test/third_party/test_unqlite). For a CRLF file, the '\r' sat between
 * the backslash and the '\n' the check was matching, so the continuation
 * silently never fired: the backslash stayed as literal text right before
 * the physical newline, terminating the #define's replacement-list after
 * only its first physical line -- the remaining continuation lines were
 * then mis-lexed as fresh top-level C, producing "type defaults to int" /
 * "expected specific operator" diagnostics pointing at what should have
 * been macro-body text.
 */
#include <stdio.h>

struct pair {
    int a, b;
};

#define SET_PAIR(p, x, y) \
    (p)->a = (x);\
    (p)->b = (y);

int main(void) {
    struct pair p = {0, 0};
    SET_PAIR(&p, 3, 4)
    if (p.a != 3 || p.b != 4) {
        printf("got a=%d b=%d, expected a=3 b=4\n", p.a, p.b);
        return 1;
    }
    printf("ok\n");
    return 0;
}
