/* A struct member of array type, read through a const- or
 * volatile-qualified struct/union pointer, silently lost the qualifier
 * once the array decayed to a pointer (indexing, `&arr[i]`, plain
 * `arr + i`, or passing the bare array as a function argument). Real
 * GCC/Clang give the ARRAY expression itself the qualifiers of the
 * accessing pointer (C11 6.5.2.3p3: member access through a qualified
 * struct/union additionally qualifies the result, independent of the
 * member's own declared type) and preserve them across decay (C11
 * 6.3.2.1p3). rcc's `.`/`->` member-access parsing correctly qualified
 * the array's own Type, but every array-to-pointer decay site in
 * type.c then discarded that qualifier and used the array's element
 * type verbatim, giving `arr[i]`/`&arr[i]` a plain, unqualified
 * pointer type.
 *
 * Found via postgres's `src/timezone/localtime.c`:
 *   struct pg_tm *result = ...;
 *   result->tm_zone = unconstify(char *, &sp->chars[ttisp->tt_desigidx]);
 * where `sp` is `const struct state *` and `unconstify(type, expr)`
 * expands to a `__builtin_types_compatible_p(__typeof(expr), const type)`
 * static assertion (from `src/include/c.h`) that failed to compile
 * ("wrong cast") because rcc's `&sp->chars[...]` was typed as plain
 * `char *`, not `const char *`, so the assertion's premise was false. */
struct state {
    char chars[16];
};

/* `&sp->chars[0]` must be `const char *`: sp is a const struct pointer,
 * so member access through it additionally qualifies chars[], and that
 * qualifier must survive the array's decay to a pointer. */
static int check_addr_const(const struct state *sp) {
    return __builtin_types_compatible_p(__typeof(&sp->chars[0]), const char *);
}

/* Through a non-const struct pointer, the same expression stays plain
 * `char *` -- confirms the fix isn't unconditionally qualifying. */
static int check_addr_plain(struct state *sp) {
    return __builtin_types_compatible_p(__typeof(&sp->chars[0]), char *);
}

/* Plain pointer arithmetic on the decayed array (no `&`/`[]`) must
 * qualify the same way: `sp->chars + i` is `const char *`. */
static int check_ptr_add_const(const struct state *sp, int i) {
    return __builtin_types_compatible_p(__typeof(sp->chars + i), const char *);
}

int main(void) {
    struct state s = { "hello" };
    int ok = 1;

    ok = ok && check_addr_const(&s);
    ok = ok && check_addr_plain(&s);
    ok = ok && check_ptr_add_const(&s, 2);

    /* Runtime sanity: the qualifier change is compile-time only --
     * the actual pointer value/content must still be correct. */
    const struct state *csp = &s;
    const char *p = &csp->chars[1];
    ok = ok && *p == 'e';
    ok = ok && (csp->chars + 2) == p + 1;

    return ok ? 0 : 1;
}
