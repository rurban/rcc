/* GCC Bug #102967 - confusing location in -Waddress for a subexpression of a ternary expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102967
 */


#include <stddef.h>

    #define MACRO(buf, off) (off < 0 ? NULL : (void*)&buf[off])

    void func(char *buf, long off)
    {
        if (off < 0 ? NULL : (void*)&buf[off]) { } // warning (correct)
        if (!(off < 0 ? NULL : (void*)&buf[off])) { } // warning (correct)
        if (MACRO(buf, off)) { } // no warning (correct)
        if (!MACRO(buf, off)) { } // gives a false positive -Waddress
    }


