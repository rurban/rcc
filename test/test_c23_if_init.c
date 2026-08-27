/* C23 if/switch init-statement and labeled continue/break.
 *
 * `if (T v = expr; cond)` — the old parser's init-statement scan stopped
 * at the FIRST `)` (the init expression's own parens), so an init whose
 * RHS contained a function call or parenthesized subexpression was
 * misparsed as the C99 decl-in-condition form and errored on the leftover
 * `;` (c23doku's brute_force_c2y.c: `if (int num = encode_num(...); ...)`).
 *
 * `continue label;` / `break label;` — the label was silently dropped and
 * the statement compiled as a plain continue/break of the INNERMOST loop,
 * so a labeled continue targeting an outer loop re-ran the outer body
 * tail instead of jumping to the loop's continuation (c23doku's
 * graph_color_c2y.c solver diverged and smashed a static sort buffer).
 */

static int enc(char *c) { return *c - '1'; }

static int test_if_init_call(void)
{
    /* Init RHS is a function call with an array-subscript argument: the
     * paren scan must find the real `;` past the call's own `)`. */
    char puzzle[] = ".21";
    int hits = 0;
    for (int y = 0; y < 2; y++)
        for (int x = 0; x < 2; x++) {
            if (int num = enc(&puzzle[x + y * 2]); num >= 0 && num < 9)
                hits += num;
        }
    /* indices: 0='.' skipped, 1='2' -> 1, 2='1' -> 0, 3='\0' skipped */
    if (hits != 1) return 1;
    return 0;
}

static int test_if_init_cond_parens(void)
{
    int x = 5;
    /* The condition itself carries parens past the init-statement `;`. */
    if (int v = x; (v & 1) == 1)
        return 0;
    return 1;
}

static int test_if_init_plain(void)
{
    int x = 3;
    if (int v = x; v > 0)
        return 0;
    return 1;
}

static int test_switch_init(void)
{
    int x = 2;
    switch (int v = x; v) {
    case 1: return 1;
    case 2: return 0;
    default: return 2;
    }
}

static int test_labeled_continue(void)
{
    /* continue OuterLoop must jump to the OUTER loop's continuation,
     * skipping the outer body's tail after the inner loop. */
    int idx = 0, tail = 0;
    OuterLoop:
    for (;;) {
        if (idx == -8)
            break;
        for (int j = 0; j < 3; j++) {
            if (j == 1) {
                idx -= 2;
                continue OuterLoop;
            }
        }
        tail++;
        idx++;
    }
    /* gcc reference: continue skips tail every time (j==1 always hits),
     * idx decrements by 2 per outer iteration. */
    if (idx != -8) return 1;
    if (tail != 0) return 2;
    return 0;
}

static int test_labeled_break(void)
{
    int count = 0;
    Outer:
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            count++;
            if (j == 2)
                break Outer; /* must exit the OUTER loop, not the inner */
        }
    }
    /* gcc reference: break Outer fires on the FIRST outer iteration at
     * j==2 (count=3); if it broke only the inner loop, i would run all 4
     * iterations and count would reach 16. */
    if (count != 3) return 1;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = test_if_init_call())) return rc;
    if ((rc = test_if_init_cond_parens())) return rc;
    if ((rc = test_if_init_plain())) return rc;
    if ((rc = test_switch_init())) return rc;
    if ((rc = test_labeled_continue())) return rc;
    if ((rc = test_labeled_break())) return rc;
    return 0;
}
