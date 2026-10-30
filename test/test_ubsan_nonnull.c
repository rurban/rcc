// -fsanitize=undefined: non-null C-string (a null pointer passed to a libc
// string function such as strlen). Compiled with -fsanitize=undefined (see
// extra_ldflags in run_tests.c) and run with
// UBSAN_OPTIONS=halt_on_error=1 (see test_unit_env in run_tests.c), the gcc
// libubsan runtime reports the fault and halts with a deterministic exit 1
// (see test_unit_expected_exit). The link must resolve
// __ubsan_handle_nonnull_arg from libubsan, so a broken -fsanitize link
// fails this test at compile time.
//
// strlen(null) is a runtime fault: the string length cannot be computed for a
// null pointer. gcc inlines strlen (repne scasb) at -O1 and still keeps the
// nonnull check; rcc mirrors that (the check is emitted in the inlined
// builtin and in the call path).

#include <string.h>

int main(void) {
    char *s = 0;
    size_t n = strlen(s); /* null string -> UBSan halt */
    (void)n;
    return 0;
}
