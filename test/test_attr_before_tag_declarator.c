/* C23 6.7.13's "attribute-specifier-sequence immediately before a
 * struct/union/enum specifier" constraint only forbids a genuinely EMPTY
 * declaration (nothing after the specifier but ';', e.g. `[[]] struct s {
 * int a; };` or `[[]] struct s;`). rcc's `read_type_attrs()` implemented
 * this check by looking only at the single token right after the
 * attribute list: any `struct`/`union`/`enum` there was flagged as an
 * error unconditionally, without checking whether a real declarator (a
 * member/variable name) followed the specifier. This misfired on any
 * legitimate declaration that happens to open with `[[some-attr]]
 * struct/union/enum ... name;` -- most commonly an empty/no-op attribute
 * expansion (a feature-detected `[[gnu::counted_by(n)]]`-style macro that
 * expands to nothing when the compiler doesn't advertise the extended
 * attribute via `__has_c_attribute`) directly preceding a flexible array
 * member whose element type is itself a struct, e.g.
 * `[[]] struct thread threads[];` -- found via bfs's `src/ioq.c`
 * (`_counted_by(nthreads)` from bfs.h, which rcc's `__has_c_attribute`
 * correctly reports as unsupported and macro-expands to nothing).
 *
 * Fixed by scanning past the optional tag name, optional enum fixed
 * underlying type, and optional `{ ... }` body to see whether the
 * specifier is immediately followed by ';' (truly empty) or by a
 * declarator (a real declaration) before deciding whether to error.
 */

struct thread {
    int id;
};

struct pool {
    unsigned long nthreads;
    /* Empty attribute (as a feature-detected macro would expand to)
     * directly before a struct-typed flexible array member -- must NOT
     * be treated as an empty declaration. */
    [[]]
    struct thread threads[];
};

/* Also exercise a non-empty attribute in the same position (already
 * worked before this fix, kept as a sanity check it still does). */
struct pool2 {
    unsigned long nthreads;
    [[maybe_unused]]
    struct thread threads[];
};

/* Plain int flexible array member with an empty attribute (already
 * worked before this fix too). */
struct pool3 {
    unsigned long n;
    [[]]
    int items[];
};

int main(void)
{
    struct pool *p = 0;
    struct pool2 *p2 = 0;
    struct pool3 *p3 = 0;
    (void)p;
    (void)p2;
    (void)p3;
    return 0;
}
