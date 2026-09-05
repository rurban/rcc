/* GCC PR #64843: atomic_fetch_add/sub on a genuinely atomic pointer object
 * (`int *_Atomic p`, i.e. the POINTER itself is atomic-qualified) must scale
 * the operand by the pointee's size, exactly like ordinary `p += n` pointer
 * arithmetic (C11 7.17.7.5). rcc used to add/sub the raw operand unscaled,
 * miscompiling `atomic_fetch_add(&p, 1)` on `int *` into "add 1" instead of
 * "add 4".
 */
#include <stdatomic.h>
#include <stdio.h>

int arr[10];

int main(void) {
    int *_Atomic p = &arr[0];
    int *old = atomic_fetch_add(&p, 3);
    if (old != &arr[0]) return 1;
    if (p != &arr[3]) return 2;

    int *old2 = atomic_fetch_sub(&p, 1);
    if (old2 != &arr[3]) return 3;
    if (p != &arr[2]) return 4;

    printf("OK\n");
    return 0;
}
