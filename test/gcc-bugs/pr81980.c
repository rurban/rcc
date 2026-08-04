/* GCC Bug #81980 - Spurious -Wmissing-format-attribute and missing -Wformat for va_list in 32-bit mode
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81980
 */
/* { dg-do compile } */


#include <stdarg.h>

char a;
void set_message(const char *fmt, va_list ap)
  __attribute__((format(printf, 1, 0)));
void set_message_by_errcode(va_list ap) { set_message(&a, ap); }

// test.cc: In function ‘void set_message_by_errcode(va_list)’:
// test.cc:6:61: warning: function ‘void set_message_by_errcode(va_list)’ might be a candidate for ‘gnu_printf’ format attribute [-Wsuggest-attribute=format]
 void set_message_by_errcode(va_list ap) { set_message(&a, ap); }

// I believe the warning is spurious, since there's no way you could construct a valid printf format attribute for set_message_by_errcode (it doesn't take in a string parameter).


