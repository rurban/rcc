/* __builtin_types_compatible_p must treat VLA / unspecified-length array
 * types as compatible with any other array type (VLA or fixed-size) of the
 * same element type -- C11 6.7.6.2p6-7: a VLA's exact runtime length is
 * never a compile-time compatibility criterion. rcc's
 * types_compatible_p_qual() had no TY_VLA case at all: VLA vs VLA, and
 * TY_ARRAY vs TY_VLA cross-comparisons, both fell through to the strict
 * `a->kind != b->kind` check (or, for two same-kind arrays, to an ordinary
 * constant-length comparison), wrongly rejecting lengths that can never be
 * compared at compile time.
 *
 * `[*]` (C99 6.7.6.2) is only legal in function *prototype* scope, not in
 * a definition, so `f1` below is a bare prototype -- the static_asserts
 * live inside its trailing anonymous-struct parameter, whose member list
 * is still parsed (matching upstream cproc's own test/compatible-vla-
 * types.c, which this test mirrors).
 */
void f1(int n, int (*a)[n], int (*b)[*], int (*c)[3],
        struct {
            int x;
            static_assert(__builtin_types_compatible_p(typeof(a), typeof(b)));
            static_assert(__builtin_types_compatible_p(typeof(a), typeof(c)));
            static_assert(__builtin_types_compatible_p(typeof(a), int (*)[3]));
            static_assert(__builtin_types_compatible_p(typeof(b), int (*)[3]));
            static_assert(__builtin_types_compatible_p(typeof(a), int (*)[]));
            static_assert(__builtin_types_compatible_p(typeof(b), int (*)[]));
        } s);

void f2(void) {
    int n = 12, m = 6 * 2;
    static_assert(__builtin_types_compatible_p(int[n], int[12]));
    static_assert(__builtin_types_compatible_p(int[], int[n]));
    static_assert(__builtin_types_compatible_p(int[n], int[m]));
    static_assert(__builtin_types_compatible_p(int[2][n], int[1 + 1][n]));
    static_assert(!__builtin_types_compatible_p(int[4][n], int[5][n]));
    /* Element type still matters -- a VLA of a different element type must
     * stay incompatible regardless of the length-agnostic relaxation. */
    static_assert(!__builtin_types_compatible_p(int[n], long[n]));
}

int main(void) {}
