/* GCC Bug #88716 - Improved diagnostics: No detection of conflicting function definitions in some cases.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88716
 */


void f(x)
    int x;
  {
  }

  void f(int, int);

  void f();

  int main() {}

// and (prog2.c):

  void f(x)
    int x;
  {
  }

  void f();

  void f(int, int);

  int main() {}
// Both have compile time undefined behaviour because of conflicting declarations
// of the function f.
// The only difference between them is the reordering of the two last declarations.
// When the first test case is compiled with 
// The undefined behaviour is detected and an error message is outputed.
// When the second test case is compiled with
// The undefined behaviour is not detected.
// It would be good if gcc could detect the undefined behaviour even in the second
// case.


