/* GCC Bug #116357 - The item's address of the array is not correct if aligned is used
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116357
 */


>      {
>      }
typedef double adouble __attribute__((aligned(8)));

struct X
{
  int n;
  adouble x[1024];
};
// not sure if there's a more commonly under-aligned base type to
// check this with.  An aggregate would of course work as well but
// code paths might be subtly different.


