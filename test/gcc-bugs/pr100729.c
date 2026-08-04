/* GCC Bug #100729 - Inconsistency in -Wformat-extra-args when first-to-check is 0 and format string is NULL
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100729
 */
/* { dg-do compile } */
/* { dg-options "-Wformat -Wformat-extra-args" } */

#include <stddef.h>

__attribute__ ((format(printf, 1, 0)))
void format1(const char *fmt);

__attribute__ ((format(printf, 1, 0)))
void format2(const char *fmt, int extra);

void test(void) {
    format1(NULL);   /* inconsistent: no -Wformat-extra-args here */
    format2(NULL, 0); /* { dg-warning "too many arguments for format" } */
}


