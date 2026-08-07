/* rcc's <stdint.h> used to define its own `typedef long long ptrdiff_t;`
 * (C99 7.18.1 doesn't require this -- real glibc's <stdint.h> has no
 * such typedef, ptrdiff_t is <stddef.h>-only), duplicating and
 * CONFLICTING with <stddef.h>'s correct `typedef long int ptrdiff_t;`
 * (same size on LP64, but a structurally different type: long vs long
 * long). Harmless as long as nothing checked the two typedefs agreed
 * -- but once the parser started diagnosing incompatible
 * redeclarations, any TU including both headers and later defining a
 * ptrdiff_t-parametered function hit a spurious "conflicting types"
 * error. Found via zfp's (bundled by blosc2) C "template" source
 * files, which include <stdint.h> after <stddef.h> and then define
 * functions taking a `ptrdiff_t` stride parameter. */
#include <stddef.h>
#include <stdint.h>

ptrdiff_t stride_of(const int *p, const int *q);
ptrdiff_t stride_of(const int *p, const int *q) {
    return q - p;
}

int main(void) {
    int arr[4] = {0, 1, 2, 3};
    return stride_of(&arr[0], &arr[3]) == 3 ? 0 : 1;
}
