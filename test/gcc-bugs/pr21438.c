/* GCC Bug #21438 - Warning about division by zero depends on lexical form
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=21438
 */


int f2(int t)
{
        return t/0;
}

int tt1 = f2<0>(1);
// ```
// Or none at all:
// ```
// template <typename C>
int f(C t)
{
        return t/0;
}
// template <int C>
int f1(int t)
{
        return t/C;
}

int tt = f<int>(1);
int tt1 = f1<0>(1);
// ```
// Note clang warns twice for f2 but only once for f/f1.


