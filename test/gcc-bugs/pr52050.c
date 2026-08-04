/* GCC Bug #52050 - Want an option to warn about a declaration inside a for/while/if statements.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52050
 */
/* { dg-do compile } */


void foo(void)
{
    for (int i = 0; i < 2; i++);
}
// Compiling the above code with -std=c99 -Wdeclaration-after-statement
// does not produce a warning as expected.


