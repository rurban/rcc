/* rcc's bundled <math.h> declared no ilogb()/ilogbf()/ilogbl() prototypes
 * at all, and defined neither FP_ILOGB0 nor FP_ILOGBNAN (the C99 7.12.6.5p2
 * special return values for an argument of 0 or NaN respectively) --
 * real glibc's <math.h> provides both. Blocks test/third_party's
 * test_chibischeme (lib/srfi/144/math.c calls `ilogb()` directly and
 * uses `FP_ILOGBNAN`/`FP_ILOGB0` as ordinary named constants). Fixed by
 * adding the three prototypes (double/float/long double forms, backed
 * by libm at link time like every other math.h function here) and the
 * two macros, matching glibc's own values on this target (both INT_MIN,
 * confirmed via a direct `gcc`-compiled probe). Guarded out on the mingw
 * target: mingw-w64's own toolchain provides neither a declaration nor a
 * linkable symbol for ilogb/ilogbf/ilogbl at all (confirmed: absent from
 * both its bundled math.h and every libm.a/libmsvcrt.a export list) --
 * math.h mirrors that gap under _WIN32 rather than declaring a function
 * that can never actually link.
 */
#include <math.h>
#include <limits.h>

int main(void)
{
#ifndef _WIN32
    if (FP_ILOGB0 != INT_MIN) return 1;
    if (FP_ILOGBNAN != INT_MIN) return 2;

    if (ilogb(8.0) != 3) return 3;   /* 8 = 1.0 * 2^3 */
    if (ilogb(1.0) != 0) return 4;
    if (ilogb(0.5) != -1) return 5;
    if (ilogbf(8.0f) != 3) return 6;
    if (ilogbl(8.0L) != 3) return 7;
#endif

    return 0;
}
