// -fsanitize=undefined: signed integer overflow (add/sub/mul). Compiled with
// -fsanitize=undefined (see extra_ldflags in run_tests.c) and run with
// UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c), the gcc
// libubsan runtime reports the fault and halts with a deterministic exit 1
// (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_add_overflow / _sub_ / _mul_ from libubsan, so a broken
// -fsanitize link fails this test at compile time.
//
// The overflow is placed across a call boundary so it is not a constant the
// optimizer can prove and fold away (the framework compiles at -O1; a plain
// constant overflow would be folded and never trap, matching gcc which also
// drops the check when it can prove the overflow).

static int add(int a, int b) { return a + b; }

int main(void) {
    int c = add(2000000000, 2000000000); /* signed overflow -> UBSan halt */
    (void)c;
    return 0;
}
