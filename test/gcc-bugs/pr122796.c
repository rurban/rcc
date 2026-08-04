/* GCC Bug #122796 - assert/macros from a system-header is incorrectly supressing a fallthrough warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122796
 */


#include <assert.h>

int foo(int a, void *p)
{
    int b = 0;

    switch (a)
    {
    case 0:
//             assert(p != 0);
//             // no warning here
    case 1:
            b = 11;
            break;
    }

    return b;
}
// ```
// Curiously, it does produce the warning if I replace the assert() with its preprocessed equivalent.  Is there something magic about assert() here?  Still, doesn't seem right.


