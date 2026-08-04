/* GCC Bug #86833 - missing warning for out-of-bounds array access without optimization
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86833
 */


int main()
{
    double array[2]={0, 0};
//     printf("%f\n", array[999]);

    return 0;
}
// Compiling with "gcc -Wall -O1" throws:
// main.c: In function 'main':
// main.c:6:5: warning: 'array[999]' is used uninitialized in this function [-Wuninitialized]
//      printf("%f\n", array[999]);
//      ^~~~~~~~~~~~~~~~~~~~~~~~~~
// While compiling with "gcc -Wall" or equivalent "gcc -Wall -O0" throws no error at all.
// I got told, that this is typical, because unless O1 is set GCC performs no checks related to such memory management.
// It was also mentioned, that clang throws an error in all cases. 
// Usually Linters use no Optimization and will miss that.


