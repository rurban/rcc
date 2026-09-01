/* C29 (WG14 N3353's "future language directions", generalized in the
 * C29 changelog): remove `_Imaginary` from the keyword list -- no
 * mainstream compiler ever implemented C99's `_Imaginary` type, and
 * rcc has never reserved it either. This pins that down as a
 * regression test: `_Imaginary` must remain usable as an ordinary
 * identifier for a variable, function, struct member, and macro name.
 */
#include "test_common.h"

struct point {
    int _Imaginary; /* struct member named _Imaginary */
};

#define _Imaginary_helper(x) ((x) * 2)

static int _Imaginary(int x) /* function named _Imaginary */
{
    return x + 1;
}

int main(void)
{
    int _Imaginary = 42; /* local variable named _Imaginary */
    if (_Imaginary != 42) {
        printf("FAIL: variable named '_Imaginary' misbehaved\n");
        return 1;
    }

    struct point p;
    p._Imaginary = 7;
    if (p._Imaginary != 7) {
        printf("FAIL: struct member named '_Imaginary' misbehaved\n");
        return 2;
    }

    if (_Imaginary_helper(3) != 6) {
        printf("FAIL: macro named '_Imaginary_helper' misbehaved\n");
        return 3;
    }

    /* Shadowed by the local variable above inside main(); call the
     * function directly via a function pointer obtained before the
     * shadowing local's scope, to also confirm the *function* itself
     * compiled and links correctly. */
    int (*fp)(int) = 0;
    {
        extern int _Imaginary(int);
        fp = _Imaginary;
    }
    if (fp(9) != 10) {
        printf("FAIL: function named '_Imaginary' misbehaved\n");
        return 4;
    }

    return 0;
}
