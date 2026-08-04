/* GCC Bug #64639 - missing warning by -Wunused-value in compound expressions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64639
 */
/* { dg-do compile } */


int *a;
int b;
void f() { 
  b = (0, (a = 0) != 0, 0);
}
// $: 
// $: gcc-trunk -c -Wunused-value s.c
// s.c: In function ‘f’:
// s.c:4:9: warning: left-hand operand of comma expression has no effect [-Wunused-value]
// b = (0, (a = 0) != 0, 0);
//          ^
// $: 
// $: clang-trunk -c -Wunused-value s.c
// s.c:4:8: warning: expression result unused [-Wunused-value]
// b = (0, (a = 0) != 0, 0);
//        ^
// s.c:4:19: warning: expression result unused [-Wunused-value]
// b = (0, (a = 0) != 0, 0);
//           ~~~~~~~ ^  ~
// 2 warnings generated.
// $: 
// $:


