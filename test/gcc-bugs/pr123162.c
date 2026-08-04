/* GCC Bug #123162 - ICE: tree check: expected class 'type', have 'exceptional' (error_mark) in types_match, at generic-match-head.cc:63 with -Wlogical-op -mno-sse2 and __bf16
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123162
 */
/* { dg-do compile } */


// Created <span class=""><a href="attachment.cgi?id=63067" name="attach_63067" title="reduced testcase">attachment 63067</a> <a href="attachment.cgi?id=63067&action=edit" title="reduced testcase">[details]</a></span>
// reduced testcase
// Compiler output:
// $ x86_64-pc-linux-gnu-gcc -Wlogical-op -mno-sse2 testcase.c 
// testcase.c: In function 'foo':
// testcase.c:4:3: error: invalid conversion from type '__bf16' without option '-msse2'
//     4 |   (0 ? : f) && c;
//       |   ^
// testcase.c:4:3: internal compiler error: tree check: expected class 'type', have 'exceptional' (error_mark) in types_match, at generic-match-head.cc:63
// 0x2c7684d internal_error(char const*, ...)
//         /repo/gcc-trunk/gcc/diagnostic-global-context.cc:787
// 0x924176 tree_class_check_failed(tree_node const*, tree_code_class, char const*, int, char const*)
//         /repo/gcc-trunk/gcc/tree.cc:9242
// 0x9f1d78 tree_class_check(tree_node*, tree_code_class, char const*, int, char const*)
//         /repo/gcc-trunk/gcc/tree.h:3915
// 0x9f1d78 types_match
//         /repo/gcc-trunk/gcc/generic-match-head.cc:63
// 0x9f1d78 generic_simplify_247(unsigned long, tree_node*, tree_node*, tree_node*, tree_node**, tree_code)
//         /repo/build-gcc-trunk-amd64/gcc/generic-match-10.cc:1158
// 0x213aa05 generic_simplify_EQ_EXPR(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/build-gcc-trunk-amd64/gcc/generic-match-8.cc:8170
// 0x1194a6a fold_binary_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:11045
// 0x119c70e fold_build2_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:13962
// 0x11b0c00 fold_binary_op_with_conditional_arg
//         /repo/gcc-trunk/gcc/fold-const.cc:7081
// 0x1194df3 fold_binary_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:11114
// 0x119c70e fold_build2_loc(unsigned long, tree_code, tree_node*, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:13962
// 0x11ac761 build_range_check(unsigned long, tree_node*, tree_node*, int, tree_node*, tree_node*)
//         /repo/gcc-trunk/gcc/fold-const.cc:5685
// 0xf9d609 warn_logical_operator(unsigned long, tree_code, tree_node*, tree_code, tree_node*, tree_code, tree_node*)
//         /repo/gcc-trunk/gcc/c-family/c-warn.cc:276
// 0xe82858 parser_build_binary_op(unsigned long, tree_code, c_expr, c_expr)
//         /repo/gcc-trunk/gcc/c/c-typeck.cc:5072
// 0xeb3a8e c_parser_binary_expression
//         /repo/gcc-trunk/gcc/c/c-parser.cc:10518
// 0xeb4c73 c_parser_conditional_expression
//         /repo/gcc-trunk/gcc/c/c-parser.cc:10083
// 0xeb54a4 c_parser_expr_no_commas
//         /repo/gcc-trunk/gcc/c/c-parser.cc:9996
// 0xeb5907 c_parser_expression
//         /repo/gcc-trunk/gcc/c/c-parser.cc:14212
// 0xeb6137 c_parser_expression_conv
//         /repo/gcc-trunk/gcc/c/c-parser.cc:14271
// 0xede7ff c_parser_statement_after_labels
//         /repo/gcc-trunk/gcc/c/c-parser.cc:8531
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.
// $ x86_64-pc-linux-gnu-gcc -v
// Using built-in specs.
// COLLECT_GCC=/repo/gcc-trunk/binary-latest/bin/x86_64-pc-linux-gnu-gcc
// COLLECT_LTO_WRAPPER=/repo/gcc-trunk/binary-trunk-20251216201148-<a href="https://gcc.gnu.org/cgit/gcc/commit/?id=866bc8a9214b1d">r16-6177-g866bc8a9214b1d</a>-checking-yes-rtl-df-extra-nobootstrap-amd64/bin/../libexec/gcc/x86_64-pc-linux-gnu/16.0.0/lto-wrapper
// Target: x86_64-pc-linux-gnu
// Configured with: /repo/gcc-trunk//configure --enable-languages=c,c++ --enable-valgrind-annotations --disable-nls --enable-checking=yes,rtl,df,extra --disable-bootstrap --build=x86_64-pc-linux-gnu --host=x86_64-pc-linux-gnu --target=x86_64-pc-linux-gnu --with-ld=/usr/bin/x86_64-pc-linux-gnu-ld --with-as=/usr/bin/x86_64-pc-linux-gnu-as --enable-libsanitizer --disable-libstdcxx-pch --prefix=/repo/gcc-trunk//binary-trunk-20251216201148-<a href="https://gcc.gnu.org/cgit/gcc/commit/?id=866bc8a9214b1d">r16-6177-g866bc8a9214b1d</a>-checking-yes-rtl-df-extra-nobootstrap-amd64
// Thread model: posix
// Supported LTO compression algorithms: zlib zstd
// gcc version 16.0.0 20251216 (experimental) (GCC)


