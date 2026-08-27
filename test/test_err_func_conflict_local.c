/* Regression test: a block-scope function prototype conflicting with the
 * file-scope declaration must be diagnosed ("conflicting types"), not
 * silently accepted.
 *
 * The file-scope redeclaration path checked type compatibility, but a
 * LOCAL (block-scope) function declaration never compared against the
 * file-scope symbol. gnulib's ioctl POSIX-signature configure probe
 * declares `int ioctl (int, int, ...);` inside main() to test whether it
 * conflicts with glibc's `int ioctl(int, unsigned long, ...)`; rcc
 * silently accepted it, so gnutls' configure concluded the POSIX
 * signature holds, set REPLACE_IOCTL=0 and its generated sys/ioctl.h
 * took the SYS branch (redeclaring ioctl with `int request`), failing
 * the build with "conflicting types for 'ioctl'" at src/gl/tests
 * vma-iter.o. gcc errors on the probe, REPLACE_IOCTL=1, and the
 * rpl_ioctl path is taken instead.
 *
 * test_err-style: this file must FAIL to compile. run_tests compiles
 * test_err_*.c expecting a compile error (see the harness's
 * "compile error" classification).
 */
#include <sys/ioctl.h>

int g(int);

int main(void) {
    int ioctl (int, int, ...);   /* conflicts with glibc's unsigned long */
    int h(int);
    int h(unsigned);             /* local-vs-local conflict */
    int g(unsigned);             /* local-vs-global conflict */
    return 0;
}
