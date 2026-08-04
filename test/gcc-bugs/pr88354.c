/* GCC Bug #88354 - Please warn on the use of a va_list argument in *printf functions instead of v*printf
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88354
 */


// A {,f,d,s,sn}printf function can mistakenly be used instead of v{,f,d,s,sn}printf. for instance, this is what happened in atop:
// Type checking should warn when this occurs. Implementing the warning is possible because the {,f,d,s,sn}printf functions cannot take a va_list argument, so that this is necessarily an error.
#include <stdio.h>
#include <stdarg.h>

void f (int i, const char *s, ...)
{
  if (i)
    {
      va_list args;
      va_start (args, s);
      fprintf (stderr, s, args);
      va_end (args);
    }
}

int main (void)
{
  unsigned long d = 17;
  f (1, "n = %lu\n", d);
  return 0;
}

// (The correct code is obtained by replacing fprintf with vfprintf.)


