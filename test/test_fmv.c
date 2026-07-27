/* FMV (Function Multi-Versioning) IFUNC support.
 *
 * - __attribute__((target_clones("default","avx2","sse4.2"))) parses,
 *   emits foo.resolver, renames body to foo.default, and emits foo as
 *   STT_GNU_IFUNC.
 * - __attribute__((target("avx2"))) parses and stores the target string.
 * - The IFUNC resolver correctly returns the default clone at runtime
 *   (dynamic linker calls it, we verify the return value).
 * - Multiple FMV functions can coexist in one translation unit.
 */
#include <stdio.h>
/* Single clone variant */
__attribute__((target_clones("default")))
static int single_clone(int x) { return x * 2; }

/* Multi-clone variant — resolver picks default */
__attribute__((target_clones("default","avx2","sse4.2")))
static int multi_clone(int x) { return x * 3; }

/* FMV with __attribute__((target(...))) on a bare function */
__attribute__((target("avx2")))
static int target_attr_fn(int x) { return x + 1; }

/* Two FMV functions with different clone lists in same TU */
__attribute__((target_clones("default","avx2")))
static int fmv_a(int x) { return x + 10; }

__attribute__((target_clones("default","sse4.2")))
static int fmv_b(int x) { return x + 20; }

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } } while (0)

int main(void) {
    /* Basic functionality: single_clone(21) = 42 */
    CHECK(single_clone(21) == 42, "single_clone(21) != 42");

    /* Multi-clone: resolver returns default, multi_clone(7) = 21 */
    CHECK(multi_clone(7) == 21, "multi_clone(7) != 21");

    /* target() attribute function works normally */
    CHECK(target_attr_fn(41) == 42, "target_attr_fn(41) != 42");

    /* Two FMV functions in same TU */
    CHECK(fmv_a(32) == 42, "fmv_a(32) != 42");
    CHECK(fmv_b(22) == 42, "fmv_b(22) != 42");

    /* Call multiple times — IFUNC resolver runs once at load */
    for (int i = 0; i < 10; i++) {
        CHECK(multi_clone(i) == i * 3, "multi_clone stability");
        CHECK(single_clone(i) == i * 2, "single_clone stability");
    }

    if (failures == 0)
        printf("PASS: all FMV tests passed\n");
    else
        printf("FAIL: %d checks failed\n", failures);

    return failures;
}
