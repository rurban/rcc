/* GCC Bug #32643 - Wrong error message with unsigned char a = uchar&512
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=32643
 */
/* { dg-do compile } */


unsigned char p;
unsigned char p1 = p & 512;
// ------------- cut -------
// We get:
// Now there is an overflow but only because we optimize the IR (unsigned char)(((int)p)& 512) into:
// p & (unsigned char)512 and (unsigned char)512 is converted into 0 (with overflow) so we have p & 0 which is then optimized into 0(with overflow).  So we are only rejecting this because of the overflow due to the conversion (which was due to fold).
// We don't reject as invalid code either:
unsigned char p;
unsigned char p1 = p & 0;


