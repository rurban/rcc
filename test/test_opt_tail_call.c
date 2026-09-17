/* -O2 direct scalar tail calls restore the caller frame then jump. */
#ifdef RCC_TAIL_STRESS
#define COUNTDOWN_DEPTH 200000
#else
#define COUNTDOWN_DEPTH 40
#endif

static int countdown(int n) {
    if (n == 0)
        return 0;
    return countdown(n - 1);
}

static int count_up(int n, int limit) {
    if (n == limit)
        return n;
    return count_up(n + 1, limit);
}

int main(void) {
    if (countdown(COUNTDOWN_DEPTH) != 0) return 1;
    if (count_up(-10, 40) != 40) return 2;
    return 0;
}
