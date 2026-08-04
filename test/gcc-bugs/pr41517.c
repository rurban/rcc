/* GCC Bug #41517 - Unexpected behaviour of #pragma in statement context
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41517
 */


#include <stdio.h>

int x;
void foo (void)
{
  int i, j;
  for (i = 0; i < 10; i++)
#pragma GCC visibility push(default)
    for (j = 0; j < 10; j++)
//       x++;
}

int main(void)
{
  foo ();
//   printf ("x = %d (should be 100)\n", x);
  return 0;
}
// If I compile and run using gcc 4.3.3 I get:
// x = 10 (should be 100)

// If I compile it as C++ (same gcc version), I get the behaviour I was expecting:

// x = 100 (should be 100)
// It seems that the C parser is treating the pragma as a statement
// which becomes the entire body of the first for loop, whereas the
// C++ parser is not treating the pragma as a statement.
// (Disclaimer: I know the example is silly.  In the original code,
// the pragma is one we have added locally to control the
// degree of loop unrolling in the following loop.  But I wanted to
// present here an example which is reproducible using stock gcc.)
// Here's more complete information on the gcc I am using:
// Using built-in specs.
// Target: i686-pc-linux-gnu
// Configured with: ../configure --prefix=/sw/st/gnu_compil/gnu/Linux-RH-WS-3/.package/gcc-4.3.3 --with-gnu-as --with-gnu-ld --enable-shared --enable-languages=c,c++ --disable-libgcj --disable-nls --with-mpfr=/sw/st/gnu_compil/gnu/Linux-RH-WS-3 --with-gmp=/sw/st/gnu_compil/gnu/Linux-RH-WS-3 --with-local-prefix=/sw/st/gnu_compil/gnu/Linux-RH-WS-3
// Thread model: posix


