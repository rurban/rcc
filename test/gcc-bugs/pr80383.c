/* GCC Bug #80383 - wrong caret location and missing detail in warning: initializer element is not a constant expression on a signed overflow
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80383
 */
/* { dg-do compile } */


const int i = 1 << (sizeof (int) * __CHAR_BIT__ - 1);
const int j = 1 << (sizeof (int) * __CHAR_BIT__);
// b.c:1:15: warning: initializer element is not a constant expression [-Wpedantic]
 const int i = 1 << (sizeof (int) * __CHAR_BIT__ - 1);
//                ^
// b.c:2:17: warning: left shift count >= width of type [-Wshift-count-overflow]
 const int j = 1 << (sizeof (int) * __CHAR_BIT__);
//                  ^~
// b.c:2:15: warning: initializer element is not a constant expression [-Wpedantic]
 const int j = 1 << (sizeof (int) * __CHAR_BIT__);
//                ^
// In contrast, with -Wshift-overlow=2, GCC prints the following.  With -Wshift-overflow and -Wshift-count-overflow the caret is in the right place (but the operands aren't underlined).

// b.c:1:17: warning: result of ‘1 << 31’ requires 33 bits to represent, but ‘int’ only has 32 bits [-Wshift-overflow=]
 const int i = 1 << (sizeof (int) * __CHAR_BIT__ - 1);
//                  ^~
// b.c:1:15: warning: initializer element is not a constant expression [-Wpedantic]
 const int i = 1 << (sizeof (int) * __CHAR_BIT__ - 1);
//                ^
// b.c:2:17: warning: left shift count >= width of type [-Wshift-count-overflow]
 const int j = 1 << (sizeof (int) * __CHAR_BIT__);
//                  ^~
// b.c:2:15: warning: initializer element is not a constant expression [-Wpedantic]
 const int j = 1 << (sizeof (int) * __CHAR_BIT__);
//                ^
// With -Weverything Clang prints the following warnings for the problems discussed above:
// t.c:1:17: warning: signed shift result (0x80000000) sets the sign bit of the
//       shift expression's type ('int') and becomes negative
//       [-Wshift-sign-overflow]
const int i = 1 << (sizeof (int) * __CHAR_BIT__ - 1);
//               ~ ^  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// t.c:2:17: warning: shift count >= width of type [-Wshift-count-overflow]
const int j = 1 << (sizeof (int) * __CHAR_BIT__);
//                 ^  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


