/* GCC Bug #22241 - completion by initializer incompatible with type in inner scope should be diagnosed
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=22241
 */


extern int a[];
void f(void) { extern int a[2]; }
int a[] = { 0 };

// should receive an error for the incompatible types of "a", int[2] and int[1].
// Not a regression.  Previously mentioned at
// <<a href="http://gcc.gnu.org/ml/gcc-patches/2005-05/msg00944.html">http://gcc.gnu.org/ml/gcc-patches/2005-05/msg00944.html</a>>.


