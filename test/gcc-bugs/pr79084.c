/* GCC Bug #79084 - Missed -Wpedantic for implicit double with complex specifier due to "complex" being a macro defined in a system header
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79084
 */


#include <complex.h>
complex c;
 complex c;

// As the location (c.c:2:1) is not in a system header the warning should not be suppressed. When user code misuses a macro defined in a system header that is not system code, and should not be suppressed.


