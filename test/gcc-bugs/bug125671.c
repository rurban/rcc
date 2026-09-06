/* GCC Bug #125671 - ICE: SIGSEGV in contains_struct_check (tree.h:3937) with __builtin_stdc_rotate_left()
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125671
 */
/* { dg-do compile } */


// Created <span class=""><a href="attachment.cgi?id=64675" name="attach_64675" title="reduced testcase">attachment 64675</a> <a href="attachment.cgi?id=64675&action=edit" title="reduced testcase">[details]</a></span>
// reduced testcase
// Compiler output:
// $ x86_64-pc-linux-gnu-gcc testcase.c -wrapper valgrind,-q
// ==2897584== Invalid read of size 2
// ==2897584==    at 0x11A0140: contains_struct_check (tree.h:3937)
// ==2897584==    by 0x11A0140: tree_invalid_nonnegative_p (fold-const.cc:14794)
// ==2897584==    by 0x11A0140: tree_expr_nonnegative_p (fold-const.cc:14908)
// ==2897584==    by 0x11A0140: tree_expr_nonnegative_p(tree_node*, int) (fold-const.cc:14851)
// ==2897584==    by 0x20D1EC7: generic_simplify_COND_EXPR(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*) (generic-match-4.cc:16704)
// ==2897584==    by 0x11CABDC: fold_ternary_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*) (fold-const.cc:12581)
// ==2897584==    by 0x11CCD17: fold_build3_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*) (fold-const.cc:13673)
// ==2897584==    by 0x11B18F1: fold_binary_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*) (fold-const.cc:10857)
// ==2897584==    by 0x11B934E: fold_build2_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*) (fold-const.cc:13613)
// ==2897584==    by 0x107A2FE: do_narrow(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*, unsigned int, unsigned int, bool) [clone .constprop.0] (convert.cc:453)
// ==2897584==    by 0x107B6AD: convert_to_integer_1(tree_node*, tree_node*, bool) (convert.cc:864)
// ==2897584==    by 0xEB32B3: c_convert(tree_node*, tree_node*, bool) (c-convert.cc:148)
// ==2897584==    by 0xF706C8: c_gimplify_expr(tree_node**, gimple**, gimple**) (c-gimplify.cc:943)
// ==2897584==    by 0x12A566D: gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int) (gimplify.cc:20280)
// ==2897584==    by 0x12A5D84: gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int) (gimplify.cc:21201)
// ==2897584==  Address 0x0 is not stack'd, malloc'd or (recently) free'd
// ==2897584== 
// testcase.c: In function 'foo':
// testcase.c:7:8: internal compiler error: Segmentation fault
//     7 |    a = __builtin_stdc_rotate_left(b, 0x856bbcd3eebcf2a9 << a);
//       |        ^~~~~~~~~~~~~~~~~~~~~~~~~~
// 0x2d275fd internal_error(char const*, ...)
//         /repo/gcc-trunk/gcc/diagnostic-global-context.cc:787
// 0x165d3af crash_signal
//         /repo/gcc-trunk/gcc/toplev.cc:325
// 0x11a0140 contains_struct_check(tree_node*, tree_node_structure_enum, char const*, int, char const*)
//         /repo/gcc-trunk/gcc/tree.h:3937
// 0x11a0140 tree_invalid_nonnegative_p
//         /repo/gcc-trunk/gcc/fold-const.cc:14794
// 0x11a0140 tree_expr_nonnegative_p(tree_node*, int)
//         /repo/gcc-trunk/gcc/fold-const.cc:14908
// 0x11a0140 tree_expr_nonnegative_p(tree_node*, int)
//         /repo/gcc-trunk/gcc/fold-const.cc:14851
// 0x20d1ec7 generic_simplify_COND_EXPR(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*)
//         /repo/build-gcc-trunk-amd64/gcc/generic-match-4.cc:16704
// 0x11cabdc fold_ternary_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:12581
// 0x11ccd17 fold_build3_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:13673
// 0x11b18f1 fold_binary_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:10857
// 0x11b934e fold_build2_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:13613
// 0x107a2fe do_narrow
//         /repo/gcc-trunk/gcc/convert.cc:453
// 0x107b6ad convert_to_integer_1
//         /repo/gcc-trunk/gcc/convert.cc:864
// 0xeb32b3 c_convert
//         /repo/gcc-trunk/gcc/c/c-convert.cc:148
// 0xf706c8 c_gimplify_expr(tree_node**, gimple**, gimple**)
//         /repo/gcc-trunk/gcc/c-family/c-gimplify.cc:943
// 0x12a566d gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20280
// 0x12a5d84 gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:21201
// 0x12a5dda gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20679
// 0x12c88af gimplify_modify_expr
//         /repo/gcc-trunk/gcc/gimplify.cc:7296
// 0x12a64dd gimplify_expr(tree_node**, gimple**, gimple**, bool (*)(tree_node*), int)
//         /repo/gcc-trunk/gcc/gimplify.cc:20374
// /repo/gcc-trunk/binary-trunk-20260608130957-<a href="https://gcc.gnu.org/cgit/gcc/commit/?id=de3a13dd7f6c28">r17-1421-gde3a13dd7f6c28</a>-checking-yes-rtl-df-extra-nobootstrap-amd64/bin/../libexec/gcc/x86_64-pc-linux-gnu/17.0.0/cc1 -quiet -iprefix /repo/gcc-trunk/binary-trunk-20260608130957-<a href="https://gcc.gnu.org/cgit/gcc/commit/?id=de3a13dd7f6c28">r17-1421-gde3a13dd7f6c28</a>-checking-yes-rtl-df-extra-nobootstrap-amd64/bin/../lib/gcc/x86_64-pc-linux-gnu/17.0.0/ testcase.c -quiet -dumpdir a- -dumpbase testcase.c -dumpbase-ext .c -mtune=generic -march=x86-64 -o /tmp/ccFzcrS3.s
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.


