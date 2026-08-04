/* GCC Bug #71610 - Improve location for "warning: ISO C restricts enumerator values to range of ‘int’ [-Wpedantic]"?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71610
 */


int main()
{
  enum { c = -3000000000 };
}
// ----------------------------------------------------------------------
Results:
// ----------------------------------------------------------------------
// test.c: In function ‘main’:
// test.c:3:14: warning: ISO C restricts enumerator values to range of ‘int’ [-Wpedantic]
   enum { c = -3000000000 };
//               ^
// ----------------------------------------------------------------------
// For comparison:
// ----------------------------------------------------------------------
// test.c:3:10: warning: ISO C restricts enumerator values to range of 'int' (-3000000000 is too small) [-Wpedantic]
  enum { c = -3000000000 };
//          ^   ~~~~~~~~~~~
// 1 warning generated.
// ----------------------------------------------------------------------
// clang version: clang version 3.9.0 (trunk 271312)


