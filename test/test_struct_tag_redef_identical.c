/* A struct/union tag redeclared with a byte-for-byte identical member
 * body in the same scope is a real, GCC-verified GNU idiom -- real gcc
 * silently treats the SECOND `struct foo { ... }` as the SAME type as
 * the first, not a distinct one. Load-bearing for type-generic macro
 * libraries that re-emit a tag's full definition at every use site
 * (e.g. noplate's `#define span(T) struct CONCAT(span_, T) { ... }`),
 * since `_Generic`/`__builtin_types_compatible_p` key struct identity on
 * exact member-list identity (type_equal()'s `a->members == b->members`
 * in src/parser.c). rcc used to always allocate a FRESH Type for a
 * redefinition, even when the member list was identical, so `_Generic`
 * comparing against a later `struct foo{...}` occurrence never matched
 * the type captured at the object's own declaration.
 */
#include <stdio.h>

int main(void) {
    struct foo { int x; float y; } a = {1, 2.0f};
    int r = _Generic(&a, struct foo { int x; float y; } *: 1, default: 0);
    if (r != 1) {
        printf("FAIL: identical struct-tag redefinition treated as a distinct type\n");
        return 1;
    }

    /* __builtin_types_compatible_p must agree too. */
    if (!__builtin_types_compatible_p(struct foo, struct foo { int x; float y; })) {
        printf("FAIL: __builtin_types_compatible_p disagrees on identical redefinition\n");
        return 2;
    }

    printf("OK\n");
    return 0;
}
