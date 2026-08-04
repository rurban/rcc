/* GCC Bug #64619 - No -Wsign-conversion warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64619
 */


int a;
void fn() /*    */
{
  a ^ 0x1UL;
}
// $: 
// $: gcc-trunk -Wsign-conversion s.c -c
// $: 
// $: clang-trunk -Wno-unused-value -Wsign-conversion s.c -c
// s.c:4:3: warning: implicit conversion changes signedness: 'int' to
//       'unsigned long' [-Wsign-conversion]
  a ^ 0x1UL;
//   ^ ~
// 1 warning generated.
// $: 
// $:


