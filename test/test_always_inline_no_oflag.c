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
 * Two bugs in rcc's try_inline():
 *
 * 1. The cost-budget check ("only inline an ordinary `inline` function if
 *    its body is cheap enough") applied unconditionally, even to callees
 *    marked always_inline. A callee whose body happens to desugar into a
 *    bigger expression tree than the budget silently refused to inline
 *    despite always_inline, leaving a real call to a function with no
 *    definition anywhere: "undefined reference" at link time. This test's
 *    poly() body is deliberately large (many chained operators) to exceed
 *    the ordinary `inline`-keyword cost budget on its own.
 *
 * 2. try_inline() is only ever invoked from optimize()/optimize_node(),
 *    which itself only runs when the compile passes -O1, -finline, or
 *    -funroll. A translation unit built with NO such flag (this test's
 *    own build command, and how run_tests.c invokes rcc for every
 *    test/test_*.c file) never called try_inline() a single time -- so
 *    always_inline never actually forced anything, contradicting real
 *    GCC's -O0 behavior.
 *
 * Found via a real PHP build: GCC's own <xmmintrin.h> _mm_move_ss()
 * (`extern __inline __m128 __attribute__((__gnu_inline__,
 * __always_inline__, __artificial__))`), called from
 * ext/hash/hash_sha_sse2.c with no -O flag on that specific compile.
 */
#if defined(__GNUC__)
extern __inline int __attribute__((__gnu_inline__, __always_inline__))
poly(int a, int b, int c, int d)
{
    /* Purely a chain of scalar arithmetic on the parameters -- no locals,
     * no compound literals -- so the only thing exercised here is the
     * cost-budget bypass (regression 1) and the -O0 pass wiring
     * (regression 2), not any other inliner corner case. */
    return a*a + a*b + a*c + a*d
         + b*a + b*b + b*c + b*d
         + c*a + c*b + c*c + c*d
         + d*a + d*b + d*c + d*d
         + a+b+c+d + a-b+c-d + a*2+b*2+c*2+d*2;
}

int main(void)
{
    int r = poly(1, 2, 3, 4);
    /* Reference value computed independently below via plain_poly(). */
    int a = 1, b = 2, c = 3, d = 4;
    int expected = a*a + a*b + a*c + a*d
                 + b*a + b*b + b*c + b*d
                 + c*a + c*b + c*c + c*d
                 + d*a + d*b + d*c + d*d
                 + a+b+c+d + a-b+c-d + a*2+b*2+c*2+d*2;
    if (r != expected) return 1;
    return 0;
}
#else
int main(void) { return 0; }
#endif
