/* GCC's `__attribute__((always_inline))` forces inlining at every
 * optimization level, including -O0/no flags at all -- it's not an
 * optimization hint like plain `inline`, it's a LINKAGE requirement:
 * headers commonly declare their wrappers
 *   extern __inline TYPE __attribute__((__gnu_inline__, __always_inline__))
 *   fn(...) { ... }
 * and `extern` + `gnu_inline` together mean the compiler NEVER emits a
 * standalone out-of-line definition for `fn` -- every call must actually
 * be inlined, or the program fails to link.
 *
 * Regression 1: try_inline()'s cost-budget check (an ordinary `inline`
 * function may only inline if its body is "cheap enough") applied
 * UNCONDITIONALLY, even to an always_inline callee -- so a callee whose
 * body happens to desugar into a bigger expression tree than the budget
 * (e.g. __builtin_shuffle() lowers to several per-lane compare/gather
 * nodes) silently refused to inline despite always_inline, leaving a
 * real call to a function with no definition anywhere: "undefined
 * reference" at link time.
 *
 * Regression 2: even with (1) fixed, try_inline() is only ever invoked
 * from optimize()/optimize_node(), which itself only runs when the
 * compile passes -O1, -finline, or -funroll. A translation unit built
 * with NO such flag (this test's own build command, and how run_tests.c
 * invokes rcc for every test/test_*.c file) never called try_inline() a
 * single time -- so always_inline never actually forced anything,
 * contradicting real GCC's -O0 behavior.
 *
 * Found via a real PHP build: GCC's own <xmmintrin.h> _mm_move_ss()
 * (`extern __inline __m128 __attribute__((__gnu_inline__,
 * __always_inline__, __artificial__))`), called from
 * ext/hash/hash_sha_sse2.c with no -O flag on that specific compile.
 * (rcc's own bundled <xmmintrin.h> uses a different, always-emits-a-
 * real-definition style and doesn't exercise this path at all -- see
 * test_xmmintrin_move_ss.c for that separate, simpler bug. This test
 * reproduces the underlying always_inline/extern/gnu_inline mechanism
 * directly instead of relying on a specific header's exact wording.)
 */
#if defined(__GNUC__)
typedef int v4si __attribute__((vector_size(16)));

extern __inline v4si __attribute__((__gnu_inline__, __always_inline__))
rotate_lanes(v4si a, v4si b)
{
    /* __builtin_shuffle desugars into several per-lane compare/gather
     * AST nodes -- enough to exceed the ordinary `inline`-keyword cost
     * budget, which is exactly what regression 1 needs to expose. */
    return __builtin_shuffle(a, b,
                              __extension__
                              (__attribute__((__vector_size__(16))) int)
                              {4, 1, 2, 3});
}

int main(void)
{
    v4si a = {1, 2, 3, 4};
    v4si b = {10, 20, 30, 40};
    v4si r = rotate_lanes(a, b);
    if (r[0] != 10) return 1; /* lane 0 from b (index 4 = b[0]) */
    if (r[1] != 2) return 2; /* lanes 1-3 from a */
    if (r[2] != 3) return 3;
    if (r[3] != 4) return 4;
    return 0;
}
#else
int main(void) { return 0; }
#endif
