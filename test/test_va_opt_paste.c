/* Regression test: C23 __VA_OPT__ placemarker paste (## __VA_OPT__).
 *
 * C23 6.10.4.4: when ## immediately precedes __VA_OPT__(content) in a
 * variadic macro replacement list, __VA_OPT__ is evaluated first.
 * If the variadic arguments are empty, __VA_OPT__ expands to nothing
 * and ## is deleted (placemarker).  If non-empty, the content is
 * substituted and ## pastes it with the preceding token as usual.
 *
 * Before the fix, ## pasted the literal __VA_OPT__(content) text
 * onto the lhs, producing `BFS_VA_IF_AB__VA_OPT__(C)` — the
 * __VA_OPT__ never evaluated.
 */

#include <stdio.h>

static int failures;
#define assert_eq(a, b, msg) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        printf("FAIL: %s: expected %lld, got %lld\n", msg, _b, _a); \
        failures++; \
    } \
} while (0)

/* Macro using ## __VA_OPT__ placemarker:
 * - empty args: produces just BFS_VA_IF_AB (## deleted)
 * - non-empty:  produces BFS_VA_IF_ABC (## pastes C) */
#define BFS_VA_IF(...) BFS_VA_IF_AB ## __VA_OPT__(C)

/* Verify the expansions produce distinct identifiers.
 * We can't use assert_eq on identifiers directly, so check via sizeof
 * of differently-named local variables. */
int main(void) {
    /* empty: BFS_VA_IF_AB */
    int BFS_VA_IF_AB = 1;
    assert_eq(BFS_VA_IF(), 1, "empty va_args: placemarker deletes ##");

    /* non-empty: BFS_VA_IF_ABC */
    int BFS_VA_IF_ABC = 2;
    assert_eq(BFS_VA_IF(x), 2, "non-empty va_args: ## pastes content");

    /* multiple args: still BFS_VA_IF_ABC (C is fixed, not arg-derived) */
    assert_eq(BFS_VA_IF(x, y), 2, "multiple va_args: same paste result");

    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL VA_OPT PLACEMARKER TESTS PASSED\n");
    return failures ? 1 : 0;
}
