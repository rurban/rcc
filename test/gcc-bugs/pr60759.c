/* GCC Bug #60759 - improve -Wlogical-op
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60759
 */


int foo(int x);

// int a = foo(1) || foo(2);  // Confusion with Perl, Python behaviour,
// int one = 2 && 3;          // or perhaps bitwise operation was intended.
while (foo(1) || foo(2));
int zero = (1 != 2) && (3 == 4);
static int x = 2 || 3;
int main() { return x; }


