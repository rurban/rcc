/* GCC Bug #109553 - Atomic operations vs const locations
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109553
 */
/* { dg-do compile } */

#include <stdint.h>

uint32_t
load_uint32_t (const uint32_t *a)
{
  return __atomic_load_n (a, __ATOMIC_ACQUIRE);
}
void
casa_uint32_t (const uint32_t *a, uint32_t *b, uint32_t *c)
{
  __atomic_compare_exchange_n (a, b, 3, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
}

/* Both functions compile fine with GCC (the bug: no diagnostic for atomic
 * operations on const locations).  Clang errors on casa_uint32_t:
 * "address argument to atomic operation must be a pointer to non-const
 *  type ('const uint32_t *' invalid)". */