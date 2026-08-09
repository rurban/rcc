/* __builtin_cpu_init(): real GCC/clang's companion to
 * __builtin_cpu_supports("feature") (already implemented, see
 * test/test_ia32_pause.c-adjacent history in test/third_party/TODO.md),
 * called once before any __builtin_cpu_supports() checks to lazily
 * populate libgcc's own static __cpu_model cache. rcc had no wiring for
 * it at all -- an unrecognized name falls back to an ordinary implicitly-
 * declared external function, emitting a real relocation to a symbol
 * nothing ever defines, so any call site links with "undefined reference
 * to '__builtin_cpu_init'".
 *
 * Found via test/third_party/test_libucl's bundled mum.h, which calls it
 * exactly this way before its own AVX2 dispatch check.
 */
#if !defined(__aarch64__) && !defined(_M_ARM64)
#include <stdio.h>

static int avx2_support = 0;

int check_avx2(void) {
    if (!avx2_support) {
        __builtin_cpu_init();
        avx2_support = __builtin_cpu_supports("avx2") ? 1 : -1;
    }
    return avx2_support;
}

int main(void) {
    int r = check_avx2();
    if (r != 1 && r != -1) {
        printf("unexpected avx2_support value %d\n", r);
        return 1;
    }
    printf("ok\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
