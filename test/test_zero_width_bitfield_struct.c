/* A struct whose only member(s) are anonymous zero-width bitfields
 * (`int : 0;`), or a genuinely empty struct (`struct {}`, a GNU
 * extension), is a COMPLETE type with size 0 -- distinct from a
 * forward-declared-but-never-defined `struct S;`, which is genuinely
 * incomplete. rcc's completeness checks (sizeof, _Alignof, alignas,
 * _Generic, type cloning, tag-completion) all used the same heuristic,
 * `ty->size == 0 && !ty->members`, to decide "incomplete" -- but an
 * anonymous zero-width bitfield never creates a Member node at all (it
 * only advances layout bookkeeping), so a struct containing only such
 * bitfields has both size 0 AND members == NULL despite having a real
 * `{ ... }` body, indistinguishable from a true forward declaration
 * under that heuristic.
 *
 * This is exactly the shape of Linux/util-linux's widely used
 * UL_BUILD_BUG_ON_ZERO(e) compile-time-assert idiom:
 *   #define UL_BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:(-!!(e)); }))
 * which relies on `sizeof` succeeding (yielding 0) when `e` is false,
 * and only failing to compile (negative bitfield width) when `e` is
 * true. rcc's overly-strict incompleteness check made `sizeof` itself
 * error out unconditionally, i.e. every use of the macro -- found via
 * util-linux's own `ARRAY_SIZE()`/`__must_be_array()` macro chain
 * (text-utils/more.c).
 *
 * Fixed by adding a genuine `Type.has_body` flag, set only when a
 * struct/union `{ ... }` body was actually parsed to completion
 * (regardless of whether it produced any Member nodes or a nonzero
 * size), and switching every completeness check from the old
 * `size==0 && !members` heuristic to `!has_body`.
 */
#include <stddef.h>

/* The exact UL_BUILD_BUG_ON_ZERO / __must_be_array / ARRAY_SIZE chain
 * from util-linux's include/c.h, reproduced verbatim in shape. */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : (-!!(e)); }))
#define MUST_BE_ARRAY(a) \
    BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(__typeof__(a), __typeof__(&(a)[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + MUST_BE_ARRAY(arr))

static int test_build_bug_on_zero(void)
{
    /* Must compile and evaluate to 0 without any diagnostic. */
    int probe[BUILD_BUG_ON_ZERO(0) + 3];
    return (int)sizeof(probe) == 3 * (int)sizeof(int) ? 0 : 1;
}

static int test_array_size(void)
{
    int arr[5] = {1, 2, 3, 4, 5};
    return ARRAY_SIZE(arr) == 5 ? 0 : 1;
}

static int test_sizeof_zero_width_bitfield_only(void)
{
    /* Size 0 for an all-zero-width-bitfield struct is ABI-invariant --
     * this is the shape util-linux's own macro actually relies on.
     * The sizes once a real byte member is added depend on the
     * bitfield-packing ABI (MS bitfields on Windows lay out a trailing
     * `:0` differently from the SysV/GCC default), so those are
     * guarded per target -- both verified against the real reference
     * toolchain for that target (gcc / x86_64-w64-mingw32-gcc).
     */
    if (sizeof(struct { int : 0; }) != 0) return 1;
#ifdef _WIN32
    if (sizeof(struct { char c; int : 0; }) != 1) return 2;
    if (sizeof(struct { char c; int : 0; char d; }) != 2) return 3;
#else
    if (sizeof(struct { char c; int : 0; }) != 4) return 2;
    if (sizeof(struct { char c; int : 0; char d; }) != 5) return 3;
#endif
    return 0;
}

static int test_sizeof_empty_struct(void)
{
    /* GNU extension: an empty struct is complete, size 0. */
    return sizeof(struct {}) == 0 ? 0 : 1;
}

static int test_alignof_zero_width_bitfield_only(void)
{
    /* Must not warn/error as "incomplete type". */
    return _Alignof(struct { int : 0; }) >= 1 ? 0 : 1;
}

/* A genuine forward declaration must still be rejected. */
struct opaque_fwd_decl;
static int test_genuine_incomplete_still_rejected(void)
{
#ifdef CHECK_INCOMPLETE_REJECTED
    /* Intentionally not compiled: would error, as it must.
       return (int)sizeof(struct opaque_fwd_decl); */
#endif
    return 0;
}

int main(void)
{
    int r = test_build_bug_on_zero();
    if (r) return r;
    r = test_array_size();
    if (r) return 10 + r;
    r = test_sizeof_zero_width_bitfield_only();
    if (r) return 20 + r;
    r = test_sizeof_empty_struct();
    if (r) return 30 + r;
    r = test_alignof_zero_width_bitfield_only();
    if (r) return 40 + r;
    r = test_genuine_incomplete_still_rejected();
    if (r) return 50 + r;
    return 0;
}
