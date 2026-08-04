/* GCC Bug #121252 - No way to return large _BitInt with gimpleFE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=121252
 */


typedef unsigned _BitInt(256) u;
// u __GIMPLE() f(const u x, const unsigned n)
{
  return x;
}
// ```
// This does not work as you need to the assignment to result_decl outside of the return and there is no way currently to get the RESULT_DECL in gimple FE.
// (there might other cases where you want the RESULT_DECL even).


