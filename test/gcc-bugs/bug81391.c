/* GCC Bug #81391 - Use of parenthesis disables warning about incorrect size parameter
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81391
 */


#include <string.h>

int main()
{
    char buf[10], *ptr = buf;
//     strncpy(ptr, "a", sizeof(ptr));
//     strncpy(ptr, "a", (sizeof(ptr)));

    return 0;
}
// test.c: In function ‘main’:
// test.c:6:29: warning: argument to ‘sizeof’ in ‘strncpy’ call is the same expression as the destination; did you mean to provide an explicit length? [-Wsizeof-pointer-memaccess]
//      strncpy(ptr, "a", sizeof(ptr));
//                              ^


