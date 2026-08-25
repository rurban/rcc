/* Regression: __int128 POST_INC was missing from gen_int128's switch,
 * causing "unsupported node kind" on any ++ on a 128-bit variable. */
#include <stdio.h>

int test_post_inc(void) {
    __int128 x = 0;
    __int128 old = x++;
    if (old != 0) return 1;
    if (x != 1) return 2;
    /* test that post-inc with larger values works */
    x = 100;
    old = x++;
    if (old != 100) return 3;
    if (x != 101) return 4;
    return 0;
}

int test_arithmetic(void) {
    __int128 a = 1000;
    __int128 b = 2000;
    __int128 c = a + b;
    if (c != 3000) return 1;
    c = b - a;
    if (c != 1000) return 2;
    c = a * b;
    if (c != 2000000) return 3;
    return 0;
}

int test_bitwise(void) {
    __int128 a = 0xFF;
    __int128 b = 0x0F;
    if ((a & b) != 0x0F) return 1;
    if ((a | b) != 0xFF) return 2;
    if ((a ^ b) != 0xF0) return 3;
    if ((a << 4) != 0xFF0) return 4;
    if ((a >> 4) != 0xF) return 5;
    return 0;
}

int test_shifts(void) {
    __int128 x = 1;
    x <<= 64;
    if (x != ((__int128)1 << 64)) return 1;
    x >>= 32;
    if (x != ((__int128)1 << 32)) return 2;
    return 0;
}

int test_comparison(void) {
    __int128 a = 100;
    __int128 b = 200;
    if (!(a < b)) return 1;
    if (!(b > a)) return 2;
    if (!(a <= b)) return 3;
    if (!(b >= a)) return 4;
    if (!(a <= a)) return 5;
    if (!(a == a)) return 6;
    if (!(a != b)) return 7;
    return 0;
}

int test_divmod(void) {
    __int128 a = 100;
    __int128 b = 7;
    if ((a / b) != 14) return 1;
    if ((a % b) != 2) return 2;
    return 0;
}

int test_neg(void) {
    __int128 a = 42;
    __int128 b = -a;
    if (b != -42) return 1;
    return 0;
}

int test_cast(void) {
    int x = 42;
    __int128 y = x;
    if (y != 42) return 1;
    unsigned int u = 100;
    __int128 v = u;
    if (v != 100) return 2;
    long long ll = 123456789012345LL;
    __int128 ll128 = ll;
    if (ll128 != 123456789012345LL) return 3;
    return 0;
}

int test_comma(void) {
    __int128 x;
    x = (1, 2);
    if (x != 2) return 1;
    return 0;
}

int test_cond(void) {
    int cond = 1;
    __int128 a = 10, b = 20;
    __int128 r = cond ? a : b;
    if (r != 10) return 1;
    cond = 0;
    r = cond ? a : b;
    if (r != 20) return 2;
    return 0;
}

/* Regression: bare `if (x)` on a __int128 VARIABLE, and `if (f())` on a
 * __int128-returning FUNCTION CALL, both tested the 16-byte stack slot's
 * ADDRESS instead of its stored value in gen_cond_branch_inv()'s generic
 * truthiness fallback (codegen.c) -- an address is never zero, so both
 * forms were unconditionally "truthy" regardless of the actual value. */
__attribute__((noinline)) int returns_eq128(__int128 a, __int128 b) {
    return a == b;
}

int test_truthiness_var(void) {
    __int128 x = 0;
    if (x) return 1; /* zero must be falsy */
    x = 5;
    if (!x) return 2; /* nonzero must be truthy */
    x = -1; /* all bits set, both halves nonzero */
    if (!x) return 3;
    return 0;
}

int test_truthiness_funcall(void) {
    __int128 a = 5, b = 5;
    __int128 c = 6;
    if (!(a == b)) return 1; /* sanity: the comparison itself is correct */
    if (!(a == b ? 1 : 0)) return 2;
    /* direct `if` on the funcall result (int, not __int128, sanity check) */
    if (!returns_eq128(a, b)) return 3;
    if (returns_eq128(a, c)) return 4;
    return 0;
}

