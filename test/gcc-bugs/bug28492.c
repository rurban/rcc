/* GCC Bug #28492 - -Wsuggest-attribute=format points to vsnprintf() or related functions instead of the function that needs the attribute added
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=28492
 */
/* { dg-do compile } */


#include <stdio.h>
#include <stdarg.h>

static void vsnprintf_one(char *self, va_list arglist)
{
    vsnprintf(self, 2, "%c", arglist);
}
// The options -Wformat -Wmissing-format-attribute (now spelled
// -Wformat -Wsuggest-attribute=format) used to cause the warning
//   warning: function might be possible candidate for 'printf' format
//   attribute
// to be issued for the call to vsnprintf() above, even though vsnprintf()'s
// own prototype already has the printf format attribute.  The warning
// should instead point at (or be about) vsnprintf_one() itself, since that
// is the function whose declaration is missing the format attribute:
//   static void vsnprintf_one(char *self, va_list arglist)
//       __attribute__ ((format (printf, 1, 0)));
// Instead, the diagnostic used to point at the innocent call to
// vsnprintf(), which already has the attribute, so the message was
// misleading about which function needs to be annotated.  The same
// happened with the other members of the vprintf family of functions,
// such as vfprintf.  (As of current GCC, -Wformat
// -Wsuggest-attribute=format no longer warns at all for this reduced
// testcase; the misleading-location aspect of the report could not be
// reconfirmed here.)
