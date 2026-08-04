/* GCC Bug #124699 - [13 Regression] Expression using __builtin_ffsll() may not be considered constant
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124699
 */

// Refined testcase from comment 2 / attachment 64088 ("testcase that shows
// issue"): the condition is negated relative to comment 0's original
// snippet so the assertion holds (compiles clean) once GCC correctly
// considers __builtin_ffsll(~0ULL) + 1 < 0 a constant expression.  On
// affected GCC 13 branches this instead failed with "expression in static
// assertion is not constant".
int main() {
  sizeof(struct {
  _Static_assert(!(__builtin_ffsll(~0ULL) + 1 < 0));
  });
  }


