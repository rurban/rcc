/* A static/global array may reference itself by name within its own
 * initializer -- C11 6.2.1p7: an identifier's scope begins right after
 * its declarator is complete, well before the initializer list is
 * parsed, so "static const T arr[] = { ..., &arr[N], ... };" is legal
 * and widely used as a ROM-table self-indexing idiom (e.g. mquickjs's
 * generated mqjs_stdlib.h: "static const uint64_t js_stdlib_table[] =
 * { ..., JS_VALUE_FROM_PTR(&js_stdlib_table[offset]), ... }").
 *
 * rcc registered the array's own LVar/symbol only *after* fully parsing
 * its initializer (in the top-level global-declaration path). But an
 * unsized array's initializer is first scanned by
 * infer_array_type()/count_array_initializer() purely to COUNT
 * elements (skip_initializer() calls assign() on each element just to
 * skip its tokens correctly) -- and that counting pass ran *before*
 * the array was registered, so any element referencing the array by
 * name hard-errored "undeclared variable" even though the real,
 * value-writing pass later would have resolved it fine.
 */
#include <stdint.h>
#include <stdio.h>

/* uint64_t, not `long`: on LLP64 (Windows/mingw) `long` is only 4
 * bytes -- too narrow for a real address, and real gcc/clang reject
 * the cast there too ("initializer element is not constant"), same as
 * rcc. mquickjs's own real table uses uint64_t for exactly this
 * reason. */
static const uint64_t tbl[] = {
    100,
    200,
    (uint64_t)(uintptr_t)&tbl[0],
    (uint64_t)(uintptr_t)&tbl,
};

int main(void) {
    if (tbl[2] != (uint64_t)(uintptr_t)&tbl[0]) {
        printf("FAIL: self-reference via &tbl[0] wrong\n");
        return 1;
    }
    if (tbl[3] != (uint64_t)(uintptr_t)&tbl) {
        printf("FAIL: self-reference via &tbl wrong\n");
        return 2;
    }
    printf("OK\n");
    return 0;
}
