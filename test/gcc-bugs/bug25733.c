/* GCC Bug #25733 - missed diagnostic about assignment used as truth value.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=25733
 */
/* { dg-do compile } */


int foo(int a, int b)
{
    return ((a = b) ? 1 : 0);  // <= missed warning.
}
int bar(int a, int b)
{
    if (a = b)                 // warning present.
        return 1;
//     else
        return 0;
}


