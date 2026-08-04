/* GCC Bug #104764 - gcc hangs when compiling an invalid c program
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104764
 */


Confirmed:
#1  0x00000000009293b9 in c_expr_sizeof_expr(unsigned int, c_expr) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-typeck.cc:2977
#2  0x0000000000965aa7 in c_parser_sizeof_expression (parser=<error reading variable: dwarf2_find_location_expression: Corrupted DWARF expression.>) at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:8336
#3  c_parser_unary_expression(c_parser*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:8233
#4  0x0000000000966bb8 in c_parser_cast_expression(c_parser*, c_expr*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:8103
#5  0x0000000000966e28 in c_parser_binary_expression(c_parser*, c_expr*, tree_node*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:7906
#6  0x000000000096830c in c_parser_conditional_expression(c_parser*, c_expr*, tree_node*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:7606
#7  0x0000000000968b32 in c_parser_expr_no_commas(c_parser*, c_expr*, tree_node*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:7521
#8  0x0000000000968da2 in c_parser_expression(c_parser*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:10697
#9  0x0000000000969518 in c_parser_expression_conv (parser=0x7ffff7251b40) at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:10736
#10 0x000000000097f500 in c_parser_statement_after_labels(c_parser*, bool*, vec<tree_node*, va_heap, vl_ptr>*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:6263
#11 0x0000000000981945 in c_parser_compound_statement_nostart(c_parser*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:5800
#12 0x0000000000981e45 in c_parser_compound_statement (parser=0x7ffff7251b40, endlocp=0x7fffffffdb00) at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:5609
#13 0x0000000000983904 in c_parser_declaration_or_fndef(c_parser*, bool, bool, bool, bool, bool, tree_node**, vec<c_token, va_heap, vl_ptr>*, bool, tree_node*, oacc_routine_data*, bool*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:2544
#14 0x000000000098b9a4 in c_parser_external_declaration(c_parser*) () at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:1779
#15 0x000000000098c3bc in c_parser_translation_unit (parser=<optimized out>) at /home/apinski/src/upstream-gcc/gcc/gcc/c/c-parser.cc:1652


