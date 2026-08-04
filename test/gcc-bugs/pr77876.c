/* GCC Bug #77876 - -Wbool-operation rejects useful code involving '~'
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77876
 */


#include <time.h>
enum { BILLION = 1000 * 1000 * 1000 };
time_t foo (time_t s, int res) { return s & ~ (res == 2 * BILLION); }

// This is a simplified version of Gnulib code using '~' that runs afoul of -Wbool-operation; see the bug report here:
// <a href="http://lists.gnu.org/archive/html/bug-gnulib/2016-10/txtbxk_mHAW_p.txt">http://lists.gnu.org/archive/html/bug-gnulib/2016-10/txtbxk_mHAW_p.txt</a>
// Rather than contort user code to pacify this misguided warning, I suggest making the warning more useful. The fundamental bug here is not applying ~ to a boolean; it's storing ~x into a boolean. More generally, the problem occurs when converting an expression that GCC can't prove to be 0 or 1 to bool. GCC should check for that instead. This would catch not only thinkos with ~ and ++ and --, but also similar thinkos involving other integer and floating-point operations. And it would correctly accept the Gnulib code.


