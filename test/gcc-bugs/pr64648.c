/* GCC Bug #64648 - Incorrect message description of -Wunused-value
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64648
 */


int a;
void f() {
//   (a = 0) != 0;
//   (a = 0) >= 0;
}
// $: 
// $: gcc-trunk -c -Wunused-value t.c
// t.c: In function ‘f’:
// t.c:3:11: warning: value computed is not used [-Wunused-value]
//    (a = 0) != 0;
//            ^
// t.c:4:11: warning: right-hand operand of comma expression has no effect [-Wunused-value]
//    (a = 0) >= 0;
//            ^
// $: 
// $: clang-trunk -c -Wunused-value t.c
// t.c:3:11: warning: inequality comparison result unused [-Wunused-comparison]
//   (a = 0) != 0;
//   ~~~~~~~~^~~~
// t.c:4:11: warning: relational comparison result unused [-Wunused-comparison]
//   (a = 0) >= 0;
//   ~~~~~~~~^~~~
// 2 warnings generated.
// $:


