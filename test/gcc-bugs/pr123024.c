/* GCC Bug #123024 - -Wstringop-overread: bogus diagnostic about strncat(3)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123024
 */


#include <string.h>
#include <utmp.h>
// void
// f(struct utmp *ut, char buf[_Countof(ut->ut_line) + 1])
{
// 	strcpy(buf, "");
// 	strncat(buf, ut->ut_line, _Countof(ut->ut_line));

// 	strcpy(buf, "");
 __builtin___strncat_chk(buf, ut->ut_line, _Countof(ut->ut_line),
                         _Countof(ut->ut_line) + 1);
}
// alx@devuan:~/tmp$ /opt/local/gnu/gcc/maxof8/bin/gcc -S -Wstringop-overread s-overread.c
// s-overread.c: In function ‘f’:
// s-overread.c:11:9: warning: ‘__builtin___strncat_chk’ argument 2 declared attribute ‘nonstring’ [-Wstringop-overread]
//    11 |         __builtin___strncat_chk(buf, ut->ut_line, _Countof(ut->ut_line),
//       |         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    12 |                                 _Countof(ut->ut_line) + 1);
//       |                                 ~~~~~~~~~~~~~~~~~~~~~~~~~~
// In file included from /usr/include/utmp.h:29,
//                  from s-overread.c:2:
// /usr/include/x86_64-linux-gnu/bits/utmp.h:62:8: note: argument ‘ut_line’ declared here
//    62 |   char ut_line[UT_LINESIZE]
//       |        ^~~~~~~


