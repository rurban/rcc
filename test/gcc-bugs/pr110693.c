/* GCC Bug #110693 - internal compiler error: Segmentation fault with invalid gimple
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110693
 */
/* { dg-do compile } */


char global;

void bar (void);

void __GIMPLE (ssa)
// foo (char * p)
{
//   __BB(2):
  if (p_2(D) == _Literal (char *)&global[2])
    goto __BB3;
    goto __BB4;

//   __BB(3):
  bar ();
  goto __BB4;

//   __BB(4):
//   return;
}
//     5 | void __GIMPLE (ssa)
//     9 |   if (p_2(D) == _Literal (char *)&global[2])
//     9 |   if (p_2(D) == _Literal (char *)&global[2])
//    20 | }
// 0x2150dee internal_error(char const*, ...)
// 0xb510f3 unchecked_make_edge(basic_block_def*, basic_block_def*, int)
// 0xa7d754 c_parser_parse_gimple_body(c_parser*, char*, c_declspec_il, profile_count)
// 0xa7477d c_parse_file()
// 0xae3e59 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


