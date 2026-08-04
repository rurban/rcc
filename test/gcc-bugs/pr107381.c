/* GCC Bug #107381 - ICE in scan_omp_target, at omp-low.cc:3126 since r10-2307-g8860d2706d9bd21d
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107381
 */
/* { dg-do compile } */


{
  int *x, *y;
  #pragma omp target data map(x, y) use_device_ptr(x, y)
    #pragma omp target is_device_ptr(x, y)
      {
//         *x = 42;
      }
}
//     5 |   #pragma omp target data map(x, y) use_device_ptr(x, y)
// 0xb80ef6 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xb81110 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xb81051 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xb81110 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xb80fb1 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xb81110 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)


