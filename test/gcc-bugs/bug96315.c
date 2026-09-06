/* GCC Bug #96315 - ICE with variable size struct and openmp
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96315
 */
/* { dg-do compile } */


int main_njack;
void main() {
  struct {
    double aic[main_njack];
  } res;
#pragma omp parallel
  for (;;)
//     res;
}
// 0x72ef57 tree_contains_struct_check_failed(tree_node const*, tree_node_structure_enum, char const*, int, char const*)
// 0x6d80a8 contains_struct_check(tree_node*, tree_node_structure_enum, char const*, int, char const*)
// 0xe410d0 remap_type(tree_node*, copy_body_data*)
// 0xe410d0 remap_type(tree_node*, copy_body_data*)
// 0xaeefaa walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaef170 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaef0a1 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaef170 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaef061 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xaef170 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)


