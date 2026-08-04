/* GCC Bug #67872 - missing -Warray-bounds warning, bogus -Wmaybe-uninitialized
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67872
 */
/* { dg-do compile } */


struct A {
    int a[3];
} a;

int foo (void)
{
    return __builtin_offsetof (struct A, a[4]);
}
// This (otherwise untested) patch fixes it and makes the function diagnose this case.  (The comment about flexible array members above the block suggests that the patch might need tweaking to avoid false positives for such constructs.)
// --- a/gcc/c-family/c-common.c
// +++ b/gcc/c-family/c-common.c
// @@ -10623,7 +10623,8 @@ fold_offsetof_1 (tree expr)
//                      man's flexible array member with a very permissive
//                      definition thereof.  */
                  if (TREE_CODE (v) == ARRAY_REF
// -                     || TREE_CODE (v) == COMPONENT_REF)
// +                     || TREE_CODE (v) == COMPONENT_REF
// +                     || TREE_CODE (v) == INDIRECT_REF)
//                     warning (OPT_Warray_bounds,
//                              "index %E denotes an offset "
//                              "greater than size of %qT",


