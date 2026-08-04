/* GCC Bug #78539 - feature request: __noextension__
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78539
 */


#define assert(expr) \
        __extension__ ({ \
            if (__noextension__ (expr)) \
//                ; \
//             else
               __assert_failed (...); \
//             (void)0; \
       })
// Without that, I have to resort to using two cases: one for __STRICT_ANSI__
// and one without, so that gcc -Wpedantic can still reliably diagnose "assert( ({1;}) );"
// This was first suggested in <a href="https://gcc.gnu.org/ml/gcc/2001-04/msg00642.html">https://gcc.gnu.org/ml/gcc/2001-04/msg00642.html</a>
// If you do add this, please ensure that the __noextension__(...) parentheses do not end up suppressing the detection performed by gcc's -Wparentheses.
// For reference, this came up recently in the following thread: <a href="https://sourceware.org/ml/libc-alpha/2016-11/msg00866.html">https://sourceware.org/ml/libc-alpha/2016-11/msg00866.html</a>


