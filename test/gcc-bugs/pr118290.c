/* GCC Bug #118290 - ICE: argc.1 from nested referenced in nested
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118290
 */
/* { dg-do compile } */


#include <stddef.h>

// main(int argc, char **argv)
{
  struct s { int a; char b[argc]; }

int nested (struct s x) { return x.a + sizeof(x); }

int nested (struct s x) { return x.a + sizeof(x); }

int main(int argc, char **argv) { 
  struct s { char b[argc]; }

int nested (struct s x) { return 0; }

int nested (struct s x) { return x.a + sizeof(x); }
// 16567.c:10:1: error: expected ‘;’, identifier or ‘(’ before ‘int’
//    10 | int nested (struct s x) { return x.a + sizeof(x); }
//    12 | int nested (struct s x) { return x.a + sizeof(x); }
// 16567.c:10:5: note: previous definition of ‘nested’ with type ‘int(struct s)’
//    10 | int nested (struct s x) { return x.a + sizeof(x); }
//    14 | int main(int argc, char **argv) {
// 16567.c:17:1: error: expected ‘;’, identifier or ‘(’ before ‘int’
//    17 | int nested (struct s x) { return 0; }
//    17 | int nested (struct s x) { return 0; }
//    19 | int nested (struct s x) { return x.a + sizeof(x); }
// 16567.c:17:5: note: previous definition of ‘nested’ with type ‘int(struct s)’
//    17 | int nested (struct s x) { return 0; }
//    19 | int nested (struct s x) { return x.a + sizeof(x); }
//    19 | int nested (struct s x) { return x.a + sizeof(x); }
//    14 | int main(int argc, char **argv) {
//    19 | int nested (struct s x) { return x.a + sizeof(x); }
//     6 | main(int argc, char **argv)
//     6 | main(int argc, char **argv)
// 0x26e831f internal_error(char const*, ...)
// 0x1532b0c walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
// 0xe0a405 walk_gimple_op(gimple*, tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xe0a713 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xe0a915 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xe0a7b1 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0xe0a915 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x1251b51 lower_nested_functions(tree_node*)
// 0xc3d132 cgraph_node::analyze()
// 0xc40f2d symbol_table::finalize_compilation_unit()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


