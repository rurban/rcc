/* C99 6.7.6.3p7: a type qualifier appearing INSIDE the [] of a parameter's
 * array declarator (only valid in the OUTERMOST array derivation) qualifies
 * the DECAYED POINTER PARAMETER ITSELF, not its pointee -- `int a[const]`
 * means `int *const a` (a const pointer to non-const int), unlike
 * `const int b[]` (`const int *b`, a non-const pointer to const int).
 *
 * rcc used to silently discard the bracket qualifier (type_suffix()'s
 * bracket-content loop just skipped the token) and, separately, qualifying
 * a typedef'd array type (`const T c` where T is an array typedef) wrongly
 * set the qualifier on the ARRAY type itself instead of pushing it down to
 * the element type per C11 6.7.3p9 -- both bugs made the decayed pointer
 * come out qualified on the wrong side.
 */
typedef int T1[];
typedef const int T2[];

void f(int a[const], const int b[], const T1 c, T2 d) {
    static_assert(__builtin_types_compatible_p(typeof(&a), int *const *));
    static_assert(__builtin_types_compatible_p(typeof(&b), const int **));
    static_assert(__builtin_types_compatible_p(typeof(&c), const int **));
    static_assert(__builtin_types_compatible_p(typeof(&d), const int **));
    /* a's pointee is non-const: writing through it must still be legal. */
    *a = 5;
}

int main(void) {
    int x[3] = {1, 2, 3};
    f(x, x, x, x);
    return x[0] == 5 ? 0 : 1;
}
