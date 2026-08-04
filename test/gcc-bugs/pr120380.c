/* GCC Bug #120380 - internal compiler error: error reporting routines re-entered
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120380
 */
/* { dg-do compile } */


struct pair_t {
  char c;
  __int128_t i;
} __attribute__((packed));
typedef struct unaligned_int128_t_ {
  struct unaligned_int128_t_ {
    struct unaligned_int128_t_ {
      __int128_t value;
    }
  }
} __attribute__((packed, may_alias)) unaligned_int128_t;
struct pair_t p = {0, 1};
unaligned_int128_t *addr = (unaligned_int128_t *)&p.i;
int main() {
//   addr->value = 0;
  return 0;
}
// 0x260c68e diagnostic_context::report_diagnostic(diagnostic_info*)
// 0x260c7a5 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x2628d71 warning(diagnostic_option_id, char const*, ...)
// 0xa0d188 build_type_attribute_qual_variant(tree_node*, tree_node*, int)
// 0x264dec3 pretty_printer::format(text_info&)
// 0x260c2e6 diagnostic_context::report_diagnostic(diagnostic_info*)
// 0x260c7a5 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, diagnostic_option_id, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x2629dff error_at(unsigned long, char const*, ...)
// 0xa56ae2 build_component_ref(unsigned long, tree_node*, tree_node*, unsigned long, unsigned long, bool)
// 0xabbbee c_parse_file()
// 0xb3c959 c_common_parse_file()


