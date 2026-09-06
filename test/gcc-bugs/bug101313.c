/* GCC Bug #101313 - ICE: tree check: expected class ‘type’, have ‘exceptional’ (error_mark) in count_type_elements, at expr.c:6273
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101313
 */
/* { dg-do compile } */


struct struct_A
{
  union bar
  {
    enum test x;
  } var;
};

struct struct_B
{
  int x;
  struct struct_A a;
};
int foo()
{
  struct struct_A a = {0};
  struct struct_B b = {2, a};
}
// Copyright (C) 2021 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//     7 |     enum test x;
// test.c:21:19: internal compiler error: tree check: expected class ‘type’, have ‘exceptional’ (error_mark) in count_type_elements, at expr.c:6273
//    21 |   struct struct_B b = {2, a};
// 0x7a7886 tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
// 0x6c0d82 tree_class_check(tree_node*, tree_code_class, char const*, int, char const*)
// 0xbbf26f gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xbc2bf6 gimplify_stmt(tree_node**, gimple**)
// 0xbcc38d gimplify_and_add(tree_node*, gimple**)
// 0xbbf566 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xbc2bf6 gimplify_stmt(tree_node**, gimple**)
// 0xbc01e3 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xbc2bf6 gimplify_stmt(tree_node**, gimple**)
// 0xbc0eae gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
// 0xbd8759 gimplify_stmt(tree_node**, gimple**)
// 0xbd8759 gimplify_body(tree_node*, bool)
// Copyright (C) 2021 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//     7 |     enum test x;


