/* C11 6.10.3.3p1: the ## token-paste operator applies to BOTH
 * object-like and function-like macro replacement lists, not just
 * function-like ones. rcc's object-like macro expansion path
 * (expand_token()'s `!m->is_function` branch) simply re-scanned the
 * raw body tokens without ever routing through subst_range() -- the
 * function that actually implements ##/# processing -- so a `##` in
 * an object-like macro's body passed straight through as a literal,
 * unresolved punctuator token, AND its neighboring identifiers got
 * ordinary (unsuppressed) macro expansion first, backwards from the
 * standard's "## operands are not pre-expanded" rule.
 *
 * Found via util-linux/Parrot-style code (Parrot's own
 * `include/parrot/config.h`):
 *   #define PARROT_CORE_OPLIB_INIT  \
 *       Parrot_DynOp_core_ ## PARROT_PBC_MAJOR ## _ ## PARROT_PBC_MINOR
 * used directly as a function name in a prototype
 * (`op_lib_t *PARROT_CORE_OPLIB_INIT(PARROT_INTERP, long init);`).
 * rcc left the `##` tokens completely unresolved in the expansion,
 * producing a malformed token sequence that broke the surrounding
 * declaration's parse ("expected ';' or ','").
 *
 * Fixed by routing an object-like macro's body through subst_range()
 * (with no parameters/arguments -- every args/raw_args access in that
 * function is gated on m->is_variadic or an in-range parameter index,
 * neither of which an object-like macro ever has) whenever the body
 * contains a `##`, exactly as a function-like macro's body already
 * does.
 */

/* Two-level indirection: the classic C idiom to force macro-expand an
 * argument *before* pasting (used by CONCAT/PASTE macros everywhere in
 * real-world code) -- but here it's the plain object-like case that
 * was broken: no parameters at all, direct ## in the object-like
 * macro's own body. */
#define MAJOR 13
#define MINOR 1
#define FN_NAME Parrot_DynOp_core_ ## MAJOR ## _ ## MINOR

/* Standard says MAJOR/MINOR are NOT pre-expanded (they're directly
 * adjacent to ##), so the correct, GCC-matching result is the literal
 * pasted identifier below, not "Parrot_DynOp_core_13_1". */
static int FN_NAME(void) { return 42; }

static int test_object_like_hashhash_paste(void)
{
    return Parrot_DynOp_core_MAJOR_MINOR() == 42 ? 0 : 1;
}

/* A second, simpler case: plain two-token paste in an object-like
 * macro with no indirection at all. */
#define PREFIX foo
#define SUFFIX bar
#define GLUED PREFIX ## _ ## SUFFIX
static int test_plain_object_like_paste(void)
{
    int foo_bar = 9; /* unrelated, just to show GLUED isn't this */
    (void)foo_bar;
    int PREFIX_SUFFIX = 7;
    /* GLUED = PREFIX ## _ ## SUFFIX pastes the LITERAL operand tokens
     * (## operands are never pre-expanded), yielding the identifier
     * "PREFIX_SUFFIX" -- not "foo_bar" -- matching real gcc exactly. */
    return GLUED == 7 ? 0 : 1;
}

/* An ordinary object-like macro with no ## at all must still behave
 * exactly as before (the fast path is preserved). */
#define PLAIN_VALUE 100
static int test_plain_object_like_no_paste(void)
{
    return PLAIN_VALUE == 100 ? 0 : 1;
}

int main(void)
{
    int r = test_object_like_hashhash_paste();
    if (r) return r;
    r = test_plain_object_like_paste();
    if (r) return 10 + r;
    r = test_plain_object_like_no_paste();
    if (r) return 20 + r;
    return 0;
}
