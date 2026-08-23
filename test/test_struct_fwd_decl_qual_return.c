/* A pointer-to-forward-declared-struct used as a function's RETURN
 * type -- `const struct S *f(void);` -- was wrongly rejected as
 * "conflicting types" when the same prototype's DEFINITION appeared
 * later, still with the struct forward-declared (never completed with
 * a body in the whole translation unit).
 *
 * qualify_struct_type() never mutates a shared, still-incomplete
 * struct/union Type in place -- it mints a fresh "qualified variant"
 * copy at every `const struct S *`-style use site (chained off the
 * canonical tag's Type.qual_variants) so the qualifier never leaks
 * onto later plain (non-const) uses of the same tag. The
 * prototype's and the definition's `const struct S` copies are
 * therefore two DIFFERENT Type objects even though both derive from
 * the exact same tag: neither pointer identity (a == b) nor the
 * member-list check (a->members == b->members, both NULL since the
 * struct never completes) recognized them as the same type, so
 * types_compatible_p_qual()'s TY_STRUCT/TY_UNION case always failed
 * for this shape, wrongly diagnosing every such (extremely common,
 * e.g. wget2's libwget/http.c: `WGETAPI const wget_vector
 * *wget_http_get_no_proxy(void);` against an opaque forward-declared
 * `wget_vector`) prototype/definition pair as conflicting.
 *
 * Fixed via a new Type.struct_id identity anchor (mirroring the
 * existing Type.enum_id mechanism for enums), set to the canonical
 * tag's own address at creation and preserved verbatim across every
 * qualified-variant copy.
 */

struct opaque;

const struct opaque *get_thing(void);
const struct opaque *get_thing(void) { /* must NOT be "conflicting types" */
    return (void *)0;
}

/* A genuinely DIFFERENT forward-declared struct must still conflict. */
#ifdef RCC_TEST_EXPECT_CONFLICT
struct other;
struct opaque *bad(void);
struct other *bad(void) { return (void *)0; } /* must error */
#endif

int main(void) {
    return get_thing() != (void *)0;
}
