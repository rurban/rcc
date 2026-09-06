/* GCC Bug #112954 - Spelling hint for typos in parameter types in function prototypes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112954
 */
/* { dg-do compile } */


typedef int int32_t;
void function (int32t); /* { dg-error "parameter names .without types. in function declaration" } */

// Note without the typedef here, the C++ front-end does not suggest int,
// but with the typedef, the C++ front-end does.


