/* GCC Bug #57029 - GCC doesn't set the inexact flag on inexact compile-time int-to-float conversion
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57029
 */


#include <stdio.h>
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

void test1 (void)
{
  volatile float c;

  c = 0x7fffffbf;
  printf ("c = %a, inexact = %d\n", c, fetestexcept (FE_INEXACT));
}

void test2 (void)
{
  volatile float c;
  volatile int i = 0x7fffffbf;

  c = i;
  printf ("c = %a, inexact = %d\n", c, fetestexcept (FE_INEXACT));
}

int main (void)
{
  test1 ();
  test2 ();
  return 0;
}
// Under Linux/x86_64:
// c = 0x1.fffffep+30, inexact = 0
// c = 0x1.fffffep+30, inexact = 32
// Ditto without optimizations.
// Note: the STDC FENV_ACCESS pragma is currently not supported (<a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Optimization generates incorrect code with -frounding-math option (#pragma STDC FENV_ACCESS not implemented)"
//    href="show_bug.cgi?id=34678">PR 34678</a>), but I don't think it is directly related (this is not an instruction ordering problem...).
// This bug has been found from:
//   <a href="http://gcc.gnu.org/ml/gcc-help/2013-04/msg00164.html">http://gcc.gnu.org/ml/gcc-help/2013-04/msg00164.html</a>


