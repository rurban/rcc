// -fsanitize=undefined: misaligned pointer dereference (a load whose address is
// not aligned to the type's size). Compiled with -fsanitize=undefined (see
// extra_ldflags in run_tests.c) and run with UBSAN_OPTIONS=halt_on_error=1 (see
// test_unit_env in run_tests.c), the gcc libubsan runtime reports the fault and
// halts with a deterministic exit 1 (see test_unit_expected_exit). The link must
// resolve __ubsan_handle_type_mismatch_v1 from libubsan, so a broken -fsanitize
// link fails this test at compile time.
//
// An int loaded through a pointer to an address with low bit set (0x10001) is
// not 4-byte aligned; the check fires at both -O0 and -O1, matching gcc.

int main(void) {
    unsigned long a = 0x10001;
    int *p = (int *)a;
    int v = *p; /* misaligned int load -> UBSan halt */
    return v & 1;
}
