/* `__atomic_load(ptr, retptr, order)` on `float`/`double` (found via
 * CPython's pyatomic_gcc.h, which uses exactly this pattern for its
 * portable atomic load API -- `_Py_atomic_load_float_relaxed()` etc.
 * calls `__atomic_load(obj, &value, __ATOMIC_RELAXED)` directly).
 * Note: `__atomic_load_n` (the "_n" register-return form) does NOT
 * accept float/double at all -- both gcc and clang reject it at
 * compile time ("operand type 'float *' is incompatible") -- only the
 * generic `__atomic_load`/`__atomic_store` forms, which go through a
 * caller-supplied pointer instead of a return register, work on
 * arbitrary types including flonums.
 *
 * Two independent bugs, both in the same neighborhood:
 *
 *  1. Parser: `__atomic_load(ptr, retptr, order)` takes THREE
 *     arguments -- ptr, a *pointer to* where the loaded value is
 *     stored, and the memory order -- but rcc's parser only ever
 *     consumed TWO (ptr, order), silently misreading `retptr` itself
 *     as the memory-order argument. `__atomic_store`'s parser entry
 *     already handles the identical ptr/valptr asymmetry between its
 *     `_n` and non-`_n` forms; `__atomic_load`'s entry never did.
 *
 *  2. Codegen: rcc represents a `float`/`double` VALUE sitting in a GP
 *     register as its DOUBLE bit pattern (float widened to double),
 *     not the same-size raw bytes -- every other codegen path that
 *     loads a flonum from memory (e.g. a plain `*floatptr` deref)
 *     already does the movss+cvtss2sd+movq-to-GP widening dance.
 *     ND_ATOMIC_LOAD's codegen never special-cased flonum types at
 *     all: for a `float`, it read the raw 4-byte pattern zero-extended
 *     into a GP register, which every later flonum consumer then
 *     misinterpreted as a genuine (garbage) DOUBLE bit pattern. A
 *     `double` atomic load happened to work by accident (its raw
 *     8-byte pattern already IS a valid double bit pattern -- no
 *     widening needed), which is exactly what let this go unnoticed
 *     until a `float` atomic exposed it. */
#include <assert.h>

static float g_float = 3.5f;
static double g_double = 7.25;
static int g_int = 42;

static float load_float_via_retptr(float *obj) {
    float ret;
    __atomic_load(obj, &ret, __ATOMIC_RELAXED);
    return ret;
}

int main(void) {
    float f;
    double d;

    __atomic_load(&g_float, &f, __ATOMIC_RELAXED);
    assert(f == 3.5f);
    __atomic_load(&g_double, &d, __ATOMIC_RELAXED);
    assert(d == 7.25);

    __atomic_load(&g_float, &f, __ATOMIC_SEQ_CST);
    assert(f == 3.5f);
    __atomic_load(&g_double, &d, __ATOMIC_SEQ_CST);
    assert(d == 7.25);

    /* Cross-check the plain (non-flonum) non-_n form still works --
     * this path already existed and must stay correct. */
    int r;
    __atomic_load(&g_int, &r, __ATOMIC_SEQ_CST);
    assert(r == 42);

    /* Integer __atomic_load_n (the register-return form) is still
     * valid for integer/pointer types and must keep working. */
    int n = __atomic_load_n(&g_int, __ATOMIC_RELAXED);
    assert(n == 42);

    /* Through a real function call/return boundary. */
    assert(load_float_via_retptr(&g_float) == 3.5f);

    return 0;
}
