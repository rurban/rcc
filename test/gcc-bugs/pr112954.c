/* GCC Bug #112954 - Spelling hint for typos in parameter types in function prototypes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112954
 */


typedef int int32_t;
void function (int32t);
// ```
// Note without the typedef here, the C++ front-end does not suggust int but with the typedef, the C++ front-end does.


