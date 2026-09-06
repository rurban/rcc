/* GCC Bug #111269 - location for non-constant expressions inside static assert could be better
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111269
 */
/* { dg-do compile } */

int main(void)
{
  int x = 42;
  _Static_assert(0 || 7 > x, ""); /* { dg-error "expression in static assertion is not constant" } */
}

/* Clang points to the precise subexpression; gcc's caret covers the whole
 * "0 || 7 > x" - the fuzzy location is the bug. */