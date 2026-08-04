/* GCC Bug #61898 - Variadic functions accept va_list without warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61898
 */


#include <stdarg.h>
#include <stdio.h>

void printfBadly(const char* fmt, ...)
{
  va_list ap;
//   va_start(ap, fmt);
//   printf(fmt, ap); // oops, we wanted vprintf
//   va_end(ap);
}
// --------
// GCC 4.9.0 (and 4.7.2) with "-Wall -Wextra" compiles this program with no complaint, but it produces undefined behavior.  There are two types of errors which GCC could check for:

// (1) GCC knows how to check printf when the format string is a literal, but otherwise doesn't seem to check the arguments at all.  Passing a va_list to printf (or any function with attribute(format(printf))) is almost certainly an error.  The only conversion specifier that seems usable with a va_list is "%p", and even that is non-portable, unlikely to be useful, and probably not what the programmer intended.  Related to this, there is also no warning when passing a struct (by value) to printf, though such code is likely incorrect.

// (2) Passing a va_list via variadic arguments is almost never correct.  The example above used printf when it needed vprintf, but this is a more general point: a va_list passed anywhere within ellipsis varags is very likely to be a mistake which results in undefined behavior.  This type of error applies not only to printf-like functions, but to variadic functions in general.

// In either case, a warning would be very useful (with -Wextra at least).


