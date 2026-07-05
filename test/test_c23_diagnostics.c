// __attribute__((warning/error/diagnose_if)) diagnostics
// clang-compatible compile-time function call diagnostics

#if !defined(TEST_DIAGNOSE_IF)
__attribute__((warning("old_func is deprecated")))
void old_func(void) {}

__attribute__((error("removed_func was removed")))
void removed_func(void) {}

__attribute__((diagnose_if(1, "always true warning", "warning")))
void diag_warn_func(void) {}

__attribute__((diagnose_if(1, "always true error", "error")))
void diag_err_func(void) {}
#endif

int main(void)
{
    old_func(); // expected warning

#if 0
    removed_func(); // would be compile error
    diag_err_func(); // would be compile error
#endif

    diag_warn_func(); // expected warning

    return 0;
}
