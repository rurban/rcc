/* A trailing `_Pragma(...)` as the LAST statement inside a GNU
 * statement-expression `({ ...; expr; _Pragma(...); })`, with nothing
 * following it before the closing `}`, silently discarded the
 * expression's intended value. Found via ggrep (GNU grep) 3.12's
 * src/grep.c CAST_ALIGNED() macro:
 *
 *   #define CAST_ALIGNED(type, p) \
 *     ({ __typeof__ (p) val_ = p; \
 *        _Pragma ("GCC diagnostic push") \
 *        _Pragma ("GCC diagnostic ignored \"-Wcast-align\"") \
 *        (type) val_; \
 *        _Pragma ("GCC diagnostic pop") })
 *
 * used in skip_easy_bytes() as
 * `for (s = CAST_ALIGNED (uword const *, p); ...)`. The trailing
 * `_Pragma("GCC diagnostic pop")` after the cast expression made `s`
 * silently become NULL, causing a real SIGSEGV in the rcc-built grep
 * binary (confirmed absent with a gcc-built binary on the identical
 * source).
 *
 * Root cause: the statement-expression parser (parser.c's ND_STMT_EXPR
 * handling) finds its result value by looking at the *last* node in the
 * block's body and requiring it to be an ND_EXPR_STMT. `_Pragma(...)`
 * as a statement parses to a plain ND_NULL node (like a bare `;`), so
 * when it trails the real value expression, the "last node" check
 * failed and the statement-expression's value silently defaulted away.
 *
 * Fixed in compound_stmt_ex(): `_Pragma(...)` at statement level is now
 * consumed without ever appending any node to the block's body (same
 * treatment as the preprocessor's own `#pragma pack`/`#pragma fenv`
 * markers already got) -- matching every other `_Pragma` site in rcc
 * (type-attribute context, the primary statement fallback, file scope),
 * all of which already treated it as fully invisible. A `_Pragma` is a
 * C99 preprocessing operator, not a statement, so it must never affect
 * "what is the last statement" reasoning -- unlike a genuine trailing
 * `;`, which legitimately makes a statement-expression's value void.
 */
#include <assert.h>

#define CAST_TRAILING_PRAGMA(type, p)                                     \
    ({ __typeof__(p) val_ = p;                                            \
       _Pragma("GCC diagnostic push")                                     \
       _Pragma("GCC diagnostic ignored \"-Wcast-align\"")                 \
       (type) val_;                                                       \
       _Pragma("GCC diagnostic pop") })

#define CAST_LEADING_PRAGMA(type, p)                                      \
    ({ _Pragma("GCC diagnostic push")                                     \
       __typeof__(p) val_ = p;                                            \
       (type) val_; })

int main(void) {
    long x = 42;
    long *p = &x;

    /* Trailing pragma after the value expression: this is the exact
     * shape that lost the value under rcc. */
    long const *s = CAST_TRAILING_PRAGMA(long const *, p);
    assert(s == (long const *)p);
    assert(*s == 42);

    /* Leading pragma before other statements: was already handled
     * correctly (regression guard, not a new bug). */
    long const *s2 = CAST_LEADING_PRAGMA(long const *, p);
    assert(s2 == (long const *)p);

    /* A bare trailing `;` (a real empty statement, not a pragma) must
     * still make the statement-expression void -- only `_Pragma` is
     * invisible to the "last statement" check. Used as a discarded
     * expression-statement, since a void-valued stmt-expr cannot be
     * assigned. */
    ({ int v = 7; v; ; });

    /* Multiple statements with an interior _Pragma must not lose
     * earlier or later values either. */
    int y = ({ int a = 1; _Pragma("GCC diagnostic push") int b = a + 1; b; });
    assert(y == 2);

    return 0;
}
