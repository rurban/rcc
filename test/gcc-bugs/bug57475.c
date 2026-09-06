/* GCC Bug #57475 - "incompatible pointer type" message is not helpful enough
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57475
 */


#include <sys/un.h>
#include <sys/socket.h>
int main(void) {
  const struct msghdr msg;
  return sendmsg(0, &msg, 0);
}
// foo.c: In function 'main':
// foo.c:9:5: warning: passing argument 2 of 'sendmsg' from incompatible
// pointer type
// /usr/include/sys/socket.h:42:11: note: expected 'const struct msghdr *'
// but argument is of type 'const struct msghdr *'
// $
// Huh?  How can 'const struct msghdr *' not be compatible with itself?  [It took me a while to finally understand: the bug in cygwin's <sys/un.h> causes the declaration of sendmsg() to declare a local 'struct msghdr' rather than using the global type]

// I checked other bugs that mention this message, but none of them apply (<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED FIXED - "passing argument from incompatible pointer type" warning cannot be passed to -Werror="
//    href="show_bug.cgi?id=37866">bug 37866</a>, 14188, 30949).
// I've further reduced it down to this two-file example, as tested with gcc 4.7.2:
#ifdef SILENT
# pragma GCC system_header
#endif
extern int bar(struct foo *);
struct foo { int i; };
#include "foo.h"
int main(void) {
  struct foo f;
  return bar(&f);
}
// foo.c: In function ‘main’:
// foo.c:4:3: warning: passing argument 1 of ‘bar’ from incompatible pointer type [enabled by default]
// In file included from foo.c:1:0:
// foo.h:4:12: note: expected ‘struct foo *’ but argument is of type ‘struct foo *’
// which mirrors the fact that Cygwin's <sys/un.h> is compiled as a system header, and therefore misses the more obvious real root cause:
// In file included from foo.c:1:0:
// foo.h:4:23: warning: ‘struct foo’ declared inside parameter list [enabled by default]
// foo.h:4:23: warning: its scope is only this definition or declaration, which is probably not what you want [enabled by default]
// foo.c: In function ‘main’:
// foo.c:4:3: warning: passing argument 1 of ‘bar’ from incompatible pointer type [enabled by default]
// In file included from foo.c:1:0:
// foo.h:4:12: note: expected ‘struct foo *’ but argument is of type ‘struct foo *’
// I think it would be useful if the incompatible pointer type error message would ALSO call out a note on where the two types are first declared (if they are not built-in types), so that cases like mine, where the earlier error message about a parameter-list-local declaration was squelched is not quite so confusing.


