// -fsanitize=float-cast-overflow: float->int cast out of range. The value
// passed to the helper is not a compile-time constant, so the cast (and its
// float-cast-overflow check) survives -O1, where the framework compiles.
// Compiled with -fsanitize=float-cast-overflow (see extra_ldflags in
// run_tests.c) and run with UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env
// in run_tests.c), the gcc libubsan runtime reports the fault and halts with
// a deterministic exit 1 (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_float_cast_overflow from libubsan, so a broken -fsanitize
// link fails this test at compile time.
//
// 1e300 is out of range for int, matching gcc, which also fires at -O1.

int g(double d) {
    volatile int i = (int)d; /* d == 1e300 -> out of range -> UBSan halt */
    (void)i;
    return 0;
}

int main(void) { return g(1e300); }
