/* GCC Bug #103123 - incorrect or misleading warning "floating constant exceeds range of ..." in ISO C11 and C17
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103123
 */


double foo (void)
{
  return 1.0e999;
}
#endif
// I get the following warning:
// tst.c: In function 'foo':
// tst.c:4:3: warning: floating constant exceeds range of 'double' [-Woverflow]
//     4 |   return 1.0e999;
//       |   ^~~~~~
// Tested version:
// However, the C11 committee draft N1570 and C17 ballot N2176 say in 5.2.4.2.2p5: "The minimum range of representable values for a floating type is the most negative finite floating-point number representable in that type through the most positive finite floating-point number representable in that type. In addition, if negative infinity is representable in a type, the range of that type is extended to all negative real numbers; likewise, if positive infinity is representable in a type, the range of that type is extended to all positive real numbers."
// Note: The C2x draft N2731 says the same thing.
// So, since __STDC_IEC_559__ is defined, there is a negative and a positive infinity for the double type, and 5.2.4.2.2p5 implies that the range is the whole set of real numbers. In particular, it includes the constant 1.0e999 above, so that the warning is incorrect, or at least misleading.


