/* GCC Bug #71610 - Improve location for "warning: ISO C restricts enumerator values to range of ‘int’ [-Wpedantic]"?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71610
 */
/* { dg-do compile } */


int main()
{
  enum { c = -3000000000 };
}

