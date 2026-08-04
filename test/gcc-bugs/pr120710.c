/* GCC Bug #120710 - C23 enum member does not have during processing the type indicated by C23 6.7.3.3:12
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120710
 */


#include <stdio.h>

enum e {
//     A = 0,
//     B = 0x80000000,
//     C = 2
};

#define ty(e) _Generic(e, int:"int", unsigned:"unsigned", long:"long", unsigned long:"unsigned long")

enum e1 {
//     G = 0,
//     H = -0x80000001L,
//     I,
//     J = _Generic(I, int:147, long:1046)
};

int main(void) {
  printf ("A B C %s %s %s\n\n", ty(A), ty(B), ty(C));
  printf ("G H I J %s %s %s %s\n", ty(G), ty(H), ty(I), ty(J));
//   printf("I %d", (int)J);
}
// Compiled with GCC 15.1 for x86-64, the program outputs:
// A B C unsigned unsigned unsigned
// G H I J long long long long
// I 147
// Compiler Explorer link: <a href="https://gcc.godbolt.org/z/1dY7n7Kco">https://gcc.godbolt.org/z/1dY7n7Kco</a>
// The surprising behavior is in the last output line that shows that I had type int during the processing of enum e1. <a href="https://cigix.me/c23#6.7.3.4">https://cigix.me/c23#6.7.3.4</a> says:
// During the processing of each enumeration constant in the enumerator list, the type of the enumeration constant shall be:
// …
// - the type of the value from the previous enumeration constant with one added to it. If such an integer constant expression would overflow or wraparound the value of the previous enumeration constant from the addition of one, [this is not the case in the example above]


