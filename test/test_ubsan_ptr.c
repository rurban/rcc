// -fsanitize=undefined: pointer arithmetic overflow (base + offset wrapped the
// 64-bit address space). Compiled with -fsanitize=undefined (see extra_ldflags
// in run_tests.c) and run with UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env
// in run_tests.c), the gcc libubsan runtime reports the fault and halts with a
// deterministic exit 1 (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_pointer_overflow from libubsan, so a broken -fsanitize link
// fails this test at compile time.
//
// A volatile pointer just below the top of the address space plus an offset
// wraps around 0. The offset is a volatile variable (not a constant) so the
// add is not folded into an in-place immediate add at -O1; both volatiles keep
// the operation from being optimized away, so the check fires at -O0 and -O1.

#include <stdint.h>

int main(void) {
    volatile uintptr_t base = 0xfffffffffffffff8ULL; // -8
    volatile char *p = (char *)base;
    volatile int off = 16;
    p = p + off; /* wraps past the top -> UBSan halt */
    (void)p;
    return 0;
}
