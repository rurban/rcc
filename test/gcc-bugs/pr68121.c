/* GCC Bug #68121 - __builtin_constant_p should not warn about integer overflow
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68121
 */


enum {
//     x = 2000000000,
//     y = 2000000000,
//     a = __builtin_constant_p (x + y) ? x : -1,
  };

// GCC warns "integer overflow in expression [-Woverflow]" and points to the "+", even though the "+" is not evaluated. This sort of warning should be disabled in the argument of __builtin_constant_p, just as it's disabled in the argument of other expressions like sizeof that do not actually evaluate their arguments. In the above case, __builtin_constant_p should return 1 without warning about overflow.
// I ran into this problem when using a macro definition like this:
 #define INT_ADD_OVERFLOW(a, b) \
//    (__builtin_constant_p ((a) + (b)) \
//     ? _GL_INT_ADD_OVERFLOW (a, b) \
    : __builtin_add_overflow (a, b, &(__typeof__ ((a) + (b))) {0}))

// where _GL_INT_ADD_OVERFLOW (a, b) expands to a complicated expression that does not overflow, and that yields 1 if A+B would overflow. GCC incorrectly warned about (a)+(b) overflowing in the argument of __builtin_constant_p. I worked around the bug by changing the first "+" to "==" in the macro body, but that's kinda weird.


