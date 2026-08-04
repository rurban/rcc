/* GCC Bug #119991 - internal compiler error: tree check: expected class 'type', have 'exceptional' (error_mark) in create_tmp_from_val, at gimplify.cc:621
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119991
 */
/* { dg-do compile } */


float * fitness_table = NULL;
unsigned char * population1 = NULL;
float fitness1(unsigned char * dna){
    int i;
    float fitness = 0;
    for (i = 0; i < 400; ++i){
        fitness += fitness1_single(dna + i * 2, fitness_table[i]);
    }
    return fitness;
}
float fitness_table [400] =

{

};
//     7 |         fitness += fitness1_single(dna + i * 2, fitness_table[i]);
// 0x2ea0055 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x2eb7026 internal_error(char const*, ...)
// 0x95a5ad tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
// 0x1372490 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x1371e96 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x13745da gimplify_stmt(tree_node**, gimple**)
// 0x13720da gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x13745da gimplify_stmt(tree_node**, gimple**)
// 0x1371933 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x13745da gimplify_stmt(tree_node**, gimple**)
// 0x1371933 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x13745da gimplify_stmt(tree_node**, gimple**)
// 0x13720da gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0x137a926 gimplify_body(tree_node*, bool)
// 0x137adec gimplify_function_tree(tree_node*)
// 0x117bec7 cgraph_node::analyze()
// 0x117f241 symbol_table::finalize_compilation_unit()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


