/* GCC Bug #119555 - [avr] const _Fract: Wrong warning: variable 'f0' set but not used
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119555
 */
/* { dg-do compile } */
/* { dg-skip-if "avr fixed-point target" { x86_64-*-* i?86-*-* } } */


#define T _Fract

T line1 (T);

T func1 (T t)
{
  const T f0 = line1 (t);
  return f0;
}
// warn.c: In function 'func1':
// warn.c:7:11: warning: variable 'f0' set but not used [-Wunused-but-set-variable]
//     7 |   const T f0 = line1 (t);
//       |           ^~
// Target: avr
// Configured with: ../../source/gcc-master/configure --target=avr --disable-nls --with-dwarf2 --with-gnu-as --with-gnu-ld --with-long-double=64 --disable-libcc1 --disable-shared --enable-languages=c,c++
// Thread model: single
// Supported LTO compression algorithms: zlib


