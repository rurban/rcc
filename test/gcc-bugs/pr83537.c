/* GCC Bug #83537 - missing integer overflow in offsetof not diagnosed
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83537
 */
/* { dg-do compile } */


#include <stddef.h>
#include <stdint.h>

struct S { int i, a[]; };

// size_t f (void)
{
  return offsetof (struct S, a[PTRDIFF_MAX]);
}
// ;; Function f (f, funcdef_no=0, decl_uid=1930, cgraph_uid=0, symbol_order=0)

// f ()
{
//   <bb 2> [local count: 1073741825]:
  return 0;

}


