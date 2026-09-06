/* GCC Bug #79049 - Unknown escape sequence not correctly pointed out
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79049
 */


#include <stdio.h>

int main()
{
//   fprintf(stderr, "This statement has a \
//                    backslash\ which is not correctly pointed out by gcc\n");

  return 0;
}
// Output from GCC:
// test.c: In function ‘main’:
// test.c:5:19: warning: unknown escape sequence: '\040'
//    fprintf(stderr, "This statement has a \
//                    ^~~~~~~~~~~~~~~~~~~~~~~              
// Output from Clang:
// test.c:6:29: warning: unknown escape sequence '\ ' [-Wunknown-escape-sequence]
//                    backslash\ which is not correctly pointed out by gcc\n");
//                             ^~


