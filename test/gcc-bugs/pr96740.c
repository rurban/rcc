/* GCC Bug #96740 - frexp, modf, and remquo missing attribute nonnull
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96740
 */


double f0 (double x, int *p)
{
  double y = __builtin_frexp (x, p);
  if (!p) __builtin_abort ();      // not eliminated
  return y;

}
double f2 (const char *s)
{ 
  double x = __builtin_nan (s);
  if (!s) __builtin_abort ();      // eliminated
  return x;
}

// ;; Function f0 (f0, funcdef_no=0, decl_uid=1932, cgraph_uid=1, symbol_order=0)

// f0 (double x, int * p)
{
  double y;

  y_5 = __builtin_frexp (x_2(D), p_3(D));
  if (p_3(D) == 0B)
    goto <bb 3>; [0.00%]
    goto <bb 4>; [100.00%]

  __builtin_abort ();

  return y_5;

}
// ;; Function f2 (f2, funcdef_no=1, decl_uid=1936, cgraph_uid=2, symbol_order=1)

// f2 (const char * s)
{
  double x;

  x_2 = __builtin_nan (s_1(D)); [tail call]
  return x_2;

}


