/* GCC Bug #111810 - rtlFE: nested inline RTL function cause ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111810
 */
/* { dg-do compile } */


void = {0};
void uninliner_1() {
  {}
inline __RTL test(void) {
//   uninliner_1();
//     1 | void = {0};
//     4 | inline __RTL test(void) {
//     5 |   uninliner_1();
// 0x22ff3ee internal_error(char const*, ...)
// 0xa2621e finish_function(unsigned int)
// 0xaa468d c_parse_file()
// 0xb17919 c_common_parse_file()
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


