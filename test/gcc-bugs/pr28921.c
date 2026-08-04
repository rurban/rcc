/* GCC Bug #28921 - vector of a typedef applies the vector to the inner most type instead of erroring/warning out that vector does not apply
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=28921
 */


typedef char *cptr;

char *a;

__attribute__ ((vector_size(16))) cptr t;

int f(void)
{

__attribute__ ((vector_size(16))) int t1 =
//    (__attribute__ ((vector_size(16))) int )t;
}
// We get an error about converting t to a vector int but t looks to me a vector of a char pointer.  This happens with both the C and C++ front-ends.


