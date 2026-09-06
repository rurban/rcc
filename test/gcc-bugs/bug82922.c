/* GCC Bug #82922 - Request: add -Wstrict-prototypes to -Wextra as K&R style is obsolescent
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82922
 */


double f(t, x, y)
 double t, x, y;
{ }
int main(void) {
// 	//f(0.0, 1.0, 1.0);
// 	f(0, 1, 1);  // UB
}
// Unprototyped functions can be diagnosed by combining -Wimplicit-function-declaration and -Wstrict-prototypes. The former is in -Wall and imposed since C99.
// -Wold-style-definition cases are covered by -Wstrict-prototypes too.
// -Wold-style-declaration (obsolescent: C89 3.9.3, C99 6.11.5, C11 6.11.5) is included in -Wextra.
// There has been a bit of discussion on gcc-help:
// <a href="https://gcc.gnu.org/ml/gcc-help/2017-11/msg00001.html">https://gcc.gnu.org/ml/gcc-help/2017-11/msg00001.html</a>


