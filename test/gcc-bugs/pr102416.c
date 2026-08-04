/* GCC Bug #102416 - ICE in gimplify_expr, at gimplify.c:15570 since r12-1108-g9a5de4d5af1c10a8
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102416
 */
/* { dg-do compile } */


void *f ()
{
  #pragma omp task affinity(f()[2])
  ;
}
void f ()
{
  struct S *x;
  #pragma omp task affinity(*x)
  ;
}
//     3 |   #pragma omp task affinity(f()[2])
//     3 |   #pragma omp task affinity(f()[2])
// 0x960cc4 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x95dbeb gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x960cf8 gimplify_stmt(tree_node**, gimple**)
// 0x95ecca gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x960cf8 gimplify_stmt(tree_node**, gimple**)
// 0x961d6b gimplify_body(tree_node*, bool)
// 0x9621bf gimplify_function_tree(tree_node*)
// 0x7eaf97 cgraph_node::analyze()
// 0x7ee2ad symbol_table::finalize_compilation_unit()


