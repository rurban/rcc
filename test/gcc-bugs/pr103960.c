/* GCC Bug #103960 - Clang's -Wunknown-attributes is more useful than -Wattributes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103960
 */


struct MyStruct;
// [[gg (read_only, 1)]] int my_func (const struct MyStruct * const self);
// ----- CUT ----
// The C front-end produces:
// <source>:2:1: warning: 'gg' attribute ignored [-Wattributes]
// While the C++ front-end produces:
// <source>:2:70: warning: 'gg' attribute directive ignored [-Wattributes]


