/* GCC Bug #98621 - ICE: x from g referenced in f
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98621
 */
/* { dg-do compile } */


int f() {
  int g(x) int x;   /* { dg-warning "old-style function definition" } */
  int a[x];         /* { dg-error "declaration for parameter" } */
}                    /* { dg-error "expected declaration specifiers" } */
// GCC additionally used to crash with:
// test.c:1:5: internal compiler error: x from g referenced in f
//     1 | int f() {
//       |     ^
// 0x1181546 walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
//     /home/alecop01/toolchain/src/gcc/gcc/tree.c:12095
// 0xaa2a6c walk_gimple_op(gimple*, tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
//     /home/alecop01/toolchain/src/gcc/gcc/gimple-walk.c:202
// 0xaa384e walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
//     /home/alecop01/toolchain/src/gcc/gcc/gimple-walk.c:596
// 0xaa3b2c walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
//     /home/alecop01/toolchain/src/gcc/gcc/gimple-walk.c:51
// 0xea87ca walk_body
//     /home/alecop01/toolchain/src/gcc/gcc/tree-nested.c:811
// 0xea8857 walk_function
//     /home/alecop01/toolchain/src/gcc/gcc/tree-nested.c:822
// 0xeb3bf1 lower_nested_functions(tree_node*)
//     /home/alecop01/toolchain/src/gcc/gcc/tree-nested.c:3698
// 0x8d883b cgraph_node::analyze()
//     /home/alecop01/toolchain/src/gcc/gcc/cgraphunit.c:676
// This was fixed on trunk (GCC 14); left here since GCC no longer ICEs but
// still needs to gracefully diagnose the invalid input above.

