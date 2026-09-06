/* GCC Bug #78000 - -Wimplicit-function-declaration inhibited with macro from system headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78000
 */


#define foo bar
// x.c:
#include <a.h>
int main(){
//   foo(42);
}
// In file included from x.c:1:0:
// x.c: In function ‘main’:
// i/a.h:1:13: warning: implicit declaration of function ‘bar’ [-Wimplicit-function-declaration]
 #define foo bar
//              ^
// x.c:4:3: note: in expansion of macro ‘foo’
//    foo(42);
//    ^~~
// This problem occurs for instance with GMP headers (installed by the distribution in /usr/include) that have
#define mpf_out_str __gmpf_out_str
// but only declare the function if stdio.h was included first. Users don't get any indication that they are using an undeclared function.
// <a href="http://stackoverflow.com/q/40069876/1918193">http://stackoverflow.com/q/40069876/1918193</a>
// At least in this case, it seems that it would be better to use the location of 'foo(42)' to determine if the issue is in a system header or not. Are there cases where the current behavior is preferred?


