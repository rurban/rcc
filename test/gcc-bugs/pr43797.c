/* GCC Bug #43797 - __attribute__((deprecated("message"))) produces unexpected messages in some cases.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=43797
 */


typedef int INT1 __attribute__((deprecated("No INT1")));
INT1 f1 (void) __attribute__((deprecated("No f1")));
INT1 f2 (void) __attribute__((deprecated));

void func (void)
{
//   f1();
//   f2();
}
// ===
produces:
// warning: 'f1' is deprecated (declared at...): No f1        *** (correct)
// warning: 'f2' is deprecated (declared at...): No INT1   *** (incorrect, f2 has no deprecation message).


