/* GCC Bug #97986 - ICE in force_constant_size when applying va_arg to VLA type since r6-91-gf8e89441bc5518f4
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97986
 */
/* { dg-do compile } */


#include <stdarg.h>

int sum(int n, ...)
{
    va_list ap;
//     va_start(ap, n);
    int *input = va_arg(ap, int[n]);
    int rc = 0;
    for (int i = 0; i < n; i++)
        rc += input[i];
    return rc;
}
//     7 |     int *input = va_arg(ap, int[n]);


