// -fsanitize=undefined: shift out-of-range (ND_SHL count >= bits). Compiled
// with -fsanitize=undefined (see extra_ldflags in run_tests.c) and run with
// UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c), the gcc
// libubsan runtime reports the fault and halts with a deterministic exit 1
// (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_shift_out_of_bounds from libubsan, so a broken
// -fsanitize link fails this test at compile time.
//
// The shift count is a volatile so the shift is a genuine runtime op the
// framework's -O1 cannot fold away (a constant or static-helper shift would be
// inlined + constant-folded and never trap, matching gcc which also drops the
// check when it can prove the shift is out-of-range).

int main(void) {
    volatile int b = 33; /* >= 32: out-of-range for a 32-bit int shift */
    int a = 1;
    int c = a << b;      /* shift out-of-range -> UBSan halt */
    (void)c;
    return 0;
}
