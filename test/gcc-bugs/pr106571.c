/* GCC Bug #106571 - Implement -Wsection diag
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106571
 */


extern u64 x86_spec_ctrl_current;
__attribute__((section(".data..percpu" ""))) __typeof__(u64) x86_spec_ctrl_current;


