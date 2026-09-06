/* GCC Bug #125629 - ICE: in gimplify_expr, at gimplify.cc:21195 with __builtin_bswapg/__builtin_bireverseg() and ternary operator
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125629
 */
/* { dg-do compile } */


// Created <span class=""><a href="attachment.cgi?id=64640" name="attach_64640" title="reduced testcase">attachment 64640</a> <a href="attachment.cgi?id=64640&action=edit" title="reduced testcase">[details]</a></span>
// reduced testcase
// Compiler output:
// $ x86_64-pc-linux-gnu-gcc testcase.c 
// testcase.c: In function 'foo':
// testcase.c:6:28: internal compiler error: in gimplify_expr, at gimplify.cc:21195
//     6 |   __builtin_bitreverseg (c ? : 0u);
//       |                            ^
// 0x2d24d3d internal_error(char const*, ...)
//         /repo/gcc-trunk/gcc/diagnostic-global-context.cc:787
// 0xe3d2c7 fancy_abort(char const*, int, char const*)
//         /repo/gcc-trunk/gcc/diagnostics/context.cc:1813
// 0x81aef4 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:21195
// 0x12a501b gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:21165
// 0x12c5d04 gimplify_cond_expr
//         /repo/gcc-trunk/gcc/gimplify.cc:5513
// 0x12a6679 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20294
// 0x12a7ca6 gimplify_stmt(tree_node**, gimple**)
//         /repo/gcc-trunk/gcc/gimplify.cc:8538
// 0x12c5b6f gimplify_cond_expr
//         /repo/gcc-trunk/gcc/gimplify.cc:5473
// 0x12a6679 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20294
// 0x12bb561 gimplify_expr
//         /repo/gcc-trunk/gcc/gimplify.cc:21449
// 0x12bb561 gimplify_arg(tree_node**, gimple**, unsigned long, bool)
//         /repo/gcc-trunk/gcc/gimplify.cc:3752
// 0x12bbf20 gimplify_call_expr
//         /repo/gcc-trunk/gcc/gimplify.cc:4766
// 0x12a6c2d gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20309
// 0x12a7ca6 gimplify_stmt(tree_node**, gimple**)
//         /repo/gcc-trunk/gcc/gimplify.cc:8538
// 0x12a92a2 gimplify_bind_expr
//         /repo/gcc-trunk/gcc/gimplify.cc:1529
// 0x12a64de gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20541
// 0x12cafd6 gimplify_stmt(tree_node**, gimple**)
//         /repo/gcc-trunk/gcc/gimplify.cc:8538
// 0x12cafd6 gimplify_body(tree_node*, bool)
//         /repo/gcc-trunk/gcc/gimplify.cc:21647
// 0x12cb485 gimplify_function_tree(tree_node*)
//         /repo/gcc-trunk/gcc/gimplify.cc:21856
// 0x106eee7 cgraph_node::analyze()
//         /repo/gcc-trunk/gcc/cgraphunit.cc:691
// /repo/gcc-trunk/binary-trunk-20260605172729-g517d45af94f-checking-yes-rtl-df-extra-nobootstrap-amd64/bin/../libexec/gcc/x86_64-pc-linux-gnu/17.0.0/cc1 -quiet -iprefix /repo/gcc-trunk/binary-trunk-20260605172729-g517d45af94f-checking-yes-rtl-df-extra-nobootstrap-amd64/bin/../lib/gcc/x86_64-pc-linux-gnu/17.0.0/ testcase.c -quiet -dumpdir a- -dumpbase testcase.c -dumpbase-ext .c -mtune=generic -march=x86-64 -o /tmp/cciiqyMs.s
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.


