/* GCC Bug #123494 - ICE Segmentation fault in groktypename when using _Alignas within a cast expression with int
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123494
 */
/* { dg-do compile } */


int main() {
    return (_BitInt(9)_Alignas(long double)(volatile void *));
}
// ```
Traceback:
// ```
// <source>: In function 'main':
// <source>:2:23: error: alignment specified for type name in cast
//     2 |     return (_BitInt(9)_Alignas(long double)(volatile void *));
//       |                       ^~~~~~~~
// <source>:2:62: error: expected expression before ';' token
//     2 |     return (_BitInt(9)_Alignas(long double)(volatile void *));
//       |                                                              ^
// Segmentation fault
// 0x25c4b08 diagnostics::context::diagnostic_impl(rich_location*, diagnostics::metadata const*, diagnostics::option_id, char const*, __va_list_tag (*) [1], diagnostics::kind)
// 	???:0
// 0x25b98cb internal_error(char const*, ...)
// 	???:0
// 0x26263f3 pretty_printer::format(text_info&)
// 	???:0
// 0x25c4403 diagnostics::context::report_diagnostic(diagnostics::diagnostic_info*)
// 	???:0
// 0x25c4b08 diagnostics::context::diagnostic_impl(rich_location*, diagnostics::metadata const*, diagnostics::option_id, char const*, __va_list_tag (*) [1], diagnostics::kind)
// 	???:0
// 0x25b8cad error_at(unsigned long, char const*, ...)
// 	???:0
// 0xa58f21 groktypename(c_type_name*, tree_node**, bool*)
// 	???:0
// 0xa84c5f c_cast_expr(unsigned long, c_type_name*, tree_node*)
// 	???:0
// 0xaebfd0 c_parse_file()
// 	???:0
// 0xb6fcc9 c_common_parse_file()
// 	???:0
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// Please include the complete backtrace with any bug report.
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.
// ```


