/* GCC Bug #60103 - -Wsequence-point warning present only with -O1
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60103
 */


extern unsigned short fn2(unsigned short, unsigned short);
void fn1(int l) {
  l = fn2(l = 0, 0) || 0;
}
// $: gcc-trunk -c -Wsequence-point -O1 s.c
// s.c: In function ‘fn1’:
// s.c:3:5: warning: operation on ‘l’ may be undefined [-Wsequence-point]
   l = fn2(l = 0, 0) || 0;
//      ^
// $: gcc-trunk -c -Wsequence-point -O0 s.c


