/* Regression test: per-register spill-slot depth overflow (silent miscompile).
 *
 * codegen.c's per-register spill STACK (spill_slot[NUM_REGS][MAX_SPILL_DEPTH],
 * spill_depth[], push/pop_spill_slot) records one entry per outstanding
 * spill of a physical register. Several "fold the spilled value into the
 * operation" sites -- array dereference with a shared idx/base register,
 * binary-op chains (add/sub/and/xor/or/cmp/imul), shifts, float ops and
 * comparisons -- read the spilled value straight out of spill_offset(r)
 * and clear the spilled_regs bit WITHOUT popping the stack entry. Each
 * such fold leaks one depth level for that register.
 *
 * After MAX_SPILL_DEPTH (32) leaked levels, push_spill_slot() allocates a
 * fresh slot but can no longer record it, so the value is stored at the
 * new offset while spill_offset(r) still returns the stale top: the
 * reload reads a DIFFERENT (older) slot and the computation silently
 * diverges. Position-dependent -- the same source round computes correct
 * values in a 27-round function and wrong ones in a 28-round function,
 * because the extra round's own fold sites push the register past the
 * 32-slot ceiling mid-evaluation.
 *
 * Reproduced via GNU Emacs's gnulib lib/sha512.c (secure-hash sha384/
 * sha512 returned wrong digests); the R/M macro structure below is the
 * exact compression-round pattern, truncated to the 28 rounds needed to
 * cross the ceiling. Fixed by popping the consumed slot at every fold
 * site so outstanding depth stays bounded.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint64_t u64;

static u64 u64rol(u64 x, int n) { return (x << n) | (x >> (64 - n)); }

#define u64and(x, y) ((x) & (y))
#define u64or(x, y) ((x) | (y))
#define u64xor(x, y) ((x) ^ (y))
#define u64plus(x, y) ((x) + (y))
#define u64shr(x, n) ((x) >> (n))

#define S0(x) u64xor (u64rol (x, 63), u64xor (u64rol (x, 56), u64shr (x, 7)))
#define S1(x) u64xor (u64rol (x, 45), u64xor (u64rol (x, 3), u64shr (x, 6)))
#define SS0(x) u64xor (u64rol (x, 36), u64xor (u64rol (x, 30), u64rol (x, 25)))
#define SS1(x) u64xor (u64rol (x, 50), u64xor (u64rol (x, 46), u64rol (x, 23)))
#define F2(A, B, C) u64or (u64and (A, B), u64and (C, u64or (A, B)))
#define F1(E, F, G) u64xor (G, u64and (E, u64xor (F, G)))
#define M(I) (x[(I) & 15]                                                  \
             = u64plus (S1 (x[((I) - 2) & 15]),                            \
                        u64plus (x[((I) - 7) & 15],                        \
                                 u64plus (S0 (x[((I) - 15) & 15]),         \
                                          x[((I) - 16) & 15]))))
#define K(I) sha512_round_constants[I]
#define R(A, B, C, D, E, F, G, H, K, M)                                    \
  do                                                                       \
    {                                                                      \
      u64 t0 = u64plus (SS0 (A), F2 (A, B, C));                            \
      u64 t1 = u64plus (H, u64plus (SS1 (E), u64plus (F1 (E, F, G), u64plus (K, M)))); \
      D = u64plus (D, t1);                                                 \
      H = u64plus (t0, t1);                                                \
    }                                                                      \
  while (0)

static const u64 sha512_round_constants[28] = {
  0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
  0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
  0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
  0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
  0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
  0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
  0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL
};

int main(void) {
    u64 x[16];
    for (int i = 0; i < 16; i++) x[i] = 0;
    x[0] = 0x666f6f6261728000ULL;
    u64 a=0x6a09e667f3bcc908ULL,b=0xbb67ae8584caa73bULL,c=0x3c6ef372fe94f82bULL,d=0x5dd80e16dc279f91ULL,
        e=0x510e527fade682d1ULL,f=0x9b05688c2b3e6c1fULL,g=0x1f83d9abfb41bd6bULL,h=0xfbbcda9b1e6f5df5ULL;
    R( a, b, c, d, e, f, g, h, K( 0), x[ 0] );
    R( h, a, b, c, d, e, f, g, K( 1), x[ 1] );
    R( g, h, a, b, c, d, e, f, K( 2), x[ 2] );
    R( f, g, h, a, b, c, d, e, K( 3), x[ 3] );
    R( e, f, g, h, a, b, c, d, K( 4), x[ 4] );
    R( d, e, f, g, h, a, b, c, K( 5), x[ 5] );
    R( c, d, e, f, g, h, a, b, K( 6), x[ 6] );
    R( b, c, d, e, f, g, h, a, K( 7), x[ 7] );
    R( a, b, c, d, e, f, g, h, K( 8), x[ 8] );
    R( h, a, b, c, d, e, f, g, K( 9), x[ 9] );
    R( g, h, a, b, c, d, e, f, K(10), x[10] );
    R( f, g, h, a, b, c, d, e, K(11), x[11] );
    R( e, f, g, h, a, b, c, d, K(12), x[12] );
    R( d, e, f, g, h, a, b, c, K(13), x[13] );
    R( c, d, e, f, g, h, a, b, K(14), x[14] );
    R( b, c, d, e, f, g, h, a, K(15), x[15] );
    R( a, b, c, d, e, f, g, h, K(16), M(16) );
    R( h, a, b, c, d, e, f, g, K(17), M(17) );
    R( g, h, a, b, c, d, e, f, K(18), M(18) );
    R( f, g, h, a, b, c, d, e, K(19), M(19) );
    R( e, f, g, h, a, b, c, d, K(20), M(20) );
    R( d, e, f, g, h, a, b, c, K(21), M(21) );
    R( c, d, e, f, g, h, a, b, K(22), M(22) );
    R( b, c, d, e, f, g, h, a, K(23), M(23) );
    R( a, b, c, d, e, f, g, h, K(24), M(24) );
    R( h, a, b, c, d, e, f, g, K(25), M(25) );
    R( g, h, a, b, c, d, e, f, K(26), M(26) );
    R( f, g, h, a, b, c, d, e, K(27), M(27) );
    /* Expected values verified against gcc -O2 (28 rounds of "foobar"+0x80). */
    assert(a == 0x205063a363eefdf3ULL);
    assert(e == 0x2386566c8306515dULL);
    printf("ok\n");
    return 0;
}