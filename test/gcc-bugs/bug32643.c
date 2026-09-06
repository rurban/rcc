/* GCC Bug #32643 - Wrong error message with unsigned char a = uchar&512
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=32643
 */
/* { dg-do compile } */


unsigned char p;
unsigned char p1 = p & 512;
// ------------- cut -------
// We get (with -Wall -Wpedantic):
// pr32643.c:2:20: warning: overflow in conversion from 'int' to 'unsigned char' changes value from '(int)p & 512' to '0' [-Woverflow]
// pr32643.c:2:1: warning: overflow in constant expression [-Woverflow]
//
// Now there is an overflow but only because we optimize the IR
// (unsigned char)(((int)p)& 512) into:
// p & (unsigned char)512 and (unsigned char)512 is converted into 0 (with
// overflow) so we have p & 0 which is then optimized into 0(with
// overflow).  So we are only rejecting this because of the overflow due
// to the conversion (which was due to fold).
//
// We don't reject as invalid code either (same underlying fold issue,
// shown here rather than redeclaring p/p1 to keep this a single TU):
//   unsigned char p;
//   unsigned char p1 = p & 0;
//
// With -pedantic-errors this testcase is wrongly rejected as a hard
// error even though p is not a constant expression, so there is no
// actual overflow taking place.
