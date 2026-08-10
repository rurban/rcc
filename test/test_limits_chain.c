/* rcc's bundled include/limits.h used to shadow glibc's real <limits.h>
 * completely: it only defined the ISO C minimums (INT_MAX, ...) and
 * never chained onward, so POSIX/XSI limits that real packages rely on --
 * SSIZE_MAX, PIPE_BUF, NL_ARGMAX, and anything gated on __WORDSIZE or
 * __USE_POSIX in <bits/posix1_lim.h> -- were missing. lmdb's mdb.c
 * failed on SSIZE_MAX. The header now ends with
 * `#ifndef _LIBC_LIMITS_H_ / #include_next <limits.h> / #endif` (the same
 * pattern as GCC's own fixed-include limits.h), so the platform limits
 * are still visible. */
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    /* ISO minimums must keep working (and keep their exact values). */
    if (CHAR_BIT != 8) return 1;
    if (INT_MAX != 2147483647) return 2;
    if (UINT_MAX != 0xffffffffU) return 3;

    /* SSIZE_MAX is now defined by rcc's own limits.h (before the
     * #include_next chain). __WORDSIZE is glibc-internal and only
     * reachable through the platform chain. */
#if !defined(SSIZE_MAX)
    return 4;
#endif
#if __SIZEOF_POINTER__ == 8
    if (SSIZE_MAX != 9223372036854775807LL) return 5;
#else
    if (SSIZE_MAX != 2147483647) return 5;
#endif

#ifdef __GLIBC__
#if !defined(__WORDSIZE)
    return 6;
#endif
#if __WORDSIZE != 64
    return 7;
#endif
#endif

    /* C23 width macros from this header must still be there. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    if (INT_WIDTH != 32) return 8;
    /* `long` is 32-bit under Windows' LLP64 data model (unlike LP64
     * Linux/macOS, where it's 64-bit) -- LONG_WIDTH must track that,
     * not assume LP64 universally. */
#ifdef _WIN32
    if (LONG_WIDTH != 32) return 9;
#else
    if (LONG_WIDTH != 64) return 9;
#endif
#endif

    printf("OK\n");
    return 0;
}
