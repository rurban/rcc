/* GCC Bug #60021 - Inconsistent -Wsign-compare warnings for -O0 and -O1
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60021
 */
/* { dg-do compile } */


typedef long int64_t;
void fn1(unsigned, char, long);
void fn1(unsigned p_26, char c, long l) { /*    */
  const char l_1051 = 0;
  int64_t *l_1059 = &l;
  char *l_1062 = &c;
  p_26 < (*l_1062 = (1L == (*l_1059 = 0)) <= l_1051);
}
// Configured with: ../gcc-trunk/configure --enable-languages=c,c++ --disable-multilib --prefix=/home/chengniansun/tools/gcc-trunk-binaries : (reconfigured) ../gcc-trunk/configure --enable-languages=c,c++ --disable-multilib --prefix=/home/chengniansun/tools/gcc-trunk-binaries : (reconfigured) ../gcc-trunk/configure --enable-languages=c,c++ --disable-multilib --prefix=/home/chengniansun/tools/gcc-trunk-binaries


