/* GCC Bug #109618 - ICE: tree check: expected class ‘type’, have ‘exceptional’ (error_mark) in generic_simplify_CONVERT_EXPR, at generic-match.cc since r12-3278-g823685221de986
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109618
 */
/* { dg-do compile } */


int foo()
{
  const unsigned int var_1 = 2;

  char var_5[var_1];

  int var_1[10];

  return sizeof(var_5);
}
// Copyright (C) 2023 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//     8 |   int var_1[10];
//     4 |   const unsigned int var_1 = 2;
// test.c :10:3: internal compiler error: tree check: expected class ‘type’, have ‘exceptional’ (error_mark) in generic_simplify_CONVERT_EXPR, at generic-match.cc:28499
//    10 |   return sizeof(var_5);
// 0x840637 tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
// 0x8abac0 tree_class_check(tree_node*, tree_code_class, char const*, int, char const*)
// 0xbf6848 fold_unary_loc(unsigned int, tree_code, tree_node*, tree_node*)
// 0xbf8079 fold_build1_loc(unsigned int, tree_code, tree_node*, tree_node*)
// 0x9429bb c_expr_sizeof_expr(unsigned int, c_expr)
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


