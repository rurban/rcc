/* GCC Bug #45358 - Diagnostic could be issued for old C style  a =+ b and similar cases
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45358
 */


#include <stdio.h>
int main(void) {
        int a=1,b=1;

        a += 2;
        b =+ 2;
//         printf("a=%d b=%d\n",a,b);
        return 0;
}

// (no warning, not even with -Wall)
// a=3 b=2
// It would be nice if future version could at least throw a warning.
// Regards
// Christian Leber


