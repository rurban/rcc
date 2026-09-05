/* GCC Bug #79586 - missing -Wdeprecated depending on position of attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79586
 */
/* { dg-do compile } */


int* f0 (void)
{
  int* p __attribute__ ((deprecated)) = 0;

  return p;   // warning, ok
}

int* f1 (void)
{
  int* __attribute__ ((deprecated)) p = 0;

  return p;   // no warning, bug
}

int* f2 (void)
{
  typedef int* P __attribute__ ((deprecated));
  P p = 0;   // warning, ok

  return p;
}

int* f3 (void)
{
  typedef int* __attribute__ ((deprecated)) P;
  P p = 0;   // no warning, bug

  return p;
}

