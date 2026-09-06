/* GCC Bug #88716 - Improved diagnostics: No detection of conflicting function definitions in some cases.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88716
 */
/* { dg-do compile } */
/* { dg-options "-std=c11 -pedantic-errors" } */

void f(x)
    int x;
  {
  }

  void f();

  void f(int, int);

  int main() {}
