/* Negating an unsigned constant expression in an array-size context must
 * wrap around at the operand's OWN width, not always 64 bits.
 *
 * Regression: eval_const_expr()'s ND_NEG case correctly detected an
 * unsigned operand and used unsigned wraparound arithmetic, but always
 * performed it at 64-bit (unsigned long long) width regardless of the
 * operand's actual type. `-(uint32_t)-2` should fold to 2 (32-bit
 * wraparound: 0 - 0xfffffffe == 2, mod 2^32), but rcc computed
 * `-(unsigned long long)0xfffffffe` instead -- a 64-bit wraparound
 * giving a huge value -- which then failed the "array size must not be
 * negative" check (the huge unsigned value read back as negative once
 * narrowed to a signed 64-bit comparison), rejecting a perfectly valid
 * array declaration.
 *
 * Found via a real PHP build: Zend/zend_types.h's
 *   #define HT_MIN_MASK ((uint32_t) -2)
 * used by ext/opcache/ZendAccelerator.h as
 *   uint32_t uninitialized_bucket[-HT_MIN_MASK];
 * (a 2-element array), which rcc rejected as "size of array
 * 'uninitialized_bucket' is negative".
 */
#include <stdint.h>

#define HT_MIN_MASK ((uint32_t) -2)

/* The exact PHP shape: a 2-element array sized by negating a uint32_t
 * constant. */
uint32_t uninitialized_bucket[-HT_MIN_MASK];

/* A wider width too, to pin down the wraparound point precisely rather
 * than just "no longer errors": unsigned long is already 64 bits, so
 * its negation needs no extra masking (this path already worked). */
unsigned long arr_ulong[-((unsigned long)-3)];

/* Regression guard the other way: a narrower-than-int unsigned type
 * (uint16_t) instead PROMOTES to plain (signed) int before the
 * negation (C11 6.3.1.1p2) -- so `-((uint16_t)-4)` is `-(int)65532`
 * == -65532, genuinely negative, NOT a 16-bit wraparound to 4. Confirms
 * the fix's 32/64-bit-wraparound branch didn't overshoot into masking
 * promoted-to-int operands too.
 */
_Static_assert(-((uint16_t)-4) == -65532,
               "uint16_t negation must promote to int, not wrap at 16 bits");

int main(void) {
    if (sizeof(uninitialized_bucket) / sizeof(uninitialized_bucket[0]) != 2)
        return 1;
    if (sizeof(arr_ulong) / sizeof(arr_ulong[0]) != 3)
        return 2;
    return 0;
}
