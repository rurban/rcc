/* GCC Bug #89968 - attribute packed fails to reduce char vector member alignment
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89968
 */


struct S
{
  char c;
  __attribute__ ((aligned (64), packed, vector_size (1024))) char v;
};

int f (void) { return sizeof (struct S); }
int g (void) { return __alignof__ (struct S); }

//     4 |   __attribute__ ((aligned (64), packed, vector_size (1024))) char v;

// ;; Function f (f, funcdef_no=0, decl_uid=1909, cgraph_uid=1, symbol_order=0)

// f ()
{
  return 2048;   ;; Expected 1088

}
// ;; Function g (g, funcdef_no=1, decl_uid=1912, cgraph_uid=2, symbol_order=1)

// g ()
{
  return 1024;   ;; Expected 64

}


