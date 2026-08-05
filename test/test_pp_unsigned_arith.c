/* Preprocessor #if arithmetic on constants that exceed INTMAX_MAX must be
 * unsigned (C23 6.4.4.1 / 6.10.1): a hex/octal or over-large decimal
 * constant is unsigned, and /, % and the relational operators then use
 * uintmax_t. rcc used to evaluate 0xFFFFFFFFFFFFFFFF as the signed value
 * -1, so `#if SIZE_MAX % UINT_MAX` (0 mathematically) came out non-zero
 * and tripped lmdb's two's-complement sanity #if in mdb.c. Signed
 * arithmetic (no unsigned operand) must stay signed. */
#include <stdint.h>
#include <limits.h>
#include <stdio.h>

/* --- unsigned promotion of over-large constants --- */
#if !(0xFFFFFFFFFFFFFFFF > 0)
#error "over-large hex constant not treated as unsigned"
#endif
#if !(18446744073709551615 > 0)
#error "over-large decimal constant not treated as unsigned"
#endif

/* --- unsigned 64-bit division / modulo --- */
#if (0xFFFFFFFFFFFFFFFF % 0xFFFFFFFF) != 0
#error "unsigned 64-bit modulo wrong"
#endif
#if (0xFFFFFFFFFFFFFFFF / 0xFFFFFFFF) != 0x100000001
#error "unsigned 64-bit division wrong"
#endif

/* --- the exact term from lmdb's mdb.c --- */
#if SIZE_MAX % UINT_MAX
#error "SIZE_MAX % UINT_MAX must be 0"
#endif

/* --- signed arithmetic must remain signed when no operand is unsigned --- */
#if (-7 / 2) != -3
#error "signed division regressed"
#endif
#if (-7 % 2) != -1
#error "signed modulo regressed"
#endif
#if (-6 & 5) != 0
#error "signed bitwise-and regressed"
#endif

int main(void)
{
    printf("OK\n");
    return 0;
}
