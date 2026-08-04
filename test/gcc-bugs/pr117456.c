/* GCC Bug #117456 - ICE: in expand_expr_real_2, at expr.cc:10567 with __builtin_stdc_rotate_left() on bitfield or _BitInt() of non-mode size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117456
 */
/* { dg-do compile } */


// Created <span class=""><a href="attachment.cgi?id=59542" name="attach_59542" title="reduced testcase">attachment 59542</a> <a href="attachment.cgi?id=59542&action=edit" title="reduced testcase">[details]</a></span>
// reduced testcase
// Compiler output:
// $ x86_64-pc-linux-gnu-gcc testcase.c 
// during RTL pass: expand
// testcase.c: In function 'foo':
// testcase.c:8:10: internal compiler error: in expand_expr_real_2, at expr.cc:10567
//     8 |   return __builtin_stdc_rotate_left (b.u, 1);
//       |          ^~~~~~~~~~~~~~~~~~~~~~~~~~
// 0x2c9f89e internal_error(char const*, ...)
//         /repo/gcc-trunk/gcc/diagnostic-global-context.cc:518
// 0xe85439 fancy_abort(char const*, int, char const*)
//         /repo/gcc-trunk/gcc/diagnostic.cc:1696
// 0x795447 expand_expr_real_2(separate_ops const*, rtx_def*, machine_mode, expand_modifier)
//         /repo/gcc-trunk/gcc/expr.cc:10567
// 0x1179ff3 expand_expr_real_gassign(gassign*, rtx_def*, machine_mode, expand_modifier, rtx_def**, bool)
//         /repo/gcc-trunk/gcc/expr.cc:11146
// 0x1036b49 expand_gimple_stmt_1
//         /repo/gcc-trunk/gcc/cfgexpand.cc:4047
// 0x1036b49 expand_gimple_stmt
//         /repo/gcc-trunk/gcc/cfgexpand.cc:4111
// 0x103d15e expand_gimple_basic_block
//         /repo/gcc-trunk/gcc/cfgexpand.cc:6167
// 0x103ee47 execute
//         /repo/gcc-trunk/gcc/cfgexpand.cc:6906
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.
// $ x86_64-pc-linux-gnu-gcc -v
// Using built-in specs.
// COLLECT_GCC=/repo/gcc-trunk/binary-latest-amd64/bin/x86_64-pc-linux-gnu-gcc
// COLLECT_LTO_WRAPPER=/repo/gcc-trunk/binary-trunk-<a href="https://gcc.gnu.org/cgi-bin/gcc-gitref.cgi?r=r15-4963">r15-4963</a>-20241105175800-g161e246cf32-checking-yes-rtl-df-extra-nobootstrap-amd64/bin/../libexec/gcc/x86_64-pc-linux-gnu/15.0.0/lto-wrapper
// Target: x86_64-pc-linux-gnu
// Configured with: /repo/gcc-trunk//configure --enable-languages=c,c++ --enable-valgrind-annotations --disable-nls --enable-checking=yes,rtl,df,extra --disable-bootstrap --with-cloog --with-ppl --with-isl --build=x86_64-pc-linux-gnu --host=x86_64-pc-linux-gnu --target=x86_64-pc-linux-gnu --with-ld=/usr/bin/x86_64-pc-linux-gnu-ld --with-as=/usr/bin/x86_64-pc-linux-gnu-as --enable-libsanitizer --disable-libstdcxx-pch --prefix=/repo/gcc-trunk//binary-trunk-<a href="https://gcc.gnu.org/cgi-bin/gcc-gitref.cgi?r=r15-4963">r15-4963</a>-20241105175800-g161e246cf32-checking-yes-rtl-df-extra-nobootstrap-amd64
// Thread model: posix
// Supported LTO compression algorithms: zlib zstd
// gcc version 15.0.0 20241105 (experimental) (GCC)


