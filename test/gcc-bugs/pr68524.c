/* GCC Bug #68524 - Please support attributes between function definition and opening brace
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68524
 */


void f(void) __attribute__((const))
{
}
// test.c:1:1: error: attributes should be specified before the declarator in a function definition
 void f(void) __attribute__((const))
//  ^
// Sparse's C parser supports this.  Could GCC support this syntax as well?


