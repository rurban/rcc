// -fsanitize=undefined: pointer arithmetic with a NEGATIVE offset must not
// trip the pointer-overflow check. Compiled with -fsanitize=undefined and run
// with UBSAN_OPTIONS=halt_on_error=1 (see run_tests.c), any ubsan report
// halts with exit 1; this test expects exit 0.
//
// Regression: the check originally rode on the add's raw carry flag, which is
// set for EVERY base + negative-offset add whose result stays a perfectly
// valid address (high base + two's-complement offset carries out of bit 63).
// glibc's isprint() indexes its ctype table exactly like this
// (*__ctype_b_loc())[ch] with a negative ch, which is how FreeBSD 15's
// ssp <string.h> expansion made tomlc17's scankey test report a spurious
// "addition of unsigned offset ... overflowed" under -fsanitize=undefined
// where gcc reports nothing. gcc's semantics: fire only when the offset is
// non-negative and the result wrapped past the top (result <u base), or the
// offset is negative and the result wrapped past zero (result >u base).

#include <stdint.h>

int main(void) {
    char buf[256];
    volatile int neg = -108; // ch * 2 for ch = -54, as in the isprint case
    volatile char *p = buf + 200;
    volatile char c = p[neg]; /* valid address: must NOT report */
    p = buf + 8;
    c = p[neg];               /* buf - 100: still a valid mapped address, no report */
    (void)c;
    return 0;
}
