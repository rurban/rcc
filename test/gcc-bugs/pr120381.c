/* GCC Bug #120381 - internal compiler error: in composite_type_internal, at c/c-typeck.cc:848
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120381
 */
/* { dg-do compile } */


struct A {
  struct A { /* { dg-error "nested redefinition of .struct A." } */
    struct A *p;
  } *p;
};
int foo(const struct A *q) { return q->p == q; }
void bar(int);
void baz() {
  struct A a;
  while (foo(&a))
    bar(foo(&a));
}

//     6 | int foo(const struct A *q) { return q->p == q; }
// 0x260c7a5 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x262a516 internal_error(char const*, ...)
// 0x9f37e0 fancy_abort(char const*, int, char const*)
// 0xa53093 build_binary_op(unsigned long, tree_code, tree_node*, tree_node*, bool)
// 0xa55496 parser_build_binary_op(unsigned long, tree_code, c_expr, c_expr)
// 0xabbbee c_parse_file()
// 0xb3c959 c_common_parse_file()


