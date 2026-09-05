/* GCC Bug #89051 - -Wno-error= does not work for warning groups
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89051
 */
/* { dg-do compile } */


void f() { g(1); }
// x.c: In function ‘f’:
// x.c:1:12: error: implicit declaration of function ‘g’ [-Werror=implicit-function-declaration]
// void f() { g(1); }
//             ^
// cc1: some warnings being treated as errors
// Using built-in specs.
// COLLECT_GCC=gcc
// COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-pc-linux-gnu/8.2.1/lto-wrapper
// Target: x86_64-pc-linux-gnu
// Configured with: /build/gcc/src/gcc/configure --prefix=/usr --libdir=/usr/lib --libexecdir=/usr/lib --mandir=/usr/share/man --infodir=/usr/share/info --with-bugurl=<a href="https://bugs.archlinux.org/">https://bugs.archlinux.org/</a> --enable-languages=c,c++,ada,fortran,go,lto,objc,obj-c++ --enable-shared --enable-threads=posix --enable-libmpx --with-system-zlib --with-isl --enable-__cxa_atexit --disable-libunwind-exceptions --enable-clocale=gnu --disable-libstdcxx-pch --disable-libssp --enable-gnu-unique-object --enable-linker-build-id --enable-lto --enable-plugin --enable-install-libiberty --with-linker-hash-style=gnu --enable-gnu-indirect-function --enable-multilib --disable-werror --enable-checking=release --enable-default-pie --enable-default-ssp --enable-cet=auto
// Thread model: posix
// -Wno-error=* should work like -Wno-* and disable errors for more specific groups of warnings.


