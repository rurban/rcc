/* GCC Bug #117945 - -Wuseless-cast could be suppressed when casting to/from a type expanded from typedef or macro
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117945
 */


#include <stdlib.h>  // size_t
#include <stdint.h>  // uint64_t
int main()
{
    uint64_t a;
    size_t b = (size_t)a;
}
// main.c: In function ‘main’:
// main.c:6:16: warning: useless cast to type ‘long unsigned int’ [-Wuseless-cast]
//     6 |     size_t b = (size_t)a;
//       |                ^
// x86_64 GNU/Linux
// Generally, this warning will be almost always unwanted
while casting between types declared in System Headers
// (unless doing something like size_t to/from size_t cast).
// I would suggest suppressing the warning in said cases
// or allow for alternative warning flag (or additional option for existing flag). 
// Potentially related to:
// <a href="https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85043">https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85043</a>
// (^ this bug describe alternative -Wcast-to-the-same-type warning)
// Tested on: gcc 14.2.1 Linux


