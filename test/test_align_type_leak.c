/* A declaration-level over-alignment (`_Alignas(N)` or GNU
 * `__attribute__((aligned(N)))`) on one object must only raise that one
 * declaration's own alignment requirement — it must never change the
 * *type's* alignment for any other, unrelated use of the same type
 * elsewhere in the translation unit (see test_struct_attr_align.c for
 * the "must not pad the type's own size" half of this same rule).
 *
 * apply_type_align() already followed that rule correctly for scalar and
 * array types (it clones the Type before raising align on the clone) —
 * but for a struct/union type it delegated to copy_type(), which
 * *intentionally* returns the same Type object for struct/union kinds
 * (so an incomplete forward-declared type can still be completed later
 * through every existing pointer to it). Reusing that identity-preserving
 * behavior here meant an over-aligned declaration of any struct/union
 * type — including a typedef'd one — mutated the shared type's own
 * ->align field in place, silently corrupting every *other* use of that
 * same type for the rest of the file.
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/atomic64_64.h's
 * atomic64_t typedef is used in a great many places, at least one of
 * which (transitively, in a huge translation unit) over-aligns a single
 * declaration. That silently widened every later struct netmem_desc
 * (net/netmem.h) member of type atomic_long_t to 64-byte alignment,
 * which desynced its packed layout from struct page's and tripped a
 * static_assert deep in unrelated code — a confusing, hard-to-trace
 * symptom for what was really type-identity corruption several files
 * away from the actual bug.
 */
#include <stddef.h>

typedef struct { long counter; } my_atomic_t;

/* GNU attribute form. */
static my_atomic_t over_aligned_gnu __attribute__((aligned(64)));

/* C11 _Alignas form — goes through the exact same apply_type_align(). */
_Alignas(64) static my_atomic_t over_aligned_c11;

struct later_gnu {
    char a;
    my_atomic_t b;
};

struct later_c11 {
    char a;
    my_atomic_t b;
};

static_assert(_Alignof(my_atomic_t) == _Alignof(long),
              "an over-aligned declaration must not widen the type itself");
static_assert(offsetof(struct later_gnu, b) == sizeof(long),
              "a later struct's member of the same type must not inherit "
              "another declaration's over-alignment (GNU attribute form)");
static_assert(offsetof(struct later_c11, b) == sizeof(long),
              "a later struct's member of the same type must not inherit "
              "another declaration's over-alignment (_Alignas form)");

int main(void)
{
    over_aligned_gnu.counter = 1;
    over_aligned_c11.counter = 2;
    struct later_gnu lg = {0, {3}};
    struct later_c11 lc = {0, {4}};
    if (over_aligned_gnu.counter != 1 || over_aligned_c11.counter != 2) return 1;
    if (lg.b.counter != 3 || lc.b.counter != 4) return 2;
    return 0;
}
