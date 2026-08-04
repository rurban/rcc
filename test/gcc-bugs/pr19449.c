/* GCC Bug #19449 - __builtin_constant_p cannot resolve to const when optimizing
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=19449
 */


int y;
static char a[__builtin_constant_p (y) ? -1 : 1];
extern char b[__builtin_constant_p (y) ? -1 : 1];
char d[__builtin_constant_p (y) ? -1 : 1];
// void
// foo (int x)
{
  static char e[__builtin_constant_p (x) ? -1 : 1];
  extern char f[__builtin_constant_p (x) ? -1 : 1];
  auto char g[__builtin_constant_p (x) ? -1 : 1];
  char h[__builtin_constant_p (x) ? -1 : 1];
}

// Right now this compiles fine for -O0, but for -O1 and above it errors on e and f (twice on the latter actually).  When cfun == NULL, we always fold __builtin_constant_p right away, but when cfun is NULL, we don't consider static/extern.  Not sure how to fix this issue though, because I think the declspecs aren't passed down to declarator parsing.


