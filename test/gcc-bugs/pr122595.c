/* GCC Bug #122595 - FAIL: gcc.dg/pr97986-1.c starting with - r16-4919-g5175ef7f7577a1
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122595
 */


int f(int n)
{
 __label__ b, d;
 void g(void) { n++; goto b; }
// 	g();
 int v[n];
b:
 void h(void) { n++; goto d; }
// 	h();
d:
 return n;
}

int main()
{
	if (9 != f(7))
  __builtin_abort();
 return 0;
}
// Does that not work on power9 either?


