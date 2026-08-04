/* GCC Bug #78503 - -Wint-in-bool-context false positive on unsigned multiplication
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78503
 */
/* { dg-do compile } */
/* { dg-options "-Wall" } */


void f (void*);

void g (unsigned n)
{
  void *p = (n * sizeof (int)) ? __builtin_malloc (n * sizeof (n)) : 0; /* { dg-warning "in boolean context, suggest" } */
  f (p);
}
// c.c: In function ‘g’:
// c.c:5:16: warning: ‘*’ in boolean context, suggest ‘&&’ instead [-Wint-in-bool-context]
//   void *p = (n * sizeof (int)) ? __builtin_malloc (n * sizeof (n)) : 0;
//              ~~~^~~~~~~~~~~~~~~
// Note that the test case has been distilled from a modified definition of the XALLOCAVEC macro in libiberty:
//   #define XALLOCAVEC(T, N)   ((N) ? (T *) alloca (sizeof (T) * (N)) : (T *)0)
// The modified definition which tries to prevent zero-size allocation then causes GCC to fail to bootstrap with errors like the following:
// /src/gcc/svn/gcc/fold-const.c:1499:33: error: ‘*’ in boolean context, suggest ‘&&’ instead [-Werror=int-in-bool-context]
//   elts = XALLOCAVEC (tree, nelts * 4);
// /src/gcc/svn/gcc/../include/libiberty.h:356:28: note: in definition of macro ‘XALLOCAVEC’
// #define XALLOCAVEC(T, N) ((N) ? (T *) alloca (sizeof (T) * (N)) : (T *)0)
//                             ^


