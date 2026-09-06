/* GCC Bug #107557 - ICE -fsanitize=undefined and VLA as argument type to a function
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107557
 */
/* { dg-do compile } */


int c[1][3*2];
int f(int * const m, int (**v)[*m * 2])
{
  return &(c[0][*m]) == &((*v)[0][*m]);
}
int test(int n, int (*(*fn)(void))[n])
{
  return (*fn())[0];
}
int main()
{
  int m = 3;
  int (*d)[3*2] = c;
  int (*fn[m])(void);
  return f(&m, &d) + test(m, &fn);
}
//    15 |   return f(&m, &d) + test(m, &fn);
//       |                              int (* (*)[(((m) <= 0 ? __builtin___ubsan_handle_vla_bound_not_positive(&*.Lubsan_data0, (unsigned int)(m)) : (void)0, (m))) - 1])(void)
// z1.c:6:25: note: expected 'int (* (*)(void))[(n) - 1]' but argument is of type 'int (* (*)[(((m) <= 0 ? __builtin___ubsan_handle_vla_bound_not_positive(&*.Lubsan_data0, (unsigned int)(m)) : (void)0, (m))) - 1])(void)'
//     6 | int test(int n, int (*(*fn)(void))[n])
//    15 |   return f(&m, &d) + test(m, &fn);
// 0xf0e385 make_ssa_name_fn(function*, tree_node*, gimple*, unsigned int)
// 0xd74257 copy_tree_body_r(tree_node**, int*, void*)
// 0xfa17b2 walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
// 0xd70498 remap_type(tree_node*, copy_body_data*)
// 0xd70498 remap_type(tree_node*, copy_body_data*)
// 0xd70498 remap_type(tree_node*, copy_body_data*)
// 0xd70498 remap_type(tree_node*, copy_body_data*)
// 0xd70498 remap_type(tree_node*, copy_body_data*)
// 0xd7af89 optimize_inline_calls(tree_node*)
// 0x190544e early_inliner(function*)


