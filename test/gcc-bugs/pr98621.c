/* GCC Bug #98621 - ICE: x from g referenced in f
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98621
 */
/* { dg-do compile } */


int f() {
  int g(x) int x;
  int a[x];
}
// test.c:4:1: error: expected declaration specifiers before '}' token
//     4 | }
//     3 |   int a[x];
//     4 | }
//     1 | int f() {
// 0x1181546 walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
// 0xaa2a6c walk_gimple_op(gimple*, tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaa384e walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaa3b2c walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaa38ca walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaa3b2c walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xeb3bf1 lower_nested_functions(tree_node*)
// 0x8d883b cgraph_node::analyze()
// 0x8dcc87 symbol_table::finalize_compilation_unit()


