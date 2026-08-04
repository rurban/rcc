/* GCC Bug #111911 - ICE with integer overflow converting to _Bool
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111911
 */
/* { dg-do compile } */


int main(void) {
  if ((((!((_Bool)(__INT_MAX__ + 1))) /
//           (!((_Bool)(__INT_MAX__ + 1))))))
          ;
  return 1;
}
// ```
// Complier Explorer: <a href="https://gcc.godbolt.org/z/W59roeYKP">https://gcc.godbolt.org/z/W59roeYKP</a>
// The full stack dump:
// ```
// <source>: In function 'main':
// <source>:2:32: warning: integer overflow in expression of type 'int' results in '-2147483648' [-Woverflow]
//     2 |   if ((((!((_Bool)(__INT_MAX__ + 1))) /
//       |                                ^
// <source>:3:34: warning: integer overflow in expression of type 'int' results in '-2147483648' [-Woverflow]
//     3 |           (!((_Bool)(__INT_MAX__ + 1))))))
//       |                                  ^
// cc1: warning: division by zero [-Wdiv-by-zero]
// <source>:2:10: internal compiler error: in gimplify_expr, at gimplify.cc:17510
//     2 |   if ((((!((_Bool)(__INT_MAX__ + 1))) /
//       |          ^
// 0x231f49e internal_error(char const*, ...)
// 	???:0
// 0xa00958 fancy_abort(char const*, int, char const*)
// 	???:0
// 0xdbb856 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbb3dc gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbb856 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbb3dc gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbb3dc gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbbf48 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbec8a gimplify_stmt(tree_node**, gimple**)
// 	???:0
// 0xdbcc33 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbec8a gimplify_stmt(tree_node**, gimple**)
// 	???:0
// 0xdbbcdf gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbec8a gimplify_stmt(tree_node**, gimple**)
// 	???:0
// 0xdbcc33 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 	???:0
// 0xdbec8a gimplify_stmt(tree_node**, gimple**)
// 	???:0
// 0xdc0cd3 gimplify_body(tree_node*, bool)
// 	???:0
// 0xdc112f gimplify_function_tree(tree_node*)
// 	???:0
// 0xbea6e7 cgraph_node::analyze()
// 	???:0
// 0xbee231 symbol_table::finalize_compilation_unit()
// 	???:0
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.
// ```


