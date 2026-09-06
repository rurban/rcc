/* GCC Bug #110682 - ICE: internal compiler error: in gimplify_expr after error
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110682
 */
/* { dg-do compile } */


struct a {
  const signed char b;
};

void f(volatile struct a *c) {
  c - 0 % c->b;
  struct a c = {1};
}

//     7 |   struct a c = {1};
//     5 | void f(volatile struct a *c) {
//     6 |   c - 0 % c->b;
// 0x2150dee internal_error(char const*, ...)
// 0x9ce06c fancy_abort(char const*, int, char const*)
// 0xd7057a gimplify_stmt(tree_node**, gimple**)
// 0xd6d9ba gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xd7057a gimplify_stmt(tree_node**, gimple**)
// 0xd6e4eb gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xd7057a gimplify_stmt(tree_node**, gimple**)
// 0xd6d954 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xd7057a gimplify_stmt(tree_node**, gimple**)
// 0xd71a13 gimplify_body(tree_node*, bool)
// 0xd71e6f gimplify_function_tree(tree_node*)
// 0xbaebe7 cgraph_node::analyze()
// 0xbb2731 symbol_table::finalize_compilation_unit()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


