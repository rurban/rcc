/* GCC Bug #110575 - gcc: internal compiler error: tree check: expected class 'type', have 'exceptional' (error_mark) in build_aligned_type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110575
 */
/* { dg-do compile } */


void __GIMPLE(ssa, startwith("slp")) bar() {
  a = __MEM<v16si, 32>((int *restrict)b(c));
}

//     1 | void __GIMPLE(ssa, startwith("slp")) bar() {
//     2 |   a = __MEM<v16si, 32>((int *restrict)b(c));
//     2 |   a = __MEM<v16si, 32>((int *restrict)b(c));
//     2 |   a = __MEM<v16si, 32>((int *restrict)b(c));
// 0x213983e internal_error(char const*, ...)
// 0x8959fe tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
// 0xa7aeae c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
// 0xa71add c_parse_file()
// 0xae1149 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


