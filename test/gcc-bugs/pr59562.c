/* GCC Bug #59562 - __builtin_assume_aligned loses constness when used as initializer element
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59562
 */


#define INT_PTR ((int*)0)
#define INT_PTR_BAA ((int*)__builtin_assume_aligned(0, 4))

int * i = INT_PTR;
int * i_baa = INT_PTR_BAA;


