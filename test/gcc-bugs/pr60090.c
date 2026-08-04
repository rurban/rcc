/* GCC Bug #60090 - For expression without ~, gcc -O1 emits "comparison of promoted ~unsigned with unsigned"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60090
 */


int fn1(unsigned char a, unsigned char b) {
  const unsigned l = 4294967295u;
  return (l ^ a) != b;
}
// $: gcc-trunk -c -Wsign-compare s.c
// $: gcc-trunk -c -Wsign-compare s.c -O1
// s.c: In function ‘fn1’:
// s.c:3:18: warning: comparison of promoted ~unsigned with unsigned [-Wsign-compare]
   return (l ^ a) != b;
//                   ^


