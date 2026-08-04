/* GCC Bug #113631 - FAIL: gcc.dg/pr7356.c, fix still fails with #pragma
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113631
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char** argv)
{
  return 0;
}
// The expectation is that GCC warns on the 'a', not somewhere inside stdlib.h. This now works as indicated in that PR:
// <source>:1:2: error: expected ';' before 'typedef'
//     1 | a
//       |  ^
//       |  ;
// Notably, it works differently with C++:
// <source>:1:1: error: 'a' does not name a type
//     1 | a
//       | ^
// ...but at least it marks 'a' as the issue (should that be a separate PR?)
// However, on mingw, we have certain constructs in our headers that still confuse the parser, resulting in this:
// In file included from /tmp/rt/mingw14/x86_64-w64-mingw32/include/_mingw.h:282,^M
//                  from /tmp/rt/mingw14/x86_64-w64-mingw32/include/corecrt.h:10,^M
//                  from /tmp/rt/mingw14/x86_64-w64-mingw32/include/stdlib.h:9,^M
//                  from /tmp/gcc/testsuite/gcc.dg/<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - syntax errors immediately before an #include are reported for within the included file"
//    href="show_bug.cgi?id=7356">pr7356</a>.c:4:^M
// /tmp/rt/mingw14/x86_64-w64-mingw32/include/vadefs.h:14:9: error: expected '=', ',', ';', 'asm' or '__attribute__' before '#pragma'^M
 #pragma pack(push,_CRT_PACKING)^M
//          ^~~~^M
// It turns out that the problem is target-agnostic and is really just due to pragmas, so I've reduced it and reproduced the problem on GNU/Linux (the pragma is meant to be a no-op, that was a close approximation.  GCC diagnostic push also works):
// a.c:
// a
#include "a.h"
int main() {}
// a.h:
#pragma message "foo"

// a.h:1:9: error: expected '=', ',', ';', 'asm' or '__attribute__' before '#pragma'
//     1 | #pragma message "foo"
//       |         ^~~~~~~
// a.h:1:9: note: '#pragma message: foo'


