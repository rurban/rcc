/* GCC Bug #24542 - potential unwanted truncation of operation wrapping should be warned on assignment to wider variable
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=24542
 */


unsigned x1, x2;
   unsigned long long y1;
//    ... /* here we assign to x1 and x2 */
   y1 = x1 * x2; /* no castings -- silent overflow may occur on assignment */
//    ...
   {
      unsigned long long y2 = x1 * x2; /* no castings -- silent overflow may occur on initialization */
//       ...
   }

// (Instead of multiplication, addition or left shift shold be dealt with, too.)
// When the binary operation result is assigned to lvalue of the same width, it's OK not to warn about probable overflow. But in these cases, "do what I mean" is obvious.


