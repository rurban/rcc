/* GCC Bug #116474 - GCC somehow confuses "unsigned typedef_t" with a nested function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116474
 */


int main(){
 unsigned __int128_t x = 0;
}
// Compiled with -pedantic, results in the following warning:
// <source>: In function 'main':
// <source>:2:9: warning: ISO C forbids nested functions [-Wpedantic]
//     2 |         unsigned __int128_t x = 0;
//       |         ^~~~~~~~
// <source>:2:29: error: expected '=', ',', ';', 'asm' or '__attribute__' before 'x'
//     2 |         unsigned __int128_t x = 0;
//       |                             ^
// <source>:2:29: error: 'x' undeclared (first use in this function)
// <source>:2:29: note: each undeclared identifier is reported only once for each function it appears in
// It seems quite odd that a warning for a nested function is emitted here.


