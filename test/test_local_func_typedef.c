/* Local (block-scope) typedef naming a FUNCTION TYPE — not a function
 * pointer — must register as a typedef, not misparse as a nested
 * function declaration. Before the fix, `declaration()` matched on
 * `ty->kind == TY_FUNC` before checking `attr.is_typedef`, so
 * `typedef int foo(int);` inside a function body silently skipped
 * add_typedef(): a later cast like `(foo *) fn` then failed
 * is_typename() and misparsed as "expected an expression" (found via
 * SQLite's jimsh0.c: `typedef int (qsort_comparator)(const void *,
 * const void *); ... qsort(..., (qsort_comparator *) fn);` inside
 * ListSort()). */
#include <stdlib.h>

typedef struct Obj { int key; } Obj;

static int cmp(const void *a, const void *b) {
    return (*(const Obj **)a)->key - (*(const Obj **)b)->key;
}

static int run_sort(Obj **vector, int len) {
    /* Function-type (not pointer) typedef, local to this function. */
    typedef int (comparator)(const void *, const void *);
    int (*fn)(const void *, const void *) = cmp;

    qsort(vector, len, sizeof(Obj *), (comparator *)fn);
    return 0;
}

int main(void) {
    Obj a = {3}, b = {1}, c = {2};
    Obj *vector[3] = {&a, &b, &c};

    run_sort(vector, 3);

    if (vector[0]->key != 1) return 1;
    if (vector[1]->key != 2) return 2;
    if (vector[2]->key != 3) return 3;
    return 0;
}
