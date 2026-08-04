/* GCC Bug #37874 - gcc sometimes accepts attribute in identifier list
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37874
 */


void f2(y, __attribute__(()) x);
void f3(__attribute__(()) x, y);


