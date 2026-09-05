/* `_Atomic int *p` declares p as an ordinary (non-atomic) pointer to an
 * atomic int -- the `_Atomic` qualifies the pointee, not p itself. Unlike
 * `int *_Atomic p` (a genuinely atomic pointer object, see
 * test_atomic_fetch_pointer_scale.c), `&p` here does not point to an atomic
 * object at all, so atomic_fetch_add must be rejected. Matches real GCC
 * (verified against tinycc's 125_atomic_misc.c test_atomic_error_3) and
 * regresses a fix that conflated "pointee is a pointer type" with "pointee
 * is an atomic pointer type".
 */
#include <stdatomic.h>

int main(void) {
    _Atomic int *p = 0;
    atomic_fetch_add(&p, 1); /* must error: not an atomic pointer object */
    return 0;
}
