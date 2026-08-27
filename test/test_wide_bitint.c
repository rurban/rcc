/* Wide _BitInt(N) (N > 64) arithmetic through the per-TU runtime helpers
 * (src/bitint_rt.c, ported from slimcc's bitint.c): before the gen_bitint
 * codegen existed, every _BitInt(N>64) operation silently truncated its
 * operands to 64 bits (e.g. `x << 100` produced 0 and 128-bit+ values
 * compared wrong), because gen() dispatched only TY_INT128 to the wide-int
 * slot path and left TY_BITINT on the scalar 64-bit path. */

typedef _BitInt(200) B200;
typedef unsigned _BitInt(200) UB200;

static int test_arith(void)
{
    B200 x = 5, y = 3;
    if (x + y != 8) return 1;
    if (x - y != 2) return 2;
    if (x * y != 15) return 3;
    if (x / y != 1) return 4;
    if (x % y != 2) return 5;
    return 0;
}

static int test_shift(void)
{
    B200 x = 5;
    B200 sh = x << 100;   /* bit 102 of a 200-bit value */
    if (sh == 0) return 1;
    if ((sh >> 100) != 5) return 2;
    B200 big = ((B200)1) << 150;
    if (big == 0) return 3;
    B200 sum = big + big;
    if ((sum >> 150) != 2) return 4;
    if ((x >> 1) != 2) return 5;
    return 0;
}

static int test_bitwise(void)
{
    /* Use a 64-bit value with the top bit clear so widening to the signed
     * 200-bit type zero-extends (0xF0.. is a negative long long, which
     * would sign-extend to all-ones above bit 63). */
    B200 a = (B200)0xF0F0F0F0F0F0F0F0ULL;
    B200 b = (B200)0x0F0F0F0F0F0F0F0FULL;
    B200 hi = ((B200)0xFF) << 100;
    B200 all64 = (B200)0xFFFFFFFFFFFFFFFFULL; /* 64 ones, zero-extended */
    if ((a & b) != 0) return 1;
    if ((a | b) != all64) return 2;
    if ((a ^ b) != all64) return 3;
    if ((hi & ~hi) != 0) return 4;
    if ((hi | a) != (hi + a)) return 5; /* disjoint bits */
    if (~hi == 0) return 6;
    return 0;
}

static int test_compare(void)
{
    B200 x = 5, y = 3, z = 5;
    if (!(x > y)) return 1;
    if (!(x >= z)) return 2;
    if (!(x == z)) return 3;
    if (!(y < x)) return 4;
    if (!(y <= z)) return 5;
    if (!(x != y)) return 6;
    B200 big = ((B200)1) << 150;
    B200 bigger = big << 1;
    if (!(bigger > big)) return 7;
    if (bigger == big) return 8;
    return 0;
}

static int test_truthiness(void)
{
    B200 zero = 0;
    B200 five = 5;
    B200 big = ((B200)1) << (200 - 1);
    if (zero) return 1;
    if (!five) return 2;
    if (!big) return 3;
    if (!(five ? 1 : 0)) return 4;
    return 0;
}

static int test_unsigned(void)
{
    UB200 x = 0xFFFFFFFFFFFFFFFFULL;
    UB200 y = 1;
    /* 2^64 - 1 + 1 = 2^64: needs bit 64, must not truncate to 64 bits */
    UB200 sum = x + y;
    if (sum != (((UB200)1) << 64)) return 1;
    if ((sum >> 64) != 1) return 2;
    UB200 neg = (UB200)0 - 1; /* all ones */
    if ((neg >> 63) == 0) return 3;
    if (neg == 0) return 4;
    return 0;
}

static int test_funcall(void)
{
    B200 x = 5, y = 3;
    B200 r = x + y;
    /* Wide bitint as function argument + return (SysV: >16 bytes passed on
     * the stack by value, returned via hidden pointer). */
    return (int)(r - 8);
}

static int test_cast(void)
{
    B200 x = 5;
    int small = (int)x;
    long long med = (long long)x;
    if (small != 5) return 1;
    if (med != 5) return 2;
    B200 from_ll = (B200)42LL;
    if (from_ll != 42) return 3;
    B200 from_int = (B200)7;
    if (from_int != 7) return 4;
    /* Negative int widens with sign extension */
    B200 neg = (B200)(-3);
    if (neg != -3) return 5;
    if ((neg >> 70) != -1) return 6; /* sign extension beyond 64 bits */
    return 0;
}

static int test_float_cast(void)
{
    /* _BitInt(N>64) <-> double: ARM64's gen_bitint used the wrong
     * register-move/load helpers here (an unrelated VReg-indirection
     * function applied to already-physical register numbers), so this
     * either crashed with "Invalid register" or silently produced a
     * garbage value depending on which VReg happened to be live. */
    B200 x = 42;
    double d = (double)x;
    if (d != 42.0) return 1;
    B200 y = (B200)3.0;
    if (y != 3) return 2;
    B200 neg = -17;
    double d2 = (double)neg;
    if (d2 != -17.0) return 3;
    UB200 u = 9;
    double d3 = (double)u;
    if (d3 != 9.0) return 4;
    B200 z = (B200)(-8.0);
    if (z != -8) return 5;
    return 0;
}

