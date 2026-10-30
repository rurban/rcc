// -fsanitize=undefined: null-pointer dereference (load of a null pointer).
// Compiled with -fsanitize=undefined (see extra_ldflags in run_tests.c) and
// run with UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c),
// the gcc libubsan runtime reports the fault and halts with a deterministic
// exit 1 (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_type_mismatch_v1 from libubsan, so a broken -fsanitize link
// fails this test at compile time.
//
// The dereference is placed across a call boundary (deref() is not folded at
// the framework's -O1) so the null load is a genuine runtime op, matching gcc
// which also keeps the check when it cannot prove the pointer non-null.

static int deref(int *p) { return *p; }

int main(void) {
    int *p = 0;
    int v = deref(p); /* null deref -> UBSan halt */
    (void)v;
    return 0;
}
