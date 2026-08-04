/* GCC Bug #117380 - ICE: tree check: expected class 'type', have 'exceptional' (error_mark) in get_unwidened, at tree.cc:8019
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117380
 */
/* { dg-do compile } */


int a, b;
void c()
{
    a >> b + 503359447364223024; 
}
__attribute__((vector_size(4))) b;

//     6 | __attribute__((vector_size(4))) b;
// mutant.c:6:33: error: conflicting types for 'b'; have '__vector(1) int'
//     1 | int a, b;
// mutant.c:4:7: internal compiler error: tree check: expected class 'type', have 'exceptional' (error_mark) in get_unwidened, at tree.cc:8019
//     4 |     a >> b + 503359447364223024;
// 0x5071bcf diagnostic_context::report_diagnostic(diagnostic_info*)
// 0x50724a1 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, int, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x50924c7 internal_error(char const*, ...)
// 0x26856d2 tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
// 0xd4a676 tree_class_check(tree_node*, tree_code_class, char const*, int, char const*)
// 0x267dac4 get_unwidened(tree_node*, tree_node*)
// 0x126cd01 convert_to_integer(tree_node*, tree_node*)
// 0xe4febf convert(tree_node*, tree_node*)
// 0xfd948f c_gimplify_expr(tree_node**, gimple**, gimple**)
// 0x1727e44 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x16d5172 gimplify_stmt(tree_node**, gimple**)
// 0x1729b35 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x16d5172 gimplify_stmt(tree_node**, gimple**)
// 0x17307ed gimplify_body(tree_node*, bool)
// 0x17319cc gimplify_function_tree(tree_node*)
// 0x124b024 cgraph_node::analyze()
// 0x125521b symbol_table::finalize_compilation_unit()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


