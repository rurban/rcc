/* GCC Bug #123623 - Split out the extension that the Linux kernel uses from -fms-extensions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123623
 */
/* { dg-do compile } */


struct s1 {
  int t;
};

struct s2 {
  struct s1;
};

int f(struct s2 a)
{
  return a.t;
}
// ```
// It would be useful to split this out into its own option.
// See thread at <a href="https://hachyderm.io/@kees/115896006860477202">https://hachyderm.io/@kees/115896006860477202</a> .
// Aaron Ballman agrees (but his reply is private) and says clang should do the same.


