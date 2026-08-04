/* GCC Bug #56048 - -Werror=format=2 does not work
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56048
 */


#include <stdio.h>

int main(void)
{
 const char *s = "%s\n";
// 	printf(s, "abcd");
 return 0;
}
// format-warning.c: Na função ‘main’:
// format-warning.c:6:2: error: format not a string literal, argument types not checked [-Werror=format-nonliteral]


