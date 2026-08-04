/* GCC Bug #54192 - -fno-trapping-math by default?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54192
 */


int f(double a,double b){
  if(a>b) if(a<b) return 1;
  return 0;
}
// ```
// Which since GCC 13 gets optimized (again) to return 0 at -O2. Because evrp figures out that `a<b` will be zero and then cleanup cfg comes along and removes the `a>b` comparison too.
// So Maybe it is time to declare -fno-trapping-math as the default and go through the testsuite and add -ftrapping-math as needed for the testcases that are testing the bits.


