/* GCC Bug #99950 - Option -Wchar-subscripts leads to wrong fixes when used with functions from <ctype.h>
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99950
 */


extern int a[255];

int f (char c)
{
  return a[c];                   // -Wchar-subscripts
}

int g (char c)
{
  return a[(int)c];              // same bug, no warning
}
//     5 |   return a[c];                   // -Wchar-subscripts