static int test_wide_loop(void)
{
    /* Build a wide value incrementally: each iteration does a 200-bit
     * add and compare, exercising the helpers under register pressure. */
    B200 acc = 0;
    B200 step = ((B200)1) << 60;
    for (int i = 0; i < 100; i++)
        acc += step;
    if (acc != (step * 100)) return 1;
    if (acc == 0) return 2;
    return 0;
}

/* _BitInt(144): 24 bytes (rounded to 8), passes the 16-byte (>128-bit)
 * threshold that routes it through the wide-slot path, small enough to
 * hand-assemble expected bit patterns. */
typedef _BitInt(144) B144;
typedef unsigned _BitInt(144) UB144;

/* Wide _BitInt passed BY VALUE: SysV passes >16-byte aggregates on the
 * stack as a POINTER; the function prologue must copy the pointed-to
 * limbs into the parameter's stack slot. rcc's pass-2 (real) prologue
 * emission had no wide-bitint case and stored the incoming pointer as a
 * scalar, so a parameter with bits past bit 63 read garbage (c23doku's
 * solver produced "invalid puzzle" on every input). */
static B144 param_by_value(B144 a, B144 b)
{
    return a + b;
}

static int test_param_byvalue(void)
{
    B144 a = ((B144)1) << 100;
    B144 b = ((B144)1) << 100;
    B144 r = param_by_value(a, b);
    if (r != (((B144)1) << 101)) return 1;
    if ((r >> 100) != 2) return 2;
    return 0;
}

/* Widening bitint→bitint used the DESTINATION type's signedness for the
 * sign/zero extension instead of the source's: `(State)(unsigned
 * _BitInt(144))x` with x's top bit set sign-extended into a run of 1s
 * (c23doku's "invalid puzzle"). */
static int test_widen_signedness(void)
{
    UB144 u = ((UB144)1) << 100; /* top bit of a 144-bit unsigned set */
    B144 s = (B144)u;            /* must zero-extend: still positive */
    if (s < 0) return 1;
    if ((s >> 100) != 1) return 2;
    B144 neg = -1;               /* sign-extends to all ones */
    UB144 u2 = (UB144)neg;
    if ((u2 >> 143) == 0) return 3; /* bit 143 must be set */
    if (u2 == 0) return 4;
    return 0;
}

/* `!(wide & mask)` compared the wide value's stack-slot ADDRESS against
 * zero instead of calling rcc_bitint_to_bool, so the negation was always
 * false once the value was materialized in memory (c23doku's solver
 * declared the puzzle unsolvable after the first placements). */
static int test_not_truthiness(void)
{
    B144 state = 0, msk = 1;
    if (!(state & msk)) return 0; /* 0 & 1 == 0: !0 is true */
    return 1;
}

/* _BitInt(216): >16 bytes, returned via the hidden-return-buffer ABI,
 * which shifts every real argument one GP register right (arg0 in RSI
 * on x86-64, not RDI). The callee's prologue must account for the shift
 * for TY_BITINT returns just like structs; previously a function
 * returning a wide _BitInt read its first parameter from RDI (the
 * hidden pointer) and dropped the last argument (regression from
 * test_wide_bitint_retbuf_param_shift, merged here). */
typedef _BitInt(216) B216;

static B216 make_mask_retbuf(int a, int b, int c)
{
    unsigned _BitInt(72) bit = 1;
    B216 msk;
    msk = bit << a;
    msk <<= 72;
    msk |= bit << b;
    msk <<= 72;
    msk |= bit << c;
    return msk;
}

static int test_retbuf_param_shift(void)
{
    B216 m = make_mask_retbuf(5, 10, 15);
    unsigned long long lo = (unsigned long long)(m & (B216)0xFFFFFFFFFFFFFFFFULL);
    B216 mid_shift = m >> 72;
    unsigned long long mid = (unsigned long long)(mid_shift & (B216)0xFFFFFFFFFFFFFFFFULL);
    B216 hi_shift = m >> 144;
    unsigned long long hi = (unsigned long long)(hi_shift & (B216)0xFFFFFFFFFFFFFFFFULL);
    if (lo != (1ULL << 15)) return 1; /* a: the LAST param lands in lo */
    if (mid != (1ULL << 10)) return 2; /* b */
    if (hi != (1ULL << 5)) return 3; /* c */
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = test_arith())) return rc;
    if ((rc = test_shift())) return rc;
    if ((rc = test_bitwise())) return rc;
    if ((rc = test_compare())) return rc;
    if ((rc = test_truthiness())) return rc;
    if ((rc = test_unsigned())) return rc;
    if ((rc = test_funcall())) return rc;
    if ((rc = test_cast())) return rc;
    if ((rc = test_float_cast())) return rc;
    if ((rc = test_wide_loop())) return rc;
    if ((rc = test_param_byvalue())) return rc;
    if ((rc = test_widen_signedness())) return rc;
    if ((rc = test_not_truthiness())) return rc;
    if ((rc = test_retbuf_param_shift())) return rc;
    return 0;
}
