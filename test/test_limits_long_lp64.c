/* rcc's bundled include/limits.h hardcoded LONG_MIN/LONG_MAX/ULONG_MAX
 * to their 32-bit (ILP32/LLP64) values UNCONDITIONALLY, even though the
 * very same file's C23 LONG_WIDTH/ULONG_WIDTH macros a few lines below
 * were already correctly hardcoded to 64 (an internal inconsistency:
 * whoever added the C23 width macros got the LP64 value right, but the
 * traditional LONG_MIN/LONG_MAX/ULONG_MAX macros right above them were
 * never updated to match). On Linux/macOS x86-64 and ARM64 (LP64,
 * `long` is 8 bytes), this is simply wrong: LONG_MAX read back as
 * 2147483647 instead of the true 9223372036854775807.
 *
 * A quick standalone printf of LONG_MAX from `main()` doesn't actually
 * catch this, though: this header ends with `#include_next <limits.h>`
 * to chain onward to glibc's real header (see test_limits_chain.c),
 * and glibc's own definitions DO eventually redefine LONG_MAX
 * correctly -- so by the time `main()`'s own code reads LONG_MAX, it's
 * already right. The bug only becomes externally visible when some
 * OTHER header further down the very same include chain computes
 * something FROM LONG_MAX's value at the point it's reached -- before
 * glibc's own correct redefinition -- and bakes in the wrong answer
 * permanently. glibc's own <bits/xopen_lim.h> (pulled in by
 * _XOPEN_SOURCE_EXTENDED, itself pulled in by, e.g., CPython's own
 * Python.h) does exactly this for `LONG_BIT`:
 *   #ifdef LONG_MAX
 *   # if LONG_MAX == 2147483647
 *   #  define LONG_BIT 32
 *   # else
 *   #  define LONG_BIT 64
 *   # endif
 * With the bug, LONG_BIT bakes in 32 despite LONG_MAX itself later
 * reading back correctly as 64-bit -- and CPython's own pyport.h
 * cross-checks `#if LONG_BIT != 8 * SIZEOF_LONG` and refuses to
 * compile rather than silently miscompile, which is how this was
 * found (via samba's waf `pyembed` configure probe, `#include
 * <Python.h>`). */
#define _XOPEN_SOURCE_EXTENDED 1
#include <limits.h>
#include <stdio.h>

int main(void) {
#ifdef _WIN32
    /* Win64 LLP64: `long` is 4 bytes even in a 64-bit build. */
    if (LONG_MAX != 2147483647L) return 1;
    if (LONG_MIN != (-2147483647L - 1L)) return 2;
    if (ULONG_MAX != 0xffffffffUL) return 3;
#else
    /* Linux/macOS LP64: `long` is 8 bytes. */
    if (LONG_MAX != 9223372036854775807L) return 1;
    if (LONG_MIN != (-9223372036854775807L - 1L)) return 2;
    if (ULONG_MAX != 0xffffffffffffffffUL) return 3;
#endif
    if ((unsigned long)sizeof(long) * 8 != (unsigned long)(LONG_MAX == 2147483647L ? 32 : 64))
        return 4;

    /* The actual regression: glibc's <bits/xopen_lim.h> (reached via
     * _XOPEN_SOURCE_EXTENDED above) must see the correct, final
     * LONG_MAX -- not a stale 32-bit value from earlier in the same
     * include chain -- when it derives LONG_BIT. */
#if !defined(_WIN32) && defined(LONG_BIT)
    if (LONG_BIT != 64)
        return 5;
    if (LONG_BIT != 8 * (int)sizeof(long))
        return 6;
#endif

    printf("OK\n");
    return 0;
}
