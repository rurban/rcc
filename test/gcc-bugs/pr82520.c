/* GCC Bug #82520 - Missing warning when stack addresses escape the current scope
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82520
 */


#include <malloc.h>

struct foo { void *v; };

struct foo *bar(void)
{
    int a[10];
    struct foo *ret = malloc(sizeof(struct foo));

//     ret->v = &a;

    return ret;
}
// -----
// The address of 'a' is just somewhere on the stack. There might be rare cases where you'd want to do this, but usually this would be a bug.
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - GCC should warn about "obvious" bugs in binding a reference to temporary"
//    href="show_bug.cgi?id=63181">Bug 63181</a> is perhaps a C++ variation of the same kind of problem, but clang doesn't warn for this one.


