/* Test builtin inline expansion: memset, memcpy, memcmp, strlen, strcmp, strchr */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int failures;

#define assert_eq(a, b, msg) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        printf("FAIL %s: got %lld, expected %lld\n", msg, _a, _b); \
        failures++; \
    } \
} while (0)

#define assert_ok(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        failures++; \
    } \
} while (0)

int main(void) {
    char buf[128], ref[128];

    /* --- memset --- */
    memset(buf, 0, sizeof(buf));
    for (int i = 0; i < 128; i++) assert_eq(buf[i], 0, "memset zero");

    memset(buf, 0xAA, 64);
    for (int i = 0; i < 64; i++) assert_eq((unsigned char)buf[i], 0xAA, "memset partial");
    assert_eq(buf[64], 0, "memset past end");

    char *r = memset(buf, 0xBB, 32);
    assert_eq(r, buf, "memset retval");

    /* --- memcpy --- */
    memset(ref, 'X', 128);
    memcpy(buf, ref, 64);
    for (int i = 0; i < 64; i++) assert_eq(buf[i], 'X', "memcpy fwd");
    assert_eq(buf[64], 0, "memcpy past end");

    r = memcpy(buf, ref, 32);
    assert_eq(r, buf, "memcpy retval");

    /* --- memcmp --- */
    memset(buf, 'A', 10);
    memset(ref, 'A', 10);
    assert_eq(memcmp(buf, ref, 10), 0, "memcmp eq");
    buf[5] = 'B';
    assert_ok(memcmp(buf, ref, 10) > 0, "memcmp gt");
    buf[5] = '@';
    assert_ok(memcmp(buf, ref, 10) < 0, "memcmp lt");
    assert_eq(memcmp(buf, ref, 0), 0, "memcmp zero");

    /* --- strlen --- */
    memcpy(buf, "hello, world!", 14);
    assert_eq(strlen(buf), 13, "strlen");
    assert_eq(strlen(""), 0, "strlen empty");
    assert_eq(strlen("x"), 1, "strlen single");
    for (int i = 0; i < 100; i++) buf[i] = 'X';
    buf[100] = 0;
    assert_eq(strlen(buf), 100, "strlen long");

    /* --- strcmp --- */
    assert_eq(strcmp("abc", "abc"), 0, "strcmp eq");
    assert_ok(strcmp("abc", "abb") > 0, "strcmp gt");
    assert_ok(strcmp("abc", "abd") < 0, "strcmp lt");
    assert_ok(strcmp("ab", "abc") < 0, "strcmp prefix");
    assert_ok(strcmp("abc", "ab") > 0, "strcmp longer");
    assert_eq(strcmp("", ""), 0, "strcmp empty");

    /* --- strchr --- */
    memcpy(buf, "hello world", 12);
    char *p = strchr(buf, 'w');
    assert_eq(p - buf, 6, "strchr find");
    p = strchr(buf, 'h');
    assert_eq(p - buf, 0, "strchr first");
    p = strchr(buf, 'd');
    assert_eq(p - buf, 10, "strchr last");
    p = strchr(buf, 'z');
    assert_eq((long long)p, 0, "strchr miss");
    p = strchr(buf, '\0');
    assert_eq(*p, 0, "strchr terminator");

    /* --- builtin aliases --- */
    memset(buf, 0, 128);
    __builtin_memcpy(buf, "ABCD", 5);
    assert_eq(buf[0], 'A', "alias memcpy");
    assert_eq(__builtin_strlen(buf), 4, "alias strlen");
    assert_eq(__builtin_memcmp(buf, "ABCD", 4), 0, "alias memcmp");
    r = __builtin_memset(buf, 0, 16);
    assert_eq(r, buf, "alias memset");
    assert_eq(__builtin_strcmp("xyz", "xyz"), 0, "alias strcmp");
    p = __builtin_strchr("abcaba", 'b');
    assert_eq(*p, 'b', "alias strchr");

    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL BUILTIN TESTS PASSED\n");
    return failures ? 1 : 0;
}
