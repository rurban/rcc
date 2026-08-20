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
    printf("\n%d failures\n", failures);
    return failures;
}
