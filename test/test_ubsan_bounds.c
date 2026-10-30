// -fsanitize=undefined: local array out-of-bounds. The index passed to the
// helper is not a compile-time constant, so the access (and its OOB check)
// survives -O1, where the framework compiles. Compiled with
// -fsanitize=undefined (see extra_ldflags in run_tests.c) and run with
// UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c), the gcc
// libubsan runtime reports the fault and halts with a deterministic exit 1
// (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_out_of_bounds from libubsan, so a broken -fsanitize link
// fails this test at compile time.
//
// Index 100 is out of bounds for a 10-element array, matching gcc, which also
// fires at -O1.

int g(int i) {
    int a[10];
    a[0] = 1;
    int v = a[i]; /* i == 100 -> out of bounds -> UBSan halt */
    return v;
}

int main(void) { return g(100); }
