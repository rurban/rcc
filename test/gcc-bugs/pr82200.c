/* GCC Bug #82200 - Unhelpful diagnostic for incorrectly ordered attribute and asm on function declaration in system header
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82200
 */


#include <syslog.h>

int main() {}
// ```
// with gcc-7.2.0 under macOS 10.13, I got these error messages:
// ```
// In file included from /usr/include/sys/cdefs.h:587:0,
//                  from /usr/include/sys/syslog.h:65,
//                  from /usr/include/syslog.h:23,
//                  from sl.c:1:
// /usr/include/sys/syslog.h:227:124: error: expected ‘,’ or ‘;’ before ‘__asm’
 void syslog(int, const char *, ...) __printflike(2, 3) __not_tail_called __DARWIN_ALIAS_STARTING(__MAC_10_13, __IPHONE_NA, __DARWIN_EXTSN(syslog));
// ```
// where __DARWIN_EXTSN is defined as:
// (in /usr/include/sys/cdefs.h)

#define __DARWIN_EXTSN(sym)		__asm("_" __STRING(sym) __DARWIN_SUF_EXTSN)
// It seems that __asm is not implemented.
// I compiled gcc-7.2.0 by this configuration:
// ../gcc-7.2.0/configure --prefix=/opt --enable-languages=c,c++,objc,obj-c++


