/* GCC Bug #79164 - -Wduplicated-branches and macros
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79164
 */
/* { dg-do compile } */


#include <stddef.h>
int *
f (int i)
{
  if (i > 9)
    return NULL;
  else
    return NULL;
}


