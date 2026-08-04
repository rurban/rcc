/* GCC Bug #78155 - missing warning on invalid usage of functions/macros from <ctype.h> (isalpha et al.)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78155
 */


int main (void)
{
    __builtin_printf ("%i\n", __builtin_isalpha (999999));
}
// ;; Function main (main, funcdef_no=2, decl_uid=1965, cgraph_uid=2, symbol_order=2)

// main ()
{
  int D.1968;
  int _1;
  int _4;
//   <bb 2>:
  _1 = __builtin_isalpha (999999);
  __builtin_printf ("%i\n", _1);
  _4 = 0;
// <L0>:
  return _4;

}
// Segmentation fault (core dumped)


