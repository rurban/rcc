/* GCC Bug #80806 - gcc does not warn if local array is memset only
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80806
 */


#include <string.h>

void test()
{
    char buf[10];
//     memset(buf, 0, sizeof(buf));

    int c;
    c = 1;
}
// test.c: In function ‘test’:
// test.c:8:9: warning: variable ‘c’ set but not used [-Wunused-but-set-variable]
     int c;
//          ^


