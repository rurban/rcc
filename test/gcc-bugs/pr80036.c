/* GCC Bug #80036 - Source line not printed for diagnostic if expanded from a macro
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80036
 */


extern void print_it(const char *fmt, int i);
#define print_int(x) print_it("%d",(x))
// code.c:
#include "macros.h"
void test(void)

{

  int unint;

//   print_int(uninit);

}
// In file included from test.c:1:0:
// test.c: In function 'test':
// macros.h:1:22: warning: 'uninit' is used uninitialized in this function [-Wuninitialized]
 #define print_int(x) print_it("%d",(x))
//                       ^~~~~~~~
// test.c:7:7: note: 'uninit' was declared here
   int uninit;
//        ^~~~~~
// so the offending line print_int() is not shown.


