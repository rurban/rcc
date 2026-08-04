/* GCC Bug #65275 - Disappearing -Wsequence-point warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65275
 */


int printf (const char *, ...);
int main (void) {
  int n = 2;
  int a[(++n, 7)];
  printf ("%d %d\n", n, (int) (sizeof a / sizeof a[0]));
}
// <span class="quote">> gcc -std=c99 -Wall foo.c</span >
// foo.c: In function 'main':
// foo.c:4:10: warning: operation on 'n' may be undefined [-Wsequence-point]
   int a[(++n, 7)];
// Curiously, it's not the 'sizeof' division or 'n' by themselves.  A slightly altered copy of the file has the following in the body of 'main', and there are no warnings with the same command line:
  int n = 2;
  int a[(++n, 7)];
//   printf("%d ", n);
//   printf("%d\n", (int) (sizeof a / sizeof a[0]));
// And I can assign the result of the 'sizeof' division to a variable as well to make the warning disappear:
  int n = 2;
  int a[(++n, 7)];
  int len = (int) (sizeof a / sizeof a[0]);
  printf ("%d %d\n", n, len);
// I've reduced the file to avoid using #include <stdio.h>, but I received the exact same results even when using the system header instead of the 'printf' prototype shown on the first line, so I've ruled out that possibility.
// I'm not certain whether this is undefined behavior or not, but even if it is, why does the warning disappear, depending on the code used?


