/* _mm_move_ss must be a pure bit-level lane merge: low lane from b,
 * upper three lanes from a, unchanged. SIMD code commonly reinterprets
 * plain integer data as __m128 purely to use this as a cheap blend
 * (real floating-point arithmetic never intended) -- so the "float"
 * values passed through it are frequently NaN-shaped bit patterns that
 * must survive completely unmodified.
 *
 * Regression: _mm_move_ss was implemented as
 * `(__m128){__b[0], __a[1], __a[2], __a[3]}`. rcc keeps scalar float
 * values in GP registers as double-precision bit patterns, so
 * extracting __a[1..3]/__b[0] into GP registers and rebuilding the
 * vector round-trips each lane through cvtss2sd/cvtsd2ss. Per the SSE
 * spec those conversions MUST quiet a signaling NaN (force the
 * mantissa's top bit to 1) and can shift its payload between single-
 * and double-precision's differently-positioned quiet bit -- silently
 * corrupting any lane holding a NaN-shaped bit pattern, even though
 * `_mm_move_ss` never performs real arithmetic on it.
 *
 * Found via a real PHP build: Zend/ext/hash/hash_sha_sse2.c's SHA256
 * SSE2 transform uses exactly this pattern (its SPAN_ONE_THREE macro)
 * to combine message-schedule words built from arbitrary hashed data.
 * Every hash()/hash_hmac() call whose message-schedule happened to
 * produce a NaN-shaped 32-bit word (unavoidable: hash input is
 * arbitrary bytes) silently corrupted that word, breaking the
 * resulting digest -- reproduced against real SHA-256 RFC/HMAC test
 * vectors, where rcc's hash("sha256", ...) diverged from the correct
 * value for specific multi-block messages.
 */
#include <stdio.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <xmmintrin.h>

static unsigned int lane_bits(__m128 v, int i)
{
    unsigned int out[4];
    _mm_storeu_ps((float *)out, v);
    return out[i];
}

int main(void)
{
    /* A signaling-NaN-shaped 32-bit pattern (exponent all-1s, nonzero
     * mantissa with the quiet bit clear): must pass through untouched. */
    unsigned int a_bits[4] = {0xc97fa5faU, 0x55e80ab6U, 0xffbffe4aU, 0x750a4d47U};
    unsigned int b_bits[4] = {0xb11b83e4U, 0x47f39f51U, 0x8cd005b2U, 0xb92e423dU};

    __m128 a = _mm_loadu_ps((const float *)a_bits);
    __m128 b = _mm_loadu_ps((const float *)b_bits);

    __m128 r = _mm_move_ss(a, b);

    unsigned int expect[4] = {b_bits[0], a_bits[1], a_bits[2], a_bits[3]};
    for (int i = 0; i < 4; i++) {
        unsigned int got = lane_bits(r, i);
        if (got != expect[i]) {
            printf("FAIL: lane %d = %08x, want %08x\n", i, got, expect[i]);
            return 1;
        }
    }

    printf("OK _mm_move_ss preserves NaN-shaped bit patterns\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
