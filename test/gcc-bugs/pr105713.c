/* GCC Bug #105713 - [gimplefe] need a way to specify TREE_ADDRESSABLE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105713
 */


typedef char v2qi __attribute__((vector_size(2)));

void __GIMPLE (ssa,startwith("optimized"))
// foo (__complex__ char c)
{
//   __BB(2):
  __MEM <v2qi, 8> (&c) = _Literal (v2qi) { _Literal (char) 0, _Literal (char) 0 };
//   return;
}
// but here 'c' ends up TREE_ADDRESSABLE while with the original setup it is not.
// That's because the C FE parsing marks 'c' addressable when parsing &c.  The
// GIMPLE FE should fix this up on the optimistic side somehow and allow
// specifying TREE_ADDRESSABLE at the declaration.


