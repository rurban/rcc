/* GCC Bug #70742 - Support div as a builtin
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70742
 */


#define div(a, b) ({ div_t res; _Complex int r = __builtin_div(a,b); res.quot = _Real r; res.rem = _Imag r; res; })
// that's without an inline function.  It's also way nicer to GCC internally,
// not requiring memory for the return value - we'd have to lower to this kind
// of internal code anyways to make code-generation good.


