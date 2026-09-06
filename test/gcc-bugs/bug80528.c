/* GCC Bug #80528 - reimplement gnulib's "useless-if-before-free" script as a compiler warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80528
 */
/* { dg-do compile } */


void free (void*);

void g (void *p)
{
  if (p)        // test retained
    free (p);
//   else
    free (p);   // eliminated
}
