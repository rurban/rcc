// -fsanitize=undefined: adding a negative offset to a pointer as a standalone
// add must not trip the pointer-overflow check. test_ubsan_ptr_neg covers a
// subscript, which rcc folds into the load and does not check at all.
//
// Regression: the arm64 check still rode on the add's raw carry flag after the
// x86-64 one was fixed, so base + negative offset was always reported, and
// the handler was given a garbage result. A volatile offset keeps the add
// from being folded, so the check runs at -O0 and -O1.

int main(void) {
    static char buf[256];
    volatile long neg = -108;
    volatile long pos = 20;
    char *volatile base = buf + 200;
    char *p = base + neg; /* buf + 92: valid, must NOT report */
    char *q = base + pos; /* buf + 220 */
    return (p == buf + 92 && q == buf + 220) ? 0 : 1;
}
