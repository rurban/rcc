/* GCC Bug #119993 - ICE: populationSize.0 from writePopulation referenced in readPopulation
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119993
 */
/* { dg-do compile } */


#include <stdlib.h>
int main(int argc, char** argv)
{
// 	FILE* input;
 long i,j;

	if(input==stdin) {
#include <stdio.h>

long populationSize = 10;

typedef struct _Population
{
 double fitness[populationSize];
 char genome[2][1];
}Population;

// Population readPopulation(FILE* input)
{
 Population population;
 return population;
}

void writePopulation(FILE* output, Population population)
{
}
//     2 | int main(int argc, char** argv)
// 0x2ea0055 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x2eb7026 internal_error(char const*, ...)
// 0x1a98d5c walk_tree_1(tree_node**, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*, tree_node* (*)(tree_node**, int*, tree_node* (*)(tree_node**, int*, void*), void*, hash_set<tree_node*, false, default_hash_traits<tree_node*> >*))
// 0x135c020 walk_gimple_op(gimple*, tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x135c33e walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x135c575 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x135c439 walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x135c575 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x135c3fd walk_gimple_stmt(gimple_stmt_iterator*, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x135c575 walk_gimple_seq_mod(gimple**, tree_node* (*)(gimple_stmt_iterator*, bool*, walk_stmt_info*), tree_node* (*)(tree_node**, int*, void*), walk_stmt_info*)
// 0x17c71e8 lower_nested_functions(tree_node*)
// 0x117bca4 cgraph_node::analyze()
// 0x117f241 symbol_table::finalize_compilation_unit()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


