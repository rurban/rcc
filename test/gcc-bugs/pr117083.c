/* GCC Bug #117083 - ICE: in get_expr_operands, at tree-ssa-operands.cc:939
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117083
 */
/* { dg-do compile } */


void foo (unsigned *);
void bar (const unsigned char *block)
{
  unsigned buf[4];
  __builtin_memcpy (buf +  0, block +  0, 4);
  __builtin_memcpy (buf +  1, block +  4, 4);
  __builtin_memcpy (buf +  2, block +  8, 4);
  __builtin_memcpy (buf +  3, block + 12, 4);
  foo (buf);
}

void __GIMPLE (ssa,guessed_local(1073741824))
// bar (const unsigned char * block)
{
  vector(4) unsigned int vect__3.5;
  unsigned int buf[4];

//   __BB(2,guessed_local(1073741824)):
  vect__3.5_14 = __MEM <vector(4) unsigned int, 8> ((char * {ref-all})block_2(D));
  __MEM <vector(4) unsigned int> ((char * {ref-all})&buf) = vect__3.5_14;
  foo (&buf);
  buf ={v} _Literal (unsigned int[4]) {CLOBBER(eol)};
//   return;
}

//    12 | void __GIMPLE (ssa,guessed_local(1073741824))
//    13 | bar (const unsigned char * block)
// bugreport_0_1.c:2:6: note: previous definition of ‘bar’ with type ‘void(const unsigned char *)’
//     2 | void bar (const unsigned char *block)
//    15 |   vector(4) unsigned int vect__3.5;
//    15 |   vector(4) unsigned int vect__3.5;
//    16 |   unsigned int buf[4];
//    19 |   vect__3.5_14 = __MEM <vector(4) unsigned int, 8> ((char * {ref-all})block_2(D));
//    20 |   __MEM <vector(4) unsigned int> ((char * {ref-all})&buf) = vect__3.5_14;
// bugreport_0_1.c:20:42: error: expected ‘)’ before ‘{’ token
//    20 |   __MEM <vector(4) unsigned int> ((char * {ref-all})&buf) = vect__3.5_14;
//       |                                          )
// bugreport_0_1.c:20:54: error: ‘buf’ undeclared (first use in this function)
//    20 |   __MEM <vector(4) unsigned int> ((char * {ref-all})&buf) = vect__3.5_14;
//    20 |   __MEM <vector(4) unsigned int> ((char * {ref-all})&buf) = vect__3.5_14;
// bugreport_0_1.c:22:8: error: expected expression before ‘{’ token
//    22 |   buf ={v} _Literal (unsigned int[4]) {CLOBBER(eol)};
// unhandled expression in get_expr_operands():

//    24 | }
// 0x260066e internal_error(char const*, ...)
// 0x9dc7a1 fancy_abort(char const*, int, char const*)
// 0x12fc701 operands_scanner::get_expr_operands(tree_node**, int)
// 0x12fcbb0 operands_scanner::parse_ssa_operands()
// 0x12fd9bb operands_scanner::build_ssa_operands()
// 0x12fdaa3 update_stmt_operands(function*, gimple*)
// 0xd69507 update_stmt_if_modified(gimple*)
// 0xd69507 update_stmt_if_modified(gimple*)
// 0xd69507 update_modified_stmts(gimple*)
// 0xd69619 gsi_insert_seq_after(gimple_stmt_iterator*, gimple*, gsi_iterator_update)
// 0xa9f9b8 c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
// 0xa95bbd c_parse_file()
// 0xb11cf1 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


