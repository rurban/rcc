/* GCC Bug #71589 - Atomic operation on a non-atomic variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71589
 */
/* { dg-do compile } */


// file X.C
#include "stdatomic.h"
int* p0;

void bug (void)
{
    atomic_fetch_add_explicit (&p0, 1, memory_order_relaxed);
}
// The compiler accepts this programm even though p0 is a non-atomic pointer.
// I think this is not standard conforming. The C11 standard decribes the
// atomic_fetch functions as expecting a pointer to an atomic type (see
// 7.17.1/5 and 7.17.7.5/1).
// With kind regards
// W. Roehrl


