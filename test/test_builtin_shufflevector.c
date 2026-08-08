/* __builtin_shufflevector(vec1, vec2, i0, i1, ..., iN-1) was entirely
 * unimplemented. GCC 15's own <avxintrin.h>/<avx2intrin.h> (used
 * whenever a project includes <immintrin.h> and rcc, having no bundled
 * AVX/AVX2 headers of its own, falls through to the real system
 * headers) define several _mm_reduce_* / _mm256_reduce_* functions whose
 * bodies use __builtin_shufflevector unconditionally, so every TU that
 * merely includes <immintrin.h> failed to parse ("expected an
 * expression") regardless of whether those specific functions were
 * ever called. Found via blake3's blake3_dispatch.c.
 *
 * Unlike __builtin_shuffle (a runtime mask vector, output shape ==
 * input shape), every index is a compile-time-constant expression and
 * the output lane count is simply however many indices were given --
 * may differ from either input vector's own lane count. Index i
 * selects vec1[i] for i in [0, N1), or vec2[i - N1] for i in
 * [N1, N1+N2). This is a portable parser-level feature (no
 * architecture-specific codegen), so it is tested unconditionally,
 * not gated to x86. */
#include <assert.h>

typedef short v8hi __attribute__((vector_size(16)));
typedef short v4hi __attribute__((vector_size(8)));
typedef int v4si __attribute__((vector_size(16)));

int main(void) {
    /* Two 8-lane vectors interleaved, mixing selections from both
     * operands and reading past the halfway point of each. */
    v8hi a = {1, 2, 3, 4, 5, 6, 7, 8};
    v8hi b = {10, 11, 12, 13, 14, 15, 16, 17};
    v8hi c = __builtin_shufflevector(a, b, 0, 8, 1, 9, 7, 15, 3, 11);
    short ce[8] = {1, 10, 2, 11, 8, 17, 4, 13};
    for (int i = 0; i < 8; i++) assert(c[i] == ce[i]);

    /* Output lane count smaller than either input's (a plain sub-slice). */
    v4hi d = __builtin_shufflevector(a, a, 4, 5, 6, 7);
    for (int i = 0; i < 4; i++) assert(d[i] == a[4 + i]);

    /* Reversal of a single vector (vec1 == vec2, only vec1 indices used). */
    v8hi rev = __builtin_shufflevector(a, a, 7, 6, 5, 4, 3, 2, 1, 0);
    for (int i = 0; i < 8; i++) assert(rev[i] == a[7 - i]);

    /* Different element type/width (int, not short) sanity check. */
    v4si x = {100, 200, 300, 400};
    v4si y = {500, 600, 700, 800};
    v4si z = __builtin_shufflevector(x, y, 3, 2, 4, 7);
    int ze[4] = {400, 300, 500, 800};
    for (int i = 0; i < 4; i++) assert(z[i] == ze[i]);

    return 0;
}
