// Test that __attribute__((__fallthrough__)) is accepted as a no-op statement.
// Must not produce "expected type name" or other parse errors.

int printf(const char *, ...);

static int fallthrough_test(int x) {
    int r = 0;
    switch (x) {
    case 1:
        __attribute__((__fallthrough__));
    case 2:
        r += 2;
        break;
    case 3:
        __attribute__((fallthrough));
    default:
        r += 4;
        break;
    }
    return r;
}

int main(void) {
    int r = 0;
    r += fallthrough_test(1); // falls through: r = 2
    r += fallthrough_test(2); // direct hit:  r = 2
    r += fallthrough_test(3); // falls through: r = 4
    r += fallthrough_test(4); // default:       r = 4
    printf("fallthrough: %d\n", r);
    return r == 12 ? 0 : 1;
}
