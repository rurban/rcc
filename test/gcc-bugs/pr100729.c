/* GCC Bug #100729 - Inconsistency in -Wformat-extra-args when first-to-check is 0 and format string is NULL
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100729
 */


#include <stddef.h>

__attribute__ ((format(printf, 1, 0)))
void format(const char *fmt);

void test(void) {
//     format(NULL);
}
#include <stddef.h>

__attribute__ ((format(printf, 1, 0)))
void format(const char *fmt, int extra);

void test(void) {
//     format(NULL, 0);
}


