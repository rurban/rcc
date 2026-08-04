/* GCC Bug #71613 - Useful warnings silenced when macros from system headers are used
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71613
 */


#include <stdatomic.h>

int main(void) {
//         atomic_thread_fence(memory_order_relaxed);
        __atomic_thread_fence(__ATOMIC_RELAXED);
        return 0;
}
//     5 |         __atomic_thread_fence(__ATOMIC_RELAXED);


