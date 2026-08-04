/* GCC Bug #101545 - copy attribute is not copying the C++11/C23 attributes correctly.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101545
 */
/* { dg-do compile } */


int foo(void)
{
 return 42;
}

int foobar(void)
{
// 	return bar();  /* This will return 42 */
}

[[gnu::alias("foo")]] [[gnu::copy(foo)]] extern __typeof__(foo) bar;
// nodiscard.c:3:1: warning: 'nodiscard' attribute directive ignored [-Wattributes]
//     3 | [[gnu::copy(foo)]] extern __typeof__(foo) bar;
//       | ^
// nodiscard.c:18:1: warning: 'nodiscard' attribute directive ignored [-Wattributes]
//    18 | [[gnu::alias("foo")]] [[gnu::copy(foo)]] extern __typeof__(foo) bar;
//       | ^


