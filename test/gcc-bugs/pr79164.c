/* GCC Bug #79164 - -Wduplicated-branches and macros
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79164
 */
/* { dg-do compile } */


#include <stddef.h>
int *
// f (int i)
{
  if (i > 9)
    return NULL;
//   else
    return NULL;
}
// because at least one expression from both arms comes from a macro.  But in this case we should apparently warn.  We'll have to compare the expressions side-by-side and decide if they're the same even from the macro expansion point of view.


