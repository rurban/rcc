/* C11 6.5.2.5p10: a compound literal occurring OUTSIDE the body of a
 * function has STATIC storage duration even without an explicit
 * `static` keyword (only inside a function does it default to
 * automatic). rcc's general expression-parser path for `(T){...}`
 * always treated a non-`static`-qualified compound literal as a local
 * (automatic-storage) object regardless of nesting depth, so one
 * reached while parsing a global initializer (e.g. the integer-typed-
 * field "cast address" fallback: `(uintptr_t) &(T){...}`) got a fake
 * local var whose address can't fold into a link-time relocation --
 * silently writing 0 into the field instead of the real address.
 * Matches njs's "(uintptr_t) &(njs_webcrypto_algorithm_t){...}" nested
 * inside a static njs_webcrypto_entry_t[] array element.
 */
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int a;
    int b;
} pair_t;

typedef struct {
    int name;
    uintptr_t value;
} entry_t;

/* Array context (matches the real njs shape). */
entry_t table[] = {
    {1, (uintptr_t)&(pair_t){10, 20}},
    {2, (uintptr_t)&(pair_t){30, 40}},
};

/* Single-struct context. */
entry_t single = {3, (uintptr_t)&(pair_t){50, 60}};

int main(void) {
    pair_t *p0 = (pair_t *)table[0].value;
    pair_t *p1 = (pair_t *)table[1].value;
    pair_t *ps = (pair_t *)single.value;
    if (!p0 || p0->a != 10 || p0->b != 20) {
        printf("FAIL: table[0].value broken\n");
        return 1;
    }
    if (!p1 || p1->a != 30 || p1->b != 40) {
        printf("FAIL: table[1].value broken\n");
        return 2;
    }
    if (!ps || ps->a != 50 || ps->b != 60) {
        printf("FAIL: single.value broken\n");
        return 3;
    }
    printf("OK\n");
    return 0;
}
