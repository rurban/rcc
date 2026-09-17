/* -O1 dense switch dispatch through a code jump table. */
static int dense(int n) {
    switch (n) {
    case 10: return 100;
    case 11: return 101;
    case 13: return 103;
    case 15: return 105;
    default: return -1;
    }
}

static int fallthrough(int n) {
    int value = 0;
    switch (n) {
    case 20:
        value++;
        /* fall through */
    case 21:
        value += 2;
        break;
    case 22:
        value = 8;
        break;
    case 23:
        value = 9;
        break;
    default:
        value = -1;
    }
    return value;
}

static int no_default(int n) {
    int value = -7;
    switch (n) {
    case 0: value = 0; break;
    case 1: value = 1; break;
    case 2: value = 2; break;
    case 3: value = 3; break;
    }
    return value;
}

int main(void) {
    if (dense(9) != -1 || dense(10) != 100 || dense(11) != 101) return 1;
    if (dense(12) != -1 || dense(13) != 103 || dense(14) != -1 || dense(15) != 105) return 2;
    if (fallthrough(19) != -1 || fallthrough(20) != 3 || fallthrough(21) != 2) return 3;
    if (fallthrough(22) != 8 || fallthrough(23) != 9 || fallthrough(24) != -1) return 4;
    if (no_default(-1) != -7 || no_default(0) != 0 || no_default(3) != 3 || no_default(4) != -7) return 5;
    return 0;
}
