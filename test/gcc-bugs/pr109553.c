/* GCC Bug #109553 - Atomic operations vs const locations
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109553
 */


#include <stdint.h>
// uint32_t
// load_uint32_t (const uint32_t *a)
{
  return __atomic_load_n (a, __ATOMIC_ACQUIRE);
}
// void
// casa_uint32_t (const uint32_t *a, uint32_t *b, uint32_t *c)
{
  __atomic_compare_exchange_n (a, b, 3, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
}
// Both of these functions compile fine with GCC.
// With Clang casa_uint32_t  gives a hard error:
// error: address argument to atomic operation must be a pointer to non-const type ('const uint32_t *' (aka 'const unsigned int *') invalid)
  __atomic_compare_exchange_n (a, b, 3, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
// I would argue that for both cases the compiler should emit something. I think an error is a appropriate for the __atomic_compare_exchange_n case, but even for atomic load we may want to hint to the user to avoid doing an atomic load from const types.


