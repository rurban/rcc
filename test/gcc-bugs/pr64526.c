/* GCC Bug #64526 - No warning on function call with excessive arguments
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64526
 */


void fn1 () {}

void fn2 (int p) {  fn1 (p); }
// test.c:2:27: warning: too many arguments in call to 'fn1'
void fn2 (int p) {  fn1 (p); }
//                     ~~~   ^
// 1 warning generated.
// $


