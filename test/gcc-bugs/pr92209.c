/* GCC Bug #92209 - Imprecise column number for -Wstrict-prototypes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92209
 */


static int func_1();
int func_1(int a) { return a; }


