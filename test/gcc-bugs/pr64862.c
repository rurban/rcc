/* GCC Bug #64862 - printf attribute should accept other string types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64862
 */


#include <stdarg.h>
#include <uchar.h>

extern void p (const char16_t *fmt, ...)
  __attribute__((__printf__ (1, 2)));

void f()
{
//   p(u"%d\n", 23);
}
// Currently gcc rejects this attribute:
// pokyo. gcc --syntax-only -std=c11 -Wall r.c
// r.c:5:3: warning: ‘__printf__’ attribute directive ignored [-Wattributes]
   __attribute__((__printf__ (1, 2)));
//    ^
// I don't see why it should, though.
// It seems to me that it could accept wchar_t, char16_t, or char32_t
// strings as the format argument.


