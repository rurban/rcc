/* GCC Bug #103996 - Provide Better diagnostic for invalid reuse of a function name
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103996
 */
/* { dg-do compile } */


#include <strings.h>
void f() {
    index[0] = 0;
}
#gcc is 11.2.0
// a.c:4:7: error: subscripted value is neither array nor pointer nor vector
//     4 |  index[1] = 0;
//       |       ^
// I could not find a compiler option that would tell me what the root cause was.  I wasn't getting a simple "undeclared" error, so I surmised the name must already be in use, but I was confused as to how or where it was being used.  My first thought was that maybe index was a global variable, so I tried using -Wshadow.  Eventually, I found that it was a function from strings.h which was included several layers down an include spiral.
// I think an effective note from gcc such as "index was previously declared here as a function", or something similar that often happens in other diagnostics, would aid in finding the root cause of my bug.
// Incidentally, in my actual scenario, the real bug was that I forgot to access index as a member of a struct: x.index vs index.  But I'd wager that expecting GCC to look for all nearby structs for a similarly named member is unreasonable.  Simply telling me where index originates, though, would have helped greatly.


