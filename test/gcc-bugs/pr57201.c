/* GCC Bug #57201 - --save-temps shows correct warning about macro in system-header (Wsystem-header)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57201
 */
/* { dg-do compile } */


#include <stdio.h>
void f(void)
{
  stdout;
}
#define stdout stdout


