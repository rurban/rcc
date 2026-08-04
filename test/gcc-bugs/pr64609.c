/* GCC Bug #64609 - No -Wbool-compare warning on "(a = 0 && 0) <= 4"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64609
 */
/* { dg-do compile } */


int a;
void fn1() { ((a = 0 && 0) <= 4); }
// $: 
// $: gcc-trunk -c s.c -Wbool-compare
// $: 
// $: clang-trunk -c s.c -Wno-unused-value
// s.c:2:28: warning: comparison of constant 4 with boolean expression is always
//       true [-Wtautological-constant-out-of-range-compare]
void fn1() { ((a = 0 && 0) <= 4); }
//               ~~~~~~~~~~~~ ^  ~
// 1 warning generated.
// $: 
// $:


