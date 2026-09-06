/* GCC Bug #92249 - ICE in c_parser_gimple_compound_statement w/ GIMPLE testcases
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92249
 */
/* { dg-do compile } */


void __GIMPLE (ssa)
// foo (void)
{
//   return;
}

//     4 |   return;
// 0x7b7ede c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
// 0x7b0852 c_parse_file()
// 0x8075f5 c_common_parse_file()
//     5 | int __GIMPLE (ssa,guessed_local(118111600),startwith("dce3"))
//    15 |   __SIZETYPE__ _7;
//    16 |   __SIZETYPE__ _8;
//    19 |   __SIZETYPE__ _11;
//    20 |   __SIZETYPE__ _12;
//    23 |   __SIZETYPE__ _15;
//    24 |   __SIZETYPE__ _16;
//    48 |   _9 = x_23(D) + _8;
// 0x6c3725 build2(tree_code, tree_node*, tree_node*, tree_node*)
// 0x7b7ede c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
// 0x7b0852 c_parse_file()
// 0x8075f5 c_common_parse_file()


