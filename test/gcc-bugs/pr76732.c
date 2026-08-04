/* GCC Bug #76732 - Improve Woverride-init
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=76732
 */


int x[] = { [0] = 1, +3, [1] = 1 };

// This double-initialization of x[1] should cause a warning.  Similarly:

   struct s { int a, b; } s = { .a = 1, .a = 2};
// Thanks!


