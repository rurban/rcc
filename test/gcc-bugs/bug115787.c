/* GCC Bug #115787 - [GIMPLE-FE] ICE: in gimple_build_switch_nlabels, at gimple.cc:807
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115787
 */
/* { dg-do compile } */


__GIMPLE a(argc) {
  switch (argc_20) {
  case 1:
//     b;
  }
// *******************************************************************************
// Command Lines:
// test035.c:1:1: error: ‘__GIMPLE’ only valid with ‘-fgimple’
//     1 | __GIMPLE a(argc) {
//       | ^~~~~~~~
// test035.c:1:10: error: return type defaults to ‘int’ [-Wimplicit-int]
//     1 | __GIMPLE a(argc) {
//       |          ^
// test035.c: In function ‘a’:
// test035.c:1:10: error: type of ‘argc’ defaults to ‘int’ [-Wimplicit-int]
// test035.c:5:3: internal compiler error: in gimple_build_switch_nlabels, at gimple.cc:807
//     5 |   }
//       |   ^
// 0x7e5713 gimple_build_switch_nlabels(unsigned int, tree_node*, tree_node*)
//         /root/gdbtest/gcc/obj/../gcc/gcc/gimple.cc:807
// 0x7e5713 gimple_build_switch_nlabels(unsigned int, tree_node*, tree_node*)
//         /root/gdbtest/gcc/obj/../gcc/gcc/gimple.cc:804
// 0xd09040 gimple_build_switch(tree_node*, tree_node*, vec<tree_node*, va_heap, vl_ptr> const&)
//         /root/gdbtest/gcc/obj/../gcc/gcc/gimple.cc:827
// 0xa64ac1 c_parser_gimple_switch_stmt
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/gimple-parser.cc:2552
// 0xa64ac1 c_parser_gimple_compound_statement
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/gimple-parser.cc:440
// 0xa64d61 c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/gimple-parser.cc:253
// 0xa505a4 c_parser_declaration_or_fndef
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/c-parser.cc:3011
// 0xa5af4b c_parser_external_declaration
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/c-parser.cc:2046
// 0xa5b935 c_parser_translation_unit
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/c-parser.cc:1900
// 0xa5b935 c_parse_file()
//         /root/gdbtest/gcc/obj/../gcc/gcc/c/c-parser.cc:26889
// 0xad3a51 c_common_parse_file()
//         /root/gdbtest/gcc/obj/../gcc/gcc/c-family/c-opts.cc:1311
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.
// *******************************************************************************
// Also ICE on trunk, compiler explorer:<a href="https://godbolt.org/z/5sYY7a47c">https://godbolt.org/z/5sYY7a47c</a>
// *******************************************************************************


