/* GCC treats the libatomic helper names __atomic_<op>_<N> (N = 1/2/4/8/16)
 * as builtins equivalent to the __atomic_<op>_n / generic __atomic_<op>
 * forms, and inlines them for basic sizes. glib's own gatomic.h calls
 * __atomic_load_4/8 and __atomic_store_4/8 directly (the libatomic entry
 * points, not the _n builtins). rcc only recognized the _n/generic forms,
 * so it emitted these helper names as ordinary unresolved function calls --
 * `undefined reference to __atomic_load_4` etc. at link time -- instead of
 * inlining the instruction. Found via test_glib (glib/gatomic.h), whose
 * whole library compiles but fails to link gtester/libglib-2.0.so.
 *
 * Fixed by recognizing the __atomic_<op>_<N> spelling (N in 1/2/4/8/16)
 * and routing it through the same ND_ATOMIC_* lowering as __atomic_<op>_n.
 */
#include <stdint.h>

static int test_atomic_load_store_4(void)
{
    int32_t x = 0;
    __atomic_store_4(&x, 0x12345678, __ATOMIC_SEQ_CST);
    if (__atomic_load_4(&x, __ATOMIC_SEQ_CST) != 0x12345678) return 1;
    return 0;
}

static int test_atomic_load_store_8(void)
{
    int64_t x = 0;
    __atomic_store_8(&x, 0x123456789abcdef0LL, __ATOMIC_SEQ_CST);
    if (__atomic_load_8(&x, __ATOMIC_SEQ_CST) != 0x123456789abcdef0LL) return 1;
    return 0;
}

static int test_atomic_exchange_4(void)
{
    int32_t x = 1;
    int32_t old = __atomic_exchange_4(&x, 42, __ATOMIC_SEQ_CST);
    if (old != 1 || __atomic_load_4(&x, __ATOMIC_SEQ_CST) != 42) return 1;
    return 0;
}

static int test_atomic_compare_exchange_4(void)
{
    int32_t x = 10, expected = 10;
    /* success: returns 1, stores 99 */
    if (!__atomic_compare_exchange_4(&x, &expected, 99, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        return 1;
    if (x != 99) return 2;
    /* failure: returns 0, loads x into expected */
    expected = 10;
    if (__atomic_compare_exchange_4(&x, &expected, 77, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        return 3;
    if (x != 99 || expected != 99) return 4;
    return 0;
}

/* The exact glib gatomic.h idiom: the helper name used inside a macro in
 * a statement-expression context. */
static int test_glib_gatomic_shape(void)
{
    int32_t v = 5;
    int32_t got = (__atomic_load_4(&v, __ATOMIC_SEQ_CST));
    __atomic_store_4(&v, got + 1, __ATOMIC_SEQ_CST);
    return __atomic_load_4(&v, __ATOMIC_SEQ_CST) == 6 ? 0 : 1;
}

int main(void)
{
    int r = test_atomic_load_store_4();
    if (r) return r;
    r = test_atomic_load_store_8();
    if (r) return 10 + r;
    r = test_atomic_exchange_4();
    if (r) return 20 + r;
    r = test_atomic_compare_exchange_4();
    if (r) return 30 + r;
    r = test_glib_gatomic_shape();
    if (r) return 40 + r;
    return 0;
}
