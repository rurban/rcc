/* GCC Bug #70407 - alignment of array elements is greater than element size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70407
 */


> typedef struct S_ { short f[3] __attribute((aligned(8))); } S;
    typedef int more_aligned_int __attribute__ ((aligned (8)));
// it's still an issue.


