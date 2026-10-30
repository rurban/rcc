// -fsanitize=returns-nonnull-attribute (part of the default -fsanitize=undefined
// group): a function declared __attribute__((returns_nonnull)) must not return
// a NULL pointer. The helper returns its (opaque) parameter, so the `cmp $0`
// survives -O1, where the framework compiles — the compiler cannot fold a
// parameter away. Compiled with -fsanitize=undefined (see extra_ldflags in
// run_tests.c) and run with UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env),
// gcc's libubsan reports the fault and halts with a deterministic exit 1 (see
// test_unit_expected_exit). The link must resolve
// __ubsan_handle_nonnull_return_v1 from libubsan, so a broken -fsanitize link
// fails this test at compile time.

__attribute__((returns_nonnull)) int *get_ptr(int *p) {
    return p;
}

int main(void) {
    int *r = get_ptr(0); /* NULL through opaque param -> returns_nonnull fault */
    (void)r;
    return 0;
}
