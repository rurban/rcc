/* GCC Bug #44949 - extend Wparentheses from & to &=
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44949
 */
/* { dg-do compile } */


void f(int i)
{
  if (i&=2 == 0)
    i = 1;
}


