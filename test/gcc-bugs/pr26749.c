/* GCC Bug #26749 - too few arguments to function va_start is caught too late
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=26749
 */


void f (void)
{
  __builtin_va_start ();
}
// t.c: In function ‘f’:
// t.c:3:3: error: too few arguments to function ‘__builtin_va_start’
   __builtin_va_start ();
//    ^~~~~~~~~~~~~~~~~~
// If I understand correctly, the problem pointed out in that bug is that -fsyntax-only shouldn't involve running the gimplifier, but without running the gimplifier these syntax errors wouldn't be emitted.  GCC is designed to detect some syntax problem in the gimplifier.
// It seems that not only has the problem not improved since 2006 but more syntax checking has been introduced into the gimplifier (mainly OpenMP stuff as far as I can tell).  So I'm not sure that there is value in keeping these bugs open with no effort to do anything about them but I'll leave it to others to decide.  (As I said, my goal only here is to try to review old bugs to see which ones can be closed, to help avoid having to repeatedly review them each release.)


