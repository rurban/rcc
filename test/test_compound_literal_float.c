/* Regression test for a KNOWN BUG (expected to FAIL until fixed):
 *
 * Compound-literal initialization of floating-point struct/vector members from
 * *integer* constants does not perform the int->float conversion. The value is
 * stored as raw bits (yielding 0.0 / garbage) instead of the converted float.
 *
 *     struct S { float a, b; } s = (struct S){1, 2};   // rcc: 0 0   (WRONG)
 *     struct S             s2   = { 1, 2 };            // rcc: 1 2   (correct)
 *
 * C requires the initializer to be converted "as if by assignment" to the
 * member type (C11 6.7.9p11, 6.5.2.5), so {1,2} and {1.0f,2.0f} must be
 * identical, in both plain initializers and compound literals.
 *
 * Root cause: src/parser.c compound-literal scalar-member init sets
 * asgn->ty = mem->ty directly instead of calling check_type(asgn), bypassing
 * the ND_ASSIGN typing rule that inserts the implicit conversion cast.
 * See the FIXME there. This test verifies the correct behavior and therefore
 * fails until that path is fixed.
 */
#include <stdio.h>

static int fail;
#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); fail++; } } while (0)

struct S {
    float a, b, c, d;
};

typedef float f4 __attribute__((__vector_size__(16)));

int main(void) {
    /* Control: plain brace initializer already converts int->float correctly. */
    struct S ok = {1, 2, 3, 4};
    CHECK(ok.a == 1.0f && ok.b == 2.0f && ok.c == 3.0f && ok.d == 4.0f);

    /* Control: compound literal with float literals is correct. */
    struct S okf = (struct S){1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(okf.a == 1.0f && okf.b == 2.0f && okf.c == 3.0f && okf.d == 4.0f);

    /* BUG: compound literal with int constants must convert to float. */
    struct S s = (struct S){1, 2, 3, 4};
    CHECK(s.a == 1.0f);
    CHECK(s.b == 2.0f);
    CHECK(s.c == 3.0f);
    CHECK(s.d == 4.0f);

    /* BUG: same for vector_size compound literals with int constants. */
    f4 v = (f4){1, 2, 3, 4};
    CHECK(v[0] == 1.0f);
    CHECK(v[1] == 2.0f);
    CHECK(v[2] == 3.0f);
    CHECK(v[3] == 4.0f);

    if (fail) {
        fprintf(stderr, "%d check(s) failed\n", fail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
