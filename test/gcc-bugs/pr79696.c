/* GCC Bug #79696 - missing -Wunused-result on calls to malloc() and calloc()
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79696
 */


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

// ;; Function f (f, funcdef_no=2, decl_uid=2364, cgraph_uid=2, symbol_order=2)

// f (unsigned int n)
{
  long unsigned int _1;

//   <bb 2> [0.00%] [count: INV]:
  _1 = (long unsigned int) n_2(D);
  __builtin_malloc (_1);
//   return;

}
// ;; Function g (g, funcdef_no=3, decl_uid=2367, cgraph_uid=3, symbol_order=3)

// g (unsigned int n)
{
  long unsigned int _1;
  long unsigned int _2;

//   <bb 2> [0.00%] [count: INV]:
  _1 = (long unsigned int) n_3(D);
  _2 = (long unsigned int) n_3(D);
  __builtin_calloc (_2, _1);
//   return;

}
// ;; Function h (h, funcdef_no=4, decl_uid=2370, cgraph_uid=4, symbol_order=4)

// h (unsigned int n)
{
  long unsigned int _1;

//   <bb 2> [0.00%] [count: INV]:
  _1 = (long unsigned int) n_2(D);
  __builtin_aligned_alloc (_1, 2);
//   return;

}


