/* GCC Bug #112716 - LTO optimization with struct with variable size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112716
 */


// y.c
void bar(void*);

// [[gnu::noinline,gnu::noipa]]
int foo(void *p, void *q)
{
 int n = 5;
 struct foo { int x; typeof(T1) y; } *p2 = p;
// 	p2->x = 1;
// 	bar(q);
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
// y2
void bar(void* q)
{	
 int n = 5;
 struct foo { int x; typeof(T2) y; } *q2 = q;
// 	q2->x = 2;
}


