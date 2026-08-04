/* GCC Bug #115513 - attribute nonstring should be used for format warnings
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115513
 */


#include <stdio.h>
#include <string.h>

struct Data
{
 char name[32] __attribute__((nonstring));
};

int f (struct Data *pd, const char *s)
{
// 	strncpy(pd->name, s, sizeof pd->name);

// 	printf("%s", pd->name);  // unsafe, no warning!?!

 return strlen(pd->name);   // unsafe, gets a warning
}
// ```
// Compile with, e.g.: gcc-14 -c -Wall -Wextra -O2 test.c
// As per the documentation, this will warn about the strlen() call.

// But it doesn't warn about the printf() call.  This would be quite useful and seems to be a gap in the warning coverage of this attribute.


