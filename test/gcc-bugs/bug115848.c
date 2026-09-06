/* GCC Bug #115848 - ICE: 'verify_type' failed with -flto and strub attribute and typedef of the function type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115848
 */
/* { dg-do compile } */


typedef void __attribute__((__strub__)) a(int, int);
// a(b);
void c() { b(0, 0); }
// 0x4ddb72b diagnostic_context::report_diagnostic(diagnostic_info*)
// 0x4ddbffd diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, int, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x4dfc327 internal_error(char const*, ...)
// 0x23e928b verify_type(tree_node const*)
// 0xdef502 read_cgraph_and_symbols(unsigned int, char const**)
// 0xd8f963 lto_main()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


