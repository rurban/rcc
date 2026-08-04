/* GCC Bug #117245 - ICE: verify_ssa failed (error: definition in block 2 follows the use) with VLA types in struct with a vector type rebuild and nested functions since r13-6128-g47821ba07a19b6
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117245
 */
/* { dg-do compile } */


void a() {
  int b;
  struct {
    char c[b];
  } bar() {
  }
  struct bar {
    __attribute__((vector_size(4))) char c[b];
  } (*d)();
  struct bar e() { struct bar f; }
  d = e;
  sizeof(d());
}

//    13 | }
// _4 = _16 * 4;
// 0x5071bcf diagnostic_context::report_diagnostic(diagnostic_info*)
// 0x50724a1 diagnostic_context::diagnostic_impl(rich_location*, diagnostic_metadata const*, int, char const*, __va_list_tag (*) [1], diagnostic_t)
// 0x50924c7 internal_error(char const*, ...)
// 0x24866d5 verify_ssa(bool, bool)
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


