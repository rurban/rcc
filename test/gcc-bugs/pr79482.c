/* GCC Bug #79482 - _Static_assert(__builtin_constant_p(x)):
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79482
 */


int main(int argc, char *argv[]) { 
    int x = 0;

    _Static_assert(__builtin_constant_p(x) ? 1 : 2, "error");

    return 0;
}
// ===================
// fails with the error :
//   error: expression in static assertion is not constant
// With optimizations disabled (O0), it compiles fine.
// Also, if we replace the '?' operator by __builtin_choose_expr, the compiler seems to behave as expected :
// ===== main.c =====
int main(int argc, char *argv[]) { 
    int x = 0;

//     _Static_assert(__builtin_choose_expr(__builtin_constant_p(x), 1, 2),
//                    "error");

    return 0;
}
// ===================
// This compiles fine with any level of optimisation (while __builtin_choose_expr should expect the same 'constness' as _Static assert).

// Clang 3.9 compiles both versions without errors (with O3).
// Tested with gcc 6.3.1.


