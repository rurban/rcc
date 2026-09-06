/* GCC Bug #82100 - gcc does not warn about code that is unreachable due to conflicting conditions [subset of reviving -Wunreachable-code]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82100
 */
/* { dg-do compile } */


extern void g( int);

void f( int a, int b)
{
	if (a != b)
 {
		if (a == b)
  {
// 			g( a);
  }
 }
	if (a < b)
 {
		if (a >= b)
  {
// 			g( a);
  }
 }
	if (a > b)
 {
		if (a <= b)
  {
// 			g( a);
  }
 }
}
// then recent gcc trunk doesn't have much to say, even when provoked:
// $ 
// Here is static analyser cppcheck finding the three bugs:
// [sep4a.cc:6] -> [sep4a.cc:8]: (warning) Opposite inner 'if' condition leads to a dead code block.
// [sep4a.cc:13] -> [sep4a.cc:15]: (warning) Opposite inner 'if' condition leads to a dead code block.
// [sep4a.cc:20] -> [sep4a.cc:22]: (warning) Opposite inner 'if' condition leads to a dead code block.
// $ 
// Runng cppcheck over the current Linux kernel version 4.13 suggests about
// half a dozen bugs would be detected.


