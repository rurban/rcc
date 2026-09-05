/* GCC Bug #83433 - missing signed integer overflow diagnostic for abs(INT_MIN)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83433
 */
/* { dg-do compile } */


#define INT_MIN (-__INT_MAX__ - 1)

int f (void)
{
  return -INT_MIN;
}

int g (void)
{
  return __builtin_abs (INT_MIN);
}
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - missing signed integer overflow diagnostic for abs(INT_MIN)"
//    href="show_bug.cgi?id=83433">pr83433</a>.c: In function ‘f’:
// <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - missing signed integer overflow diagnostic for abs(INT_MIN)"
//    href="show_bug.cgi?id=83433">pr83433</a>.c:5:10: warning: integer overflow in expression ‘-2147483648’ of type ‘int’ results in ‘-2147483648’ [-Woverflow]
//   return -INT_MIN;
//           ^
// ;; Function f (f, funcdef_no=0, decl_uid=1950, cgraph_uid=0, symbol_order=0)

// int f (void)
// {
//   <bb 2> [local count: 1073741825]:
//   return -2147483648;
//
// }
// ;; Function g (g, funcdef_no=3, decl_uid=1953, cgraph_uid=1, symbol_order=1)

// int g (void)
// {
//   <bb 2> [local count: 1073741825]:
//   return -2147483648;
//
// }


