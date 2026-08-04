/* GCC Bug #67661 - Wrong warning when declare VLAs: operation on 'x' may be undefined [-Wsequence-point]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67661
 */
/* { dg-do compile } */


#include <stdio.h>

int main (void)
{
    int x = 0, y [++ x], z [++ x];
    printf ("%d, %d, %d\n", sizeof x, sizeof y, sizeof z);
    return 0;
}
// and are compiled with option '-Wall'.for example:
// then produce a warning 'operation on 'x' may be undefined [-Wsequence-point]'


