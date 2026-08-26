/* Regression test: static pointer initializers with integer literals that
 * overflow int32 were sign-extended to 64 bits.
 *
 * extract_reloc()'s ND_NUM / ND_NEG / ND_NOT / ND_BITNOT cases truncated
 * the constant to `int` (the reloc-addend width) before the caller wrote
 * it as the full object width, so `static void *p = (void*)0xdeadbeef;`
 * stored 0xffffffffdeadbeef instead of 0x00000000deadbeef -- an
 * unsigned-int literal that does not fit int32 became a negative int and
 * sign-extended. Values that fit int32 (including -1) were unaffected.
 * The corrupted pointer also broke __atomic_compare_exchange drain loops
 * that compare the stored pointer against a full-width expected value
 * (glib's register_lazy_static_resources_unlocked).
 *
 * Fixed by making extract_reloc() decline values that overflow its int
 * addend (return false) and falling back to the 64-bit const-expr
 * evaluator at both global-initializer call sites.
 */
#include <assert.h>
#include <stdio.h>

static void *a = (void*)0xdeadbeef;      /* unsigned int, does not fit int32 */
static char *b = (char*)0xffffffff;      /* unsigned int, does not fit int32 */
static void *c = (void*)-1;              /* fits int32, must stay -1 */
static long d = (long)0xdeadbeef;        /* non-pointer int control */
static void *lazy = (void*)0xdeadbeef;   /* sentinel for the drain loop */

int main(void) {
    assert(a == (void *)(unsigned long long)0x00000000deadbeefULL);
    assert(b == (void *)(unsigned long long)0x00000000ffffffffULL);
    assert(c == (void *)-1);
    assert(d == 0xdeadbeefL);

    /* Drain loop like glib's register_lazy_static_resources_unlocked:
     * compares the full 64-bit stored pointer against the expected value. */
    void *list;
    int spins = 0;
    do {
        list = lazy;
        spins++;
    } while (!__atomic_compare_exchange_n(&lazy, &list, NULL, 0,
                                           __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
    assert(list == (void *)(unsigned long long)0x00000000deadbeefULL);
    assert(lazy == NULL);
    assert(spins == 1);

    printf("ok\n");
    return 0;
}
