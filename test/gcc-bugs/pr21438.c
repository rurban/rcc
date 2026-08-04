/* GCC Bug #21438 - Warning about division by zero depends on lexical form
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=21438
 */
/* { dg-do compile } */


float f[] = {
 1.0f/0.0f,  /* No warning */
 1.0f/0      /* Warning (and diagnostic is on the wrong line in 3.3 at least) */
};
// GCC is supposed to suppress division-by-zero warnings for floating point.
// However the warning was based on the lexical form (i.e. the unpromoted
// type of the operands) rather than on the promoted type, so
//   float f[] = { 1.0f/0.0f, 1.0f/0 };
// warns for the second element (an int 0 divisor) but not the first (a
// float 0.0f divisor), even though both compute the same floating-point
// division.  It was suggested that the warning should instead depend on
// the promoted type so both cases are treated the same way.
//
// The C++ front-end additionally warns twice for templates in cases like:
//   template <int C>
//   int f2(int t)
//   {
//           return t/0;
//   }
//   int tt1 = f2<0>(1);
// but only once (or not at all) for:
//   template <typename C>
//   int f(C t) { return t/0; }
//   template <int C>
//   int f1(int t) { return t/C; }
//   int tt = f<int>(1);
//   int tt1 = f1<0>(1);
// (that C++-specific behavior does not apply to this C testcase).
