/* GCC Bug #112504 - ICE in operand_equal_p, at fold-const.cc:3313 with function return struct containing an array and comparing the array to the same thing
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112504
 */
/* { dg-do compile } */


struct s {
  char c[1];
};
extern struct s foo(void);
void bar(void) {
//   foo().c == foo().c;
}

//     6 |   foo().c == foo().c;
// 0x238b15e internal_error(char const*, ...)
// 0xa11270 fancy_abort(char const*, int, char const*)
// 0xd0bd67 operand_compare::operand_equal_p(tree_node const*, tree_node const*, unsigned int)
// 0xd0b6e4 operand_compare::operand_equal_p(tree_node const*, tree_node const*, unsigned int)
// 0xd25837 fold_binary_loc(unsigned int, tree_code, tree_node*, tree_node*, tree_node*)
// 0xd45c84 fold(tree_node*)
// 0xac0aef c_fully_fold(tree_node*, bool, bool*, bool)
// 0xa574e3 c_process_expr_stmt(unsigned int, tree_node*)
// 0xa57711 c_finish_expr_stmt(unsigned int, tree_node*)
// 0xabd40d c_parse_file()
// 0xb30909 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


