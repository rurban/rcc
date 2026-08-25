/* Regression test: `sizeof(...)` (and `sizeof(type-name)`) produced a
 * VALUE with the correct magnitude but the WRONG TYPE -- signed `int`/
 * `long`/`long long` instead of the standard-mandated unsigned
 * `size_t` (C11 6.5.3.4p5: "The value of the result of `sizeof` ...
 * has type `size_t`", an unsigned integer type).
 *
 * `unary()`'s `sizeof` handling (parser.c) constructed its result via
 * `new_num(ty->size, tok)`, where `tok` is whatever token happened to
 * follow the `sizeof` expression in the source (a `)`, an operator,
 * ...) -- NOT the numeric-literal token `new_num()`'s own suffix-
 * sniffing logic expects (it reads backwards from `tok->ptr+tok->len`
 * looking for `u`/`l` characters to pick the literal's type, exactly
 * like it does for a real `123ULL`-style token). Fed a non-literal
 * token, that heuristic never finds a `u`/`l` suffix, so it silently
 * fell through to signed `int`/`long`/`long long` based purely on the
 * VALUE's magnitude -- e.g. `sizeof(char)` (value 1) came out plain
 * signed `int`. This corrupted any comparison/arithmetic relying on
 * `sizeof`'s unsigned usual-arithmetic-conversion semantics: mixing a
 * signed `int` with `sizeof(...)` in a subtraction should promote to
 * `size_t` and wrap around on underflow (e.g. `3 < sizeof(char) -
 * 5` is TRUE, since `1u - 5u` wraps to a huge value) -- with the bug,
 * it stayed signed and evaluated the mathematically-signed (and
 * therefore wrong per the standard) result.
 *
 * Fixed by explicitly overriding the result type to rcc's target
 * `size_t` at all sizeof-result construction sites
 * (`sizeof(type-name)`, `sizeof expr`, and their shared VLA-runtime-size
 * helper `type_size_node()`) -- matching the `_Alignof` handling below it.
 *
 * Found via valkey's `src/entry.c`: `static_assert(FIELD_SDS_AUX_BIT_MAX
 * < sizeof(char) - SDS_TYPE_BITS, "...")`, where `sizeof(char) - 3`
 * must wrap around to a huge unsigned value (making the assertion
 * pass, matching real gcc) rather than evaluate as the signed `-2`
 * (which would make it a hard compile-time assertion failure).
 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
    /* sizeof's own type must be unsigned (size_t), not signed. */
    int is_size_t_generic = _Generic(sizeof(char),
        size_t: 1,
        default: 0);
    assert(is_size_t_generic == 1);

    /* The exact valkey static_assert shape: unsigned wraparound makes
     * a small sizeof() minus a larger constant compare as huge, not
     * negative. */
    _Static_assert(3 < sizeof(char) - 5, "sizeof-3 must wrap unsigned");

    /* Same wraparound via a runtime (non-constant-folded) comparison,
     * exercising the actual generated code's signedness handling, not
     * just the parser's static_assert evaluator. */
    volatile size_t five = 5;
    assert(3 < sizeof(char) - five);

    /* sizeof(expr) form (not just sizeof(type-name)) must also be
     * unsigned. */
    char c = 'x';
    int is_size_t_expr = _Generic(sizeof(c),
        size_t: 1,
        default: 0);
    assert(is_size_t_expr == 1);

    printf("OK\n");
    return 0;
}
