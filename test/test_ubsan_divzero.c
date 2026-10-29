// -fsanitize=undefined: integer division by zero. Compiled with
// -fsanitize=undefined (see extra_ldflags in run_tests.c) and run with
// UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c), the gcc
// libubsan runtime reports the fault and halts with a deterministic exit 1
// (see test_unit_expected_exit) -- halt avoids the UBSan FPE signal handler,
// whose exit code is build-dependent. The link must resolve
// __ubsan_handle_divrem_overflow from libubsan, so a broken -fsanitize link
// fails this test at compile time.

int main(void) {
    int a = 100, b = 0;
    int c = a / b; /* divide by zero -> UBSan report + SIGFPE */
    (void)c;
    return 0;
}
