/* GCC Bug #79696 - missing -Wunused-result on calls to malloc() and calloc()
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79696
 */
/* { dg-do compile } */


void f (unsigned n)
{
  __builtin_malloc (n);
}

void g (unsigned n)
{
  __builtin_calloc (n, n);
}

void h (unsigned n)
{
  __builtin_aligned_alloc (n, 2);
}
