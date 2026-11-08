/* A ## paste result keeps the UNION of its operands' hide sets. BSD's ssp
 * <string.h> pastes `__builtin___ ## fun ## _chk` with `fun` = the function's
 * own name, coming out of that name's macro expansion; rcc's
 * `__builtin_X` -> `X` alias then reaches the same macro again. The pasted
 * token must still carry `fun` so that expansion stops: with the empty
 * (intersection) set it nested without end (NetBSD: parser stack overflow in
 * test_fortify).
 */
#include <stdio.h>

static int(memset2)(int d, int c, int l) { return d + c + l; }
#define SSP_CHK3(fun, d, c, l) __builtin___##fun##_chk(d, c, l, 0)
#define memset2(d, c, l) SSP_CHK3(memset2, d, c, l)
#define __builtin___memset2_chk(d, c, l, bos) memset2(d, c, l)

int main(void) {
    if (memset2(1, 2, 3) != 6) {
        printf("FAIL\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
