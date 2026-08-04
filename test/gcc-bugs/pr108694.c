/* GCC Bug #108694 - need a new warning option for preparing migration to ISO C23
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108694
 */


void func1 () {}
void func2 (void) {}
void func3 ();
void func4 (void);

void code ()
{
  void (*fp) (void);
  void (*fp1) (int);

  fp = func1;
  fp = func2;
  fp = func3;
  fp = func4;

  fp1 = func3;

  func1 (1); /* error */
  func2 (2); /* error */
  func3 (3); /* error */
  func4 (4); /* error */

  (void) fp;
  (void) fp1;
}

void func3 (int x, int y) {} /* error */
void func1 () {} /* No warning */
void func2 (void) {} /* No warning */
void func3 (); /* No warning */
void func4 (void); /* No warning */

void code ()
{
  void (*fp) (void);
  void (*fp1) (int);

  fp = func1; /* No warning */
  fp = func2; /* No warning */
  fp = func3; /* No warning */
  fp = func4; /* No warning */

  fp1 = func3; /* warning */

//   func1 (1); /* warning (if -std=c17) or error (if -std=c23) */
  func2 (2); /* error */
//   func3 (3); /* warning (if -std=c17) or error (if -std=c23) */
  func4 (4); /* error */

  (void) fp;
  (void) fp1;
}

// void func3 (int x, int y) {} /* warning (if -std=c17) or error (if -std=c23) */
// In the line 'fp1 = func3;' a warning should be shown because it's an unsafe function pointer conversion in C23. (Even though in C17 it is not dangerous code and even though in C23 it's not an error!) 'clang15 -std=c2x -Wincompatible-function-pointer-types' does show a "warning: incompatible function pointer types" there.
// But '-Wdeprecated-non-prototype' does not exactly have the behaviour you want: while it warns for 'func1 (1);' and 'func3 (3);' (good!), it warns also for 'void func3 ();', that is, where you don't want to see a warning.

// So, no existing GCC or clang warning option has the desired behaviour. What we need (and even independently of programming style) is


