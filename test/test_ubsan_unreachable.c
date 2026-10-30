// -fsanitize=undefined: __builtin_unreachable reached (execution reached an
// unreachable program point). Compiled with -fsanitize=undefined (see
// extra_ldflags in run_tests.c) and run with
// UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c), the gcc
// libubsan runtime reports the fault and halts with a deterministic exit 1
// (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_builtin_unreachable from libubsan, so a broken -fsanitize
// link fails this test at compile time.
//
// The unreachable point is genuinely reached at runtime (the function falls
// through to it), so the check fires deterministically; the framework's -O1
// keeps it because it cannot prove the path unreachable.

int main(void) {
    __builtin_unreachable(); /* reached -> UBSan halt */
}
