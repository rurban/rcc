/* Regression test: __DATE__ and __TIME__ predefined macros.
 *
 * C89 requires these to expand to string literals with the compilation
 * date ("Mmm dd yyyy") and time ("hh:mm:ss").  rcc was missing both;
 * every project using them (mimalloc via options.c:236) failed with
 * 'undeclared variable'.
 */

#include <stdio.h>
#include <string.h>

int main(void) {
    const char *d = __DATE__;
    const char *t = __TIME__;
    int failures = 0;

    /* __DATE__ format: "Mmm dd yyyy" (11 chars) */
    if (strlen(d) != 11) {
        printf("FAIL: __DATE__ length: expected 11, got %zu (%s)\n", strlen(d), d);
        failures++;
    }
    if (!(d[0] >= 'A' && d[0] <= 'Z')) {
        printf("FAIL: __DATE__ doesn't start with month: %s\n", d);
        failures++;
    }
    if (d[3] != ' ') {
        printf("FAIL: __DATE__ day separator not space: %s\n", d);
        failures++;
    }
    if (!(d[4] == ' ' || (d[4] >= '0' && d[4] <= '3'))) {
        printf("FAIL: __DATE__ bad day: %s\n", d);
        failures++;
    }

    /* __TIME__ format: "hh:mm:ss" (8 chars) */
    if (strlen(t) != 8) {
        printf("FAIL: __TIME__ length: expected 8, got %zu (%s)\n", strlen(t), t);
        failures++;
    }
    if (t[2] != ':' || t[5] != ':') {
        printf("FAIL: __TIME__ bad format: %s\n", t);
        failures++;
    }

    if (failures)
        printf("%d FAILURES\n", failures);
    else
        printf("ALL DATE/TIME TESTS PASSED\n");
    return failures ? 1 : 0;
}
