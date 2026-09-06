/* GCC Bug #111836 - ICE with undefined types and undefined variables with _GIMPLE and starting at ssa
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111836
 */
/* { dg-do compile } */


void __GIMPLE (ssa) t () {
//   i = __PHI (__BB6: _Literal (i32) 1, __BB4: j);
}

//     1 | void __GIMPLE (ssa) t () {
//     2 |   i = __PHI (__BB6: _Literal (i32) 1, __BB4: j);
//     2 |   i = __PHI (__BB6: _Literal (i32) 1, __BB4: j);
//     2 |   i = __PHI (__BB6: _Literal (i32) 1, __BB4: j);
// unhandled expression in get_expr_operands():

//     3 | }
// 0x230184e internal_error(char const*, ...)
// 0x9fb842 fancy_abort(char const*, int, char const*)
// 0x12e301d operands_scanner::get_expr_operands(tree_node**, int)
// 0x12e35c2 operands_scanner::parse_ssa_operands()
// 0x12e43ea operands_scanner::build_ssa_operands()
// 0x12e4634 update_stmt_operands(function*, gimple*)
// 0xd62007 update_modified_stmts(gimple*)
// 0xd620e9 gsi_insert_seq_after(gimple_stmt_iterator*, gimple*, gsi_iterator_update)
// 0xaad40e c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
// 0xaa3dfd c_parse_file()
// 0xb17139 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Copyright (C) 2023 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


