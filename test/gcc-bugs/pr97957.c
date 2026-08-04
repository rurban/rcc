/* GCC Bug #97957 - ICE in init_dynamic_diag_info, at c-family/c-format.c:5024
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97957
 */
/* { dg-do compile } */


typedef long __gcc_host_wide_int__;
typedef long __gcc_host_wide_int__;
__attribute__ ((__format__ (__gcc_diag__, 1, 2)))
void f () {}
//     4 | void f () {}
// 0x6d1b97 handle_format_attribute(tree_node**, tree_node*, tree_node*, int, bool*)
// 0x62a101 decl_attributes(tree_node**, tree_node*, int, tree_node*)
// 0x641eef start_function(c_declspecs*, c_declarator*, tree_node*)
// 0x691569 c_parse_file()
// 0x6e0772 c_common_parse_file()


