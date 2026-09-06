/* GCC Bug #108423 - ICE in make_ssa_name_fn with VLA types in arguments and inlining since r12-5338-g4e6bf0b9dd5585df
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108423
 */
/* { dg-do compile } */


int f (int n, int (**(*a)(void))[n])
{
  return (*a())[0];
}
int g ()
{
  int m = 3;
  int (*a[m])(void);
  return f(m, &a);
}
//     3 |   return (*a())[0];
//     9 |   return f(m, &a);
//       |               int (* (*)[m])(void)
// z1.c:1:24: note: expected 'int (** (*)(void))[n]' but argument is of type 'int (* (*)[m])(void)'
//     1 | int f (int n, int (**(*a)(void))[n])
//     9 |   return f(m, &a);
// 0x11b1edd make_ssa_name_fn(function*, tree_node*, gimple*, unsigned int)
// 0xfa5717 copy_tree_body_r(tree_node**, int*, void*)
// 0x1271d23 walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
// 0x1272784 walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
// 0xf9fa28 remap_type(tree_node*, copy_body_data*)
// 0xf9fa28 remap_type(tree_node*, copy_body_data*)
// 0xf9fa28 remap_type(tree_node*, copy_body_data*)
// 0xf9fa28 remap_type(tree_node*, copy_body_data*)
// 0xf9fa28 remap_type(tree_node*, copy_body_data*)
// 0xfaf1e9 optimize_inline_calls(tree_node*)


