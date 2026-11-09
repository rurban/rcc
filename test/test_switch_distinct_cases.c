/* The duplicate-case check must not reject valid switches: adjacent values,
 * touching but non-overlapping GNU ranges, negative labels, labels in nested
 * switches that repeat the outer switch's values, and a large generated
 * switch (the check is a sort + scan, not quadratic).
 */
#include <stdio.h>

#define C8(n) case n: return n + 1; case n + 1: return n + 2; case n + 2: return n + 3; case n + 3: return n + 4; \
              case n + 4: return n + 5; case n + 5: return n + 6; case n + 6: return n + 7; case n + 7: return n + 8;
#define C64(n) C8(n) C8(n + 8) C8(n + 16) C8(n + 24) C8(n + 32) C8(n + 40) C8(n + 48) C8(n + 56)
#define C512(n) C64(n) C64(n + 64) C64(n + 128) C64(n + 192) C64(n + 256) C64(n + 320) C64(n + 384) C64(n + 448)

static int big(int x) {
    switch (x) {
        C512(1000)
        C512(1512)
        C512(2024)
        C512(2536)
    default:
        return -1;
    }
}

static int small(int x, int y) {
    switch (x) {
    case -3:
        return 1;
    case -2 ... -1:
        return 2;
    case 0:
        return 3;
    case 1 ... 5:
        switch (y) { /* same values as the outer switch: separate scope */
        case 0:
            return 40;
        case 1 ... 5:
            return 50;
        }
        return 4;
    case 6 ... 9:
        return 5;
    case 10:
        return 6;
    }
    return 0;
}

int main(void) {
    if (big(1000) != 1001 || big(3047) != 3048 || big(2535) != 2536 || big(999) != -1 || big(3048) != -1) {
        printf("FAIL: big\n");
        return 1;
    }
    if (small(-3, 0) != 1 || small(-1, 0) != 2 || small(0, 0) != 3 || small(5, 0) != 40 || small(5, 3) != 50 ||
        small(5, 9) != 4 || small(9, 0) != 5 || small(10, 0) != 6 || small(11, 0) != 0) {
        printf("FAIL: small\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
