/* add_type_internal()'s ND_COMMA case (type.c) unconditionally
 * dereferenced its own freshly-assigned `node->ty` (`node->ty =
 * node->rhs->ty;` then `if (node->ty->kind == ...)`) to apply array/
 * function decay -- but several node kinds never get a type assigned at
 * all (they're statement-like, not expressions: ND_NULL, ND_ZERO_INIT,
 * ND_LABEL, ...), leaving `node->rhs->ty` NULL. A comma expression's
 * rightmost operand can legally be one of those -- e.g. `__builtin_apply()`
 * synthesizes a bare `ND_NULL` placeholder node (parser.c) -- and
 * `node->ty->kind` on a NULL `node->ty` segfaulted the compiler outright.
 *
 * Found via ksh93's own sfio/sfvprintf.c, which crashed rcc (SIGSEGV)
 * while compiling a comma expression through this exact path.
 *
 * Fixed by only applying the array/function decay when `node->ty` is
 * non-NULL, leaving a void-rhs comma expression's own type NULL too
 * (matching its rhs) instead of dereferencing garbage.
 */
void noop(void) {}

int main(void)
{
    int x = 5;
    /* A comma expression whose rightmost operand is a bare, untyped
     * placeholder node (__builtin_apply's synthesized ND_NULL) used to
     * crash the compiler outright while computing this expression's own
     * type -- reaching this line at all is the test. */
    (x, __builtin_apply(noop, 0, 0));
    return x == 5 ? 0 : 1;
}
