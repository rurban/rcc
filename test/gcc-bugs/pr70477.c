/* GCC Bug #70477 - -Wtautological-compare too aggressive?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70477
 */
/* { dg-do compile } */


int var;

#define A var
#define B var

int foo (int a, int b)
{
  if (A == B)
    return a;
  else
    return b;
}