/* Regression: `f(a,b)` returning __int128 (e.g. the boolean result of an
 * internal comparison, sign-extended to 128 bits) used directly as a
 * condition -- `assert(f(a,b))`-shaped ternaries with void branches, and
 * `int r = f(a,b) ? 1 : 0`-shaped value ternaries -- both misbehaved:
 * the generic value-ternary path in gen() passed the __int128 operand's
 * size (16) to asm_cmp_zero(), a width it silently mishandles (no size-16
 * case in rex_for_size()), so the emitted `cmp` dropped the REX.B bit
 * needed to address a VReg mapped to r8-r15 and instead tested an
 * unrelated physical register (whatever aliased the low 3 ModRM bits --
 * here %rdx, which happened to hold the just-returned hi word). On top of
 * the wrong size, it was also testing the int128 slot's ADDRESS rather
 * than its value, same as the plain `if`/funcall bug above. */
__attribute__((noinline)) __int128 eq128(__int128 a, __int128 b) {
    return a == b;
}

int test_truthiness_int128_funcall_cond(void) {
    __int128 a = ((__int128)5 << 64) + 3;
    __int128 b = ((__int128)5 << 64) + 3;
    __int128 c = ((__int128)5 << 64) + 4;
    int aborted = 0;

    /* void-branch ternary, exactly the `assert(eq128(a,b))` shape. */
    (eq128(a, b) ? (void)0 : (void)(aborted = 1));
    if (aborted) return 1;
    (eq128(a, c) ? (void)0 : (void)(aborted = 1));
    if (!aborted) return 2;

    /* int-typed value ternary. */
    int r = eq128(a, b) ? 1 : 0;
    if (r != 1) return 3;
    r = eq128(a, c) ? 1 : 0;
    if (r != 0) return 4;
    return 0;
}

/* Regression: a __int128-RESULT ternary (gen_int128()'s own ND_COND case)
 * whose CONDITION is itself __int128-typed (e.g. nested inside another
 * int128 expression) hit the same address/size bug as above, just inside
 * gen_int128() instead of gen(). */
int test_truthiness_nested_int128_cond(void) {
    __int128 a = 10, b = 20;
    __int128 cond_true = 1, cond_false = 0;
    __int128 r = cond_true ? a : b;
    if (r != 10) return 1;
    r = cond_false ? a : b;
    if (r != 20) return 2;
    /* condition itself a funcall returning __int128 */
    r = eq128(a, a) ? a : b;
    if (r != 10) return 3;
    r = eq128(a, b) ? a : b;
    if (r != 20) return 4;
    return 0;
}

int main(void) {
    int failures = 0;
    printf("test_post_inc: %s\n", test_post_inc() == 0 ? "PASS" : "FAIL");
    if (test_post_inc()) failures++;
    printf("test_arithmetic: %s\n", test_arithmetic() == 0 ? "PASS" : "FAIL");
    if (test_arithmetic()) failures++;
    printf("test_bitwise: %s\n", test_bitwise() == 0 ? "PASS" : "FAIL");
    if (test_bitwise()) failures++;
    printf("test_shifts: %s\n", test_shifts() == 0 ? "PASS" : "FAIL");
    if (test_shifts()) failures++;
    printf("test_comparison: %s\n", test_comparison() == 0 ? "PASS" : "FAIL");
    if (test_comparison()) failures++;
    printf("test_divmod: %s\n", test_divmod() == 0 ? "PASS" : "FAIL");
    if (test_divmod()) failures++;
    printf("test_neg: %s\n", test_neg() == 0 ? "PASS" : "FAIL");
    if (test_neg()) failures++;
    printf("test_cast: %s\n", test_cast() == 0 ? "PASS" : "FAIL");
    if (test_cast()) failures++;
    printf("test_comma: %s\n", test_comma() == 0 ? "PASS" : "FAIL");
    if (test_comma()) failures++;
    printf("test_cond: %s\n", test_cond() == 0 ? "PASS" : "FAIL");
    if (test_cond()) failures++;
    printf("test_truthiness_var: %s\n", test_truthiness_var() == 0 ? "PASS" : "FAIL");
    if (test_truthiness_var()) failures++;
    printf("test_truthiness_funcall: %s\n", test_truthiness_funcall() == 0 ? "PASS" : "FAIL");
    if (test_truthiness_funcall()) failures++;
    printf("test_truthiness_int128_funcall_cond: %s\n", test_truthiness_int128_funcall_cond() == 0 ? "PASS" : "FAIL");
    if (test_truthiness_int128_funcall_cond()) failures++;
    printf("test_truthiness_nested_int128_cond: %s\n", test_truthiness_nested_int128_cond() == 0 ? "PASS" : "FAIL");
    if (test_truthiness_nested_int128_cond()) failures++;
    printf("\n%d failures\n", failures);
    return failures;
}
