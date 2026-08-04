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
// ;; Function g (g, funcdef_no=0, decl_uid=1817, cgraph_uid=0, symbol_order=0)
// Removing basic block 5
// g (void * p)
{
//   <bb 2> [100.00%] [count: INV]:
  if (p_2(D) != 0B)
    goto <bb 3>; [53.47%] [count: INV]
//   else
    goto <bb 4>; [46.53%] [count: INV]

//   <bb 3> [53.47%] [count: INV]:
  free (p_2(D)); [tail call]

//   <bb 4> [100.00%] [count: INV]:
//   return;

}


