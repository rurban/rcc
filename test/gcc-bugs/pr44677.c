/* GCC Bug #44677 - Warn for variables incremented but not used (+=, ++)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44677
 */
/* { dg-do compile } */


void f0 (int *p)
{
  p = 0;           // -Wunused-but-set-parameter (expected)
}

void f1 (int *p)
{
  p += 1;          // missing warning
}

void f2 (int *p)
{
  p = p + 1;       // missing warning
}

void f3 (int *p)
{
  ++p;             // missing warning
}

