/* GCC Bug #87038 - diagnostics: Have -Wjump-misses-init be enabled by -Wall or -Wextra
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87038
 */
/* { dg-do compile } */

#include <stdio.h>

void func(int x) {
        switch (x) {
                case 1: {
                        int foo = 3;
                        case 0:
                                printf("foo is %d\n", foo);
                }
        }
}

void func2(int x) {
        if (x == 0) goto lbl;
        int foo = 3;
lbl:
        printf("foo is %d\n", foo);
}
