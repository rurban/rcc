/* GCC Bug #52600 - OpenMP: declaration as structured block
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52600
 */


int foo1(void);
void foo()
{
//  a:  int x = foo1();
}
// But still reject:
int foo1(void);
void foo()
{
    #pragma omp task
//  a:  int x = foo1();
}
// While clang accepts the case with labels.


