/* offsetof() must return size_t (C11 7.19p3: "offsetof(type, member-
 * designator) which expands to an integer constant expression that
 * has type size_t"), not a plain `int`.
 *
 * Found via ggrep (GNU grep) 3.12's gnulib-tests/test-stddef-h.c:
 *
 *   static_assert (sizeof (offsetof (struct d, e)) == sizeof (size_t));
 *   static_assert ((offsetof (struct d, e) < -1) == (INT_MAX < (size_t) -1));
 *
 * rcc's __builtin_offsetof implementation (parser.c's unary()) built its
 * result as a plain new_num()/ND_ADD chain with no explicit type, which
 * fell back to the generic ND_NUM default of `int`. Two observable
 * consequences: sizeof(offsetof(...)) came out as sizeof(int) instead
 * of sizeof(size_t), and `offsetof(...) < -1` compared as SIGNED (a
 * small positive offset is never less than -1) instead of the correct
 * unsigned comparison (any small offset is far less than (size_t)-1's
 * huge wraparound value).
 *
 * Fixed by explicitly typing both the constant-offset and runtime
 * (VLA-indexed) result paths as ty_ullong (size_t is 64-bit on every
 * rcc target), matching the existing convention already used by
 * __builtin_object_size/__builtin_dynamic_object_size.
 */
#include <assert.h>
#include <stddef.h>
#include <limits.h>

struct point {
    char pad;
    int x;
    int y;
};

_Static_assert(sizeof(offsetof(struct point, pad)) == sizeof(size_t),
               "offsetof's own type is size_t, not int");
_Static_assert(offsetof(struct point, pad) == 0, "first member at offset 0");
_Static_assert(offsetof(struct point, x) == 4, "x follows pad plus padding");
_Static_assert(offsetof(struct point, y) == 8, "y follows x");

/* The classic "offsetof compares as unsigned" check: a small offset
 * must never appear to be "greater than" -1 the way a signed int
 * comparison would report. */
_Static_assert((offsetof(struct point, pad) < -1) == (INT_MAX < (size_t) -1),
               "offsetof participates in unsigned comparison like size_t");

/* Runtime (array-indexed, non-constant-foldable) offsetof path must
 * also be size_t-typed. */
struct arr_holder {
    char pad;
    int vals[8];
};

int main(void) {
    assert(sizeof(offsetof(struct point, y)) == sizeof(size_t));

    volatile int idx = 3;
    size_t off = offsetof(struct arr_holder, vals[idx]);
    assert(off == offsetof(struct arr_holder, vals[0]) + idx * sizeof(int));

    return 0;
}
