/* GCC Bug #86647 - Test on constant expression (unsigned) -1 < 0 triggers a spurious -Wtype-limits warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86647
 */
/* { dg-do compile } */


int foo (void)
{
  return (unsigned) -1 < 0;
}
// zira:~> gcc-snapshot -Wtype-limits -c tst.c
// tst.c: In function 'foo':
// tst.c:3:24: warning: comparison of unsigned expression < 0 is always false [-Wtype-limits]
   return (unsigned) -1 < 0;
//                         ^
// Note that 1U < 0 does not trigger a warning, as expected. But 2147483648U < 0 triggers it (with 32-bit int). So, it seems that one gets a warning when the unsigned constant converted to signed would give a negative integer.
// The consequence of this bug is that the following macro used to detect signed integer types
#define SIGNED(T) ((T) -1 < 0)
// triggers a warning on unsigned integer types.


