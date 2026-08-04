/* GCC Bug #119476 - [-Wdiscarded-qualifiers] False negative with -fplan9-extensions inheritance
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119476
 */
/* { dg-do compile } */
/* { dg-options "-Wall -Wextra -fplan9-extensions" } */

struct foo {
	int a;
};

struct bar {
	struct foo;
};

void f(struct foo *);
void g(char *);

int
main(void)
{
	const struct bar  b;

	f(&b);   /* missing -Wdiscarded-qualifiers: the plan9 anonymous-field
	            inheritance drops the const from struct bar (the bug) */

	const char *s = "foo";

	g(s); /* { dg-warning "discards .const. qualifier from pointer target type" } */
}