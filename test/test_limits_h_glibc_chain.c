/* Regression: glibc's <limits.h> does #include_next <limits.h> to find
 * GCC's own limits.h.  rcc defines __GNUC__ but formerly lacked
 * _GCC_LIMITS_H_, so that #include_next found nothing and every TU
 * touching <limits.h> (or any system header that pulls it in) failed.
 * Verify that <limits.h> compiles and its standard macros are usable. */
#include <limits.h>
#include <stdio.h>

int main(void) {
    /* ISO C minimums — must be present per C99 7.10 */
    (void)CHAR_BIT;
    (void)INT_MAX;
    (void)LLONG_MAX;

    /* If POSIX limits are available, just verify they are usable */
#ifdef _POSIX_PATH_MAX
    (void)_POSIX_PATH_MAX;
#endif
#ifdef PATH_MAX
    (void)PATH_MAX;
#endif
#ifdef PIPE_BUF
    (void)PIPE_BUF;
#endif

    printf("OK\n");
    return 0;
}
