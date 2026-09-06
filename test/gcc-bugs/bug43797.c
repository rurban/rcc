/* GCC Bug #43797 - __attribute__((deprecated("message"))) produces unexpected messages in some cases.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=43797
 */
/* { dg-do compile } */


typedef int INT1 __attribute__((deprecated("No INT1")));
INT1 f1 (void) __attribute__((deprecated("No f1")));
INT1 f2 (void) __attribute__((deprecated));

void func (void)
{
  f1();
  f2();
}


