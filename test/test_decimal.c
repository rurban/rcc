/* _Decimal32/64/128 (IEEE 754-2008 decimal floating point, BID encoding).
 *
 * Before the decimal codegen existed, _Decimal32/64/128 were aliased to
 * float/double/long double: literals were converted to binary doubles at
 * lex time, so e.g. 0.1dd lost its exact decimal representation and
 * decimal arithmetic used binary FP. Now they are real types whose values
 * are IEEE 754-2008 BID bit patterns, every operation goes through the
 * bundled libbid runtime (lib/libdfp.a, __bid_*3/__bid_*2 calls), and
 * literals are folded exactly at compile time.
 *
 * All tests are pure decimal arithmetic/compare/cast — no printf of
 * decimal values (glibc's %Hf/%Df/%DDf hooks are not wired).
 */

typedef _Decimal32 D32;
typedef _Decimal64 D64;
typedef _Decimal128 D128;

static D64 add64(D64 a, D64 b) { return a + b; }
static D32 add32(D32 a, D32 b) { return a + b; }
static D128 add128(D128 a, D128 b) { return a + b; }
static D128 mul128(D128 a, D128 b) { return a * b; }

static int test_basics(void)
{
    D64 a = 1.25dd, b = 3.75dd;
    if (a + b != 5.0dd) return 1;
    if (b - a != 2.5dd) return 2;
    if (a * b != 4.6875dd) return 3;
    if (b / a != 3.0dd) return 4;
    return 0;
}

static int test_decimal32(void)
{
    D32 a = 1.5df, b = 2.25df;
    if (a + b != 3.75df) return 1;
    if (a * b != 3.375df) return 2;
    if (b / a != 1.5df) return 3;
    if (a > 1.4df && a < 1.6df) return 0;
    return 4;
}

static int test_decimal128(void)
{
    D128 c = 1.5dl, d = 2.5dl;
    if (c + d != 4.0dl) return 1;
    if (c * d != 3.75dl) return 2;
    if (3.75dl / 1.5dl != 2.5dl) return 3;
    if (mul128(1.5dl, 2.5dl) != 3.75dl) return 4;
    return 0;
}

static int test_compare(void)
{
    D64 x = 5.0dd, y = 3.0dd;
    if (!(x > y)) return 1;
    if (!(x >= 5.0dd)) return 2;
    if (!(x == 5.0dd)) return 3;
    if (!(y < x)) return 4;
    if (!(y <= 3.0dd)) return 5;
    if (!(x != y)) return 6;
    return 0;
}

static int test_casts(void)
{
    /* decimal -> int truncates toward zero */
    D64 d = 3.75dd;
    if ((long)d != 3) return 1;
    if ((int)(-3.75dd) != -3) return 2;
    /* int -> decimal */
    D64 e = 7;
    if (e != 7.0dd) return 3;
    D128 f = 42;
    if (f != 42.0dl) return 4;
    /* decimal <-> binary float */
    double g = (double)d;
    if (g != 3.75) return 5;
    D64 h = (D64)2.5;
    if (h != 2.5dd) return 6;
    /* decimal size changes */
    D128 i = (D128)d;
    if (i != 3.75dl) return 7;
    D32 j = (D32)d;
    if (j != 3.75df) return 8;
    return 0;
}

static int test_neg(void)
{
    D64 x = 1.25dd;
    if (-x != -1.25dd) return 1;
    if (-(-x) != x) return 2;
    D128 y = 2.5dl;
    if (-y != -2.5dl) return 3;
    return 0;
}

static int test_funcall(void)
{
    /* decimal32/64 pass in GP regs; decimal128 in 2 GP regs */
    if (add64(1.25dd, 3.75dd) != 5.0dd) return 1;
    if (add32(1.5df, 2.25df) != 3.75df) return 2;
    if (add128(1.5dl, 2.5dl) != 4.0dl) return 3;
    if (mul128(1.5dl, 2.5dl) != 3.75dl) return 4;
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = test_basics())) return rc;
    if ((rc = test_decimal32())) return 100 + rc;
    if ((rc = test_decimal128())) return 200 + rc;
    if ((rc = test_compare())) return 300 + rc;
    if ((rc = test_casts())) return 400 + rc;
    if ((rc = test_neg())) return 500 + rc;
    if ((rc = test_funcall())) return 600 + rc;
    return 0;
}
