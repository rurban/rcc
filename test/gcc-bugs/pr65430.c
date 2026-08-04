/* GCC Bug #65430 - Missing -Wsequence-point warning with COMPOUND_EXPRs
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65430
 */
/* { dg-do compile } */


int a;
int fn1() { 
//   (a = 3, 8) * (a = 0); 
  return a;
}
// $: gcc-trunk -c -Wsequence-point s.c 
// $: clang-trunk -c -Wsequence-point -Wno-unused-value s.c
// s.c:3:6: warning: multiple unsequenced modifications to 'a' [-Wunsequenced]
//   (a = 3, 8) * (a = 0); 
//      ^            ~
// 1 warning generated.
// $: 
// $:


