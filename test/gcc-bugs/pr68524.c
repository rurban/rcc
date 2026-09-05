/* GCC Bug #68524 - Please support attributes between function definition and opening brace
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68524
 */
/* { dg-do compile } */


void f(void) __attribute__((const))
{
}


