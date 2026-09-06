/* GCC Bug #53232 - No warning for main() without a return statement with -std=c99
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53232
 */


#include <stdio.h>

static int i = 0;

int main (void)
{
  if (i++ == 0)
    printf ("%d\n", main ());
}
// According to the C99 rules, it seems that the return statement is optional only for program termination. Though this is ambiguous, someone else at least has the same interpretation as me:
//   <a href="http://groups.google.com/group/comp.std.c/msg/c2f56fecfb699952">http://groups.google.com/group/comp.std.c/msg/c2f56fecfb699952</a>
// Before seeing this message, I posted
//   <a href="http://groups.google.com/group/comp.std.c/browse_thread/thread/0187ef7b23bedf16">http://groups.google.com/group/comp.std.c/browse_thread/thread/0187ef7b23bedf16</a>
// to comp.std.c (Subject: main function without a return statement in C99/C11).
// Also, I think that the warning should be given in every case for the following reasons:
// * A missing return statement may be unintentional (I think that implicit values like here should be discouraged in general, and that the C99 rule is there more to avoid undefined behavior than to save space).
// * Compatibility with C90 and with freestanding environments.
// * It is difficult to guarantee that main() will not be called from another C file.


