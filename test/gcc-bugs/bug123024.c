/* GCC Bug #123024 - -Wstringop-overread: bogus diagnostic about strncat(3)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123024
 */
/* { dg-do compile } */
/* { dg-options "-Wstringop-overread" } */

#include <string.h>
#include <utmp.h>

#define _Countof(a) (sizeof(a) / sizeof(*(a)))

void f(struct utmp *ut, char buf[_Countof(ut->ut_line) + 1])
{
	strcpy(buf, "");
	strncat(buf, ut->ut_line, _Countof(ut->ut_line));

	strcpy(buf, "");
	__builtin___strncat_chk(buf, ut->ut_line, _Countof(ut->ut_line),
	                        _Countof(ut->ut_line) + 1); /* { dg-bogus "argument 2 declared attribute .nonstring." } */
}
/* gcc warns: '__builtin___strncat_chk' argument 2 declared attribute
 * 'nonstring' [-Wstringop-overread] - bogus, since ut_line IS the
 * nonstring source being concatenated (the bug). */