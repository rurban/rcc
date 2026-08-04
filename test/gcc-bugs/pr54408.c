/* GCC Bug #54408 - sqrt for vector types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54408
 */


template<class T> T f(T x){return x+sqrt(x);}


