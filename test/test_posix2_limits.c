/* rcc's bundled <limits.h> must chain to the platform's real limits.h
 * for the feature-macro-guarded content it doesn't track itself: glibc's
 * _POSIX2_* / _XOPEN_* limits (bits/posix2_lim.h, bits/xopen_lim.h),
 * which getconf-style code reads. Without the include_next, getconf.c
 * failed with "undeclared variable _POSIX2_BC_BASE_MAX".
 */
#include <limits.h>

int main(void) {
#if defined(__linux__)
    return (_POSIX2_BC_BASE_MAX == 99 && _POSIX2_LINE_MAX == 2048) ? 0 : 1;
#else
    return 0;
#endif
}
