/* GCC Bug #88887 - Warn on unexpected continuation of 'return' to new line in if statement.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88887
 */
/* { dg-do compile } */


void bar();
//   void
//   foo (int x)
  {
    if (x) return

    bar ();
  }

// Their intention was to return immediately if (x) holds, but they missed the semicolon after 'return' and because bar() is declared with a void return type didn't hit any warnings.
// In my opinion, it would be reasonable for -wmisleading-indentation to cover a case like this. The related case:
//   void
//   foo2 (int x)
  {
    if (x)
//       return
    bar ();
  }
// Could also be warned.


