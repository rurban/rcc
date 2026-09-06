/* GCC Bug #17170 - add warning for bitfield declarations where the presence of a signbit (or lack thereof) could lead to confusion [-Wdefault-bitfield-sign]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=17170
 */
/* { dg-do compile } */


struct oink {
  int foo : 5;        // warning: the signedness of foo is implementation defined
  signed int bar : 5; // no warning
  int : 5;            // no warning (since no name)
  int booze : 1;      // warning: dubious signed one-bit bitfield
  signed int baz : 1; // no warning.
};

// (A signed bitfield on width one can contain {-1;0} or {0} depending on integer
// representation.  The latter allows for impressive optimizations, but surely
// someone meant to use unsigned.)
// Adding "signed" or "unsigned" as shown would silence the warnings.  Further,
// I would assume that foo's warning above would be silenced by the use of one
// of
// `-fsigned-bitfields'
// `-funsigned-bitfields'
// `-fno-signed-bitfields'
// `-fno-unsigned-bitfields'
// booze's warning should remain, though.


