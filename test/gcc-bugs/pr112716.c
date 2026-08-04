/* GCC Bug #112716 - LTO optimization with struct with variable size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112716
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu23" } */

#ifndef T1
#define T1 "int[n]"
#endif
#ifndef T2
#define T2 "int[n]"
#endif

/* NOTE: genuinely multi-TU (LTO wrong-code): needs y.c + y2.c compiled
 * as `gcc -flto -O2 y.c y2.c -DT1="int[n]" -DT2="int[n]"`.  The bug is
 * that the two function-local `struct foo { int x; typeof(T1) y; }`
 * types across TUs should be compatible (same tag, compatible members
 * incl. the GNU VLA extension) but get distinct alias sets, so LTO
 * miscompiles `foo`: bar() stores 2 through its own struct type and
 * foo() re-loads 1 through its type.  Kept here as TU y.c for reference
 * (comment 0 of the bug). */

// y.c
void bar(void*);

[[gnu::noinline, gnu::noipa]]
int foo(void *p, void *q)
{
	int n = 5;
	struct foo { int x; typeof(T1) y; } *p2 = p;
	p2->x = 1;
	bar(q);
	return p2->x;
}

int main()
{
	int n = 5;
	void *p = __builtin_malloc(sizeof(struct foo { int x; typeof(T1) y; }));

	if (!p)
		return 0;

	if (2 != foo(p, p))
		__builtin_abort();

	return 0;
}