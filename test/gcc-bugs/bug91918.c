/* GCC Bug #91918 - Const pointer argument to atomic_compare_exchange doesn't cause an error.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91918
 */


#include <stdatomic.h>
static const int desired = 42;
static _Atomic int data = 0;

int main(void) {
//   atomic_compare_exchange_weak(&data, &desired, 64);
}


