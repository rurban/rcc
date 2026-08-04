/* GCC Bug #109526 - Broken type name for anonymous struct containing a VLA member in diagnostic message
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109526
 */


void test(__SIZE_TYPE__ len, char *x)
{
//     struct
    {
        char data[len];
    } *p = x;
}
// The diagnostic message is:
// <source>: In function 'test':
// <source>:6:12: warning: initialization of '<U87b8> *' from incompatible pointer type 'char *' [-Wincompatible-pointer-types]
//     6 |     } *p = x;
//       |            ^
// "<U87b8>" does not make any sense to me.


