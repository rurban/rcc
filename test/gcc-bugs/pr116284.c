/* GCC Bug #116284 - incorrect classification of zero-sized array as variably modified
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116284
 */


static int a[0];
static int b[sizeof a];

void foo(int (*x)[*]);

static int c[0];
static int d[sizeof c];
// <source>:11:12: error: variably modified 'd' at file scope
//    11 | static int d[sizeof c];
// <a href="https://godbolt.org/z/a8Ej6c5jr">https://godbolt.org/z/a8Ej6c5jr</a>
// Found by Alejandro Colomar.


