/* GCC Bug #92479 - missing warnings for unreachable codes with break (i.e. revive the subset of -Wunreachable-code that fits under clang's -Wunreachable-code-break)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92479
 */


extern void g( int);

void f( int a)
{
	if (a >= 0)
 {
		if (a < 0)
  {
// 			g(a);
  }
 }
}
  if (a < 0)
 if (a >= 0)


