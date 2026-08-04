/* GCC Bug #3481 - function attributes should apply to function pointers too
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=3481
 */


extern void (*error)(int, const char *, ...)
    __attribute__((noreturn))
    __attribute__((format(printf, 2, 3)));
// t.c:3: argument format specified for non-function `error'
Release:
// 3.0 (Debian)
Environment:
// System: Debian GNU/Linux
// Architecture: i386
// host: i386-linux
// build: i386-linux
// target: i386-linux
// configured with: ../src/configure -v --enable-languages=c,c++,java,f77,proto,objc --prefix=/usr --infodir=/share/info --mandir=/share/man --enable-shared --with-gnu-as --with-gnu-ld --with-system-zlib --enable-long-long --enable-nls --without-included-gettext --disable-checking --enable-threads=posix --enable-java-gc=boehm --with-cpp-install-dir=bin --enable-objc-gc i386-linux


