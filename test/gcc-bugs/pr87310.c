/* GCC Bug #87310 - -Wc90-c99-compat does not warn about bool usage
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87310
 */


#include <stdio.h>
#include <stdbool.h>
main() {
 bool ok = true;
// 	printf("%u\n", ok);
}
// when compiled with
// there's no warning, but I expected a warn about bool usage.
// When it looks like this
#include <stdio.h>
main() {
 _Bool ok = 1;
// 	printf("%u\n", ok);
}

// the warning `warning: ISO C90 does not support boolean types [-Wc90-c99-compat]' is raised.
// Tried with multiple version of gcc. Attached the output of different gcc -v runs.
// Try it here at tio.run: <a href="https://bit.ly/2CXnvXm">https://bit.ly/2CXnvXm</a>
// StackOverflow thread: <a href="https://stackoverflow.com/questions/52307780">https://stackoverflow.com/questions/52307780</a>


