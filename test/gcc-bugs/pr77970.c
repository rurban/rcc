/* GCC Bug #77970 - inconsistent and unhelpful -Wformat warning for %lc
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77970
 */


typedef __WCHAR_TYPE__ wchar_t;

void f (void)
{
  __builtin_printf ("%lc", 0);
  __builtin_printf ("%lc", L'a');
}
// + for t in i386-rehat-linux sparc-sun-solaris2.12 x86_64-unknown-solaris2.10 bfin-uclinux m68k-linux hppa-unknown-linux-gnu
// + /build/sysroot/i386-rehat-linux/bin/i386-rehat-linux-gcc -S -Wall -o/dev/null a.c
// a.c: In function 'f':
// a.c:6:24: warning: format '%lc' expects argument of type 'wint_t', but argument 2 has type 'long int' [-Wformat=]
   __builtin_printf ("%lc", L'a');
//                       ~~^
//                       %ld
// + for t in i386-rehat-linux sparc-sun-solaris2.12 x86_64-unknown-solaris2.10 bfin-uclinux m68k-linux hppa-unknown-linux-gnu
// + /build/sysroot/sparc-sun-solaris2.12/bin/sparc-sun-solaris2.12-gcc -S -Wall -o/dev/null a.c
// a.c: In function 'f':
// a.c:5:24: warning: format '%lc' expects argument of type 'wint_t', but argument 2 has type 'int' [-Wformat=]
   __builtin_printf ("%lc", 0);
//                       ~~^
//                       %c
// + for t in i386-rehat-linux sparc-sun-solaris2.12 x86_64-unknown-solaris2.10 bfin-uclinux m68k-linux hppa-unknown-linux-gnu
// + /build/sysroot/x86_64-unknown-solaris2.10/bin/x86_64-unknown-solaris2.10-gcc -S -Wall -o/dev/null a.c
// + for t in i386-rehat-linux sparc-sun-solaris2.12 x86_64-unknown-solaris2.10 bfin-uclinux m68k-linux hppa-unknown-linux-gnu
// + /build/sysroot/bfin-uclinux/bin/bfin-uclinux-gcc -S -Wall -o/dev/null a.c
// + for t in i386-rehat-linux sparc-sun-solaris2.12 x86_64-unknown-solaris2.10 bfin-uclinux m68k-linux hppa-unknown-linux-gnu
// + /build/sysroot/m68k-linux/bin/m68k-linux-gcc -S -Wall -o/dev/null a.c
// a.c: In function 'f':
// a.c:6:24: warning: format '%lc' expects argument of type 'wint_t', but argument 2 has type 'long int' [-Wformat=]
   __builtin_printf ("%lc", L'a');
//                       ~~^
//                       %ld
// + for t in i386-rehat-linux sparc-sun-solaris2.12 x86_64-unknown-solaris2.10 bfin-uclinux m68k-linux hppa-unknown-linux-gnu
// + /build/sysroot/hppa-unknown-linux-gnu/bin/hppa-unknown-linux-gnu-gcc -S -Wall -o/dev/null a.c
// a.c: In function 'f':
// a.c:6:24: warning: format '%lc' expects argument of type 'wint_t', but argument 2 has type 'long int' [-Wformat=]
   __builtin_printf ("%lc", L'a');
//                       ~~^
//                       %ld


