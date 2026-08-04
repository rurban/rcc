/* GCC Bug #80793 - three signed conversion warnings for the same expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80793
 */


int f (int i)
{
  unsigned char c = i ? (-__SCHAR_MAX__ - 1) : 1U;
  return c;
}
// t.c: In function ‘f’:
// t.c:3:46: warning: signed and unsigned type in conditional expression [-Wsign-compare]
   unsigned char c = i ? (-__SCHAR_MAX__ - 1) : 1U;
//                                               ^
// t.c:3:46: warning: negative integer implicitly converted to unsigned type [-Wsign-conversion]
// t.c:3:21: warning: conversion to ‘unsigned char’ alters ‘unsigned int’ constant value [-Wconversion]
   unsigned char c = i ? (-__SCHAR_MAX__ - 1) : 1U;
//                      ^
// In contrast, Clang issues the following:
// t.c:3:41: warning: operand of ? changes signedness: 'int' to 'unsigned char'
//       [-Wsign-conversion]
  unsigned char c = i ? (-__SCHAR_MAX__ - 1) : 1U;
//                 ~        ~~~~~~~~~~~~~~~^~~
// t.c:1:5: warning: no previous prototype for function 'f' [-Wmissing-prototypes]
int f (int i)
//     ^
// 2 warnings generated.


