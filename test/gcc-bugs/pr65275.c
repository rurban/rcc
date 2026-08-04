/* GCC Bug #65275 - Disappearing -Wsequence-point warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65275
 */


int printf (const char *, ...);

// Original testcase: with "gcc -std=c99 -Wall foo.c" this warns:
//   foo.c: In function 'main':
//   foo.c:4:10: warning: operation on 'n' may be undefined [-Wsequence-point]
//      int a[(++n, 7)];
int variant1 (void)
{
  int n = 2;
  int a[(++n, 7)];
  printf ("%d %d\n", n, (int) (sizeof a / sizeof a[0]));
  return 0;
}

// Curiously, it's not the 'sizeof' division or 'n' by themselves.  Splitting
// the single printf call into two calls makes the warning disappear with
// the exact same command line.
int variant2 (void)
{
  int n = 2;
  int a[(++n, 7)];
  printf ("%d ", n);
  printf ("%d\n", (int) (sizeof a / sizeof a[0]));
  return 0;
}

// Assigning the result of the 'sizeof' division to a variable also makes
// the warning disappear.
int variant3 (void)
{
  int n = 2;
  int a[(++n, 7)];
  int len = (int) (sizeof a / sizeof a[0]);
  printf ("%d %d\n", n, len);
  return 0;
}

int main (void)
{
  variant1 ();
  variant2 ();
  variant3 ();
  return 0;
}
// I've reduced the file to avoid using #include <stdio.h>, but I received
// the exact same results even when using the system header instead of the
// 'printf' prototype shown on the first line, so I've ruled out that
// possibility.
// I'm not certain whether this is undefined behavior or not, but even if it
// is, why does the warning disappear, depending on the code used?
