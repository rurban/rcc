/* GCC Bug #104693 - Can't disable "comparison between pointer and integer"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104693
 */
/* { dg-do compile } */


#include <stddef.h>
#include <stdbool.h>

bool foo(unsigned long not_a_pointer) {
  return not_a_pointer == NULL;
}
// =========================================
// <source>: In function 'foo':
// <source>:5:24: warning: comparison between pointer and integer
//     5 |   return not_a_pointer == NULL;
//       |                        ^~
// =========================================
// Godbolt link: <a href="https://gcc.godbolt.org/z/chcxYevvf">https://gcc.godbolt.org/z/chcxYevvf</a>
// I don't think anyone would argue that this is "good" (portable, defined, etc) C code; it is vendor code that I'm stuck with. Additionally, on the architecture I'm targeting, the implementation does the right thing: the unsigned long is pointer-sized, and is compared against the NULL literal 0, which is used in this context as a sentinel value. The code should use the literal 0 instead of NULL, but it does not.
// I have 3 suggestions, if they're helpful:
// 1. If this warning can be disabled, update the diagnostic message to include the name as a hint on how to disable it.
// 2. If this warning can not be disabled, consider giving it a formal diagnostic name and making it controllable via the standard methods (-Wno-, pragma, etc)
// 3. If this is truly heinous / unsafe enough, promote it to an error.
// Thanks for all of your efforts on gcc!
// Best,
// Charles


