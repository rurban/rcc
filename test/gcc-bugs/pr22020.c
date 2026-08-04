/* GCC Bug #22020 - poor error message for invalid cast in constant initializer
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=22020
 */


int a;
int b = (int)&a;
short c = (short)&a;


