/* GCC Bug #38047 - -Wredundant-decls does not take scope into account
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38047
 */


// /* red.c */
void foo(void)
{
    extern int x;
//     /* use x */
    x = 1;
}

void bar(void)
{
    extern int x;
//     /* use x */
    x = 2;
}
// red.c: In function 'bar':
// The two "extern int x;" declarations are *not* redundant, and removing either will cause this code to fail to compile.
// Also happens with GCC 4.1.2.


